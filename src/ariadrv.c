/*
 * $Id: ariadrv.c 1.6 1996/08/05 18:51:19 chasan released $
 *
 * Sierra Semiconductors' Aria soundcard audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <malloc.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"

/*
 * Aria DSP chip register offsets
 */
#define ARIA_CMD            0x000       /* Aria DSP command port */
#define ARIA_STAT           0x002       /* Aria DSP write status */
#define ARIA_ADDR           0x004       /* Aria DSP memory address */
#define ARIA_DATA           0x006       /* Aria DSP indexed memory data */

/*
 * Aria DSP playback mode defines
 */
#define ARIA_MODE_0         0x0000      /* 4 mono or 2 stereo PCM channels */
#define ARIA_MODE_1         0x0001      /* 2 mono or 1 stereo PCM channels */
#define ARIA_MODE_2         0x0002      /* 2 mono or 1 stereo PCM channels */

/*
 * Aria DSP playback format defines
 */
#define ARIA_FMT_11025      0x0000      /* select 11025 Hz sampling rate */
#define ARIA_FMT_22050      0x0010      /* select 22050 Hz sampling rate */
#define ARIA_FMT_44100      0x0020      /* select 44100 Hz sampling rate */
#define ARIA_FMT_MONO       0x0000      /* select mono output */
#define ARIA_FMT_STEREO     0x0001      /* select stereo output */
#define ARIA_FMT_8BIT       0x0000      /* select 8-bit PCM encoding */
#define ARIA_FMT_16BIT      0x0002      /* select 16-bit PCM encoding */
#define ARIA_FMT_ADPCM      0x0004      /* select ADPCM encoding */

#define BUFFERSIZE          50          /* buffer length in milliseconds */
#define PACKETSIZE          1024        /* packet size in bytes */
#define TIMEOUT             60000       /* timeout counter for DSP writes */


/*
 * Aria DSP configuration structure
 */
static struct {
    WORD    wFormat;                    /* playback encoding format */
    WORD    nSampleRate;                /* sampling frequency */
    WORD    wFmt;                       /* Aria DSP format command */
    WORD    wPort;                      /* base I/O port address */
    BYTE    nIrqLine;                   /* interrupt request line */
    BYTE    nDmaChannel;                /* direct memory access channel */
    LPBYTE  lpBuffer;                   /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length in bytes */
    UINT    nPosition;                  /* DMA buffer playing position */
    UINT    nWritePosition;             /* DMA buffer DSP write position */
    LPFNAUDIOWAVE lpfnAudioWave;        /* user wave callback function */
} Aria;


/*
 * Aria DSP low level programming routines
 */
static VOID ARPortW(WORD wCmd)
{
    UINT n;

    /* wait until the Aria DSP is ready to receive a command */
    for (n = 0; n < TIMEOUT; n++) {
        if (!(INW(Aria.wPort + ARIA_STAT) & 0x8000))
            break;
    }
    /* send the command to the Aria DSP chip */
    OUTW(Aria.wPort + ARIA_CMD, wCmd);
}

static VOID ARPokeW(WORD nIndex, WORD wData)
{
    OUTB(Aria.wPort + ARIA_ADDR, nIndex);
    OUTB(Aria.wPort + ARIA_DATA, wData);
}

static WORD ARPeekW(WORD nIndex)
{
    OUTW(Aria.wPort + ARIA_ADDR, nIndex);
    return INW(Aria.wPort + ARIA_DATA);
}

static VOID ARSetPlaybackMode(WORD nMode)
{
    /*
     * Setup Aria's playback mode to one of the following:
     *
     * ARIA_MODE_0    4 mono or 2 stereo PCM channels, no synth operators
     * ARIA_MODE_1    2 mono or 1 stereo PCM channels, 20 synth operators
     * ARIA_MODE_2    2 mono or 1 stereo PCM channels, 32 synth operators
     *
     */
    ARPortW(0x0006);
    ARPortW(nMode);
    ARPortW(0xFFFF);
}

