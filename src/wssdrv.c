/*
 * $Id: wssdrv.c 1.8 1996/09/08 00:25:13 chasan released $
 *               1.9 1998/10/13 21:45:40 chasan (ENSONIQ AudioPCI fix)
 *
 * Analog Devices AD1848 SoundPort Stereo Codec low level driver.
 * Windows Sound System, Ultrasound Max and Ensoniq Soundscape audio drivers.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"

#define DEBUG(code)

/*----------------- Analog Devices AD1848 Codec routines -----------------*/

/*
 * The AD1848 direct control register offsets
 */
#define CODEC_ADDR              0x00    /* index address register */
#define CODEC_DATA              0x01    /* indexed data register */
#define CODEC_STAT              0x02    /* status register */
#define CODEC_PIO               0x03    /* PIO data registers */

/*
 * The AD1848 indirect register indexes
 */
#define CODEC_LIC               0x00    /* left input control */
#define CODEC_RIC               0x01    /* right input control */
#define CODEC_LX1C              0x02    /* left aux #1 input control */
#define CODEC_RX1C              0x03    /* right aux #1 input control */
#define CODEC_LX2C              0x04    /* left aux #2 input control */
#define CODEC_RX2C              0x05    /* right aux #2 input control */
#define CODEC_LDC               0x06    /* left output control */
#define CODEC_RDC               0x07    /* right output control */
#define CODEC_CDF               0x08    /* clock and data format */
#define CODEC_INTF              0x09    /* interface configuration */
#define CODEC_PIN               0x0A    /* PIN control */
#define CODEC_TEST              0x0B    /* test and initialization */
#define CODEC_MISC              0x0C    /* miscellaneous information */
#define CODEC_DIGMIX            0x0D    /* digital mix */
#define CODEC_UPR_COUNT         0x0E    /* upper base count */
#define CODEC_LWR_COUNT         0x0F    /* lower base count */

/*
 * Index register bit fields (CODEC_ADDR)
 */
#define CODEC_ADDR_TRD          0x20    /* transfer request disable */
#define CODEC_ADDR_MCE          0x40    /* mode change enable */
#define CODEC_ADDR_INIT         0x80    /* AD1848 initialization */

/*
 * Status register bit fields (CODEC_STAT)
 */
#define CODEC_STAT_INT          0x01    /* interrupt status */
#define CODEC_STAT_PRDY         0x02    /* playback data register ready */
#define CODEC_STAT_PLR          0x04    /* playback left/right sample */
#define CODEC_STAT_PUL          0x08    /* playback upper/lower byte */
#define CODEC_STAT_SOUR         0x10    /* sample over/underrun */
#define CODEC_STAT_CRDY         0x20    /* capture data register ready */
#define CODEC_STAT_CLR          0x40    /* capture left/right sample */
#define CODEC_STAT_CUL          0x80    /* capture upper/lower byte */

/*
 * Left input control register (CODEC_LIC)
 */
#define CODEC_LIG               0x0F    /* left input gain select mask */
#define CODEC_LMGE              0x20    /* left input microphone gain enable */
#define CODEC_LSS               0xC0    /* left source select mask */
#define CODEC_LSS_LINE          0x00    /* left line source selected */
#define CODEC_LSS_AUX1          0x40    /* left aux #1 source selected */
#define CODEC_LSS_MIC           0x80    /* left microphone source selected */
#define CODEC_LSS_DAC           0xC0    /* left line post-mixed DAC source */

/*
 * Right input control register (CODEC_RIC)
 */
#define CODEC_RIG               0x0F    /* right input gain select */
#define CODEC_RMGE              0x20    /* right microphone gain enable */
#define CODEC_RSS               0xC0    /* right source select mask */
#define CODEC_RSS_LINE          0x00    /* right line source selected */
#define CODEC_RSS_AUX1          0x40    /* right aux #1 source selected */
#define CODEC_RSS_MIC           0x80    /* right microphone source selected */
#define CODEC_RSS_DAC           0xC0    /* right line post-mixed DAC source */

/*
 * Left auxiliary #1 input control register (CODEC_LX1C)
 */
#define CODEC_LX1A              0x1F    /* left aux #1 attenuate select */
#define CODEC_LMX1              0x80    /* left aux #1 mute */

/*
 * Right auxiliary #1 input control register (CODEC_RX1C)
 */
#define CODEC_RX1A              0x1F    /* right aux #1 attenuate select */
#define CODEC_RMX1              0x80    /* right aux #1 mute */

/*
 * Left auxiliary #2 input control register (CODEC_LX2C)
 */
#define CODEC_LX2A              0x1F    /* left aux #2 attenuate select */
#define CODEC_LMX2              0x80    /* left aux #2 mute */

/*
 * Right auxiliary #2 input control register (CODEC_RX2C)
 */
#define CODEC_RX2A              0x1F    /* right aux #2 attenuate select */
#define CODEC_RMX2              0x80    /* right aux #2 mute */

/*
 * Left DAC control register (CODEC_LDC)
 */
#define CODEC_LDA               0x3F    /* left DAC attenuate select */
#define CODEC_LDM               0x80    /* left DAC mute */

/*
 * Right DAC control register (CODEC_RDC)
 */
#define CODEC_RDA               0x3F    /* right DAC attenuate select */
#define CODEC_RDM               0x80    /* right DAC mute */

/*
 * Clock and data format register (CODEC_CDF)
 */
