/* example4.c - play module and waveform file */

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
    LPAUDIOMODULE lpModule;
    LPAUDIOWAVE lpWave;
    HAC hVoice;
    BOOL stopped;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* load module and waveform file */
    ALoadModuleFile("test.s3m", &lpModule, 0);
    ALoadWaveFile("test.wav", &lpWave, 0);

    /* open voices for module and waveform */
    AOpenVoices(lpModule->nTracks + 1);

    /* play the module file */
    APlayModule(lpModule);
    ASetModuleVolume(64);

    /* play the waveform through a voice */
    ACreateAudioVoice(&hVoice);
    APlayVoice(hVoice, lpWave);
    ASetVoiceVolume(hVoice, 48);
    ASetVoicePanning(hVoice, 128);

    /* program main execution loop */
    printf("Playing module and waveform, press any key to stop.\n");
    while (!kbhit()) {
        /* update audio system */
        AUpdateAudio();

        /* restart waveform if stopped */
        AGetVoiceStatus(hVoice, &stopped);
        if (stopped) APlayVoice(hVoice, lpWave);

        /* check if the module is stopped */
        AGetModuleStatus(&stopped);
        if (stopped) break;
    }

    /* stop playing the waveform */
    AStopVoice(hVoice);
    ADestroyAudioVoice(hVoice);

    /* stop playing the module */
    AStopModule();
    ACloseVoices();

    /* release the waveform & module */
    AFreeWaveFile(lpWave);
    AFreeModuleFile(lpModule);

    /* close audio device */
    ACloseAudio();
    return 0;
}
