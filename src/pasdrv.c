/*
 * $Id: pasdrv.c 1.5 1996/08/05 18:51:19 chasan released $
 *
 * Pro Audio Spectrum series audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"

/*
 * Pro Audio Spectrum cards default I/O base addresses
 */
#define PAS_DEFAULT_BASE        0x0388  /* default base port address */
#define PAS_ALT_BASE_1          0x0384  /* first alternative base address */
#define PAS_ALT_BASE_2          0x038C  /* second alternative base address */
#define PAS_ALT_BASE_3          0x0288  /* third alternative base address */

/*
 * Pro Audio Spectrum direct register offsets
 */
#define PAS_AUDIOMXR            0x0B88  /* audio mixer control register */
#define PAS_AUDIOFILT           0x0B8A  /* audio filter control register */
#define PAS_INTRSTAT            0x0B89  /* interrupt status register */
#define PAS_INTRCTLR            0x0B8B  /* interrupt control register */
#define PAS_PCMDATA             0x0F88  /* PCM data PIO register */
#define PAS_CROSSCHANNEL        0x0F8A  /* cross channel control register */
#define PAS_SAMPLERATE          0x1388  /* sample rate timer register */
#define PAS_SAMPLECNT           0x1389  /* sample count register */
#define PAS_SPKRTMR             0x138A  /* local speaker timer address */
#define PAS_TMRCTLR             0x138B  /* local timer control register */

/*
 * Pro Audio Spectrum PAS2/CDPC (digital ASIC) direct register offsets
 */
#define PAS_MASTERCHIPR         0xFF88  /* master chip revision */
#define PAS_ENHSCSI             0x7F89  /* Enhanced SCSI detect port */
#define PAS_SYSCONFIG2          0x8389  /* system configuration register */
#define PAS_COMPATREGE          0xF788  /* compatible register enable */
#define PAS_MASTERMODRD         0xFF8B  /* master mode read */
#define PAS_SLAVEMODRD          0xEF8B  /* slave mode read */

/*
 * Interrupt control register bit fields
 */
#define IC_INTRMASK             0x1F    /* interrupt mask bit fields */
#define IC_REVBITS              0xE0    /* revision mask bit fields */
#define IC_LEFTFM               0x01    /* left FM interrupt enable */
#define IC_RIGHTFM              0x02    /* right FM interrupt enable */
#define IC_SAMPRATE             0x04    /* sample rate timer intr enable */
#define IC_SAMPBUFF             0x08    /* sample buffer timer intr enable */
#define IC_MIDI                 0x10    /* MIDI interrupt enable */

/*
 * Interrupt status register bit fields
 */
#define IS_INTRMASK             0x1F    /* interrupts mask bit fields */
#define IS_LEFTFM               0x01    /* FM left interrupt active */
#define IS_RIGHTFM              0x02    /* FM right interrupt active */
#define IS_SAMPRATE             0x04    /* sample rate interrupt active */
#define IS_SAMPBUFF             0x08    /* sample buffer interrupt active */
#define IS_MIDI                 0x10    /* MIDI interrupt active */
#define IS_PCMLR                0x20    /* PCM channel interrupt active */
#define IS_ACTIVE               0x40    /* hardware is active */
#define IS_CLIPPING             0x80    /* sample clipping has occured */

/*
 * Audio filter select register bit fields
 */
#define AF_FILTMASK             0x1F    /* filter select and decode bits */
#define AF_FILT1                0x01    /* select 17897 Hz filter */
#define AF_FILT2                0x02    /* select 15909 Hz filter */
#define AF_FILT3                0x09    /* select 11931 Hz filter */
#define AF_FILT4                0x11    /* select  8948 Hz filter */
#define AF_FILT5                0x19    /* select  5965 Hz filter */
#define AF_FILT6                0x04    /* select  2982 Hz filter */
#define AF_UNMUTE               0x20    /* filter unmute control */
#define AF_SAMPRATE             0x40    /* sample rate timer enable */
#define AF_SAMPBUFF             0x80    /* sample buffer counter enable */

