/*
 * $Id: sgidrv.c 1.4 1996/05/24 08:30:44 chasan released $
 *
 * Silicon Graphics Indigo audio driver.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
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
    USHORT   wFormat;
    UCHAR    aBuffer[BUFFERSIZE];
    PFNAUDIOWAVE pfnAudioWave;
} Audio;


/*
 * SGI audio driver API interface
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
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

    memcpy(pCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    long op[2];

    memset(&Audio, 0, sizeof(Audio));

    if ((Audio.Config = ALnewconfig()) != NULL) {
        /* setup encoding format for playback */
        ALsetwidth(Audio.Config, pInfo->wFormat & AUDIO_FORMAT_16BITS ?
            AL_SAMPLE_16 : AL_SAMPLE_8);
        ALsetchannels(Audio.Config, pInfo->wFormat & AUDIO_FORMAT_STEREO ?
            AL_STEREO : AL_MONO);

        /* open audio port and configure it */
        Audio.Port = ALopenport("Audio Port", "w", Audio.Config);
        Audio.wFormat = pInfo->wFormat;

        /* setup playback sampling frequency */
        if (Audio.Port != NULL) {
            op[0] = AL_OUTPUT_RATE;
            op[1] = pInfo->nSampleRate;
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

static UINT AIAPI UpdateAudio(VOID)
{
    if (Audio.pfnAudioWave != NULL) {
        Audio.pfnAudioWave(Audio.aBuffer, sizeof(Audio.aBuffer));
        ALwritesamps(Audio.Port, Audio.aBuffer, sizeof(Audio.aBuffer) >>
            ((Audio.wFormat & AUDIO_FORMAT_16BITS) != 0));
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    Audio.pfnAudioWave = pfnAudioWave;
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