#define CODEC_CSS               0x01    /* clock source select */
#define CODEC_CFS               0x0E    /* clock frequency divide select */
#define CODEC_SM                0x10    /* stereo/mono select */
#define CODEC_LC                0x20    /* linear/companded select */
#define CODEC_FMT               0x40    /* format select */
#define CODEC_CSS_XTAL1         0x00    /* XTAL1 24.576 MHz */
#define CODEC_CSS_XTAL2         0x01    /* XTAL2 16.9344 MHz */
#define CODEC_LC_FMT_8BIT       0x00    /* 8-bit unsigned PCM linear */
#define CODEC_LC_FMT_16BIT      0x40    /* 16-bit signed PCM linear */
#define CODEC_LC_FMT_ULAW       0x20    /* 8-bit u-law companded */
#define CODEC_LC_FMT_ALAW       0x60    /* 8-bit A-law companded */

/*
 * Interface configuration register (CODEC_INTF)
 */
#define CODEC_PEN               0x01    /* playback enable */
#define CODEC_CEN               0x02    /* capture enable */
#define CODEC_SDC               0x04    /* single DMA channel */
#define CODEC_ACAL              0x08    /* autocalibration enable */
#define CODEC_PPIO              0x40    /* playback PIO enable */
#define CODEC_CPIO              0x80    /* capture PIO enable */

/*
 * Pin control register (CODEC_PIN)
 */
#define CODEC_IEN               0x02    /* interrupt enable */
#define CODEC_XCTL0             0x40    /* external control #1 */
#define CODEC_XCTL1             0x80    /* external control #2 */

/*
 * Test and initialization register (CODEC_TEST)
 */
#define CODEC_ORL               0x03    /* overrange left detect */
#define CODEC_ORR               0x0C    /* overrange right detect */
#define CODEC_DRS               0x10    /* data request status */
#define CODEC_ACI               0x20    /* autocalibrate in progress */
#define CODEC_PUR               0x40    /* playback underrun */
#define CODEC_COR               0x80    /* capture overrun */

/*
 * Miscellaneous control register (CODEC_MISC)
 */
#define CODEC_REVID             0x0F    /* AD1848 revision ID code */

/*
 * Digital mix control register (CODEC_DIGMIX)
 */
#define CODEC_DME               0x01    /* digital mix enable */
#define CODEC_DMA               0xFC    /* digital mix attenuation mask */

/*
 * Timeout for autocalibrate and DMA buffer length defines
 */
#define TIMEOUT                 100000  /* delay for sync and autocalib. */
#define BUFFERSIZE              50      /* buffer length in milliseconds */
#define BUFFRAGSIZE             32      /* buffer fragment size in bytes */


/*
 * AD1848 supported sampling frequencies table
 */
static WORD aFreqFmtTable[] =
{
    5512, (0 << 1) | 0x01,     /* CFS=0 XTAL2 */
    6615, (7 << 1) | 0x01,     /* CFS=7 XTAL2 */
    8000, (0 << 1) | 0x00,     /* CFS=0 XTAL1 */
    9600, (7 << 1) | 0x00,     /* CFS=7 XTAL1 */
    11025, (1 << 1) | 0x01,     /* CFS=1 XTAL2 */
    16000, (1 << 1) | 0x00,     /* CFS=1 XTAL1 */
    18900, (2 << 1) | 0x01,     /* CFS=2 XTAL2 */
    22050, (3 << 1) | 0x01,     /* CFS=3 XTAL2 */
    27428, (2 << 1) | 0x00,     /* CFS=2 XTAL1 */
    32000, (3 << 1) | 0x00,     /* CFS=3 XTAL1 */
    33075, (6 << 1) | 0x01,     /* CFS=6 XTAL2 */
    37800, (4 << 1) | 0x01,     /* CFS=4 XTAL2 */
    44100, (5 << 1) | 0x01,     /* CFS=5 XTAL2 */
    48000, (6 << 1) | 0x00      /* CFS=6 XTAL1 */
};


/*
 * AD1848 configuration structure
 */
static struct {
    WORD    wId;                        /* audio device identifier */
    WORD    wFormat;                    /* encoding data format */
    WORD    nSampleRate;                /* sampling frequency */
    WORD    wBasePort;                  /* audio device base port */
    WORD    wPort;                      /* codec base port */
    BYTE    nIrqLine;                   /* IRQ interrupt line */
    BYTE    nDmaChannel;                /* playback DMA channel */
    BYTE    nAdcDmaChannel;             /* capture DMA channel */
    BYTE    nClockFormat;               /* clock and data format */
    LPBYTE  lpBuffer;                   /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length */
    UINT    nPosition;                  /* playing DMA position */
    LPFNAUDIOWAVE lpfnAudioWave;        /* user callback function */
} Codec;


static VOID ADPortB(BYTE nIndex, BYTE bData)
{
    OUTB(Codec.wPort + CODEC_ADDR, nIndex);
    OUTB(Codec.wPort + CODEC_DATA, bData);
}

static BYTE ADPortRB(BYTE nIndex)
{
    OUTB(Codec.wPort + CODEC_ADDR, nIndex);
    return INB(Codec.wPort + CODEC_DATA);
}

static BOOL ADWaitSync(VOID)
{
    UINT n;

    /* wait until resynchronization finishes */
    for (n = 0; n < TIMEOUT; n++) {
        if (!(INB(Codec.wPort + CODEC_ADDR) & CODEC_ADDR_INIT))
            break;
    }
    return (n >= TIMEOUT);
}