static VOID ARStartPlayback(VOID)
{
    ARPortW(0x0011);
    ARPortW(0x0000);
    ARPortW(0xFFFF);
}

static VOID ARStopPlayback(VOID)
{
    ARPortW(0x0012);
    ARPortW(0x0000);
    ARPortW(0xFFFF);
}

static VOID ARSetFormat(WORD wFmt)
{
    /* we have to set some parameters to play at 44100 Hz */
    if (wFmt & ARIA_FMT_44100) {
        ARSetPlaybackMode(ARIA_MODE_0);
        OUTW(Aria.wPort + ARIA_STAT, 0x008A);
    }
    ARPortW(0x0003);
    ARPortW(wFmt);
    ARPortW(0xFFFF);
}

static VOID ARInit(VOID)
{
    UINT n;

    /*
     * Set up Aria in native mode (disable SB emulation mode),
     * first initialize the chip and then clear the init flag.
     */
    OUTW(Aria.wPort + ARIA_STAT, 0x00C8);
    ARPokeW(0x6102, 0x0000);

    /* Aria DSP initialization */
    ARPortW(0x0000);
    ARPortW(0x0000);
    ARPortW(0x0000);
    ARPortW(0x0000);
    ARPortW(0xFFFF);

    /* wait until Aria DSP initialization finishes */
    for (n = 0; n < TIMEOUT; n++) {
        if (ARPeekW(0x6102) == 1)
            break;
    }

    /* complete Aria initialization */
    OUTW(Aria.wPort + ARIA_CMD, 0x00CA);
}

static VOID ARReset(WORD wFmt)
{
    /*
     * We need to reset the Aria chip back to 22050 Hz
     * when it was programmed to play at 44100 Hz.
     */
    if (wFmt & ARIA_FMT_44100) {
        ARSetPlaybackMode(ARIA_MODE_2);
        OUTW(Aria.wPort + ARIA_STAT, 0x00CA);
    }

    /* switch back to SB emulation mode */
    OUTW(Aria.wPort + ARIA_STAT, 0x0040);
}

static VOID AIAPI ARInterruptHandler(VOID)
{
    LPWORD lpPacket;
    WORD wAddress;
    UINT n;

    /* check interrupt type and exit if not generated by Aria */
    if (INW(Aria.wPort + ARIA_CMD) == 1) {
        /* play a packet of samples from our buffer */
        wAddress = 0x8000 - PACKETSIZE + ARPeekW(0x6100);
        OUTW(Aria.wPort + ARIA_ADDR, wAddress);
        lpPacket = (LPWORD) (Aria.lpBuffer + Aria.nWritePosition);
        for (n = 0; n < PACKETSIZE/2; n++) {
            OUTW(Aria.wPort + ARIA_DATA, *lpPacket++);
        }
        if ((Aria.nWritePosition += PACKETSIZE) >= Aria.nBufferSize)
            Aria.nWritePosition -= Aria.nBufferSize;
        ARPortW(0x0010);
        ARPortW(0xFFFF);
    }
}


/*
 * Aria audio driver API interface
 */
