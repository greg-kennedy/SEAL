/*
 * $Id: dsdrv2.c 0.4 1998/12/26 chasan Exp $
 *
 * DirectSound accelerated audio driver (Experimental)
 *
 * Copyright (C) 1998-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <windows.h>
#include <objbase.h>
#include <dsound.h>
#include <math.h>
#include "audio.h"
#include "drivers.h"

#if 0
#define MSGBOX(text) MessageBox(NULL, text, "DirectSound", MB_OK)
#else
#define MSGBOX(text)
#endif

static struct {
    LPDIRECTSOUND  lpDirectSound;
    LPDIRECTSOUNDBUFFER lpPrimaryBuffer;
    LPDIRECTSOUNDBUFFER aSoundBuffer[AUDIO_MAX_VOICES];
    LPFNAUDIOTIMER lpfnTimerHandler;

    LONG    aLogVolumeTable[AUDIO_MAX_VOLUME+1];
    LONG    aLogPanningTable[AUDIO_MAX_PANNING+1];

    DWORD   aFormatTable[AUDIO_MAX_VOICES];
    LONG    aFrequencyTable[AUDIO_MAX_VOICES];    
    UINT    aVolumeTable[AUDIO_MAX_VOICES];
    UINT    aPanningTable[AUDIO_MAX_VOICES];

    LONG    dwTimer;
    LONG    dwTimerAccum;
    LONG    dwTimerRate;
} DS;


static HRESULT _DirectSoundCreate(GUID FAR *lpGuid,
				  LPDIRECTSOUND *lplpDirectSound, IUnknown FAR *lpUnkOuter)
{
    HRESULT hresult;

    CoInitialize(NULL);
    hresult = CoCreateInstance(&CLSID_DirectSound, lpUnkOuter,
			       CLSCTX_ALL, &IID_IDirectSound, lplpDirectSound);
    if (!FAILED(hresult))
        hresult = IDirectSound_Initialize(*lplpDirectSound, NULL);
    return hresult;
}

static HRESULT _DirectSoundDestroy(LPDIRECTSOUND lpDirectSound)
{
    HRESULT hresult;

    hresult = IDirectSound_Release(lpDirectSound);
    CoUninitialize();
    return hresult;
}

static UINT AIAPI SetAudioMixerValue(UINT, UINT);


/*
 * DirectSound audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_NONE, "DirectSound (experimental)",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    LPDIRECTSOUND lpDirectSound;
    if (_DirectSoundCreate(NULL, &lpDirectSound, NULL) != DS_OK)
        return AUDIO_ERROR_NODEVICE;
    _DirectSoundDestroy(lpDirectSound);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    LPDIRECTSOUNDBUFFER lpPrimaryBuffer;
    DSBUFFERDESC dsbd;
    WAVEFORMATEX wfx;
    DSCAPS dsc;
    LONG nPanning;

    memset(&DS, 0, sizeof(DS));

    if (lpInfo == NULL)
	return AUDIO_ERROR_INVALPARAM;

    /* create direct sound object */
    if (_DirectSoundCreate(NULL, &DS.lpDirectSound, NULL) != DS_OK)
        return AUDIO_ERROR_NODEVICE;

    /* set direct sound cooperative level */
    if (DS.lpDirectSound->lpVtbl->SetCooperativeLevel(DS.lpDirectSound,
						      GetForegroundWindow(), DSSCL_PRIORITY) != DS_OK) {
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* get direct sound capabilities */
    dsc.dwSize = sizeof(dsc);
    if (DS.lpDirectSound->lpVtbl->GetCaps(DS.lpDirectSound, &dsc) != DS_OK) {
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* adjust the format settings */
    lpInfo->nSampleRate = 44100;
    if (!(dsc.dwFlags & DSCAPS_PRIMARYSTEREO))
        lpInfo->wFormat &= ~AUDIO_FORMAT_STEREO;
    if (!(dsc.dwFlags & DSCAPS_PRIMARY16BIT))
        lpInfo->wFormat &= ~AUDIO_FORMAT_16BITS;

    /* setup wave format structure */
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1;
    wfx.wBitsPerSample = lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
    wfx.nSamplesPerSec = lpInfo->nSampleRate;
    wfx.nAvgBytesPerSec = lpInfo->nSampleRate;
    wfx.nBlockAlign = 1;
    wfx.cbSize = 0;
    if (lpInfo->wFormat & AUDIO_FORMAT_STEREO) {
        wfx.nAvgBytesPerSec <<= 1;
        wfx.nBlockAlign <<= 1;
    }
    if (lpInfo->wFormat & AUDIO_FORMAT_16BITS) {
        wfx.nAvgBytesPerSec <<= 1;
        wfx.nBlockAlign <<= 1;
    }

    /* create primary sound buffer */
    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = NULL;
    if (DS.lpDirectSound->lpVtbl->CreateSoundBuffer(DS.lpDirectSound,
						    &dsbd, &DS.lpPrimaryBuffer, NULL) != DS_OK) {
        MSGBOX("Can't create primary buffer");
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }
        
    /* setup primary buffer format */
    while (DS.lpPrimaryBuffer->lpVtbl->SetFormat(DS.lpPrimaryBuffer, &wfx) != DS_OK) {
        if (lpInfo->nSampleRate <= 11025) {
            MSGBOX("Can't change primary buffer format");
            _DirectSoundDestroy(DS.lpDirectSound);
            return AUDIO_ERROR_NODEVICE;
        }
        wfx.nSamplesPerSec >>= 1;
        wfx.nAvgBytesPerSec >>= 1;
        wfx.nBlockAlign >>= 1;
        lpInfo->nSampleRate >>= 1;
    }

    /* start playing primary buffer */
    DS.lpPrimaryBuffer->lpVtbl->Play(DS.lpPrimaryBuffer, 0, 0, DSBPLAY_LOOPING);

    /* set the timer settings */
    DS.dwTimer = GetTickCount();
    DS.dwTimerAccum = 0;

    /* set default master volume */
    SetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME, 96);
    
    /* create log panning table */
    for (nPanning = 0; nPanning <= AUDIO_MAX_PANNING; nPanning++) {
        DS.aLogPanningTable[nPanning] = (2000 * (nPanning - 128)) >> 7;
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    UINT i;

    if (DS.lpDirectSound != NULL) {
        /* stop and release primary buffer */
        if (DS.lpPrimaryBuffer != NULL) {
            DS.lpPrimaryBuffer->lpVtbl->Stop(DS.lpPrimaryBuffer);
            DS.lpPrimaryBuffer->lpVtbl->Release(DS.lpPrimaryBuffer);
        }

        /* release duplicated sound buffers */
        for (i = 0; i < AUDIO_MAX_VOICES; i++) {
            if (DS.aSoundBuffer[i] != NULL) {
                DS.aSoundBuffer[i]->lpVtbl->Stop(DS.aSoundBuffer[i]);
                DS.aSoundBuffer[i]->lpVtbl->Release(DS.aSoundBuffer[i]);
            }
        }
        
        /* release direct sound buffer */
        _DirectSoundDestroy(DS.lpDirectSound);
    }

    /* clean up DirectSound driver variables */
    memset(&DS, 0, sizeof(DS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    LONG dwTimer = GetTickCount();

    /* call the virtual audio timer callback */
    if ((DS.dwTimerAccum += dwTimer - DS.dwTimer) >= DS.dwTimerRate) {
        DS.dwTimerAccum -= DS.dwTimerRate;
        if (DS.lpfnTimerHandler != NULL)
	    DS.lpfnTimerHandler();
    }
    DS.dwTimer = dwTimer;

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudioSynth(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioMixerValue(UINT nChannel, UINT nValue)
{
    LONG nVolume;

    if (nChannel != AUDIO_MIXER_MASTER_VOLUME &&
        nChannel != AUDIO_MIXER_TREBLE &&
        nChannel != AUDIO_MIXER_BASS &&
        nChannel != AUDIO_MIXER_CHORUS &&
        nChannel != AUDIO_MIXER_REVERB)
        return AUDIO_ERROR_INVALPARAM;

    if (nChannel == AUDIO_MIXER_MASTER_VOLUME && nValue <= 256) {
        DS.aLogVolumeTable[0] = -10000;
        for (nVolume = 1; nVolume <= AUDIO_MAX_VOLUME; nVolume++) {
	    LONG value = (nValue * nVolume) >> 6;
	    DS.aLogVolumeTable[nVolume] = (LONG) (value == 0 ? -10000.0 : -2000.0 * log(value / 256.0) / log(1.0/256.0));
        }
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    if (nVoices <= AUDIO_MAX_VOICES) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    UINT i;

    for (i = 0; i < AUDIO_MAX_VOICES; i++) {
        /* stop and release duplicated sound buffers */
        if (DS.aSoundBuffer[i] != NULL) {
            DS.aSoundBuffer[i]->lpVtbl->Stop(DS.aSoundBuffer[i]);
            DS.aSoundBuffer[i]->lpVtbl->Release(DS.aSoundBuffer[i]);
        }
    }

    /* clean up duplicated sound buffer array */
    memset(DS.aSoundBuffer, 0, sizeof(DS.aSoundBuffer));
    
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    if (lpfnAudioWave != NULL) {
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    if (lpfnAudioTimer != NULL) {
        /* start up the timer settings */
        if (DS.lpfnTimerHandler == NULL) {
	    DS.dwTimer = GetTickCount();
	    DS.dwTimerAccum = 0;
        }
        DS.lpfnTimerHandler = lpfnAudioTimer;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nBPM)
{
    if (nBPM >= 0x20 && nBPM <= 0xFF) {
        /* set timer rate in milliseconds */
        DS.dwTimerRate = (5 * 1000L) / (2 * nBPM);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static LONG AIAPI GetAudioDataAvail(VOID)
{
    return 0L;
}

static UINT AIAPI CreateAudioData(LPAUDIOWAVE lpWave)
{
    LPDIRECTSOUNDBUFFER lpSoundBuffer;
    DSBUFFERDESC dsbd;
    DSBCAPS dsbc;
    WAVEFORMATEX wfx;

    if (lpWave != NULL) {
        lpWave->dwHandle = 0;

        /* setup waveform format */
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 1;
        wfx.wBitsPerSample = lpWave->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
        wfx.nSamplesPerSec = lpWave->nSampleRate;
        wfx.nAvgBytesPerSec = lpWave->nSampleRate;
        wfx.nBlockAlign = 1;
        wfx.cbSize = 0;
        if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
	    wfx.nAvgBytesPerSec <<= 1;
	    wfx.nBlockAlign <<= 1;
        }
        
        /* setup sound buffer description */
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
	    DSBCAPS_CTRLFREQUENCY | DSBCAPS_STICKYFOCUS |
	    DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
        dsbd.dwBufferBytes = lpWave->dwLength;
        dsbd.dwReserved = 0;
        dsbd.lpwfxFormat = &wfx;

        /* create sound buffer object */
        if (DS.lpDirectSound->lpVtbl->CreateSoundBuffer(DS.lpDirectSound,
							&dsbd, &lpSoundBuffer, NULL) != DS_OK) {
            MSGBOX("Can't create DirectSoundBuffer object");
            return AUDIO_ERROR_NOMEMORY;
        }

        lpWave->dwHandle = (DWORD) lpSoundBuffer;    
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI DestroyAudioData(LPAUDIOWAVE lpWave)
{
    LPDIRECTSOUNDBUFFER lpSoundBuffer;

    if (lpWave != NULL && lpWave->dwHandle != 0) {
        lpSoundBuffer = (LPDIRECTSOUNDBUFFER) lpWave->dwHandle;
        lpSoundBuffer->lpVtbl->Stop(lpSoundBuffer);
        lpSoundBuffer->lpVtbl->Release(lpSoundBuffer);
        lpWave->dwHandle = 0;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI WriteAudioData(LPAUDIOWAVE lpWave, DWORD dwOffset, UINT nCount)
{
    LPDIRECTSOUNDBUFFER lpSoundBuffer;
    LPVOID lpPtr1, lpPtr2;
    DWORD dwBytes1, dwBytes2;
    UINT i;

    if (lpWave != NULL && lpWave->dwHandle != 0 && lpWave->lpData != NULL) {
        if (dwOffset + nCount <= lpWave->dwLength) {
            lpSoundBuffer = (LPDIRECTSOUNDBUFFER) lpWave->dwHandle;

            /* lock sound buffer in memory */
            if (lpSoundBuffer->lpVtbl->Lock(lpSoundBuffer, dwOffset, nCount,
					    &lpPtr1, &dwBytes1, &lpPtr2, &dwBytes2, 0) == DS_OK) {

                /* copy samples to the sound buffer (convert to unsigned) */
                if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
                    memcpy(lpPtr1, lpWave->lpData + dwOffset, dwBytes1);
                }
                else {
                    for (i = 0; i < dwBytes1; i++)
                        ((LPBYTE)lpPtr1)[i] = lpWave->lpData[dwOffset++] ^ 0x80;
                }

                /* unlock the sound buffer from memory */
                lpSoundBuffer->lpVtbl->Unlock(lpSoundBuffer, lpPtr1,
					      dwBytes1, lpPtr2, dwBytes2);

                return AUDIO_ERROR_NONE;
            }
            else {
                MSGBOX("Can't lock DirectSoundBuffer memory");
                lpSoundBuffer->lpVtbl->Restore(lpSoundBuffer);
            }
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, LPAUDIOWAVE lpWave)
{
    LPDIRECTSOUNDBUFFER lpSoundBuffer;
    
    if (nVoice < AUDIO_MAX_VOICES && lpWave != NULL) {
        lpSoundBuffer = (LPDIRECTSOUNDBUFFER) lpWave->dwHandle;

        /* release the sound buffer for this voice */
        if (DS.aSoundBuffer[nVoice] != NULL) {
            DS.aSoundBuffer[nVoice]->lpVtbl->Stop(DS.aSoundBuffer[nVoice]);
            DS.aSoundBuffer[nVoice]->lpVtbl->Release(DS.aSoundBuffer[nVoice]);
            DS.aSoundBuffer[nVoice] = NULL;
        }

        /* create a duplicate sound buffer */
        if (DS.lpDirectSound->lpVtbl->DuplicateSoundBuffer(DS.lpDirectSound,
							   lpSoundBuffer, &DS.aSoundBuffer[nVoice]) != DS_OK) {
            MSGBOX("Can't duplicate DirectSoundBuffer object");
            return AUDIO_ERROR_NOMEMORY;
        }
        
        /* setup frequency, volume and panning */
        DS.aSoundBuffer[nVoice]->lpVtbl->SetCurrentPosition(DS.aSoundBuffer[nVoice], 0);
        DS.aSoundBuffer[nVoice]->lpVtbl->SetFrequency(DS.aSoundBuffer[nVoice],
						      DS.aFrequencyTable[nVoice]);
        DS.aSoundBuffer[nVoice]->lpVtbl->SetVolume(DS.aSoundBuffer[nVoice],
						   DS.aLogVolumeTable[DS.aVolumeTable[nVoice]]);
        DS.aSoundBuffer[nVoice]->lpVtbl->SetPan(DS.aSoundBuffer[nVoice],
						DS.aLogPanningTable[DS.aPanningTable[nVoice]]);

        /* save format of the sound buffer */
        DS.aFormatTable[nVoice] = lpWave->wFormat;

        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    DWORD dwFlags;

    if (nVoice < AUDIO_MAX_VOICES) {
        // FIXME: handle looping samples!
        if (DS.aSoundBuffer[nVoice] != NULL) {
            dwFlags = (DS.aFormatTable[nVoice] & AUDIO_FORMAT_LOOP ? DSBPLAY_LOOPING : 0);
            if (DS.aSoundBuffer[nVoice]->lpVtbl->Play(DS.aSoundBuffer[nVoice],
						      0, 0, dwFlags) != DS_OK) {
                MSGBOX("Can't play DirectSoundBuffer object");
                return AUDIO_ERROR_INVALHANDLE;
            }
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (DS.aSoundBuffer[nVoice] != NULL) {
            if (DS.aSoundBuffer[nVoice]->lpVtbl->Stop(DS.aSoundBuffer[nVoice]) != DS_OK) {
                MSGBOX("Can't stop DirectSoundBuffer object");
                return AUDIO_ERROR_INVALHANDLE;
            }
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (dwPosition >= AUDIO_MIN_POSITION &&
            dwPosition <= AUDIO_MAX_POSITION) {
            // FIXME: adjust position for 16-bit samples
            if (DS.aSoundBuffer[nVoice] != NULL) {
                DS.aSoundBuffer[nVoice]->lpVtbl->SetCurrentPosition(DS.aSoundBuffer[nVoice], dwPosition);
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceFrequency(UINT nVoice, LONG dwFrequency)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (dwFrequency >= AUDIO_MIN_FREQUENCY &&
            dwFrequency <= AUDIO_MAX_FREQUENCY) {
            DS.aFrequencyTable[nVoice] = dwFrequency;
            if (DS.aSoundBuffer[nVoice] != NULL) {
                if (DS.aSoundBuffer[nVoice]->lpVtbl->SetFrequency(DS.aSoundBuffer[nVoice], dwFrequency) != DS_OK)
                    MSGBOX("Can't change DirectSoundBuffer frequency");
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceVolume(UINT nVoice, UINT nVolume)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (nVolume < AUDIO_MAX_VOLUME) {
            DS.aVolumeTable[nVoice] = nVolume;
            if (DS.aSoundBuffer[nVoice] != NULL) {
                if (DS.aSoundBuffer[nVoice]->lpVtbl->SetVolume(DS.aSoundBuffer[nVoice], DS.aLogVolumeTable[nVolume]) != DS_OK)
                    MSGBOX("Can't change DirectSoundBuffer volume");
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePanning(UINT nVoice, UINT nPanning)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (nPanning < AUDIO_MAX_PANNING) {
            DS.aPanningTable[nVoice] = nPanning;
            if (DS.aSoundBuffer[nVoice] != NULL) {
                if (DS.aSoundBuffer[nVoice]->lpVtbl->SetPan(DS.aSoundBuffer[nVoice], DS.aLogPanningTable[nPanning]) != DS_OK)
                    MSGBOX("Can't change DirectSoundBuffer panning");
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, LPLONG lpdwPosition)
{
    DWORD dwWritePosition;

    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpdwPosition != NULL) {
            *lpdwPosition = 0L;
            if (DS.aSoundBuffer[nVoice] != NULL) {
                if (DS.aSoundBuffer[nVoice]->lpVtbl->GetCurrentPosition(
                    DS.aSoundBuffer[nVoice], lpdwPosition, &dwWritePosition) != DS_OK)
                    *lpdwPosition = 0L;
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, LPLONG lpdwFrequency)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpdwFrequency != NULL) {
            *lpdwFrequency = DS.aFrequencyTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, LPUINT lpnVolume)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpnVolume != NULL) {
            *lpnVolume = DS.aVolumeTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, LPUINT lpnPanning)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpnPanning != NULL) {
            *lpnPanning = DS.aPanningTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, LPBOOL lpnStatus)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpnStatus != NULL) {
            *lpnStatus = 1;
            if (DS.aSoundBuffer[nVoice] != NULL) {
                if (DS.aSoundBuffer[nVoice]->lpVtbl->GetStatus(DS.aSoundBuffer[nVoice], lpnStatus) != DS_OK) 
                    MSGBOX("Can't getDirectSoundBuffer status");;
                *lpnStatus = (*lpnStatus & DSBSTATUS_PLAYING ? 0 : 1);
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}


/*
 * DirectSound audio driver public interface
 */
static AUDIOWAVEDRIVER DirectSoundWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

static AUDIOSYNTHDRIVER DirectSoundSynthDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudioSynth, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate, SetAudioMixerValue,
    GetAudioDataAvail, CreateAudioData, DestroyAudioData,
    WriteAudioData, PrimeVoice, StartVoice, StopVoice,
    SetVoicePosition, SetVoiceFrequency, SetVoiceVolume,
    SetVoicePanning, GetVoicePosition, GetVoiceFrequency,
    GetVoiceVolume, GetVoicePanning, GetVoiceStatus
};

AUDIODRIVER DirectSoundAccelDriver =
{
    &DirectSoundWaveDriver, &DirectSoundSynthDriver
};
