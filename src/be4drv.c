/*
 * $Id: bedrv.c 1.1 1998/10/24 18:24:26 chasan Exp $
 *
 * Intel BeOS Release 4 audio driver.
 *
 * Copyright (C) 1998-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "BeOS/audio_driver.h"
#include "audio.h"
#include "drivers.h"

#define BUFFERSIZE 200	/* 200 ms */

/*
 * BeOS driver configuration structure
 */
static struct {
    int     nHandle;
    char    szDeviceName[MAXNAMLEN];
    LPBYTE  lpBuffer;
    int     nBufferSize;
    LPFNAUDIOWAVE lpfnAudioWave;
    WORD    wFormat;
} Audio;


/*
 * BeOS driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_BEOS, "BeOS Sound Driver",
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
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    
    /* search an audio device */
    if ((dir = opendir("/dev/audio")) != 0) {
	while ((entry = readdir(dir)) != 0) {
	    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
		strcat(strcpy(Audio.szDeviceName, "/dev/audio/"), entry->d_name);
		if (stat(Audio.szDeviceName, &st) < 0 || S_ISDIR(st.st_mode))
		    continue;
		if (access(Audio.szDeviceName, W_OK) == 0) {
		    closedir(dir);
		    return AUDIO_ERROR_NONE;
		}
	    }
	}
	closedir(dir);
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    struct audio_format config;

    memset(&Audio, 0, sizeof(Audio));

    /* search an old audio device for playback */
    if (PingAudio() != AUDIO_ERROR_NONE)
	return AUDIO_ERROR_NODEVICE;

    /* try to open audio device for playback */
    if ((Audio.nHandle = open(Audio.szDeviceName, O_WRONLY)) < 0)
        return AUDIO_ERROR_DEVICEBUSY;

    /* compute 200 ms latency buffer size */
    Audio.nBufferSize = lpInfo->nSampleRate;
    if (lpInfo->wFormat & AUDIO_FORMAT_16BITS)
	Audio.nBufferSize <<= 1;
    if (lpInfo->wFormat & AUDIO_FORMAT_STEREO)
	Audio.nBufferSize <<= 1;
    Audio.nBufferSize /= (1000 / BUFFERSIZE);
    Audio.nBufferSize += 15;
    Audio.nBufferSize &= ~15;

    /* setup audio playback encoding format and sampling frequency */
    config.sample_rate = lpInfo->nSampleRate;
    config.channels = (lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1);
    config.format = (lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 0x02 : 0x11);
    config.big_endian = 0;
    config.buf_header = 0;
    config.play_buf_size = Audio.nBufferSize;
    config.rec_buf_size = Audio.nBufferSize;
    if (ioctl(Audio.nHandle, B_AUDIO_SET_AUDIO_FORMAT, &config) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_NODEVICE;
    }
    if (ioctl(Audio.nHandle, B_AUDIO_GET_AUDIO_FORMAT, &config) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_NODEVICE;
    }
    lpInfo->nSampleRate = config.sample_rate;
    lpInfo->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
    if (config.format == 0x02)
        lpInfo->wFormat |= AUDIO_FORMAT_16BITS;
    if (config.channels == 2)
        lpInfo->wFormat |= AUDIO_FORMAT_STEREO;

    /* allocate audio buffer area */
    if ((Audio.lpBuffer = (LPBYTE) malloc(Audio.nBufferSize)) == NULL) {
        close(Audio.nHandle);
        return AUDIO_ERROR_NOMEMORY;
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
    if (nFrames <= 0 || nFrames > Audio.nBufferSize)
        nFrames = Audio.nBufferSize;

    /* send PCM samples to the DSP audio device */
    if (Audio.lpfnAudioWave != NULL) {
        Audio.lpfnAudioWave(Audio.lpBuffer, nFrames);
        write(Audio.nHandle, Audio.lpBuffer, nFrames);
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