/*
 * Cross channel control register bit fields
 */
#define CC_CROSSMASK            0x0F    /* cross channels bit fields */
#define CC_PCMMASK              0xF0    /* PCM/DMA control bit fields */
#define CC_R2R                  0x01    /* cross channel right->right */
#define CC_L2R                  0x02    /* cross channel left->right */
#define CC_R2L                  0x04    /* cross channel right->left */
#define CC_L2L                  0x08    /* cross channel left->left */
#define CC_DAC                  0x10    /* select DAC output mode */
#define CC_ADC                  0x00    /* select ADC input mode */
#define CC_MONO                 0x20    /* select mono mode */
#define CC_STEREO               0x00    /* select stereo mode */
#define CC_ENAPCM               0x40    /* enable PCM machine */
#define CC_DRQ                  0x80    /* enable DMA transfers */

/*
 * Local timer control register bit fields
 */
#define TC_BIN                  0x00    /* select binary counting mode */
#define TC_BCD                  0x01    /* select BCD counting mode */
#define TC_SQUAREWAVE           0x04    /* select square wave generator */
#define TC_RATE                 0x06    /* select rate generator */
#define TC_16BIT                0x30    /* set first LSB and then MSB */
#define TC_SAMPRATE             0x00    /* select sample rate timer */
#define TC_SAMPBUFF             0x40    /* select buffer counter timer */
#define TC_PCSPKR               0x80    /* select PC speaker counter timer */

/*
 * Sample size and oversampling register bit fields
 */
#define SC2_OVRSMP              0x03    /* oversampling bit fields */
#define SC2_OVRSMP1X            0x00    /* select over sampling rate to 1X */
#define SC2_OVRSMP2X            0x01    /* select over sampling rate to 2X */
#define SC2_OVRSMP4X            0x03    /* select over sampling rate to 4X */
#define SC2_16BIT               0x04    /* select 16 bit audio */
#define SC2_12BIT               0x08    /* select 12 bit interleaving */

/*
 * Compatible register enable bit fields
 */
#define CP_MPUEMUL              0x01    /* MPU-401 emulation enabled */
#define CP_SBEMUL               0x02    /* SB emulation enabled */

/*
 * Slave mode read register bit fields
 */
#define SMRD_DRVTYPE            0x03    /* driver interface type */
#define SMRD_FMTYPE             0x04    /* 3812(0) or OPL3(1) FM chip */
#define SMRD_DACTYPE            0x08    /* 8-bit DAC(0) or 16-bit DAC(1) */
#define SMRD_IMIDI              0x10    /* use internal MIDI */
#define SMRD_SWREP              0x80    /* switch is auto repeating */

/*
 * Master mode read register bit fields
 */
#define MMRD_ATPS2              0x01    /* PS2(0) or AT(1) bus */
#define MMRD_TMREMUL            0x02    /* timer emulation enabled */
#define MMRD_MSMD               0x04    /* master(0) or slave(1) mode */
#define MMRD_SLAVE              0x08    /* slave power on or device present */
#define MMRD_ATTIM              0x10    /* XT/AT timing */
#define MMRD_MSTREV             0xE0    /* master revision level */

/*
 * Pro Audio Spectrum product feature bits
 */
#define MVA508                  0x0001  /* MVA508(1) or National(0) */
#define MVPS2                   0x0002  /* PS2(1) or AT(0) bus */
#define MVSLAVE                 0x0004  /* CDPC slave device */
#define MVSCSI                  0x0008  /* SCSI interface */
#define MVENHSCSI               0x0010  /* Enhanced SCSI interface */
#define MVSONY                  0x0020  /* Sony 535 interface */
#define MVDAC16                 0x0040  /* 16 bit DAC */
#define MVSBEMUL                0x0080  /* SB hardware emulation */
#define MVMPUEMUL               0x0100  /* MPU-401 hardware emulation */
#define MVOPL3                  0x0200  /* OPL3(1) or 3812(0) chip */
#define MV101                   0x0400  /* MV101 ASIC */
#define MV101_REV_BITS          0x3800  /* MV101 revision bit fields */

