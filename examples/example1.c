/* example1.c - initialize and print device information */

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
    AUDIOCAPS caps;
    UINT rc, nDevId;

    /* initialize audio library */
    AInitialize();

    /* show registered device drivers */
    printf("List of registered devices:\n");
    for (nDevId = 0; nDevId < AGetAudioNumDevs(); nDevId++) {
        AGetAudioDevCaps(nDevId, &caps);
        printf("  %2d. %s\n", nDevId, caps.szProductName);
    }
    printf("\n");

    /*
     * NOTE: Here we can use any of the above devices, or we can
     * use the virtual device AUDIO_DEVICE_MAPPER for detection.
     */

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    if ((rc = AOpenAudio(&info)) != AUDIO_ERROR_NONE) {
        CHAR szText[80];
        AGetErrorText(rc, szText, sizeof(szText) - 1);
        printf("ERROR: %s\n", szText);
        exit(1);
    }

    /*
     * NOTE: Since the audio device may not support the playback
     * format and sampling frequency, the audio system uses the
     * closest configuration which is then returned to the user
     * in the AUDIOINFO structure.
     *
     */

    /* print information */
    AGetAudioDevCaps(info.nDeviceId, &caps);
    printf("%s at %d-bit %s %u Hz detected\n",
        caps.szProductName,
        info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
        info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
        info.nSampleRate);


    /* close audio device */
    ACloseAudio();
    return 0;
}
