/* example5.c - update the system using the timer interrupt, please compile
 *              using WATCOM C/C++32 and assume that the stack segment is
 *              not pegged to the DGROUP segment:
 *
 *   wcl386 -l=dos4g -zu -s -I..\include example5.c ..\lib\dos\audiowcf.lib
 */

#ifndef __WATCOMC__

#include <stdio.h>

int main(void)
{
    printf("This example only works with WATCOM C/C++32 and DOS4GW\n");
    return 0;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio.h>
#include <conio.h>
#include <dos.h>

/* call the timer handler 70 times per second */
#define TIMER_RATE  (1193180/70)

volatile void (interrupt far *lpfnBIOSTimerHandler)(void) = NULL;
volatile long dwTimerAccum = 0L;

void SetBorderColor(BYTE nColor)
{
    outp(0x3c0, 0x31);
    outp(0x3c0, nColor);
}

void interrupt far TimerHandler(void)
{
    SetBorderColor(1);
    AUpdateAudio();
    SetBorderColor(0);
    if ((dwTimerAccum += TIMER_RATE) >= 0x10000L) {
        dwTimerAccum -= 0x10000L;
        lpfnBIOSTimerHandler();
    }
    else {
        outp(0x20, 0x20);
    }
}

VOID InitTimerHandler(VOID)
{
    lpfnBIOSTimerHandler = _dos_getvect(0x08);
    _dos_setvect(0x08, TimerHandler);
    outp(0x43, 0x34);
    outp(0x40, LOBYTE(TIMER_RATE));
    outp(0x40, HIBYTE(TIMER_RATE));
}

VOID DoneTimerHandler(VOID)
{
    outp(0x43, 0x34);
    outp(0x40, 0x00);
    outp(0x40, 0x00);
    _dos_setvect(0x08, lpfnBIOSTimerHandler);
}


int main(void)
{
    static AUDIOINFO info;
    static AUDIOCAPS caps;
    static LPAUDIOMODULE lpModule;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* show device information */
    AGetAudioDevCaps(info.nDeviceId, &caps);
    printf("%s detected. Please type EXIT to return.\n", caps.szProductName);

    /* load module file */
    ALoadModuleFile("test.s3m", &lpModule, 0);

    /* open voices for module */
    AOpenVoices(lpModule->nTracks);

    /* play the module file */
    APlayModule(lpModule);

    /* initialize the timer routines */
    InitTimerHandler();

    /* invoke the DOS command processor */
    system(getenv("COMSPEC"));

    /* terminate the timer routines */
    DoneTimerHandler();

    /* stop playing the module */
    AStopModule();
    ACloseVoices();

    /* release the waveform & module */
    AFreeModuleFile(lpModule);

    /* close audio device */
    ACloseAudio();
    return 0;
}

#endif
