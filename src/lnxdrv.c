/*
 * $Id: lnxdrv.c 1.4 1996/05/24 08:30:44 chasan released $
 *
 * Linux's Voxware audio driver.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
    UCHAR   aBuffer[BUFFERSIZE];
    PFNAUDIOWAVE pfnAudioWave;
} Audio;


/*
 * Linux driver API interface
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
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

    memcpy(pCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return access("/dev/dsp", W_OK) ? AUDIO_ERROR_NODEVICE : AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
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
    nBitsPerSample = pInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
    nStereoOn = pInfo->wFormat & AUDIO_FORMAT_STEREO ? 1 : 0;
    nSampleRate = pInfo->nSampleRate;
    if (ioctl(Audio.nHandle, SNDCTL_DSP_SAMPLESIZE, &nBitsPerSample) < 0 ||
        ioctl(Audio.nHandle, SNDCTL_DSP_STEREO, &nStereoOn) < 0 ||
        ioctl(Audio.nHandle, SNDCTL_DSP_SPEED, &nSampleRate) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_BADFORMAT;
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    /* close DSP audio device */
    close(Audio.nHandle);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    /* send PCM samples to the DSP audio device */
    if (Audio.pfnAudioWave != NULL) {
        Audio.pfnAudioWave(Audio.aBuffer, sizeof(Audio.aBuffer));
        write(Audio.nHandle, Audio.aBuffer, sizeof(Audio.aBuffer));
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    /* set up DSP audio device user's callback function */
    Audio.pfnAudioWave = pfnAudioWave;
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
