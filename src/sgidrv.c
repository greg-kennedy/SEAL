/*
 * $Id: sgidrv.c 1.5 1996/08/05 18:51:19 chasan released $
 *
 * Silicon Graphics Indigo audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <audio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "audio.h"
#include "drivers.h"

/*
 * SGI audio driver buffer size
 */
#define BUFFERSIZE      2048

/*
 * SGI audio driver configuration structure
 */
static struct {
    ALport   Port;
    ALconfig Config;
    WORD     wFormat;
    BYTE     aBuffer[BUFFERSIZE];
    LPFNAUDIOWAVE lpfnAudioWave;
} Audio;


/*
 * SGI audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_SGI, "Silicon Graphics",
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
    long op[2];

    memset(&Audio, 0, sizeof(Audio));

    if ((Audio.Config = ALnewconfig()) != NULL) {
        /* setup encoding format for playback */
        ALsetwidth(Audio.Config, lpInfo->wFormat & AUDIO_FORMAT_16BITS ?
		   AL_SAMPLE_16 : AL_SAMPLE_8);
        ALsetchannels(Audio.Config, lpInfo->wFormat & AUDIO_FORMAT_STEREO ?
		      AL_STEREO : AL_MONO);

        /* open audio port and configure it */
        Audio.Port = ALopenport("Audio Port", "w", Audio.Config);
        Audio.wFormat = lpInfo->wFormat;

        /* setup playback sampling frequency */
        if (Audio.Port != NULL) {
            op[0] = AL_OUTPUT_RATE;
            op[1] = lpInfo->nSampleRate;
            ALsetparms(AL_DEFAULT_DEVICE, op, 2);
            return AUDIO_ERROR_NONE;
        }
    }
    return AUDIO_ERROR_DEVICEBUSY;
}

static UINT AIAPI CloseAudio(VOID)
{
    while (ALgetfilled(Audio.Port) > 0)
        sleep(1);
    ALcloseport(Audio.Port);
    ALfreeconfig(Audio.Config);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    if (Audio.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (Audio.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames >= sizeof(Audio.aBuffer))
        nFrames = sizeof(Audio.aBuffer);

    if (Audio.lpfnAudioWave != NULL) {
        Audio.lpfnAudioWave(Audio.aBuffer, nFrames);
        ALwritesamps(Audio.Port, Audio.aBuffer, nFrames >>
		     ((Audio.wFormat & AUDIO_FORMAT_16BITS) != 0));
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    Audio.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * SGI audio driver public interface
 */
AUDIOWAVEDRIVER SiliconWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER SiliconDriver =
{
    &SiliconWaveDriver, NULL
};
