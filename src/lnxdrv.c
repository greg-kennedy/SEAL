/*
 * $Id: lnxdrv.c 1.5 1996/08/05 18:51:19 chasan released $
 *
 * Linux's Voxware audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __LINUX__
#include <linux/soundcard.h>
#endif
#ifdef __FREEBSD__
#include <machine/soundcard.h>
#endif
#include "audio.h"
#include "drivers.h"


/*
 * Linux driver buffer fragments defines
 */
#define NUMFRAGS        16
#define FRAGSIZE        12
#define BUFFERSIZE      (1 << FRAGSIZE)

/*
 * Linux driver configuration structure
 */
static struct {
    int     nHandle;
    BYTE    aBuffer[BUFFERSIZE];
    LPFNAUDIOWAVE lpfnAudioWave;
    WORD    wFormat;
} Audio;


/*
 * Linux driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_LINUX, "Linux Voxware",
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
    return access("/dev/dsp", W_OK) ? AUDIO_ERROR_NODEVICE : AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    int nBitsPerSample, nStereoOn, nSampleRate, nFrags;

    memset(&Audio, 0, sizeof(Audio));

    /* try to open DSP audio device for playback */
    if ((Audio.nHandle = open("/dev/dsp", O_WRONLY | O_NDELAY)) < 0)
        return AUDIO_ERROR_DEVICEBUSY;

    /* setup number and size of buffer fragments */
    nFrags = (NUMFRAGS << 16) + (FRAGSIZE);
    ioctl(Audio.nHandle, SNDCTL_DSP_SETFRAGMENT, &nFrags);

    /* setup audio playback encoding format and sampling frequency */
    nBitsPerSample = lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
    nStereoOn = lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 1 : 0;
    nSampleRate = lpInfo->nSampleRate;
    if (ioctl(Audio.nHandle, SNDCTL_DSP_SAMPLESIZE, &nBitsPerSample) < 0 ||
        ioctl(Audio.nHandle, SNDCTL_DSP_STEREO, &nStereoOn) < 0 ||
        ioctl(Audio.nHandle, SNDCTL_DSP_SPEED, &nSampleRate) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_BADFORMAT;
    }

    Audio.wFormat = lpInfo->wFormat;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    /* close DSP audio device */
    close(Audio.nHandle);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    /* compute frame size */
    if (Audio.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (Audio.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > sizeof(Audio.aBuffer))
        nFrames = sizeof(Audio.aBuffer);

    /* send PCM samples to the DSP audio device */
    if (Audio.lpfnAudioWave != NULL) {
        Audio.lpfnAudioWave(Audio.aBuffer, nFrames);
        write(Audio.nHandle, Audio.aBuffer, nFrames);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    /* set up DSP audio device user's callback function */
    Audio.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * Linux driver public interface
 */
AUDIOWAVEDRIVER LinuxWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER LinuxDriver =
{
    &LinuxWaveDriver, NULL
};
