/*
 * $Id: ariadrv.c 1.5 1996/06/08 16:23:05 chasan released $
 *
 * Sierra Semiconductors' Aria soundcard audio driver.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
    USHORT  wFormat;                    /* playback encoding format */
    USHORT  nSampleRate;                /* sampling frequency */
    USHORT  wFmt;                       /* Aria DSP format command */
    USHORT  wPort;                      /* base I/O port address */
    UCHAR   nIrqLine;                   /* interrupt request line */
    UCHAR   nDmaChannel;                /* direct memory access channel */
    PUCHAR  pBuffer;                    /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length in bytes */
    UINT    nPosition;                  /* DMA buffer playing position */
    UINT    nWritePosition;             /* DMA buffer DSP write position */
    PFNAUDIOWAVE pfnAudioWave;          /* user wave callback function */
} Aria;


/*
 * Aria DSP low level programming routines
 */
static VOID ARPortW(USHORT wCmd)
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

static VOID ARPokeW(USHORT nIndex, USHORT wData)
{
    OUTB(Aria.wPort + ARIA_ADDR, nIndex);
    OUTB(Aria.wPort + ARIA_DATA, wData);
}

static USHORT ARPeekW(USHORT nIndex)
{
    OUTW(Aria.wPort + ARIA_ADDR, nIndex);
    return INW(Aria.wPort + ARIA_DATA);
}

static VOID ARSetPlaybackMode(USHORT nMode)
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

static VOID ARSetFormat(USHORT wFmt)
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

static VOID ARReset(USHORT wFmt)
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
    PUSHORT pPacket;
    USHORT wAddress;
    UINT n;

    /* check interrupt type and exit if not generated by Aria */
    if (INW(Aria.wPort + ARIA_CMD) == 1) {
        /* play a packet of samples from our buffer */
        wAddress = 0x8000 - PACKETSIZE + ARPeekW(0x6100);
        OUTW(Aria.wPort + ARIA_ADDR, wAddress);
        pPacket = (PUSHORT) (Aria.pBuffer + Aria.nWritePosition);
        for (n = 0; n < PACKETSIZE/2; n++) {
            OUTW(Aria.wPort + ARIA_DATA, *pPacket++);
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
    PSZ pszAria;
    UINT nChar;

    if ((pszAria = DosGetEnvironment("ARIA")) != NULL) {
        Aria.wPort = 0x290;
        Aria.nIrqLine = 10;
        Aria.nDmaChannel = 5;
        nChar = DosParseString(pszAria, TOKEN_CHAR);
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

static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
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

    memcpy(pCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    ULONG dwBytesPerSecond;

    memset(&Aria, 0, sizeof(Aria));

    /*
     * Initialize the Aria DSP configuration parameters
     */
    Aria.wFormat = pInfo->wFormat;
    Aria.nSampleRate = pInfo->nSampleRate < 16357 ? 11025 :
        (pInfo->nSampleRate < 33075 ? 22050 : 44100);
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
    if ((Aria.pBuffer = malloc(Aria.nBufferSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;
    memset(Aria.pBuffer, Aria.wFormat & AUDIO_FORMAT_16BITS ?
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
    pInfo->wFormat = Aria.wFormat;
    pInfo->nSampleRate = Aria.nSampleRate;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    ARStopPlayback();
    ARReset(Aria.wFmt);
    DosSetVectorHandler(Aria.nIrqLine, NULL);
    if (Aria.pBuffer != NULL)
        free(Aria.pBuffer);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    int nBlockSize, nSize;

    if ((nBlockSize = Aria.nWritePosition - Aria.nPosition) < 0)
        nBlockSize += Aria.nBufferSize;
    while (nBlockSize != 0) {
        if ((nSize = Aria.nBufferSize - Aria.nPosition) > nBlockSize)
            nSize = nBlockSize;
        if (Aria.pfnAudioWave != NULL) {
            Aria.pfnAudioWave(&Aria.pBuffer[Aria.nPosition], nSize);
        }
        else {
            memset(&Aria.pBuffer[Aria.nPosition],
                Aria.wFormat & AUDIO_FORMAT_16BITS ? 0x00 : 0x80, nSize);
        }
        if ((Aria.nPosition += nSize) >= Aria.nBufferSize)
            Aria.nPosition -= Aria.nBufferSize;
        nBlockSize -= nSize;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    Aria.pfnAudioWave = pfnAudioWave;
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
