/*
 * $Id: dsdrv.c 1.5 1997/01/05 16:23:47 chasan Exp $
 *
 * Microsoft DirectSound audio driver interface
 *
 * Copyright (c) 1996-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define MSGBOX(text)

#define INITGUID
#include <windows.h>
#include <objbase.h>
#include <dsound.h>
#include "audio.h"
#include "drivers.h"

static struct {
    LPDIRECTSOUND   lpDirectSound;
    LPDIRECTSOUNDBUFFER lpPrimaryBuffer;
    LPDIRECTSOUNDBUFFER lpSoundBuffer;
    LPFNAUDIOWAVE   lpfnAudioWave;
    DWORD           dwBufferOffset;
    DWORD           dwBufferSize;

    WORD            nSampleRate;
    WORD            wFormat;
} DS;

static HRESULT _DirectSoundCreate(GUID FAR * lpGuid,
				  LPDIRECTSOUND *lplpDirectSound, IUnknown FAR * lpUnkOuter)
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

static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    strncpy(lpCaps->szProductName, "DirectSound",
	    sizeof(lpCaps->szProductName));
    lpCaps->wProductId = AUDIO_PRODUCT_DSOUND;
    lpCaps->dwFormats =
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16 ;
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
    DSBUFFERDESC dsbd;
    DSBCAPS dsbc;
    DSCAPS dsc;
    WAVEFORMATEX wfx;

    memset(&DS, 0, sizeof(DS));

    /* create the direct sound object */
    if (_DirectSoundCreate(NULL, &DS.lpDirectSound, NULL) != DS_OK) {
        MSGBOX("Failed to create DirectSound COM object");
        return AUDIO_ERROR_NODEVICE;
    }

    /* set direct sound cooperative level */
    if (DS.lpDirectSound->lpVtbl->SetCooperativeLevel(
	DS.lpDirectSound, GetForegroundWindow(), DSSCL_PRIORITY) != DS_OK) {
        MSGBOX("DirectSound SetCooperativeLevel failed");
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* get DirectSound capabilities */
    dsc.dwSize = sizeof(dsc);
    if (DS.lpDirectSound->lpVtbl->GetCaps(DS.lpDirectSound, &dsc) != DS_OK) {
        MSGBOX("Cannot get DirectSound capabilities");
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }
    if (!(dsc.dwFlags & DSCAPS_PRIMARYSTEREO))
        lpInfo->wFormat &= ~AUDIO_FORMAT_STEREO;
    if (!(dsc.dwFlags & DSCAPS_SECONDARY16BIT))
        lpInfo->wFormat &= ~AUDIO_FORMAT_16BITS;

    /* set up wave format structure */
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
    if (DS.lpDirectSound->lpVtbl->CreateSoundBuffer(
	DS.lpDirectSound, &dsbd, &DS.lpPrimaryBuffer, NULL) != DS_OK) {
        MSGBOX("Cannot create primary buffer");
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* set up primary buffer format */
    while (DS.lpPrimaryBuffer->lpVtbl->SetFormat(DS.lpPrimaryBuffer, &wfx) != DS_OK) {
        if (lpInfo->nSampleRate <= 11025) {
            MSGBOX("Can't change primary buffer format");
            DS.lpPrimaryBuffer->lpVtbl->Release(DS.lpPrimaryBuffer);
            _DirectSoundDestroy(DS.lpDirectSound);
            return AUDIO_ERROR_NODEVICE;
        }
        wfx.nSamplesPerSec >>= 1;
        wfx.nAvgBytesPerSec >>= 1;
        wfx.nBlockAlign >>= 1;
        lpInfo->nSampleRate >>= 1;
    }

    /* get primary buffer length */
    dsbc.dwSize = sizeof(dsbc);
    if (DS.lpPrimaryBuffer->lpVtbl->GetCaps(DS.lpPrimaryBuffer, &dsbc) != DS_OK) {
        MSGBOX("Can't get primary buffer capabilities");
        DS.lpPrimaryBuffer->lpVtbl->Release(DS.lpPrimaryBuffer);
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* the secondary buffer size should be between 20 ms and 100 ms */
    if (dsbc.dwBufferBytes > (100L * wfx.nAvgBytesPerSec) / 1000) {
	dsbc.dwBufferBytes = (100L * wfx.nAvgBytesPerSec) / 1000;
    }
    if (dsbc.dwBufferBytes < (20L * wfx.nAvgBytesPerSec) / 1000) {
	dsbc.dwBufferBytes = (20L * wfx.nAvgBytesPerSec) / 1000;
    }

    /* [1998/12/24] in emulation mode use at least 150 ms buffers */
    if (dsc.dwFlags & DSCAPS_EMULDRIVER) {
	if (dsbc.dwBufferBytes < (150L * wfx.nAvgBytesPerSec) / 1000)
	    dsbc.dwBufferBytes = (150L * wfx.nAvgBytesPerSec) / 1000;
    }

    dsbc.dwBufferBytes >>= 2;
    dsbc.dwBufferBytes <<= 2;

    /* set up secondary buffer description */
    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    dsbd.dwBufferBytes = dsbc.dwBufferBytes;
    dsbd.lpwfxFormat = &wfx;

    /* create secondary sound buffer */
    if (DS.lpDirectSound->lpVtbl->CreateSoundBuffer(
	DS.lpDirectSound, &dsbd, &DS.lpSoundBuffer, NULL) != DS_OK) {
        MSGBOX("Can't create secondary buffer");
        _DirectSoundDestroy(DS.lpDirectSound);
        return AUDIO_ERROR_NODEVICE;
    }

    /* set up buffer chunk variables */
    DS.dwBufferOffset = 0L;
    DS.dwBufferSize = dsbd.dwBufferBytes;

    /* start playing the primary buffer */
    DS.lpPrimaryBuffer->lpVtbl->Play(DS.lpPrimaryBuffer, 0, 0, DSBPLAY_LOOPING);

    /* start playing the secondary buffer */
    DS.lpSoundBuffer->lpVtbl->Play(DS.lpSoundBuffer, 0, 0, DSBPLAY_LOOPING);

    /* save audio format settings */
    DS.nSampleRate = lpInfo->nSampleRate;
    DS.wFormat = lpInfo->wFormat;
    
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    if (DS.lpDirectSound != NULL) {
        /* release secondary sound buffer */
        if (DS.lpSoundBuffer != NULL) {
            DS.lpSoundBuffer->lpVtbl->Stop(DS.lpSoundBuffer);
            DS.lpSoundBuffer->lpVtbl->Release(DS.lpSoundBuffer);
        }

        /* release primary sound buffer */
        if (DS.lpPrimaryBuffer != NULL) {
            DS.lpPrimaryBuffer->lpVtbl->Stop(DS.lpPrimaryBuffer);
            DS.lpPrimaryBuffer->lpVtbl->Release(DS.lpPrimaryBuffer);
        }

        /* release direct sound */
        _DirectSoundDestroy(DS.lpDirectSound);
    }
    memset(&DS, 0, sizeof(DS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    LPVOID lpPtr1, lpPtr2;
    DWORD dwBytes1, dwBytes2;
    DWORD dwPlayOffset, dwWriteOffset;
    LONG dwBytes;

    if (DS.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (DS.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (nFrames <= 0 || nFrames >= DS.dwBufferSize)
        nFrames = DS.dwBufferSize;

    if (DS.lpSoundBuffer->lpVtbl->GetCurrentPosition(
        DS.lpSoundBuffer, &dwPlayOffset, &dwWriteOffset) == DS_OK) {

        if ((dwBytes = dwPlayOffset - DS.dwBufferOffset) < 0)
            dwBytes += DS.dwBufferSize;
        if (dwBytes > nFrames)
            dwBytes = nFrames;
        dwBytes &= ~3;

        if ((dwBytes -= 128) > 0) {

            if (DS.lpSoundBuffer->lpVtbl->Lock(
                DS.lpSoundBuffer, DS.dwBufferOffset, dwBytes,
                &lpPtr1, &dwBytes1, &lpPtr2, &dwBytes2, 0) == DS_OK) {

                if (DS.lpfnAudioWave != NULL) {
                    DS.lpfnAudioWave(lpPtr1, dwBytes1);
                    if (lpPtr2 != NULL)
                        DS.lpfnAudioWave(lpPtr2, dwBytes2);
                }
                else {
                    memset(lpPtr1, 0, dwBytes1);
                    if (lpPtr2 != NULL)
                        memset(lpPtr2, 0, dwBytes2);
                }

                DS.lpSoundBuffer->lpVtbl->Unlock(DS.lpSoundBuffer,
						 lpPtr1, dwBytes1, lpPtr2, dwBytes2);

                if ((DS.dwBufferOffset += dwBytes) >= DS.dwBufferSize)
                    DS.dwBufferOffset -= DS.dwBufferSize;
            }
            else {
                DS.lpSoundBuffer->lpVtbl->Restore(DS.lpSoundBuffer);
            }
        }
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    DS.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}



/*
 * DirectSound driver public interfaces
 */
AUDIOWAVEDRIVER DirectSoundWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER DirectSoundDriver =
{
    &DirectSoundWaveDriver, NULL
};
