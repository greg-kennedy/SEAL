/* example3.c - play a module file */

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
    BOOL stopped;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;

#ifdef USE_FILTER
    /* enable antialias dynamic filtering */
    info.wFormat |= AUDIO_FORMAT_FILTER;
#endif

    AOpenAudio(&info);

    /* load module file */
    ALoadModuleFile("test.s3m", &pModule);

    /* open voices and play module */
    AOpenVoices(pModule->nTracks);
    APlayModule(pModule);

    /* program main execution loop */
    printf("Playing module file.\n");
    while (!kbhit()) {
        /* update audio system */
        AUpdateAudio();

        /* check if the module is stopped */
        AGetModuleStatus(&stopped);
        if (stopped) break;
    }

    /* stop module and close voices */
    AStopModule();
    ACloseVoices();

    /* release module file */
    AFreeModuleFile(pModule);

    /* close audio device */
    ACloseAudio();
    return 0;
}