/*
 * Pro Audio Spectrum product feature lists
 */
#define PRODUCT_PROAUDIO        (MVSCSI)
#define PRODUCT_PROPLUS         (MV101|MVSCSI|MVENHSCSI|MVSBEMUL|MVOPL3)
#define PRODUCT_PRO16           (MV101|MVA508|MVSCSI|MVENHSCSI|MVSBEMUL|MVDAC16|MVOPL3)
#define PRODUCT_CDPC            (MV101|MVSLAVE|MVSONY|MVSBEMUL|MVDAC16|MVOPL3)
#define PRODUCT_CAREBITS        (MVA508|MVDAC16|MVOPL3|MV101)

/*
 * PAS clock source frequency and buffer size
 */
#define CLOCKFREQ               1193180 /* clock frequency in Hertz */
#define BUFFERSIZE              50      /* buffer length in milliseconds */
#define BUFFRAGSIZE             32      /* buffer fragment size in bytes */


/*
 * Pro Audio Spectrum configuration structure
 */
static struct {
    WORD    wFormat;                    /* playback encoding format */
    WORD    nSampleRate;                /* sampling frequency */
    WORD    wId;                        /* audio device identifier */
    WORD    wProduct;                   /* product feature bits */
    WORD    wPort;                      /* device base port */
    BYTE    nIrqLine;                   /* interrupt line */
    BYTE    nDmaChannel;                /* output DMA channel */
    LPBYTE  lpBuffer;                   /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length */
    UINT    nPosition;                  /* DMA buffer playing position */
    LPFNAUDIOWAVE lpfnAudioWave;        /* user callback routine */
} PAS;


/*
 * Pro Audio Spectrum low level routines
 */
static VOID PASPortB(WORD nIndex, BYTE bData)
{
    OUTB(PAS.wPort ^ nIndex, bData);
}

static BYTE PASPortRB(WORD nIndex)
{
    return INB(PAS.wPort ^ nIndex);
}

