/*
 * $Id: windrv.c 1.8 1996/05/24 08:30:44 chasan released $
 *
 * Windows Wave audio driver.
 *
 * Copyright (c) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "audio.h"
#include "drivers.h"

/*
 * Windows waveform block defines
 */
#define BUFFERSIZE      50      /* buffer size in milliseconds */
#define BUFFRAGSIZE     16      /* buffer fragment size in bytes */
#define NUMBLOCKS       8       /* total number of buffers */

/*
 * Windows Wave output configuration structure
 */
static struct {
    HWAVEOUT    nHandle;
    HGLOBAL     aBlockHandle[NUMBLOCKS];
    WAVEHDR     aWaveHeader[NUMBLOCKS];
    UINT        nBlockIndex;
    PFNAUDIOWAVE pfnAudioWave;
} WaveOut;


/*
 * Windows Wave driver API routines
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
{
    WAVEOUTCAPS Caps;

    if (waveOutGetDevCaps(WAVE_MAPPER, &Caps, sizeof(WAVEOUTCAPS)) != 0) {
        strncpy(Caps.szPname, "Windows Wave", sizeof(Caps.szPname));
        Caps.dwFormats = 0x00000000L;
    }
    strncpy(pCaps->szProductName, Caps.szPname, sizeof(pCaps->szProductName));
    pCaps->wProductId = AUDIO_PRODUCT_WINDOWS;
    pCaps->dwFormats = Caps.dwFormats;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return waveOutGetNumDevs() ? AUDIO_ERROR_NONE : AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    PCMWAVEFORMAT format;
    LONG nBufferSize;
    UINT n;

    memset(&WaveOut, 0, sizeof(WaveOut));

    /* setup PCM wave format structure */
    format.wf.wFormatTag = WAVE_FORMAT_PCM;
    format.wf.nChannels = pInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1;
    format.wBitsPerSample = pInfo->wFormat & AUDIO_FORMAT_16BITS ? 16 : 8;
    format.wf.nSamplesPerSec = pInfo->nSampleRate;
    format.wf.nAvgBytesPerSec = pInfo->nSampleRate;
    format.wf.nBlockAlign = sizeof(char);

    if (pInfo->wFormat & AUDIO_FORMAT_STEREO) {
        format.wf.nAvgBytesPerSec <<= 1;
        format.wf.nBlockAlign <<= 1;
    }
    if (pInfo->wFormat & AUDIO_FORMAT_16BITS) {
        format.wf.nAvgBytesPerSec <<= 1;
        format.wf.nBlockAlign <<= 1;
    }

    /* open wave output device using the PCM format structure */
    if (waveOutOpen(&WaveOut.nHandle, WAVE_MAPPER,
            (LPWAVEFORMAT) &format, 0, 0, 0) != 0) {
        return AUDIO_ERROR_NODEVICE;
    }

    /* compute wave output block buffer size in bytes */
    nBufferSize = pInfo->nSampleRate / (1000 / BUFFERSIZE);
    if (pInfo->wFormat & AUDIO_FORMAT_16BITS)
        nBufferSize <<= 1;
    if (pInfo->wFormat & AUDIO_FORMAT_STEREO)
        nBufferSize <<= 1;
    nBufferSize = (nBufferSize + BUFFRAGSIZE - 1) & -BUFFRAGSIZE;

    /* allocate and lock memory for wave output block buffers */
    for (n = 0; n < NUMBLOCKS; n++) {
        WaveOut.aBlockHandle[n] =
            GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nBufferSize);
        WaveOut.aWaveHeader[n].lpData =
            GlobalLock(WaveOut.aBlockHandle[n]);
        WaveOut.aWaveHeader[n].dwBufferLength = nBufferSize;
        WaveOut.aWaveHeader[n].dwFlags = WHDR_DONE;
        WaveOut.aWaveHeader[n].dwLoops = 0;
        WaveOut.aWaveHeader[n].dwUser = 0;
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    UINT n, nErrorCode;

    /* reset wave output device and stop all playing blocks */
    waveOutReset(WaveOut.nHandle);

    /* make sure all wave output block buffers are done */
    nErrorCode = AUDIO_ERROR_NONE;
    for (n = 0; n < NUMBLOCKS; n++) {
        if (!(WaveOut.aWaveHeader[n].dwFlags & WHDR_DONE))
            nErrorCode = AUDIO_ERROR_DEVICEBUSY;
    }

    /* unprepare, unlock and free wave output block buffers */
    for (n = 0; n < NUMBLOCKS; n++) {
        if (WaveOut.aWaveHeader[n].dwUser != 0) {
            waveOutUnprepareHeader(WaveOut.nHandle,
                &WaveOut.aWaveHeader[n], sizeof(WAVEHDR));
        }
        GlobalUnlock(WaveOut.aBlockHandle[n]);
        GlobalFree(WaveOut.aBlockHandle[n]);
    }

    /* close wave output device */
    waveOutClose(WaveOut.nHandle);

    return nErrorCode;
}

static UINT AIAPI UpdateAudio(VOID)
{
    LPWAVEHDR lpWaveHeader;

    lpWaveHeader = &WaveOut.aWaveHeader[WaveOut.nBlockIndex];
    if ((lpWaveHeader->dwFlags & WHDR_DONE) && WaveOut.pfnAudioWave) {
        if (lpWaveHeader->dwUser != 0) {
            waveOutUnprepareHeader(WaveOut.nHandle,
                lpWaveHeader, sizeof(WAVEHDR));
            lpWaveHeader->dwUser = 0;
        }
        if (lpWaveHeader->lpData != NULL) {
            lpWaveHeader->dwFlags = 0;
            lpWaveHeader->dwUser = 1;
            WaveOut.pfnAudioWave((PUCHAR) lpWaveHeader->lpData,
                lpWaveHeader->dwBufferLength);
            waveOutPrepareHeader(WaveOut.nHandle,
                lpWaveHeader, sizeof(WAVEHDR));
            waveOutWrite(WaveOut.nHandle,
                lpWaveHeader, sizeof(WAVEHDR));
        }
        if (++WaveOut.nBlockIndex >= NUMBLOCKS)
            WaveOut.nBlockIndex = 0;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    WaveOut.pfnAudioWave = pfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * Windows Wave driver public interface
 */
AUDIOWAVEDRIVER WindowsWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER WindowsDriver =
{
    &WindowsWaveDriver, NULL
};
