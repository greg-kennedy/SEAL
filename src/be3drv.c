/*
 * $Id: be3drv.c 1.0 1998/10/12 00:33:08 chasan Exp $
 *
 * Intel BeOS Release 3 audio driver.
 *
 * Copyright (C) 1998-1999 Carlos Hasan
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
#include <dirent.h>
#include <sys/ioctl.h>
#include "BeOS/old_audio_driver.h"
#include "audio.h"
#include "drivers.h"

#define BUFFERSIZE (1 << 12)

/*
 * BeOS driver configuration structure
 */
static struct {
    int     nHandle;
    char    szDeviceName[MAXNAMLEN];
    BYTE    aBuffer[sizeof(audio_buffer_header) + BUFFERSIZE];
    LPFNAUDIOWAVE lpfnAudioWave;
} Audio;


/*
 * BeOS driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_BEOSR3, "BeOS R3 Sound Driver",
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
    
    /* first try the default sound device */
    strcpy(Audio.szDeviceName, "/dev/old/sound");
    if (access(Audio.szDeviceName, W_OK) == 0)
	return AUDIO_ERROR_NONE;

    /* search an old audio device */
    if ((dir = opendir("/dev/old")) != 0) {
	while ((entry = readdir(dir)) != 0) {
	    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
		strcat(strcpy(Audio.szDeviceName, "/dev/old/"), entry->d_name);
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
    static int sample_rates[] = {
	5510, B_AUDIO_SAMPLE_RATE_5510,
	6620, B_AUDIO_SAMPLE_RATE_6620,
	8000, B_AUDIO_SAMPLE_RATE_8000,
	9600, B_AUDIO_SAMPLE_RATE_9600,
	11025, B_AUDIO_SAMPLE_RATE_11025,
	16000, B_AUDIO_SAMPLE_RATE_16000,
	18900, B_AUDIO_SAMPLE_RATE_18900,
	22050, B_AUDIO_SAMPLE_RATE_22050,
	27420, B_AUDIO_SAMPLE_RATE_27420,
	32000, B_AUDIO_SAMPLE_RATE_32000,
	33075, B_AUDIO_SAMPLE_RATE_33075,
	37800, B_AUDIO_SAMPLE_RATE_37800,
	44100, B_AUDIO_SAMPLE_RATE_44100,
	48000, B_AUDIO_SAMPLE_RATE_48000 };

    struct audio_params params;
    int index;

    memset(&Audio, 0, sizeof(Audio));

    /* search an old audio device for playback */
    if (PingAudio() != AUDIO_ERROR_NONE)
	return AUDIO_ERROR_NODEVICE;

    /* try to open audio device for playback */
    if ((Audio.nHandle = open(Audio.szDeviceName, O_WRONLY)) < 0)
        return AUDIO_ERROR_DEVICEBUSY;

    /* setup audio playback encoding format and sampling frequency */
    if (ioctl(Audio.nHandle, B_AUDIO_GET_PARAMS, &params) < 0) {
        close(Audio.nHandle);
        return AUDIO_ERROR_NODEVICE;
    }

    for (index = 0; index < 2*13; index += 2) {
	if (sample_rates[index] >= lpInfo->nSampleRate)
	    break;
    }
    lpInfo->nSampleRate = sample_rates[index]; 
    params.sample_rate = sample_rates[index+1];

    lpInfo->wFormat |= AUDIO_FORMAT_STEREO | AUDIO_FORMAT_16BITS;
    /* params.playback_format = B_AUDIO_FORMAT_16_BIT_STEREO; */

    if (ioctl(Audio.nHandle, B_AUDIO_SET_PARAMS, &params) < 0) {
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

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    audio_buffer_header *header = (audio_buffer_header *) Audio.aBuffer;

    /* compute frame size */
    nFrames <<= 2;
    if (nFrames <= 0 || nFrames > BUFFERSIZE)
        nFrames = BUFFERSIZE;

    /* send PCM samples to the DSP audio device */
    if (Audio.lpfnAudioWave != NULL) {
        Audio.lpfnAudioWave(Audio.aBuffer + sizeof(*header), nFrames);
        header->buffer_number = 0;
        header->subscriber_count = 0;
        header->reserved_1 = sizeof(*header) + nFrames;
        ioctl(Audio.nHandle, B_AUDIO_WRITE_BUFFER, header);
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
AUDIOWAVEDRIVER BeOSR3WaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER BeOSR3Driver =
{
    &BeOSR3WaveDriver, NULL
};