static BOOL ADWaitACI(VOID)
{
    UINT n;

    /* wait until autocalibration finishes */
    for (n = 0; n < TIMEOUT; n++) {
        if (ADPortRB(CODEC_TEST) & CODEC_ACI)
            break;
    }
    if (n < TIMEOUT) {
        for (n = 0; n < TIMEOUT; n++) {
            if (!(ADPortRB(CODEC_TEST) & CODEC_ACI))
                break;
        }
    }
    return (n >= TIMEOUT);
}

static UINT ADProbe(VOID)
{
    BYTE nVersion /**, nLIC, nRIC**/;

    DEBUG(printf("ADProbe: clear interrupts and wait sync\n"));
    
    /*
     * Clear pending interrupts and wait until synchronization ends
     */
    INB(Codec.wPort + CODEC_STAT);
    OUTB(Codec.wPort + CODEC_STAT, 0x00);
    if (ADWaitSync())
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("ADProbe: check AD1848 read-only revision code\n"));

    /*
     * Check out AD1848's read-only revision ID code
     */
    nVersion = ADPortRB(CODEC_MISC);
    ADPortB(CODEC_MISC, nVersion ^ CODEC_REVID);
    if (ADPortRB(CODEC_MISC) != nVersion)
        return AUDIO_ERROR_NODEVICE;

    /**[1998/10/12]*****************************************
     * This check does not work with ENSONIQ AudioPCI cards
     * in Legacy emulation mode. Thanks to Bomb!
     *******************************************************/
#if 0
    /*
     * Check out if the left and right input control
     * registers are there.
     */
    nLIC = ADPortRB(CODEC_LIC);
    nRIC = ADPortRB(CODEC_RIC);
    ADPortB(CODEC_LIC, nLIC ^ CODEC_LIG);
    ADPortB(CODEC_RIC, nRIC ^ CODEC_RIG);
    if (ADPortRB(CODEC_LIC) != (nLIC ^ CODEC_LIG) ||
        ADPortRB(CODEC_RIC) != (nRIC ^ CODEC_RIG)) {
        ADPortB(CODEC_LIC, nLIC);
        ADPortB(CODEC_RIC, nRIC);
        return AUDIO_ERROR_NODEVICE;
    }
    ADPortB(CODEC_LIC, nLIC);
    ADPortB(CODEC_RIC, nRIC);
#endif

    DEBUG(printf("ADProbe: set interface configuration\n"));

    /*
     * Set interface configuration register (enable autocalibration
     * and DMA transfers for playback and capture, and use single
     * DMA channel if required)
     */
    ADPortB(CODEC_ADDR_MCE | CODEC_INTF, CODEC_ACAL |
	    (Codec.nDmaChannel != Codec.nAdcDmaChannel ? 0x00 : CODEC_SDC));
    ADPortB(CODEC_INTF, CODEC_ACAL | CODEC_SDC);

    DEBUG(printf("ADProbe: wait autocalibration\n"));

    /**[1998/10/12]*****************************************
     * This check timeouts with ENSONIQ AudioPCI cards, but
     * should not be removed for actual AD1848 codecs.
     */
#if 0
    if (ADWaitACI())
	return AUDIO_ERROR_NODEVICE;
#else
    ADWaitACI();
#endif

    return AUDIO_ERROR_NONE;
}

static VOID AIAPI ADInterruptHandler(VOID)
{
    /* Acknowledge AD1848's interrupt signals */
    if (INB(Codec.wPort + CODEC_STAT) & CODEC_STAT_INT)
        OUTB(Codec.wPort + CODEC_STAT, 0x00);
}

static VOID ADSetClockFormat(VOID)
{
    UINT n;

    /*
     * Determine playback format bit fields and select
     * clock source and frequency divisor values.
     */
    for (n = 0; n < sizeof(aFreqFmtTable) / sizeof(WORD) - 2; n += 2) {
        if (Codec.nSampleRate <= aFreqFmtTable[n])
            break;
    }
    Codec.nSampleRate = aFreqFmtTable[n];
    Codec.nClockFormat = aFreqFmtTable[n + 1];
    if (Codec.wFormat & AUDIO_FORMAT_16BITS) {
        Codec.nClockFormat |= CODEC_FMT;
    }
    if (Codec.wFormat & AUDIO_FORMAT_STEREO) {
        Codec.nClockFormat |= CODEC_SM;
    }

    DEBUG(printf("ADSetClockFormat: change clock and data format\n"));

    /*
     * Set AD1848 clock and data format register and waits
     * until the internal resynchronization and autocalibration
     * progress finishes.
     */
    ADPortB(CODEC_ADDR_MCE | CODEC_CDF, Codec.nClockFormat);
    ADWaitSync();
    ADPortB(CODEC_CDF, Codec.nClockFormat);
    ADWaitACI();
}

static VOID ADStartPlayback(VOID)
{
    WORD nCount;

    DEBUG(printf("ADStartPlayback: setup DMA channel\n"));

    /*
     * First setup the DMA controller parameters for
     * autoinit playback transfers.
     */
    DosSetupChannel(Codec.nDmaChannel, DMA_WRITE | DMA_AUTOINIT, 0);

    /*
     * Setup base count register, enable playback
     * and set DAC output control registers.
     */
    nCount = Codec.nBufferSize;
    if (Codec.wFormat & AUDIO_FORMAT_16BITS)
        nCount >>= 1;
    if (Codec.wFormat & AUDIO_FORMAT_STEREO)
        nCount >>= 1;
    nCount--;

    DEBUG(printf("ADStartPlayback: set frames and enable playback\n"));

    /* set sample frames per interrupt and enable playback */
    ADPortB(CODEC_LWR_COUNT, LOBYTE(nCount - 1));
    ADPortB(CODEC_UPR_COUNT, HIBYTE(nCount - 1));
    ADPortB(CODEC_INTF, ADPortRB(CODEC_INTF) | CODEC_PEN);

    DEBUG(printf("ADStartPlayback: unmute left and right DAC\n"));

    ADPortB(CODEC_LDC, ADPortRB(CODEC_LDC) & ~CODEC_LDM);
    ADPortB(CODEC_RDC, ADPortRB(CODEC_RDC) & ~CODEC_RDM);
}