static UINT PASProbe(VOID)
{
    DWORD dwSampleRate;
    BYTE nRevId, mmrd, smrd;

    /*
     * Probe PAS hardware checking the revision ID code from
     * the interrupt mask register (it should be read only).
     */
    if ((nRevId = PASPortRB(PAS_INTRCTLR)) == 0xFF)
        return AUDIO_ERROR_NODEVICE;
    PASPortB(PAS_INTRCTLR, nRevId ^ IC_REVBITS);
    if (PASPortRB(PAS_INTRCTLR) != nRevId)
        return AUDIO_ERROR_NODEVICE;

    /*
     * Checks the installed hardware for all the feature bits,
     * and determine which PAS hardware card is present.
     */
    PAS.wProduct = MVSCSI;
    if ((nRevId & IC_REVBITS) != 0x00) {
        /* all second generation PAS cards use MV101 and have SB emulation */
        PAS.wProduct |= (MVSBEMUL | MV101);

        /* determine if the enhanced SCSI interface is present */
        PASPortB(PAS_ENHSCSI, 0x00);
        if (!(PASPortRB(PAS_ENHSCSI) & 0x01))
            PAS.wProduct |= MVENHSCSI;

        /* determine AT/PS2, CDPC slave mode */
        mmrd = PASPortRB(PAS_MASTERMODRD);
        if (!(mmrd & MMRD_ATPS2))
            PAS.wProduct |= MVPS2;
        if (mmrd & MMRD_MSMD)
            PAS.wProduct |= MVSLAVE;

        /* merge in the chip revision level */
        PAS.wProduct |= (WORD)(PASPortRB(PAS_MASTERCHIPR) & 0x0F) << 11;

        /* determine CDROM type, FM chip, 8/16 bit DAC, and mixer */
        smrd = PASPortRB(PAS_SLAVEMODRD);
        if (smrd & SMRD_DACTYPE)
            PAS.wProduct |= MVDAC16;
        if (smrd & SMRD_FMTYPE)
            PAS.wProduct |= MVOPL3;
        if ((PAS.wProduct & (MVSLAVE | MVDAC16)) == MVDAC16)
            PAS.wProduct |= MVA508;
        if ((smrd & SMRD_DRVTYPE) == 2) {
            PAS.wProduct &= ~(MVSCSI | MVENHSCSI);
            PAS.wProduct |= MVSONY;
        }

        /* determine if MPU-401 emulation is active */
        if (PASPortRB(PAS_COMPATREGE) & CP_MPUEMUL)
            PAS.wProduct |= MVMPUEMUL;
    }
    /*
     * Assume we have a PAS16 compatible card if the MV101 ASIC
     * chip and the 16-bit DAC are both present.
     */
    PAS.wId = (PAS.wProduct & (MV101 | MVDAC16)) == (MV101 | MVDAC16) ?
        AUDIO_PRODUCT_PAS16 : AUDIO_PRODUCT_PAS;

    /*
     * Check out playback encoding format and sampling
     * frequencies for the specified PAS sound card.
     */
    if (PAS.wId != AUDIO_PRODUCT_PAS16)
        PAS.wFormat &= ~AUDIO_FORMAT_16BITS;
    if (PAS.nSampleRate < 5000)
        PAS.nSampleRate = 5000;
    if (PAS.nSampleRate > 44100)
        PAS.nSampleRate = 44100;

    /* get actual sampling frequency supported by hardware */
    dwSampleRate = PAS.nSampleRate;
    if (PAS.wFormat & AUDIO_FORMAT_STEREO)
        dwSampleRate <<= 1;
    dwSampleRate = CLOCKFREQ / (CLOCKFREQ / dwSampleRate);
    if (PAS.wFormat & AUDIO_FORMAT_STEREO)
        dwSampleRate >>= 1;
    PAS.nSampleRate = dwSampleRate;

    return AUDIO_ERROR_NONE;
}

