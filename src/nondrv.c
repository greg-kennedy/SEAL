/*
 * $Id: nondrv.c 1.4 1996/05/24 08:30:44 chasan released $
 *
 * Silence(tm) audio driver.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <string.h>
#include "audio.h"
#include "drivers.h"


/*
 * Silence(tm) audio driver API interface
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_NONE, "Silence",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    memcpy(pCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    return (pInfo != NULL) ? AUDIO_ERROR_NONE : AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    if (nVoices < AUDIO_MAX_VOICES) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    if (pfnAudioWave != NULL) {
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerProc(PFNAUDIOTIMER pfnAudioTimer)
{
    if (pfnAudioTimer != NULL) {
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nRate)
{
    if (nRate >= 0x20 && nRate <= 0xFF) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static LONG AIAPI GetAudioDataAvail(VOID)
{
    return 0L;
}

static UINT AIAPI CreateAudioData(PAUDIOWAVE pWave)
{
    if (pWave != NULL) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI DestroyAudioData(PAUDIOWAVE pWave)
{
    if (pWave != NULL) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI WriteAudioData(PAUDIOWAVE pWave, ULONG dwOffset, UINT nCount)
{
    if (pWave != NULL && pWave->pData != NULL) {
        if (dwOffset + nCount < pWave->dwLength) {
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, PAUDIOWAVE pWave)
{
    if (nVoice < AUDIO_MAX_VOICES && pWave != NULL) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (dwPosition >= AUDIO_MIN_POSITION &&
            dwPosition <= AUDIO_MAX_POSITION) {
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
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, PLONG pdwPosition)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (pdwPosition != NULL) {
            *pdwPosition = 0L;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, PLONG pdwFrequency)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (pdwFrequency != NULL) {
            *pdwFrequency = 0L;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, PUINT pnVolume)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (pnVolume != NULL) {
            *pnVolume = 0;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, PUINT pnPanning)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (pnPanning != NULL) {
            *pnPanning = 0;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, PBOOL pnStatus)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (pnStatus != NULL) {
            *pnStatus = 0;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}


/*
 * Silence(tm) audio driver public interface
 */
AUDIOWAVEDRIVER NoneWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIOSYNTHDRIVER NoneSynthDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate,
    GetAudioDataAvail, CreateAudioData, DestroyAudioData,
    WriteAudioData, PrimeVoice, StartVoice, StopVoice,
    SetVoicePosition, SetVoiceFrequency, SetVoiceVolume,
    SetVoicePanning, GetVoicePosition, GetVoiceFrequency,
    GetVoiceVolume, GetVoicePanning, GetVoiceStatus
};

AUDIODRIVER NoneDriver =
{
    &NoneWaveDriver, &NoneSynthDriver
};
