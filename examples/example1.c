/* example1.c - initialize audio system */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio.h>

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__DJGPP__)
#include <conio.h>
#else
#define kbhit() 0
#endif

int main(void)
{
    AUDIOINFO info;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* program main execution loop */
    while (!kbhit()) {
        /* update audio system */
        AUpdateAudio();
        /* ...
         * put your stuff here 
         * ...
         */
    }

    /* close audio device */
    ACloseAudio();
    return 0;
}
