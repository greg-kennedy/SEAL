/*
 * $Id: audiow32.c 1.5 1996/06/02 19:04:30 chasan released $
 *
 * Win32 dynamic-link library entry point routine
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <windows.h>
#include "audio.h"

#ifdef WIN32

#define UPDATE_AUDIO_PERIOD 45  /* thread period in milliseconds */

static HANDLE hThread;
static DWORD nThreadId;
static BOOL bTerminate;

DWORD WINAPI UpdateAudioThread(LPVOID lpThreadParameter)
{
    DWORD dwTickCount, dwCurrentTickCount, dwOverload;

    dwTickCount = GetTickCount();
    for (dwOverload = 0; dwOverload < 128 && !bTerminate; dwOverload++) {
        AUpdateAudio();
        dwCurrentTickCount = GetTickCount();
        if ((dwTickCount += UPDATE_AUDIO_PERIOD) > dwCurrentTickCount) {
            dwOverload = 0;
            Sleep(dwTickCount - dwCurrentTickCount);
        }
    }
    return (lpThreadParameter != NULL);
}

#endif

#ifdef __MSC__
BOOL WINAPI DllMain(HINSTANCE hinstDLL,
    DWORD fdwReason, LPVOID lpvReserved)
#else
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL,
    DWORD fdwReason, LPVOID lpvReserved)
#endif
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        AInitialize();
#ifdef WIN32
        bTerminate = FALSE;
        hThread = CreateThread(NULL, 0, UpdateAudioThread, NULL, 0, &nThreadId);
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    }
    if (fdwReason == DLL_PROCESS_DETACH) {
        bTerminate = TRUE;
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
#endif
    }
    if (hinstDLL != (HINSTANCE)0 && lpvReserved != (LPVOID)0) {
        /* avoid compiler warnings */
    }
    return TRUE;
}
