/*
 * $Id: bedrv.c 0.1 1998/12/19 18:24:26 chasan Exp $
 *
 * Intel BeOS Release 4 audio driver using the New Media Kit.
 *
 * Copyright (C) 1998 Carlos Hasan. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaDefs.h>
#include <SoundPlayer.h>
#include "audio.h"
#include "drivers.h"

/*
 * BeOS driver configuration structure
 */
static struct {
    BSoundPlayer *Player;
    LPFNAUDIOWAVE lpfnAudioWave;
} Audio;


/*
 * BeOS driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_BEOS, "BeOS Sound Player",
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

static void PlayAudio(void *cookie, void *buffer, size_t size,
                      const media_raw_audio_format& fmt)
{
    static float hitable[256], lotable[256];
    static float scale = 0.0f;

    float *float_buffer = (float *) buffer;
    short *short_buffer = (short *) buffer + (size >> 2);
    size >>= 2;

    /* lazily build the integer to float translation table */
    if (scale == 0.0f) {
        int i;
        scale = 1.0f / 32768.0f;
        for (i = -128; i < 128; i++)
            hitable[i & 255] = scale * 256.0f * i;
        for (i = 0; i < 255; i++)
            lotable[i] = scale * i;
    }

    /* streams of 44100 Hz, stereo, 32bit floats are supported */
    if (fmt.format != media_raw_audio_format::B_AUDIO_FLOAT ||
        fmt.channel_count != 2 || fmt.frame_rate != 44100 ||
        Audio.lpfnAudioWave == NULL) {
        memset(buffer, 0, size);
    }
    else {
        /* make 44100 Hz, stereo, 16-bit signed integer samples */
        Audio.lpfnAudioWave((LPBYTE) short_buffer, size << 1);

        /* convert to 32-bit float samples using lookup table */
        while (size >= 8) {
            int *sample = (int *) short_buffer;
            int s;

            /* TODO: channels will be swapped in big-endian machines */
            s = sample[0];
            float_buffer[0] = hitable[(s >> 8) & 255] + lotable[s & 255];
            float_buffer[1] = hitable[(s >> 24) & 255] + lotable[(s >> 16)&255];
            s = sample[1];
            float_buffer[2] = hitable[(s >> 8) & 255] + lotable[s & 255];
            float_buffer[3] = hitable[(s >> 24) & 255] + lotable[(s >> 16)&255];
            s = sample[2];
            float_buffer[4] = hitable[(s >> 8) & 255] + lotable[s & 255];
            float_buffer[5] = hitable[(s >> 24) & 255] + lotable[(s >> 16)&255];
            s = sample[3];
            float_buffer[6] = hitable[(s >> 8) & 255] + lotable[s & 255];
            float_buffer[7] = hitable[(s >> 24) & 255] + lotable[(s >> 16)&255];

            short_buffer += 8;
            float_buffer += 8;
            size -= 8;
        }
        while (size-- > 0) {
            short s = *short_buffer++;
            *float_buffer++ = hitable[(s >> 8) & 255] + lotable[s & 255];
        }
    }
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    /* clean up local state structure */
    memset(&Audio, 0, sizeof(Audio));

    /* create sound player */
    if ((Audio.Player = new BSoundPlayer("SEAL Player", PlayAudio)) == NULL)
	return AUDIO_ERROR_NODEVICE;

    /* force default settings */
    lpInfo->nSampleRate = 44100;
    lpInfo->wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;

    /* start sound player */
    Audio.Player->SetHasData(true);
    Audio.Player->Start();

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    if (Audio.Player != NULL) {
        /* stop sound player */
        Audio.Player->Stop();

        /* release sound player */
        delete Audio.Player;
        Audio.Player = NULL;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    /* set up audio device user's callback function */
    Audio.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * BeOS driver public interface
 */
AUDIOWAVEDRIVER BeOSWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER BeOSDriver =
{
    &BeOSWaveDriver, NULL
};
