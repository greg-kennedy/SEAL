/* example5.c - play module and waveform file */

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
    PAUDIOMODULE pModule;
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

    /* load module and waveform file */
    ALoadModuleFile("test.s3m", &pModule);
    ALoadWaveFile("test.wav", &pWave);

    /* open voices for module and waveform */
    AOpenVoices(pModule->nTracks + 1);

    /* play the module file */
    APlayModule(pModule);
    ASetModuleVolume(64);

    /* play the waveform through a voice */
    ACreateAudioVoice(&hVoice);
    APlayVoice(hVoice, pWave);
    ASetVoiceVolume(hVoice, 48);
    ASetVoicePanning(hVoice, 128);

    /* program main execution loop */
    while (!kbhit()) {
        /* update audio system */
        AUpdateAudio();

        /* restart waveform if stopped */
        AGetVoiceStatus(hVoice, &stopped);
        if (stopped) APlayVoice(hVoice, pWave);

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
    AFreeWaveFile(pWave);
    AFreeModuleFile(pModule);

    /* close audio device */
    ACloseAudio();
    return 0;
}