static VOID ADStopPlayback(VOID)
{
    DEBUG(printf("ADStopPlayback: disable playback\n"));

    /*
     * Disable playback using the interface configuration
     * register and then disable the DMA controller channel.
     */
    ADPortB(CODEC_INTF, ADPortRB(CODEC_INTF) & ~CODEC_PEN);

    DEBUG(printf("ADStopPlayback: disable DMA channel\n"));

    DosDisableChannel(Codec.nDmaChannel);
}


static UINT ADOpenAudio(LPAUDIOINFO lpInfo)
{
    DWORD dwBytesPerSecond;

    DEBUG(printf("ADOpenAudio: probe AD1848 codec\n"));

    /*
     * Check if we actually have an AD1848 audio codec device
     */
    if (ADProbe())
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("ADOpenAudio: allocate DMA buffer area\n"));

    /*
     * Allocate and clean DMA channel buffer for playback
     */
    dwBytesPerSecond = Codec.nSampleRate;
    if (Codec.wFormat & AUDIO_FORMAT_16BITS)
        dwBytesPerSecond <<= 1;
    if (Codec.wFormat & AUDIO_FORMAT_STEREO)
        dwBytesPerSecond <<= 1;
    Codec.nBufferSize = dwBytesPerSecond / (1000 / BUFFERSIZE);
    Codec.nBufferSize = (Codec.nBufferSize + BUFFRAGSIZE) & -BUFFRAGSIZE;
    if (DosAllocChannel(Codec.nDmaChannel, Codec.nBufferSize))
        return AUDIO_ERROR_NOMEMORY;
    if ((Codec.lpBuffer = DosLockChannel(Codec.nDmaChannel)) != NULL) {
        memset(Codec.lpBuffer, Codec.wFormat & AUDIO_FORMAT_16BITS ?
	       0x00 : 0x80, Codec.nBufferSize);
        DosUnlockChannel(Codec.nDmaChannel);
    }

    DEBUG(printf("ADOpenAudio: install interrupt handler\n"));

    /*
     * Install AD1848 interrupt handler and enable interrupts
     */
    DosSetVectorHandler(Codec.nIrqLine, ADInterruptHandler);

    DEBUG(printf("ADOpenAudio: enable interrupts\n"));

    ADPortB(CODEC_PIN, CODEC_IEN);

    DEBUG(printf("ADOpenAudio: set frequency and start playback\n"));

    /*
     * Set frequency and encoding format, and enable playback
     */
    ADSetClockFormat();
    ADStartPlayback();

    DEBUG(printf("ADOpenAudio: done\n\n"));

    lpInfo->wFormat = Codec.wFormat;
    lpInfo->nSampleRate = Codec.nSampleRate;
    return AUDIO_ERROR_NONE;
}

static UINT ADCloseAudio(VOID)
{
    DEBUG(printf("ADCloseAudio: stop playback and DMA channel\n"));

    /*
     * Stop DMA playback transfer and releases the DMA channel buffer
     */
    ADStopPlayback();
    DosFreeChannel(Codec.nDmaChannel);

    DEBUG(printf("ADCloseAudio: restore interrupt handler\n"));

    /*
     * Restore the interrupt handler and disable/clear pending interrupts
     */
    DosSetVectorHandler(Codec.nIrqLine, NULL);

    DEBUG(printf("ADCloseAudio: disable and clear interrupts\n"));

    INB(Codec.wPort + CODEC_STAT);
    OUTB(Codec.wPort + CODEC_STAT, 0x00);
    ADPortB(CODEC_PIN, 0x00);

    DEBUG(printf("ADCloseAudio: done\n\n"));

    return AUDIO_ERROR_NONE;
}

static UINT ADUpdateAudio(UINT nFrames)
{
    int nBlockSize, nSize;

    if (Codec.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (Codec.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > Codec.nBufferSize)
        nFrames = Codec.nBufferSize;

    if ((Codec.lpBuffer = DosLockChannel(Codec.nDmaChannel)) != NULL) {
        nBlockSize = Codec.nBufferSize - Codec.nPosition -
            DosGetChannelCount(Codec.nDmaChannel);
        if (nBlockSize < 0)
            nBlockSize += Codec.nBufferSize;

        if (nBlockSize > nFrames)
            nBlockSize = nFrames;

        nBlockSize &= -BUFFRAGSIZE;
        while (nBlockSize != 0) {
            nSize = Codec.nBufferSize - Codec.nPosition;
            if (nSize > nBlockSize)
                nSize = nBlockSize;
            if (Codec.lpfnAudioWave != NULL) {
                Codec.lpfnAudioWave(&Codec.lpBuffer[Codec.nPosition], nSize);
            }
            else {
                memset(&Codec.lpBuffer[Codec.nPosition],
		       Codec.wFormat & AUDIO_FORMAT_16BITS ?
		       0x00 : 0x80, nSize);
            }
            if ((Codec.nPosition += nSize) >= Codec.nBufferSize)
                Codec.nPosition -= Codec.nBufferSize;
            nBlockSize -= nSize;
        }
        DosUnlockChannel(Codec.nDmaChannel);
    }
    return AUDIO_ERROR_NONE;
}

static UINT ADSetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    Codec.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}

