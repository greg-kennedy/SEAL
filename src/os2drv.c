/*
 * $Id: os2drv.c 1.0 1998/10/18 14:06:31 chasan Exp $
 *
 * OS/2 MMPM audio driver.
 *
 * Copyright (C) 1998 by Martin Amodeo <marty@rochester.rr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <process.h>
#define INCL_DOS
#include <os2.h>
#define INCL_OS2MM
#include <os2me.h>

#define WINAPI
typedef VOID*           LPVOID;
typedef CHAR*           LPCHAR;
typedef INT*            LPINT;
typedef LONG*           LPLONG;
typedef BOOL*           LPBOOL;
typedef BYTE*           LPBYTE;
typedef WORD*           LPWORD;
typedef UINT*           LPUINT;
typedef DWORD*          LPDWORD;

#undef LPSTR
#include "audio.h"
#include "drivers.h"


/*
 * OS/2 MMPM driver defines
 */
#define BUFFER_SIZE 15             // buffer length in milliseconds / 2
#define THREAD_STACK_SIZE 32768    // thread stack size in bytes

#define SEMWAIT_OPERATION 9        // Semaphore wait operation for playlist
#define SEMPOST_OPERATION 10       // Semaphore post operation for playlist

/*
 * OS/2 MMPM driver global state
 */
static struct {
    LPVOID  lpBuffer[2];
    ULONG   nBufferSize;
    HEV     semNeedsBuffer[2];
    HEV     semHasBuffer[2];
    HEV     semAudioPing;
    ULONG   ulThread;
    LPCHAR  aThreadStack;
    MCI_OPEN_PARMS mop;
    LPFNAUDIOWAVE lpfnCallback;
    struct {
        ULONG operation;
        ULONG operand1;
        ULONG operand2;
        ULONG operand3;
    } playlist[8];
} OS2;


/*
 * OS/2 MMPM driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_OS2MMPM, "OS/2 MMPM",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };
    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static void ThreadProc(void *data)
{
    int i, junk;
    for (i = 0; ; i ^= 1) {
        /* wait until the buffer needs data */
        DosWaitEventSem(OS2.semNeedsBuffer[i], -1);
        DosResetEventSem(OS2.semNeedsBuffer[i], &junk);
        DosResetEventSem(OS2.semAudioPing, &junk);
    
        /* fill the needed buffer with samples */
        DosEnterCritSec();
        if (OS2.lpfnCallback != NULL)
            (*OS2.lpfnCallback)(OS2.lpBuffer[i], OS2.nBufferSize);
        else
            memset(OS2.lpBuffer[i], 0, OS2.nBufferSize);
        DosExitCritSec();
    
        /* now the buffer has data */
        DosPostEventSem(OS2.semAudioPing);
        DosPostEventSem(OS2.semHasBuffer[i]);
    }
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    MCI_WAVE_SET_PARMS wsp;
    MCI_PLAY_PARMS mpp;
  
    /* clean the OS/2 driver state */
    memset(&OS2, 0, sizeof(OS2));

    /* compute the buffer length in bytes */
#if 1
    OS2.nBufferSize = ( 266 * lpInfo->nSampleRate / 22050 ) *
        (lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 2 : 1) *
        (lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1);
#else
    OS2.nBufferSize = lpInfo->nSampleRate / (1000 / BUFFERSIZE);
    if (lpInfo->wFormat & AUDIO_FORMAT_16BITS)
        OS2.nBufferSize <<= 1;
    if (lpInfo->wFormat & AUDIO_FORMAT_STEREO)
        OS2.nBufferSize <<= 1;
