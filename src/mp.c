/*
 * $Id: mp.c 1.11 1996/06/16 00:52:23 chasan released $
 *
 * Module player demonstration
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"

#if defined(__DOS16__) || defined(__DPMI__) || defined(__WINDOWS__)
#include <conio.h>
#else
#define kbhit() 0
#endif

struct {
    AUDIOINFO Info;
    AUDIOCAPS Caps;
    PAUDIOMODULE pModule;
    UINT nVolume;
    BOOL bStopped;
} State;

void Assert(UINT nErrorCode)
{
    static CHAR szText[80];

    if (nErrorCode != AUDIO_ERROR_NONE) {
        AGetErrorText(nErrorCode, szText, sizeof(szText) - 1);
        printf("%s\n", szText);
        exit(1);
    }
}

#ifdef __MSC__
void __cdecl CleanUp(void)
#else
void CleanUp(void)
#endif
{
    ACloseAudio();
}

void Banner(void)
{

/* Linux/386 */
#ifdef __LINUX__
#define _SYSTEM_ "Linux"
#endif

/* FreeBSD/386 */
#ifdef __FREEBSD__
#define _SYSTEM_ "FreeBSD"
#endif

/* SunOS 4.1.x */
#ifdef __SPARC__
#define _SYSTEM_ "SPARC/SunOS"
#endif

/* Solaris 2.x */
#ifdef __SOLARIS__
#define _SYSTEM_ "SPARC/Solaris"
#endif

/* IRIX 4.x */
#ifdef __SILICON__
#define _SYSTEM_ "SGI/Irix"
#endif

/* Win32 */
#if defined(__WINDOWS__) && defined(__FLAT__)
#ifdef __BORLANDC__
#define _SYSTEM_ "Win32/BC32"
#endif
#ifdef __WATCOMC__
#define _SYSTEM_ "Win32/WC32"
#endif
#ifdef __MSC__
#define _SYSTEM_ "Win32/MSC32"
#endif
#endif

/* Win16 */
#if defined(__WINDOWS__) && defined(__LARGE__)
#ifdef __BORLANDC__
#define _SYSTEM_ "Win16/BC16"
#endif
#ifdef __WATCOMC__
#define _SYSTEM_ "Win16/WC16"
#endif
#endif

/* DPMI32 */
#if defined(__DPMI__) && defined(__FLAT__)
#ifdef __BORLANDC__
#define _SYSTEM_ "DPMI32/BC32"
#endif
#ifdef __WATCOMC__
#define _SYSTEM_ "DOS4GW/WC32"
#endif
#endif
#if defined(__DPMI__) && defined(__DJGPP__)
#define _SYSTEM_ "DPMI32/DJGPP"
#endif

/* DPMI16 */
#if defined(__DPMI__) && defined(__LARGE__)
#ifdef __BORLANDC__
#define _SYSTEM_ "DPMI16/BC16"
#endif
#endif

/* DOS */
#if defined(__DOS16__) && defined(__LARGE__)
#ifdef __BORLANDC__
#define _SYSTEM_ "DOS/BC16"
#endif
#ifdef __WATCOMC__
#define _SYSTEM_ "DOS/WC16"
#endif
#endif

    printf("MOD/S3M/XM Module Player Version 0.5 (" _SYSTEM_ " version)\n");
    printf("Please send bug reports to: chasan@dcc.uchile.cl\n");
}

void Usage(void)
{
    UINT n;

    printf("Usage: mp [options] file...\n");
    printf("  -c device   select audio device\n");
    for (n = 0; n < AGetAudioNumDevs(); n++) {
        AGetAudioDevCaps(n, &State.Caps);
        printf("\t\t%d=%s\n", n, State.Caps.szProductName);
    }
    printf("  -r rate     set sampling rate\n");
    printf("  -v volume   change global volume\n");
    printf("  -8          8-bit sample precision\n");
    printf("  -m          monophonic output\n");
    printf("  -i          enable filtering\n");
    printf("\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    PSZ pszOption, pszOptArg, pszFileName;
    UINT n;

    /* initialize the audio system library */
    AInitialize();

    /* display program information */
    Banner();
    if (argc < 2) Usage();

    /* setup default initialization parameters */
    State.Info.nDeviceId = AUDIO_DEVICE_MAPPER;
    State.Info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    State.Info.nSampleRate = 44100;
    State.nVolume = 64;

    /* parse command line options */
    for (n = 1; n < argc && (pszOption = argv[n])[0] == '-'; n++) {
        pszOptArg = &pszOption[2];
        if (strchr("crv", pszOption[1]) && !pszOptArg[0] && n < argc - 1)
            pszOptArg = argv[++n];
        switch (pszOption[1]) {
        case 'c':
            State.Info.nDeviceId = atoi(pszOptArg);
            break;
        case '8':
            State.Info.wFormat &= ~AUDIO_FORMAT_16BITS;
            break;
        case 'm':
            State.Info.wFormat &= ~AUDIO_FORMAT_STEREO;
            break;
        case 'i':
            State.Info.wFormat |= AUDIO_FORMAT_FILTER;
            break;
        case 'r':
            State.Info.nSampleRate = (UINT) atoi(pszOptArg);
            if (State.Info.nSampleRate < 1000)
                State.Info.nSampleRate *= 1000;
            break;
        case 'v':
            State.nVolume = atoi(pszOptArg);
            break;
        default:
            Usage();
            break;
        }
    }

    /* initialize and open the audio system */
    Assert(AOpenAudio(&State.Info));
    atexit(CleanUp);

    /* display playback audio device information */
    Assert(AGetAudioDevCaps(State.Info.nDeviceId, &State.Caps));
    printf("%s playing at %d-bit %s %u Hz\n", State.Caps.szProductName,
        State.Info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
        State.Info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
        State.Info.nSampleRate);

    while (n < argc && (pszFileName = argv[n++]) != NULL) {
        /* load module file from disk */
        printf("Loading: %s\n", pszFileName);
        Assert(ALoadModuleFile(pszFileName, &State.pModule));

        /* play the module file */
        printf("Playing: %s\n", State.pModule->szModuleName);
        Assert(AOpenVoices(State.pModule->nTracks));
        Assert(APlayModule(State.pModule));
        Assert(ASetModuleVolume(State.nVolume));
        while (!kbhit()) {
            Assert(AGetModuleStatus(&State.bStopped));
            if (State.bStopped) break;
            Assert(AUpdateAudio());
        }
        Assert(AStopModule());
        Assert(ACloseVoices());

        /* release the module file */
        Assert(AFreeModuleFile(State.pModule));
    }

    return 0;
}
