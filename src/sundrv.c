/*
 * $Id: sundrv.c 1.5 1996/08/05 18:51:19 chasan released $
 *
 * SPARCstation SunOS and Solaris audio drivers.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <memory.h>
#ifdef __SOLARIS__
#include <sun/audio.io.h>
#else
#include <sun/audioio.h>
#endif
#include "audio.h"
#include "drivers.h"


/*
 * SPARC driver audio buffer size
 */
#define BUFFERSIZE      2048

/*
 * Table for 8 bit unsigned PCM linear to companded u-law conversion
 * (Reference: CCITT Recommendation G.711)
 */
static BYTE auLawTable[256] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
    0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
    0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
    0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07,
    0x07, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09,
    0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d,
    0x0d, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f,
    0x0f, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13,
    0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17,
    0x17, 0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b,
    0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f,
    0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
    0x2f, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c,
    0x3e, 0x41, 0x45, 0x49, 0x4d, 0x53, 0x5b, 0x67,
    0xff, 0xe7, 0xdb, 0xd3, 0xcd, 0xc9, 0xc5, 0xc1,
    0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0,
    0xaf, 0xae, 0xad, 0xac, 0xab, 0xaa, 0xa9, 0xa8,
    0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0,
    0x9f, 0x9f, 0x9e, 0x9e, 0x9d, 0x9d, 0x9c, 0x9c,
    0x9b, 0x9b, 0x9a, 0x9a, 0x99, 0x99, 0x98, 0x98,
    0x97, 0x97, 0x96, 0x96, 0x95, 0x95, 0x94, 0x94,
    0x93, 0x93, 0x92, 0x92, 0x91, 0x91, 0x90, 0x90,
    0x8f, 0x8f, 0x8f, 0x8f, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8d, 0x8d, 0x8d, 0x8d, 0x8c, 0x8c, 0x8c, 0x8c,
    0x8b, 0x8b, 0x8b, 0x8b, 0x8a, 0x8a, 0x8a, 0x8a,
    0x89, 0x89, 0x89, 0x89, 0x88, 0x88, 0x88, 0x88,
    0x87, 0x87, 0x87, 0x87, 0x86, 0x86, 0x86, 0x86,
    0x85, 0x85, 0x85, 0x85, 0x84, 0x84, 0x84, 0x84,
    0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82,
    0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80
};

/*
 * Table of supported sampling frequencies of dbri devices
 */
static LONG aSampleRate[] =
{
    8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000
};

/*
 * SPARC driver configuration structure
 */
static struct {
    int     nHandle;
    int     nEncoding;
    BYTE    aBuffer[BUFFERSIZE];
    LPFNAUDIOWAVE lpfnAudioWave;
    WORD    wFormat;
} Audio;


static LONG GetSampleRate(LONG nSampleRate)
{
    int n;

    /* return the nearest supported sampling frequency */
    for (n = 0; n < sizeof(aSampleRate) / sizeof(LONG) - 1; n++) {
        if (nSampleRate <= aSampleRate[n])
            break;
    }
    return aSampleRate[n];
}


/*
 * SPARC audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_SPARC, "SPARC SunOS",
#ifdef __SOLARIS__
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
#else
        AUDIO_FORMAT_1M08
#endif
    };

    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return access("/dev/audio", W_OK) ? AUDIO_ERROR_NODEVICE : AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    int dbri, type;
    audio_info_t info;
#ifdef __SOLARIS__
    audio_device_t dev;
#endif

    memset(&Audio, 0, sizeof(Audio));

    /* try to open the audio device for playback */
#ifdef __SOLARIS__
    if ((Audio.nHandle = open("/dev/audio", O_WRONLY)) < 0)
        return AUDIO_ERROR_DEVICEBUSY;
#else
    if ((Audio.nHandle = open("/dev/audio", O_WRONLY | O_NDELAY)) < 0)
        return AUDIO_ERROR_DEVICEBUSY;
#endif

    /* check whether we know about linear encoding */
#ifdef __SOLARIS__
    dbri = (ioctl(Audio.nHandle, AUDIO_GETDEV, &dev) == 0 &&
	    strcmp(dev.name, "SUNW,dbri") == 0);
#else
#ifdef AUDIO_GETDEV
    dbri = (ioctl(Audio.nHandle, AUDIO_GETDEV, &type) == 0 &&
	    type != AUDIO_DEV_AMD);
#else
    /*
     * There is no AUDIO_GETDEV under SunOS 4.1.1 so we have to
     * assume that there is an AMD device attached to /dev/audio.
     */
    dbri = 0;
#endif
#endif

    /* setup audio configuration structure */
    AUDIO_INITINFO(&info);
    if (dbri) {
        /* configure linear encoding for dbri devices */
        info.play.encoding = AUDIO_ENCODING_LINEAR;
        info.play.channels = lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1;
        info.play.precision = lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
        info.play.sample_rate = GetSampleRate(lpInfo->nSampleRate);
    }
    else {
        /* configure companded u-law encoding for AMD devices */
        info.play.encoding = AUDIO_ENCODING_ULAW;
        info.play.channels = 1;
        info.play.precision = 8;
        info.play.sample_rate = 8000;
    }

    /* commit playback encoding format */
    Audio.nEncoding = info.play.encoding;
    if (ioctl(Audio.nHandle, AUDIO_SETINFO, &info) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_BADFORMAT;
    }

    /* refresh configuration structure */
    lpInfo->wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    lpInfo->wFormat |= info.play.precision != 16 ?
        AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS;
    lpInfo->wFormat |= info.play.channels != 2 ?
        AUDIO_FORMAT_MONO : AUDIO_FORMAT_STEREO;
    lpInfo->nSampleRate = info.play.sample_rate;

    Audio.wFormat = lpInfo->wFormat;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    ioctl(Audio.nHandle, AUDIO_DRAIN, NULL);
    close(Audio.nHandle);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    int n;

    if (Audio.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (Audio.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > sizeof(Audio.aBuffer))
        nFrames = sizeof(Audio.aBuffer);

    if (Audio.lpfnAudioWave != NULL) {
        Audio.lpfnAudioWave(Audio.aBuffer, nFrames);
        if (Audio.nEncoding == AUDIO_ENCODING_ULAW) {
            for (n = 0; n < nFrames; n++) {
                Audio.aBuffer[n] = auLawTable[Audio.aBuffer[n]];
            }
        }
        write(Audio.nHandle, Audio.aBuffer, nFrames);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    Audio.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * SPARC audio driver public interface
 */
AUDIOWAVEDRIVER SparcWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER SparcDriver =
{
    &SparcWaveDriver, NULL
};