/*--------------------- Windows Sound System driver ---------------------*/

/*
 * Windows Sound System (AD1848) direct register offsets
 */
#define WSS_CODEC_INTF          0x000   /* interface register */
#define WSS_CODEC_CHIPID        0x003   /* ID test code register */
#define WSS_CODEC_BASE          0x004   /* AD1848 codec base register */

static UINT AIAPI WSSProbe(VOID)
{
    /*
     * Windows Sound System IRQ and DMA interface latches
     */
    static BYTE aIrqTable[16] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x10, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00
    };
    static BYTE aDmaTable[8] =
    {
        0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00
    };

#if 0
    BYTE nChipID;

    /*
     * Check out for a WSS card, then probe the AD1848 audio
     * codec to make sure the hardware is present.
     * Check out if the I/O port returns a valid signature,
     * the original WSS card returns 0x04 while some other
     * cards (eg. AudioTrix Pro) returns 0x00 or 0x0F.
     */
    nChipID = INB(Codec.wBasePort + WSS_CODEC_CHIPID);
    if ((nChipID & 0x3F) != 0x04 && (nChipID & 0x3F) != 0x00 &&
        (nChipID & 0x3F) != 0x0F)
        return AUDIO_ERROR_NODEVICE;
#endif
    if (!aIrqTable[Codec.nIrqLine] || !aDmaTable[Codec.nDmaChannel])
        return AUDIO_ERROR_NODEVICE;
    OUTB(Codec.wBasePort + WSS_CODEC_INTF,
	 aIrqTable[Codec.nIrqLine] | aDmaTable[Codec.nDmaChannel]);
    return ADProbe();
}

static UINT AIAPI WSSGetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_WSS, "Windows Sound System",
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