#endif
    OS2.nBufferSize = (OS2.nBufferSize + 3) & ~3;

    /* allocate memory for double buffering */
    if ((OS2.lpBuffer[0] = (LPVOID) malloc(OS2.nBufferSize)) == NULL ||
        (OS2.lpBuffer[1] = (LPVOID) malloc(OS2.nBufferSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;

    if ((OS2.aThreadStack = (LPCHAR) malloc(THREAD_STACK_SIZE)) == NULL )
        return AUDIO_ERROR_NOMEMORY;
  
    /* create consumer/producer buffer semaphores */
    DosCreateEventSem("\\SEM32\\Buffer1NeedsData",
                      &OS2.semNeedsBuffer[0], DC_SEM_SHARED, 1);
  
    DosCreateEventSem("\\SEM32\\Buffer2NeedsData",
                      &OS2.semNeedsBuffer[1], DC_SEM_SHARED, 1);
  
    DosCreateEventSem("\\SEM32\\Buffer1HasData",
                      &OS2.semHasBuffer[0], DC_SEM_SHARED, 0);
  
    DosCreateEventSem("\\SEM32\\Buffer2HasData",
                      &OS2.semHasBuffer[1], DC_SEM_SHARED, 0);
  
    /* Ping semaphore for synchronization of streamed audio */
    DosCreateEventSem("\\SEM32\\AudioPing",
                      &OS2.semAudioPing, DC_SEM_SHARED, 0);
  
    /* create MCI audio playlist commands */
  
    /* wait until first buffer has data */
    OS2.playlist[0].operation = SEMWAIT_OPERATION;
    OS2.playlist[0].operand1  = OS2.semHasBuffer[0];
    OS2.playlist[0].operand2  = -1;
    OS2.playlist[0].operand3  = 0;
  
    /* play the first buffer data */
    OS2.playlist[1].operation = DATA_OPERATION;
    OS2.playlist[1].operand1  = (long) OS2.lpBuffer[0];
    OS2.playlist[1].operand2  = OS2.nBufferSize;
    OS2.playlist[1].operand3  = 0;
  
    /* tell first buffer needs data */
    OS2.playlist[2].operation = SEMPOST_OPERATION;
    OS2.playlist[2].operand1  = OS2.semNeedsBuffer[0];
    OS2.playlist[2].operand2  = 0;
    OS2.playlist[2].operand3  = 0;
  
    /* wait until second buffer has data */
    OS2.playlist[3].operation = SEMWAIT_OPERATION;
    OS2.playlist[3].operand1  = OS2.semHasBuffer[1];
    OS2.playlist[3].operand2  = -1;
    OS2.playlist[3].operand3  = 0;
  
    /* play the second buffer data */
    OS2.playlist[4].operation = DATA_OPERATION;
    OS2.playlist[4].operand1  = (long) OS2.lpBuffer[1];
    OS2.playlist[4].operand2  = OS2.nBufferSize;
    OS2.playlist[4].operand3  = 0;
  
    /* tell second buffer needs data */
    OS2.playlist[5].operation = SEMPOST_OPERATION;
    OS2.playlist[5].operand1  = OS2.semNeedsBuffer[1];
    OS2.playlist[5].operand2  = 0;
    OS2.playlist[5].operand3  = 0;
  
    /* branch to the first playlist entry */
    OS2.playlist[6].operation = BRANCH_OPERATION;
    OS2.playlist[6].operand1  = 0;
    OS2.playlist[6].operand2  = 0;  // goto OS2.playlist[0]
    OS2.playlist[6].operand3  = 0;
  
    /* exit the playlist */
    OS2.playlist[7].operation = EXIT_OPERATION;  
  
    /* open the MCI waveform audio device */
    OS2.mop.usDeviceID     = 0;
    OS2.mop.pszDeviceType  = MCI_DEVTYPE_WAVEFORM_AUDIO_NAME;
    OS2.mop.pszElementName = (LPCHAR) &OS2.playlist[0];
  
    if (mciSendCommand(0, MCI_OPEN, MCI_WAIT | MCI_OPEN_SHAREABLE |
                       MCI_OPEN_PLAYLIST, &OS2.mop, 0) != MCIERR_SUCCESS)
        return AUDIO_ERROR_DEVICEBUSY;
  
    /* setup the MCI waveform device parameters */
    wsp.hwndCallback = 0;
    wsp.ulSamplesPerSec = lpInfo->nSampleRate;
    wsp.usBitsPerSample = (lpInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8);
    wsp.usChannels = (lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1);
  
    if (mciSendCommand(OS2.mop.usDeviceID, MCI_SET,
                       MCI_WAIT |
                       MCI_WAVE_SET_SAMPLESPERSEC |
                       MCI_WAVE_SET_BITSPERSAMPLE |
                       MCI_WAVE_SET_CHANNELS,
                       &wsp, 0) != MCIERR_SUCCESS)
        return AUDIO_ERROR_DEVICEBUSY;
  
    /* start the OS/2 driver thread routine */
    OS2.ulThread = _beginthread(ThreadProc, OS2.aThreadStack,
                                THREAD_STACK_SIZE, NULL);
  
    DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, OS2.ulThread);
  
    /* start playing the MCI audio device */
    mpp.hwndCallback = 0;
    mpp.ulFrom = 0;
    if (mciSendCommand(OS2.mop.usDeviceID, MCI_PLAY,
                       0, &mpp, 0) != MCIERR_SUCCESS)
        return AUDIO_ERROR_DEVICEBUSY;
  
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    MCI_GENERIC_PARMS mgp;
  
    /* kill the OS/2 driver thread routine */
    DosKillThread(OS2.ulThread);
  
    /* let the branch fall through to the exit function */
    OS2.playlist[6].operand2 = 7;
  
    /* "fill" the buffers to unlock the playlist */
    DosPostEventSem(OS2.semHasBuffer[0]);
    DosPostEventSem(OS2.semHasBuffer[1]);

    /* Unblock any threads that may be synchronizing */
    DosPostEventSem(OS2.semAudioPing);

  
    /* close the MCI waveform audio device */
    mgp.hwndCallback = 0;
    mciSendCommand(OS2.mop.usDeviceID, MCI_CLOSE, MCI_WAIT, &mgp, 0);
  
    /* destroy the read/write buffer semaphores */
    DosCloseEventSem(OS2.semNeedsBuffer[0]);
    DosCloseEventSem(OS2.semNeedsBuffer[1]);
    DosCloseEventSem(OS2.semHasBuffer[0]);
    DosCloseEventSem(OS2.semHasBuffer[1]);
  
    /* release the memory buffers */
    if (OS2.lpBuffer[0] != NULL)
        free(OS2.lpBuffer[0]);
    if (OS2.lpBuffer[1] != NULL)
        free(OS2.lpBuffer[1]);
    if (OS2.aThreadStack != NULL)
        free(OS2.aThreadStack);
  
    /* clean the OS/2 driver state */
    memset(&OS2, 0, sizeof(OS2));
  
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    ULONG junk;
    DosResetEventSem(OS2.semAudioPing, &junk);
    DosWaitEventSem(OS2.semAudioPing, -1);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    /* set the user lpfnCallback routine */
    OS2.lpfnCallback = lpfnAudioWave;  
    return AUDIO_ERROR_NONE;
}


/*
 * OS/2 MMPM driver public interface
 */
AUDIOWAVEDRIVER OS2MMPMWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER OS2MMPMDriver =
{
    &OS2MMPMWaveDriver, NULL
};
