/*
 * $Id: audiow32.c 1.7 1996/09/20 23:55:38 chasan released $
 *                 1.8 1998/12/24 15:07:30 chasan released (NT fix)
 *
 * Win32 dynamic-link library entry point routine
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <windows.h>
#include "audio.h"

#ifdef WIN32

#define AUDIO_UPDATE_LATENCY 10  /* audio latency in milliseconds */

static HANDLE hThread;
static DWORD nThreadId;
static BOOL bTerminate;

DWORD WINAPI UpdateAudioThread(LPVOID lpThreadParameter)
{
    LONG dwAudioTime, dwSleepTime;
    
    dwAudioTime = GetTickCount();
    while (!bTerminate) {
        AUpdateAudio();
        dwAudioTime += AUDIO_UPDATE_LATENCY;
        if ((dwSleepTime = dwAudioTime - GetTickCount()) > 0)
	    Sleep(dwSleepTime);
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
        SetPriorityClass(hThread, HIGH_PRIORITY_CLASS);
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
