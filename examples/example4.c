/* example4.c - play a waveform file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio.h>

#if defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__DJGPP__)
#include <conio.h>
#else
#define kbhit() 0
#endif

int main(void)
{
    AUDIOINFO info;
    PAUDIOWAVE pWave;
    HAC hVoice;
    BOOL stopped;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* load waveform file */
    ALoadWaveFile("test.wav", &pWave);

    /* open and allocate a voice */
    AOpenVoices(1);
    ACreateAudioVoice(&hVoice);

    /* play the waveform through a voice */
    APlayVoice(hVoice, pWave);
    ASetVoiceVolume(hVoice, 64);
    ASetVoicePanning(hVoice, 128);

    /* program main execution loop */
    printf("Playing waveform.\n");
    while (!kbhit()) {
        /* update audio system */
        AUpdateAudio();

        /* check if the waveform is stopped */
        AGetVoiceStatus(hVoice, &stopped);
        if (stopped) break;
    }

    /* stop the voice */
    AStopVoice(hVoice);

    /* release and close the voice */
    ADestroyAudioVoice(hVoice);
    ACloseVoices();

    /* release the waveform file */
    AFreeWaveFile(pWave);

    /* close audio device */
    ACloseAudio();
    return 0;
}