static VOID PASStartPlayback(VOID)
{
    DWORD nCount;

    /*
     * Setup the DMA controller channel for playback in
     * auto init mode and start the PCM state machine.
     */
    DosSetupChannel(PAS.nDmaChannel, DMA_WRITE | DMA_AUTOINIT, 0);

    /* flush pending PCM interrupts writting interrupt status register */
    PASPortB(PAS_INTRSTAT, 0x00);

    /*
     * Disable buffer counter and timer rate gates, and disable
     * PAS16 output (these gates MUST be disabled before programming
     * the timers).
     */
    PASPortB(PAS_AUDIOFILT, AF_FILT1);

    /*
     * Disable DMA and PCM state machine (the time rate must
     * be initialized BEFORE enabling the PCM state machine).
     */
    PASPortB(PAS_CROSSCHANNEL, CC_DAC | CC_L2L | CC_R2R);

    /*
     * Set the PCM sample rate timer register (select rate timer,
     * enable 16 bit timer, select rate generator, and set
     * binary counting mode).
     */
    nCount = PAS.nSampleRate;
    if (PAS.wFormat & AUDIO_FORMAT_STEREO)
        nCount <<= 1;
    nCount = CLOCKFREQ / nCount;
    PASPortB(PAS_TMRCTLR, TC_16BIT | TC_RATE | TC_SAMPRATE | TC_BIN);
    PASPortB(PAS_SAMPLERATE, LOBYTE(nCount));
    PASPortB(PAS_SAMPLERATE, HIBYTE(nCount));

    /*
     * Set the PCM sample buffer counter register (select buffer
     * counter, enable 16 bit timer, select square wave generator
     * and set binary counting mode).
     */
    nCount = PAS.nBufferSize;
    if (PAS.wFormat & AUDIO_FORMAT_16BITS) {
        if (PAS.nDmaChannel <= 3)
            nCount <<= 1;
    }
    else {
        if (PAS.nDmaChannel >= 4)
            nCount >>= 1;
    }
    PASPortB(PAS_TMRCTLR, TC_16BIT | TC_SQUAREWAVE | TC_SAMPBUFF | TC_BIN);
    PASPortB(PAS_SAMPLECNT, LOBYTE(nCount));
    PASPortB(PAS_SAMPLECNT, HIBYTE(nCount));

    /*
     * Setup 8/16-bit linear encoding format for playback
     * (only possible for cards with the MV101 ASIC chip)
     */
    if (PAS.wProduct & MV101) {
        PASPortB(PAS_SYSCONFIG2, (PASPortRB(PAS_SYSCONFIG2) & ~SC2_16BIT) |
		 (PAS.wFormat & AUDIO_FORMAT_16BITS ? SC2_16BIT : 0x00));
    }

    /*
     * Select mono/stereo mode and enable DMA and PCM state machine
     * (DMA enable, disable PCM, DAC mode, L->L and R->R connections).
     */
    PASPortB(PAS_CROSSCHANNEL, CC_DRQ | CC_DAC | CC_L2L | CC_R2R |
	     (PAS.wFormat & AUDIO_FORMAT_STEREO ? CC_STEREO : CC_MONO));

    /* enable PCM state machine */
    PASPortB(PAS_CROSSCHANNEL, PASPortRB(PAS_CROSSCHANNEL) | CC_ENAPCM);

    /*
     * Enable buffer counter and timer rate gates, and enable PAS output
     * (enable gates and PAS16 output, select 17897 Hz filter).
     */
    PASPortB(PAS_AUDIOFILT, AF_SAMPBUFF | AF_SAMPRATE | AF_UNMUTE | AF_FILT1);
}

static VOID PASStopPlayback(VOID)
{
    /* disable buffer counter and timer rate gates and PAS16 output */
    PASPortB(PAS_AUDIOFILT, AF_FILT1);

    /* set back to 8 bit audio output */
    if (PAS.wProduct & MV101) {
        PASPortB(PAS_SYSCONFIG2, PASPortRB(PAS_SYSCONFIG2) & ~SC2_16BIT);
    }

    /* disable sample buffer interrupt */
    PASPortB(PAS_INTRCTLR, PASPortRB(PAS_INTRCTLR) & ~IC_SAMPBUFF);

    /* select mono mode, DAC mode, disable DMA and PCM state machine */
    PASPortB(PAS_CROSSCHANNEL, CC_DAC | CC_L2L | CC_R2R);

    /* reset DMA controller channel */
    DosDisableChannel(PAS.nDmaChannel);
}

static UINT PASPing(WORD wPort)
{
    static DOSREGS r;

    /*
     * Check if a PAS card is present making sure MVSOUND.SYS
     * is loaded in memory, this is the way to know if the
     * hardware is actually present.
     */
    memset(&r, 0, sizeof(r));
    r.ax = 0xBC00;
    r.bx = 0x3F3F;
    r.cx = 0x0000;
    r.dx = 0x0000;
    DosIntVector(0x2F, &r);
    if ((r.bx ^ r.cx ^ r.dx) == 0x4D56) {
        /* get the MVSOUND.SYS specified DMA and IRQ channels */
        r.ax = 0xBC04;
        DosIntVector(0x2F, &r);
        PAS.wPort = wPort ^ PAS_DEFAULT_BASE;
        PAS.nIrqLine = LOBYTE(r.cx);
        PAS.nDmaChannel = LOBYTE(r.bx);
        return PASProbe();
    }
    return AUDIO_ERROR_NODEVICE;
}




