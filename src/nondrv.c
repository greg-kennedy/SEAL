/*
 * $Id: nondrv.c 1.6 1997/01/09 23:08:57 chasan Exp $
 *               1.7 1998/10/24 18:20:54 chasan Exp (Mixer API)
 *
 * Silence(tm) audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include "audio.h"
#include "drivers.h"

#ifndef __DPMI__
#include <time.h>
#else
#include "msdos.h"
#define AAD(val) (((val)&15)+10*((val)>>4))
typedef unsigned long time_t;
static time_t time(time_t *t)
{
    long secs, val;

    /* secs */
    OUTB(0x70, 0x00);
    val = INB(0x71);
    secs = AAD(val);

    /* min */
    OUTB(0x70, 0x02);
    val = INB(0x71);
    secs += 60L * AAD(val);

    /* hour */
    OUTB(0x70, 0x04);
    val = INB(0x71);
    secs += 60L * 60L * AAD(val);

    /* day */
    OUTB(0x70, 0x07);
    val = INB(0x71);
    secs += 24L * 60L * 60L * AAD(val);

    /* month */
    OUTB(0x70, 0x08);
    val = INB(0x71);
    secs += 30L * 24L * 60L * 60L * AAD(val);

    if (t != NULL) *t = secs;
    return secs;
}
#endif



static struct {
    LONG    dwTimer;
    LONG    dwTimerAccum;
    LONG    dwTimerRate;
    LPFNAUDIOTIMER lpfnTimerHandler;
} none;


/*
 * Silence(tm) audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
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

    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    memset(&none, 0, sizeof(none));
    none.dwTimer = 1000L * time(NULL);
    return (lpInfo != NULL) ? AUDIO_ERROR_NONE : AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    LONG dwTimer = 1000L * time(NULL);

    if ((none.dwTimerAccum += dwTimer - none.dwTimer) >= none.dwTimerRate) {
        none.dwTimerAccum -= none.dwTimerRate;
        if (none.lpfnTimerHandler != NULL)
            none.lpfnTimerHandler();
    }
    none.dwTimer = dwTimer;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudioSynth(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioMixerValue(UINT nChannel, UINT nValue)
{
    if (nChannel != AUDIO_MIXER_MASTER_VOLUME &&
        nChannel != AUDIO_MIXER_TREBLE &&
        nChannel != AUDIO_MIXER_BASS &&
        nChannel != AUDIO_MIXER_CHORUS &&
        nChannel != AUDIO_MIXER_REVERB)
        return AUDIO_ERROR_INVALPARAM;
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

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    if (lpfnAudioWave != NULL) {
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    if (lpfnAudioTimer != NULL) {
        none.lpfnTimerHandler = lpfnAudioTimer;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nRate)
{
    if (nRate >= 0x20 && nRate <= 0xFF) {
        /* set timer rate in milliseconds */
        none.dwTimerRate = 60000L / (24 * nRate);
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
    if (lpWave != NULL) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI DestroyAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI WriteAudioData(LPAUDIOWAVE lpWave, DWORD dwOffset, UINT nCount)
{
    if (lpWave != NULL && lpWave->lpData != NULL) {
        if (dwOffset + nCount < lpWave->dwLength) {
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, LPAUDIOWAVE lpWave)
{
    if (nVoice < AUDIO_MAX_VOICES && lpWave != NULL) {
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

static UINT AIAPI GetVoicePosition(UINT nVoice, LPLONG lpdwPosition)
{
    if (nVoice < AUDIO_MAX_VOICES) {
        if (lpdwPosition != NULL) {
            *lpdwPosition = 0L;
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
            *lpdwFrequency = 0L;
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
            *lpnVolume = 0;
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
            *lpnPanning = 0;
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
    UpdateAudioSynth, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate, SetAudioMixerValue,
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