static UINT AIAPI WSSPingAudio(VOID)
{
    static WORD aPorts[4] = { 0x530, 0xE80, 0xF40, 0x604 };
    static BYTE aIrqs[8]  = { 7, 7, 9, 10, 11, 7, 7, 7 };
    static BYTE aDmas[4]  = { 1, 0, 1, 3 };
    WORD wIntf;
    UINT n, nChar;
    LPSTR lpszCfg;

    Codec.wId = AUDIO_PRODUCT_WSS;
    Codec.wBasePort = 0x530;
    Codec.wPort = 0x530 + WSS_CODEC_BASE;
    Codec.nIrqLine = 7;
    Codec.nDmaChannel = 1;
    Codec.nAdcDmaChannel = 1;
    if ((lpszCfg = DosGetEnvironment("WSSCFG")) != NULL) {
        nChar = DosParseString(lpszCfg, TOKEN_CHAR);
        while (nChar != 0) {
            switch (nChar) {
            case 'A':
            case 'a':
                Codec.wBasePort = DosParseString(NULL, TOKEN_HEX);
                Codec.wPort = Codec.wBasePort + WSS_CODEC_BASE;
                break;
            case 'I':
            case 'i':
                Codec.nIrqLine = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'D':
            case 'd':
                Codec.nDmaChannel = DosParseString(NULL, TOKEN_DEC);
                Codec.nAdcDmaChannel = Codec.nDmaChannel;
                break;
            }
            nChar = DosParseString(NULL, TOKEN_CHAR);
        }
        return WSSProbe();
    }
    else {
        for (n = 0; n < sizeof(aPorts) / sizeof(WORD); n++) {
            Codec.wBasePort = aPorts[n];
            Codec.wPort = Codec.wBasePort + WSS_CODEC_BASE;

            /**[1998/12/02**********************************
             * Experimental code to autodetect the IRQ and
             * DMA resource allocated for the WSS card.
             */ 
            wIntf = INB(Codec.wBasePort + WSS_CODEC_INTF);
            Codec.nIrqLine = aIrqs[(wIntf >> 3) & 0x07];
            Codec.nDmaChannel =
		Codec.nAdcDmaChannel = aDmas[wIntf & 0x03];
            if (!WSSProbe())
                return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_NODEVICE;
    }
}

static UINT AIAPI WSSOpenAudio(LPAUDIOINFO lpInfo)
{
    /*
     * Initialize AD1848 configuration structure
     * and setup the IRQ and DMA interface
     */
    memset(&Codec, 0, sizeof(Codec));
    Codec.wFormat = lpInfo->wFormat;
    Codec.nSampleRate = lpInfo->nSampleRate;
    if (WSSPingAudio())
        return AUDIO_ERROR_NODEVICE;

    /* Initialize common AD1848 codec chip audio driver */
    return ADOpenAudio(lpInfo);
}

static UINT AIAPI WSSCloseAudio(VOID)
{
    /* Shutdown common AD1848 codec chip audio driver */
    ADCloseAudio();
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI WSSUpdateAudio(UINT nFrames)
{
    return ADUpdateAudio(nFrames);
}

static UINT AIAPI WSSSetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    return ADSetAudioCallback(lpfnAudioWave);
}

/*--------------------- Gravis Ultrasound Max driver ---------------------*/

/*
 * Ultrasound Max (CS4231) direct register offsets
 */
#define GUS_BOARD_MIX_CTRL      0x000   /* (2X0) mix control register */
#define GUS_CODEC_CTRL          0x106   /* (3X6) codec control register */
#define GUS_CODEC_BASE          0x10C   /* (3XC) codec base address */
#define GUS_BOARD_VERSION       0x506   /* (7X6) board version */


static UINT GUSProbe(VOID)
{
    BYTE nRevId, nIntf;

    /*
     * Check whether we have a GUS MAX board reading
     * the board revision level register (7X6):
     *
     * Revision ID  Board Revision
     * 0xFF         Pre 3.7 boards without ICS mixer.
     * 0x05         Rev 3.7 with ICS mixer (L/R flip problems).
     * 0x06-0x09    Rev 3.7 and above. ICS mixer present.
     * 0x0A-0xNN    UltraMax boards. CS4231 present, no ICS mixer.
     */
    nRevId = INB(Codec.wBasePort + GUS_BOARD_VERSION);
    if (Codec.wId == AUDIO_PRODUCT_GUSMAX) {
        if (nRevId < 0x0A)
            return AUDIO_ERROR_NODEVICE;

        /* Setup the GUS MAX DMA interface */
        nIntf = 0x40 | ((Codec.wPort >> 4) & 0x0F);
        if (Codec.nAdcDmaChannel >= 4)
            nIntf |= 0x10;
        if (Codec.nDmaChannel >= 4)
            nIntf |= 0x20;
        OUTB(Codec.wBasePort + GUS_CODEC_CTRL, nIntf);
    }
    else {
        if (nRevId >= 0x0A)
            return AUDIO_ERROR_NODEVICE;
    }
    return ADProbe();
}

static UINT AIAPI GUSGetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_GUSMAX, "Ultrasound Max (CS4231 Codec)",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };
    static AUDIOCAPS CapsDB =
    {
        AUDIO_PRODUCT_GUSDB, "Ultrasound Daugherboard",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    memcpy(lpCaps, Codec.wId != AUDIO_PRODUCT_GUSDB ?
	   &Caps : &CapsDB, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI GUSPingAudio(VOID)
{
    LPSTR lpszUltrasnd, lpszUltra16;
    UINT wPort, nDramDmaChannel, nAdcDmaChannel, nIrqLine;
    UINT wCodecPort, nCodecDmaChannel, nCodecIrqLine, nCodecType;

    if ((lpszUltrasnd = DosGetEnvironment("ULTRASND")) != NULL &&
        (lpszUltra16 = DosGetEnvironment("ULTRA16")) != NULL) {
        if ((wPort = DosParseString(lpszUltrasnd, TOKEN_HEX)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nDramDmaChannel = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nAdcDmaChannel = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nIrqLine = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (/*nMidiIrqLine =*/ DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (wCodecPort = DosParseString(lpszUltra16, TOKEN_HEX)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nCodecDmaChannel = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nCodecIrqLine = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nCodecType = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (/*wCdromPort =*/ DosParseString(NULL, TOKEN_HEX)) != BAD_TOKEN) {
            if (nCodecType == 1) {
                Codec.wId = AUDIO_PRODUCT_GUSMAX;
                Codec.wBasePort = wPort;
                Codec.wPort = wCodecPort;
                Codec.nIrqLine = nIrqLine;
                Codec.nDmaChannel = nAdcDmaChannel;
                Codec.nAdcDmaChannel = nDramDmaChannel;
                return GUSProbe();
            }
            else {
                Codec.wId = AUDIO_PRODUCT_GUSDB;
                Codec.wBasePort = wPort;
                Codec.wPort = wCodecPort;
                Codec.nIrqLine = nCodecIrqLine;
                Codec.nDmaChannel = nCodecDmaChannel;
                Codec.nAdcDmaChannel = nCodecDmaChannel;
                return GUSProbe();
            }
        }
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI GUSOpenAudio(LPAUDIOINFO lpInfo)
{
    /*
     * Initialize AD1848 configuration structure
     */
    memset(&Codec, 0, sizeof(Codec));
    Codec.wFormat = lpInfo->wFormat;
    Codec.nSampleRate = lpInfo->nSampleRate;
    if (GUSPingAudio())
        return AUDIO_ERROR_NODEVICE;

    /*
     * Enable DAC output and interrupts, disable MIC and
     * line input on the GUS MAX mixer control register.
     */
    OUTB(Codec.wBasePort + GUS_BOARD_MIX_CTRL, 0x09);

    /* Initialize common AD1848/CS4231 codec chip audio driver */
    return ADOpenAudio(lpInfo);
}

static UINT AIAPI GUSCloseAudio(VOID)
{
    BYTE nIntf;

    /* Shutdown common AD1848 codec audio driver */
    ADCloseAudio();

    /* Clean up GUS MAX playback/capture DMA interface latches */
    if (Codec.wId == AUDIO_PRODUCT_GUSMAX) {
        nIntf = 0x40 | ((Codec.wPort >> 4) & 0x0F);
        if (Codec.nAdcDmaChannel >= 4)
            nIntf |= 0x10;
        if (Codec.nDmaChannel >= 4)
            nIntf |= 0x20;

        OUTB(Codec.wBasePort + GUS_CODEC_CTRL, nIntf & ~0x10);
        OUTB(Codec.wBasePort + GUS_CODEC_CTRL, nIntf);

        OUTB(Codec.wBasePort + GUS_CODEC_CTRL, nIntf & ~0x20);
        OUTB(Codec.wBasePort + GUS_CODEC_CTRL, nIntf);
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI GUSUpdateAudio(UINT nFrames)
{
    return ADUpdateAudio(nFrames);
}

static UINT AIAPI GUSSetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    return ADSetAudioCallback(lpfnAudioWave);
}

/*---------------------- Ensoniq Soundscape driver ----------------------*/

/*
 * * Ensoniq Soundscape (AD1848) gate-array register offsets
 */
#define ESS_HOST                0x002   /* host control register */
#define ESS_HOST_DATA           0x003   /* host data register */
#define ESS_ADDR                0x004   /* index address register */
#define ESS_DATA                0x005   /* indexed data register */
#define ESS_CODEC_BASE          0x008   /* AD1848 codec base register */

/*
 * Ensoniq Soundscape gate-array indirect registers
 */
#define ESS_DMAB                0x03    /* DMA channel B assign */
#define ESS_INTCFG              0x04    /* interrupt config register */
#define ESS_DMACFG              0x05    /* DMA config register */
#define ESS_CDCFG               0x06    /* CDROM/Codec config register */
#define ESS_HMCTRL              0x09    /* host master control register */

/*
 * Ensoniq Soundscape gate-array chip ID defines
 */
#define ESS_CHIP_ODIE           0       /* ODIE gate array */
#define ESS_CHIP_OPUS           1       /* OPUS gate array */
#define ESS_CHIP_MMIC           2       /* MiMIC gate array */

/*
 * Ensoniq Soundscape private structure
 */
static struct {
    BYTE    nChip;
    BYTE    nProduct;
    BYTE    nDmaB;
    BYTE    nIntCfg;
    BYTE    nDmaCfg;
    BYTE    nCdCfg;
} ESS;

static VOID ESSPortB(BYTE nIndex, BYTE bData)
{
    OUTB(Codec.wBasePort + ESS_ADDR, nIndex);
    OUTB(Codec.wBasePort + ESS_DATA, bData);
}

static BYTE ESSPortRB(BYTE nIndex)
{
    OUTB(Codec.wBasePort + ESS_ADDR, nIndex);
    return INB(Codec.wBasePort + ESS_DATA);
}

static UINT AIAPI ESSGetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_ESS, "Ensoniq Soundscape",
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

static UINT AIAPI ESSPingAudio(VOID)
{
    static CHAR szFileName[80], szLine[80];
    LPSTR lpszSndscape, lpszName, lpszValue;
    UINT nProduct, wPort, wWavePort, nMidiIrqLine, nWaveIrqLine, nDmaChannel;
    FILE *fpSndScape;

    DEBUG(printf("ESSPingAudio: read SNDSCAPE.INI hardware profile\n"));

    /*
     * Check out if the SNDSCAPE environment variable is present,
     * and read the hardware parameters from the SNDSCAPE.INI file.
     */
    if ((lpszSndscape = DosGetEnvironment("SNDSCAPE")) == NULL)
        return AUDIO_ERROR_NODEVICE;
    strcpy(szFileName, lpszSndscape);
    if (szFileName[strlen(szFileName) - 1] == '\\')
        szFileName[strlen(szFileName) - 1] = 0;
    strcat(szFileName, "\\SNDSCAPE.INI");
    if ((fpSndScape = fopen(szFileName, "r")) == NULL)
        return AUDIO_ERROR_NODEVICE;
    nProduct = 0;
    wPort = 0xFFFF;
    wWavePort = 0xFFFF;
    nMidiIrqLine = 0xFFFF;
    nWaveIrqLine = 0xFFFF;
    nDmaChannel = 0xFFFF;
    while (fgets(szLine, sizeof(szLine) - 1, fpSndScape) != NULL) {
        /*
         * Parse the next line from the SNDSCAPE.INI file
         * which has the following syntax:
         *
         * Line ::= {Identifier} '=' {String} {Blank}* {EOL}
         *
         * There should not be blanks between the tokens
         * and the identifiers are case-insensitive.
         *
         * Ref: ENSONIQ Soundscape Digital Audio Driver
         *      Created 09/08/95 by Andrew P. Weir
         */
        strupr(szLine);
        lpszName = szLine;
        if ((lpszValue = strchr(szLine, '=')) == NULL)
            continue;
        *lpszValue++ = 0;

        if (!strcmp(lpszName, "PRODUCT")) {
            if (strstr(lpszValue, "SOUNDFX") || strstr(lpszValue, "MEDIA_FX"))
                nProduct = 1;
        }
        else if (!strcmp(lpszName, "PORT")) {
            wPort = DosParseString(lpszValue, TOKEN_HEX);
        }
        else if (!strcmp(lpszName, "WAVEPORT")) {
            wWavePort = DosParseString(lpszValue, TOKEN_HEX);
        }
        else if (!strcmp(lpszName, "IRQ")) {
            nMidiIrqLine = DosParseString(lpszValue, TOKEN_DEC);
        }
        else if (!strcmp(lpszName, "SBIRQ")) {
            nWaveIrqLine = DosParseString(lpszValue, TOKEN_DEC);
        }
        else if (!strcmp(lpszName, "DMA")) {
            nDmaChannel = DosParseString(lpszValue, TOKEN_DEC);
        }
    }
    fclose(fpSndScape);
    if ((wPort|wWavePort|nMidiIrqLine|nWaveIrqLine|nDmaChannel) == 0xFFFF)
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("ESSPingAudio: Port=0x%03x, WavePort=0x%03x, MidiIrq=%d, WaveIrq=%d, Dma=%d\n",
		 wPort, wWavePort, nMidiIrqLine, nWaveIrqLine, nDmaChannel));

    /*
     * Initialize the AD1848 codec driver variables
     */
    ESS.nProduct = nProduct;
    Codec.wId = AUDIO_PRODUCT_ESS;
    Codec.wBasePort = wPort;
    Codec.wPort = wWavePort;
    Codec.nIrqLine = nWaveIrqLine;
    Codec.nDmaChannel = nDmaChannel;
    Codec.nAdcDmaChannel = nDmaChannel;

    DEBUG(printf("ESSPingAudio: check Ensoniq Soundscape card\n"));

    /*
     * Check if there is an Ensoniq Soundscape present
     */
    if ((INB(Codec.wBasePort + ESS_HOST) & 0x78) != 0x00)
        return AUDIO_ERROR_NODEVICE;
    if ((INB(Codec.wBasePort + ESS_ADDR) & 0xF0) == 0xF0)
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("ESSPingAudio: check Ensoniq GA chip revision\n"));

    /*
     * Check Ensoniq Soundscape gate-array chip version
     */
    OUTB(Codec.wBasePort + ESS_ADDR, 0xF5);
    ESS.nChip = INB(Codec.wBasePort + ESS_ADDR);
    if ((ESS.nChip & 0xF0) == 0xF0 || (ESS.nChip & 0x0F) != 0x05)
        return AUDIO_ERROR_NODEVICE;
    if (ESS.nChip & 0x80)
        ESS.nChip = ESS_CHIP_MMIC;
    else if (ESS.nChip & 0x70)
        ESS.nChip = ESS_CHIP_OPUS;
    else
        ESS.nChip = ESS_CHIP_ODIE;

    DEBUG(printf("ESSPingAudio: probe AD1848 codec\n"));

    return ADProbe();
}

static UINT AIAPI ESSOpenAudio(LPAUDIOINFO lpInfo)
{
    static BYTE aIrqTable[2][16] =
    {
        {
            /* Ensoniq Soundscape IRQ interface latches */
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0x02,
            0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        },
        {
            /* SoundFX and MediaFX old Ensoniq IRQ interface latches */
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0xFF, 0x01,
            0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03
        }
    };

    DEBUG(printf("ESSOpenAudio: initialize ENSONIQ card\n"));

    /*
     * Initialize AD1848 configuration structure
     */
    memset(&Codec, 0, sizeof(Codec));
    Codec.wFormat = lpInfo->wFormat;
    Codec.nSampleRate = lpInfo->nSampleRate;
    if (ESSPingAudio())
        return AUDIO_ERROR_NODEVICE;

    /* do some resource rerouting if necessary */
    if (ESS.nChip != ESS_CHIP_MMIC) {

        DEBUG(printf("ESSOpenAudio: setup CODEC DMA polarity\n"));

        /* setup Codec DMA polarity */
        ESSPortB(ESS_DMACFG, 0x50);

        DEBUG(printf("ESSOpenAudio: lock DMA and IRQ resources\n"));

        /* give Codec control of DMA and wave IRQ resources */
        ESS.nCdCfg = ESSPortRB(ESS_CDCFG);
        ESSPortB(ESS_CDCFG, 0x89 | (Codec.nDmaChannel << 4) |
		 (aIrqTable[ESS.nProduct][Codec.nIrqLine] << 1));

        DEBUG(printf("ESSOpenAudio: pull resources from SB emulation\n"));

        /* pull off the SB emulation off of those resources */
        ESS.nDmaB = ESSPortRB(ESS_DMAB);
        ESSPortB(ESS_DMAB, 0x20);
        ESS.nIntCfg = ESSPortRB(ESS_INTCFG);
        ESSPortB(ESS_INTCFG, 0xF0 | aIrqTable[ESS.nProduct][Codec.nIrqLine] |
		 (aIrqTable[ESS.nProduct][Codec.nIrqLine] << 2));
    }

    DEBUG(printf("ESSOpenAudio: open AD1848 audio driver\n"));

    return ADOpenAudio(lpInfo);
}

static UINT AIAPI ESSCloseAudio(VOID)
{
    DEBUG(printf("ESSCloseAudio: shutdown AD1848 audio driver\n"));

    /* shutdown AD1848 codec playback transfers */
    ADCloseAudio();

    /* if neccesary, restore gate array resource registers */
    if (ESS.nChip != ESS_CHIP_MMIC) {

        DEBUG(printf("ESSCloseAudio: restore Ensoniq GA resources\n"));

        ESSPortB(ESS_INTCFG, ESS.nIntCfg);
        ESSPortB(ESS_DMAB, ESS.nDmaB);
        ESSPortB(ESS_CDCFG, ESS.nCdCfg);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI ESSUpdateAudio(UINT nFrames)
{
    return ADUpdateAudio(nFrames);
}

static UINT AIAPI ESSSetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    return ADSetAudioCallback(lpfnAudioWave);
}


/*
 * Windows Sound System, Ultrasound Max & Ensoniq Soundscape public interfaces
 */
AUDIOWAVEDRIVER WinSndSystemWaveDriver =
{
    WSSGetAudioCaps, WSSPingAudio,
    WSSOpenAudio, WSSCloseAudio,
    WSSUpdateAudio, WSSSetAudioCallback
};

AUDIOWAVEDRIVER UltraSoundMaxWaveDriver =
{
    GUSGetAudioCaps, GUSPingAudio,
    GUSOpenAudio, GUSCloseAudio,
    GUSUpdateAudio, GUSSetAudioCallback
};

AUDIOWAVEDRIVER SoundscapeWaveDriver =
{
    ESSGetAudioCaps, ESSPingAudio,
    ESSOpenAudio, ESSCloseAudio,
    ESSUpdateAudio, ESSSetAudioCallback
};

AUDIODRIVER WinSndSystemDriver =
{
    &WinSndSystemWaveDriver, NULL
};

AUDIODRIVER UltraSoundMaxDriver =
{
    &UltraSoundMaxWaveDriver, NULL
};

AUDIODRIVER SoundscapeDriver =
{
    &SoundscapeWaveDriver, NULL
};