static UINT AIAPI PingAudio(VOID)
{
    LPSTR lpszAria;
    UINT nChar;

    if ((lpszAria = DosGetEnvironment("ARIA")) != NULL) {
        Aria.wPort = 0x290;
        Aria.nIrqLine = 10;
        Aria.nDmaChannel = 5;
        nChar = DosParseString(lpszAria, TOKEN_CHAR);
        while (nChar != 0) {
            switch (nChar) {
            case 'A':
            case 'a':
                Aria.wPort = DosParseString(NULL, TOKEN_HEX);
                break;
            case 'I':
            case 'i':
                Aria.nIrqLine = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'D':
            case 'd':
                Aria.nDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            }
            nChar = DosParseString(NULL, TOKEN_CHAR);
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_ARIA, "Aria sound card",
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

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    DWORD dwBytesPerSecond;

    memset(&Aria, 0, sizeof(Aria));

    /*
     * Initialize the Aria DSP configuration parameters
     */
    Aria.wFormat = lpInfo->wFormat;
    Aria.nSampleRate = lpInfo->nSampleRate < 16357 ? 11025 :
        (lpInfo->nSampleRate < 33075 ? 22050 : 44100);
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;

    /*
     * Allocate and clean DMA buffer used for playback
     */
    dwBytesPerSecond = Aria.nSampleRate;
    if (Aria.wFormat & AUDIO_FORMAT_16BITS)
        dwBytesPerSecond <<= 1;
    if (Aria.wFormat & AUDIO_FORMAT_STEREO)
        dwBytesPerSecond <<= 1;
    Aria.nBufferSize = dwBytesPerSecond / (1000 / (BUFFERSIZE >> 1));
    Aria.nBufferSize = (Aria.nBufferSize + PACKETSIZE) & -PACKETSIZE;
    Aria.nBufferSize <<= 1;
    if ((Aria.lpBuffer = malloc(Aria.nBufferSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;
    memset(Aria.lpBuffer, Aria.wFormat & AUDIO_FORMAT_16BITS ?
	   0x00 : 0x80, Aria.nBufferSize);

    /*
     * Initialize the Aria DSP chip, set playback format,
     * sampling frequency and start the DAC transfer.
     */
    ARInit();
    DosSetVectorHandler(Aria.nIrqLine, ARInterruptHandler);
    Aria.wFmt = Aria.wFormat & AUDIO_FORMAT_16BITS ?
        ARIA_FMT_16BIT : ARIA_FMT_8BIT;
    Aria.wFmt |= Aria.wFormat & AUDIO_FORMAT_STEREO ?
        ARIA_FMT_STEREO : ARIA_FMT_MONO;
    Aria.wFmt |= (Aria.nSampleRate <= 11025 ? ARIA_FMT_11025 :
		  (Aria.nSampleRate <= 22050 ? ARIA_FMT_22050 : ARIA_FMT_44100));
    ARSetFormat(Aria.wFmt);
    ARStartPlayback();

    /* refresh caller's format and sampling frequency */
    lpInfo->wFormat = Aria.wFormat;
    lpInfo->nSampleRate = Aria.nSampleRate;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    ARStopPlayback();
    ARReset(Aria.wFmt);
    DosSetVectorHandler(Aria.nIrqLine, NULL);
    if (Aria.lpBuffer != NULL)
        free(Aria.lpBuffer);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    int nBlockSize, nSize;

    if (Aria.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (Aria.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > Aria.nBufferSize)
        nFrames = Aria.nBufferSize;

    if ((nBlockSize = Aria.nWritePosition - Aria.nPosition) < 0)
        nBlockSize += Aria.nBufferSize;

    if (nBlockSize > nFrames)
        nBlockSize = nFrames;

    while (nBlockSize != 0) {
        if ((nSize = Aria.nBufferSize - Aria.nPosition) > nBlockSize)
            nSize = nBlockSize;
        if (Aria.lpfnAudioWave != NULL) {
            Aria.lpfnAudioWave(&Aria.lpBuffer[Aria.nPosition], nSize);
        }
        else {
            memset(&Aria.lpBuffer[Aria.nPosition],
		   Aria.wFormat & AUDIO_FORMAT_16BITS ? 0x00 : 0x80, nSize);
        }
        if ((Aria.nPosition += nSize) >= Aria.nBufferSize)
            Aria.nPosition -= Aria.nBufferSize;
        nBlockSize -= nSize;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    Aria.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * Aria DSP audio driver public interface
 */
AUDIOWAVEDRIVER AriaWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER AriaDriver =
{
    &AriaWaveDriver, NULL
};