/*
 * Pro Audio Spectrum driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_PAS, "Pro Audio Spectrum",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08
    };
    static AUDIOCAPS Caps16 =
    {
        AUDIO_PRODUCT_PAS16, "Pro Audio Spectrum 16",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    memcpy(lpCaps, PAS.wId != AUDIO_PRODUCT_PAS16 ?
	   &Caps : &Caps16, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    if (!PASPing(PAS_DEFAULT_BASE))
        return AUDIO_ERROR_NONE;
    if (!PASPing(PAS_ALT_BASE_1))
        return AUDIO_ERROR_NONE;
    if (!PASPing(PAS_ALT_BASE_2))
        return AUDIO_ERROR_NONE;
    if (!PASPing(PAS_ALT_BASE_3))
        return AUDIO_ERROR_NONE;
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    DWORD dwBytesPerSecond;

    memset(&PAS, 0, sizeof(PAS));

    /*
     * Initialize PAS16 configuration parameters
     * and check whether we have such a card.
     */
    PAS.wFormat = lpInfo->wFormat;
    PAS.nSampleRate = lpInfo->nSampleRate;
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;

    /* refresh configuration parameters */
    lpInfo->wFormat = PAS.wFormat;
    lpInfo->nSampleRate = PAS.nSampleRate;

    /*
     * Allocate and clean DMA channel buffer for playback
     */
    dwBytesPerSecond = PAS.nSampleRate;
    if (PAS.wFormat & AUDIO_FORMAT_16BITS)
        dwBytesPerSecond <<= 1;
    if (PAS.wFormat & AUDIO_FORMAT_STEREO)
        dwBytesPerSecond <<= 1;
    PAS.nBufferSize = dwBytesPerSecond / (1000 / BUFFERSIZE);
    PAS.nBufferSize = (PAS.nBufferSize + BUFFRAGSIZE) & -BUFFRAGSIZE;
    if (DosAllocChannel(PAS.nDmaChannel, PAS.nBufferSize))
        return AUDIO_ERROR_NOMEMORY;
    if ((PAS.lpBuffer = DosLockChannel(PAS.nDmaChannel)) != NULL) {
        memset(PAS.lpBuffer, PAS.wFormat & AUDIO_FORMAT_16BITS ?
	       0x00 : 0x80, PAS.nBufferSize);
        DosUnlockChannel(PAS.nDmaChannel);
    }

    /* start playback with the PAS16 PCM state machine */
    PASStartPlayback();

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    PASStopPlayback();
    DosFreeChannel(PAS.nDmaChannel);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    int nBlockSize, nSize;

    if (PAS.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (PAS.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > PAS.nBufferSize)
        nFrames = PAS.nBufferSize;

    if ((PAS.lpBuffer = DosLockChannel(PAS.nDmaChannel)) != NULL) {
        nBlockSize = PAS.nBufferSize - PAS.nPosition -
            DosGetChannelCount(PAS.nDmaChannel);
        if (nBlockSize < 0)
            nBlockSize += PAS.nBufferSize;

        if (nBlockSize > nFrames)
            nBlockSize = nFrames;

        nBlockSize &= -BUFFRAGSIZE;
        while (nBlockSize != 0) {
            nSize = PAS.nBufferSize - PAS.nPosition;
            if (nSize > nBlockSize)
                nSize = nBlockSize;
            if (PAS.lpfnAudioWave != NULL) {
                PAS.lpfnAudioWave(&PAS.lpBuffer[PAS.nPosition], nSize);
            }
            else {
                memset(&PAS.lpBuffer[PAS.nPosition],
		       PAS.wFormat & AUDIO_FORMAT_16BITS ?
		       0x00 : 0x80, nSize);
            }
            if ((PAS.nPosition += nSize) >= PAS.nBufferSize)
                PAS.nPosition -= PAS.nBufferSize;
            nBlockSize -= nSize;
        }
        DosUnlockChannel(PAS.nDmaChannel);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    PAS.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}



/*
 * Pro Audio Spectrum driver public interface
 */
AUDIOWAVEDRIVER ProAudioSpectrumWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER ProAudioSpectrumDriver =
{
    &ProAudioSpectrumWaveDriver, NULL
};
