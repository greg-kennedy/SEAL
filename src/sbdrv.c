/*
 * $Id: sbdrv.c 1.6 1996/05/24 14:18:19 chasan released $
 *
 * Sound Blaster series DSP audio drivers.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <string.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"


/*
 * Sound Blaster DSP and Mixer register offsets
 */
#define MIXER_ADDR              0x04    /* mixer index address register */
#define MIXER_DATA              0x05    /* mixer indexed data register */
#define DSP_RESET               0x06    /* master reset register */
#define DSP_READ_DATA           0x0A    /* read data register */
#define DSP_WRITE_DATA          0x0C    /* write data register */
#define DSP_WRITE_STATUS        0x0C    /* write status register */
#define DSP_DATA_AVAIL          0x0E    /* data available register */
#define DSP_ACK_DMA8            0x0E    /* 8 bit DMA acknowledge */
#define DSP_ACK_DMA16           0x0F    /* 16 bit DMA acknowledge */

/*
 * Sound Blaster 1.0 (DSP 1.0) command defines
 */
#define DSP_WRITE_SAMPLE        0x10    /* direct 8 bit output */
#define DSP_READ_SAMPLE         0x20    /* direct 8 bit input */
#define DSP_LS_DAC              0x14    /* 8 bit low-speed output */
#define DSP_LS_ADC              0x24    /* 8 bit low-speed input */
#define DSP_TIMECONST           0x40    /* transfer time constant */
#define DSP_PAUSE_DMA           0xD0    /* halt 8 bit DMA transfer */
#define DSP_CONTINUE_DMA        0xD4    /* continue 8 bit DMA transfer */
#define DSP_SPEAKER_ON          0xD1    /* turn speaker on */
#define DSP_SPEAKER_OFF         0xD3    /* turn speaker off */
#define DSP_GET_VERSION         0xE1    /* get DSP version */

/*
 * Sound Blaster 1.5 (DSP 2.0) command defines
 */
#define DSP_BLOCK_SIZE          0x48    /* transfer block size */
#define DSP_AI_LS_DAC           0x1C    /* 8 bit low-speed A/I output */
#define DSP_AI_LS_ADC           0x2C    /* 8 bit low-speed A/I input */

/*
 * Sound Blaster 2.0 (DSP 2.01) command defines
 */
#define DSP_AI_HS_DAC           0x90    /* 8 bit high-speed A/I output */
#define DSP_AI_HS_ADC           0x98    /* 8 bit high-speed A/I input */
#define DSP_HS_DAC              0x91    /* 8 bit high-speed output */
#define DSP_HS_ADC              0x99    /* 8 bit high-speed input */

/*
 * Sound Blaster Pro (DSP 3.0) command defines
 */
#define DSP_ADC_STEREO          0xA8    /* 8 bit stereo input? */
#define DSP_ADC_MONO            0xA0    /* 8 bit mono input? */

/*
 * Sound Blaster 16 (DSP 4.0) command defines
 */
#define DSP_OUTPUT_RATE         0x41    /* set output sample rate */
#define DSP_INPUT_RATE          0x42    /* set input sample rate */
#define DSP_FMT_DAC16           0xB6    /* 16 bit high-speed A/I output */
#define DSP_FMT_ADC16           0xBE    /* 16 bit high-speed A/I input */
#define DSP_FMT_DAC8            0xC6    /* 8 bit high-speed A/I output */
#define DSP_FMT_ADC8            0xCE    /* 8 bit high-speed A/I input */
#define DSP_PAUSE_DMA16         0xD9    /* pause 16 bit DMA transfer */
#define DSP_CONT_DMA16          0x47    /* continue 16 bit DMA transfer */
#define DSP_PAUSE_DMA8          0xDA    /* pause 8 bit DMA transfer */
#define DSP_CONT_DMA8           0x45    /* continue 8 bit DMA transfer */

/*
 * Sound Blaster Pro mixer indirect registers
 */
#define MIXER_RESET             0x00    /* mixer reset register */
#define MIXER_INPUT_CONTROL     0x0C    /* input control register */
#define MIXER_OUTPUT_CONTROL    0x0E    /* output control register */
#define MIXER_MASTER_VOLUME     0x22    /* master volume register */
#define MIXER_VOICE_VOLUME      0x04    /* voice volume register */
#define MIXER_MIDI_VOLUME       0x26    /* MIDI volume register */
#define MIXER_CD_VOLUME         0x28    /* CD volume register */
#define MIXER_LINE_VOLUME       0x2E    /* line volume register */
#define MIXER_MIC_MIXING        0x0A    /* mic volume register */
#define MIXER_IRQ_LINE          0x80    /* IRQ line register */
#define MIXER_DMA_CHANNEL       0x81    /* DMA channel register */
#define MIXER_IRQ_STATUS        0x82    /* IRQ status register */

/*
 * Sound Blaster 16 mixer indirect registers
 */
#define MIXER_MASTER_LEFT       0x30    /* master left volume register */
#define MIXER_MASTER_RIGHT      0x31    /* master right volume register */
#define MIXER_VOICE_LEFT        0x32    /* voice left volume register */
#define MIXER_VOICE_RIGHT       0x33    /* voice right volume register */
#define MIXER_MIDI_LEFT         0x34    /* MIDI left volume register */
#define MIXER_MIDI_RIGHT        0x35    /* MIDI right volume register */
#define MIXER_CD_LEFT           0x36    /* CD left volume register */
#define MIXER_CD_RIGHT          0x37    /* CD right volume register */
#define MIXER_LINE_LEFT         0x38    /* line left volume register */
#define MIXER_LINE_RIGHT        0x39    /* line right volume register */
#define MIXER_MIC_VOLUME        0x3A    /* mic input volume register */
#define MIXER_SPKR_VOLUME       0x3B    /* speaker volume register */
#define MIXER_OUT_CONTROL       0x3C    /* Output Line/CD/MIC control */
#define MIXER_INPUT_LEFT        0x3D    /* input left control bits */
#define MIXER_INPUT_RIGHT       0x3E    /* input right control bits */
#define MIXER_INPUT_GAIN        0x3F    /* input gain level */
#define MIXER_OUT_GAIN_LEFT     0x41    /* Output gain left level */
#define MIXER_OUT_GAIN_RIGHT    0x42    /* Output gain right level */
#define MIXER_INPUT_AUTO_GAIN   0x43    /* input auto gain control */
#define MIXER_TREBLE_LEFT       0x44    /* Treble left level */
#define MIXER_TREBLE_RIGHT      0x45    /* Treble right level */
#define MIXER_BASS_LEFT         0x46    /* Bass left level */
#define MIXER_BASS_RIGHT        0x47    /* Bass right level */

/*
 * Input source and filter select register bit fields (MIXER_INPUT_CONTROL)
 */
#define INPUT_SOURCE_MIC        0x00    /* select mic input */
#define INPUT_SOURCE_CD         0x02    /* select CD input */
#define INPUT_SOURCE_LINE       0x06    /* select line input */
#define INPUT_LOW_FILTER        0x00    /* select low-pass filter */
#define INPUT_HIGH_FILTER       0x08    /* select high-pass filter */
#define INPUT_NO_FILTER         0x20    /* disable filters */

/*
 * Output voice feature settings register bit fields (MIXER_OUTPUT_CONTROL)
 */
#define OUTPUT_DISABLE_DNFI     0x20    /* disable output filter */
#define OUTPUT_ENABLE_VSTC      0x02    /* enable stereo mode */

/*
 * Mixer output control register bit fields (MIXER_OUT_CONTROL)
 */
#define MUTE_LINE_LEFT          0x10    /* disable left line-out */
#define MUTE_LINE_RIGHT         0x08    /* disable right line-out */
#define MUTE_CD_LEFT            0x04    /* disable left CD output */
#define MUTE_CD_RIGHT           0x02    /* disable right CD output */
#define MUTE_MICROPHONE         0x01    /* disable MIC output */

/*
 * Start DMA DAC transfer format bit fields (DSP_FMT_DAC8/16)
 */
#define FMT_DAC_UNSIGNED        0x00    /* select unsigned samples */
#define FMT_DAC_SIGNED          0x10    /* select signed samples */
#define FMT_DAC_MONO            0x00    /* select mono output */
#define FMT_DAC_STEREO          0x20    /* select stereo output */

/*
 * Timeout and DMA buffer size defines
 */
#define TIMEOUT                 32767   /* number of times to wait for */
#define BUFFERSIZE              50      /* buffer length in milliseconds */
#define BUFFRAGSIZE             32      /* buffer fragment size in UCHARs */


/*
 * Sound Blaster configuration structure
 */
static struct {
    USHORT  wId;                        /* audio device indentifier */
    USHORT  wFormat;                    /* playback encoding format */
    USHORT  nSampleRate;                /* sampling frequency */
    USHORT  wPort;                      /* DSP base port address */
    UCHAR   nIrqLine;                   /* interrupt line */
    UCHAR   nDmaChannel;                /* output DMA channel */
    UCHAR   nLowDmaChannel;             /* 8-bit DMA channel */
    UCHAR   nHighDmaChannel;            /* 16-bit DMA channel */
    PUCHAR  pBuffer;                    /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length */
    UINT    nPosition;                  /* DMA buffer playing position */
    PFNAUDIOWAVE pfnAudioWave;          /* user callback routine */
} SB;


/*
 * Sound Blaster DSP & Mixer programming routines
 */
static VOID DSPPortB(UCHAR bData)
{
    UINT n;

    for (n = 0; n < TIMEOUT; n++)
        if (!(INB(SB.wPort + DSP_WRITE_STATUS) & 0x80))
            break;
    OUTB(SB.wPort + DSP_WRITE_DATA, bData);
}

static UCHAR DSPPortRB(VOID)
{
    UINT n;

    for (n = 0; n < TIMEOUT; n++)
        if (INB(SB.wPort + DSP_DATA_AVAIL) & 0x80)
            break;
    return INB(SB.wPort + DSP_READ_DATA);
}

static VOID DSPMixerB(UCHAR nIndex, UCHAR bData)
{
    OUTB(SB.wPort + MIXER_ADDR, nIndex);
    OUTB(SB.wPort + MIXER_DATA, bData);
}

static UCHAR DSPMixerRB(UCHAR nIndex)
{
    OUTB(SB.wPort + MIXER_ADDR, nIndex);
    return INB(SB.wPort + MIXER_DATA);
}

static USHORT DSPGetVersion(VOID)
{
    USHORT nMinor, nMajor;

    DSPPortB(DSP_GET_VERSION);
    nMajor = DSPPortRB();
    nMinor = DSPPortRB();
    return MAKEWORD(nMinor, nMajor);
}

static VOID DSPReset(VOID)
{
    UINT n;

    OUTB(SB.wPort + DSP_RESET, 1);
    for (n = 0; n < 8; n++)
        INB(SB.wPort + DSP_RESET);
    OUTB(SB.wPort + DSP_RESET, 0);
}

static BOOL DSPProbe(VOID)
{
    USHORT nVersion;
    UINT n;

    /* reset the DSP device */
    for (n = 0; n < 8; n++) {
        DSPReset();
        if (DSPPortRB() == 0xAA)
            break;
    }
    if (n >= 8)
        return AUDIO_ERROR_NODEVICE;

    /* get the DSP version */
    nVersion = DSPGetVersion();
    if (nVersion >= 0x400) {
        SB.wId = AUDIO_PRODUCT_SB16;
    }
    else if (nVersion >= 0x300) {
        SB.wId = AUDIO_PRODUCT_SBPRO;
    }
    else if (nVersion >= 0x201) {
        SB.wId = AUDIO_PRODUCT_SB20;
    }
    else if (nVersion >= 0x200) {
        SB.wId = AUDIO_PRODUCT_SB15;
    }
    else {
        SB.wId = AUDIO_PRODUCT_SB;
    }
    return AUDIO_ERROR_NONE;
}

static VOID AIAPI DSPInterruptHandler(VOID)
{
    if (SB.wId == AUDIO_PRODUCT_SB16) {
        /*
         * Acknowledge 8/16 bit mono/stereo high speed
         * autoinit DMA transfer for SB16 (DSP 4.x) cards.
         */
        INB(SB.wPort + (SB.wFormat & AUDIO_FORMAT_16BITS ?
                DSP_ACK_DMA16 : DSP_ACK_DMA8));
    }
    else if (SB.wId != AUDIO_PRODUCT_SB) {
        /*
         * Acknowledge 8 bit mono/stereo low/high speed autoinit
         * DMA transfer for SB Pro (DSP 3.x), SB 2.0 (DSP 2.01)
         * and SB 1.5 (DSP 2.0) cards.
         */
        INB(SB.wPort + DSP_ACK_DMA8);
    }
    else {
        /*
         * Acknowledge and restart 8 bit mono low speed
         * oneshot DMA transfer for SB 1.0 (DSP 1.x) cards.
         */
        INB(SB.wPort + DSP_ACK_DMA8);
        DSPPortB(DSP_LS_DAC);
        DSPPortB(0xFF);
        DSPPortB(0xFF);
    }
}

static VOID DSPStartPlayback(VOID)
{
    ULONG dwBytesPerSecond;

    /* setup the DMA channel parameters */
    DosSetupChannel(SB.nDmaChannel, DMA_WRITE | DMA_AUTOINIT, 0);

    /* setup our IRQ interrupt handler */
    DosSetVectorHandler(SB.nIrqLine, DSPInterruptHandler);

    /* reset the DSP processor */
    DSPReset();

    /* turn on the DSP speaker */
    DSPPortB(DSP_SPEAKER_ON);

    /* set DSP output playback rate */
    if (SB.wId == AUDIO_PRODUCT_SB16) {
        /* set output sample rate for SB16 cards */
        DSPPortB(DSP_OUTPUT_RATE);
        DSPPortB(HIBYTE(SB.nSampleRate));
        DSPPortB(LOBYTE(SB.nSampleRate));
    }
    else {
        /* set input/output sample rate for SB/SB20/SBPro cards */
        dwBytesPerSecond = SB.nSampleRate;
        if (SB.wFormat & AUDIO_FORMAT_STEREO)
            dwBytesPerSecond <<= 1;
        DSPPortB(DSP_TIMECONST);
        DSPPortB(256 - (1000000L / dwBytesPerSecond));
    }

    /* start DMA playback transfer */
    if (SB.wId == AUDIO_PRODUCT_SB) {
        /*
         * Start 8 bit mono low speed oneshot DMA output
         * transfer for SB 1.0 (DSP 1.x) cards.
         */
        DSPPortB(DSP_LS_DAC);
        DSPPortB(0xFF);
        DSPPortB(0xFF);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB15) {
        /*
         * Start 8 bit mono low speed autoinit DMA output
         * transfer for SB 1.5 (DSP 2.0) cards.
         */
        DSPPortB(DSP_BLOCK_SIZE);
        DSPPortB(0xFF);
        DSPPortB(0xFF);
        DSPPortB(DSP_AI_LS_DAC);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB20) {
        /*
         * Start 8 bit mono high speed autoinit DMA transfer
         * output transfer for SB 2.0 (DSP 2.01) cards.
         */
        DSPPortB(DSP_BLOCK_SIZE);
        DSPPortB(0xFF);
        DSPPortB(0xFF);
        DSPPortB(DSP_AI_HS_DAC);
    }
    else if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        /*
         * Start 8 bit mono/stereo high speed autoinit
         * DMA transfer for SB Pro (DSP 3.x) cards.
         */
        if (SB.wFormat & AUDIO_FORMAT_STEREO) {
            DSPMixerB(MIXER_OUTPUT_CONTROL,
                DSPMixerRB(MIXER_OUTPUT_CONTROL) | OUTPUT_ENABLE_VSTC);
        }
        else {
            DSPMixerB(MIXER_OUTPUT_CONTROL,
                DSPMixerRB(MIXER_OUTPUT_CONTROL) & ~OUTPUT_ENABLE_VSTC);
        }
        DSPPortB(DSP_BLOCK_SIZE);
        DSPPortB(0xFF);
        DSPPortB(0xFF);
        DSPPortB(DSP_AI_HS_DAC);
    }
    else {
        /*
         * Start 8/16 bit mono/stereo high speed autoinit
         * DMA transfer for SB16 (DSP 4.x) cards.
         */
        if (SB.wFormat & AUDIO_FORMAT_16BITS) {
            DSPPortB(DSP_FMT_DAC16);
            DSPPortB(FMT_DAC_SIGNED | (SB.wFormat & AUDIO_FORMAT_STEREO ?
                    FMT_DAC_STEREO : FMT_DAC_MONO));
            DSPPortB(0xFF);
            DSPPortB(0xFF);
        }
        else {
            DSPPortB(DSP_FMT_DAC8);
            DSPPortB(FMT_DAC_UNSIGNED | (SB.wFormat & AUDIO_FORMAT_STEREO ?
                    FMT_DAC_STEREO : FMT_DAC_MONO));
            DSPPortB(0xFF);
            DSPPortB(0xFF);
        }
    }
}

static VOID DSPStopPlayback(VOID)
{
    /* reset the DSP processor */
    DSPReset();

    /* turn off DSP speaker */
    DSPPortB(DSP_SPEAKER_OFF);

    /* turn off the stereo flag for SBPro cards */
    if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        DSPMixerB(MIXER_OUTPUT_CONTROL,
            DSPMixerRB(MIXER_OUTPUT_CONTROL) & ~OUTPUT_ENABLE_VSTC);
    }

    /* restore the interrupt handler */
    DosSetVectorHandler(SB.nIrqLine, NULL);

    /* reset the DMA channel parameters */
    DosDisableChannel(SB.nDmaChannel);
}

static UINT DSPInitAudio(VOID)
{

#define CLIP(nRate, nMinRate, nMaxRate)         \
        (nRate < nMinRate ? nMinRate :          \
        (nRate > nMaxRate ? nMaxRate : nRate))  \

    /*
     * Probe if there is a SB present and detect the DSP version.
     */
    if (DSPProbe())
        return AUDIO_ERROR_NODEVICE;

    /*
     * Check playback encoding format and sampling frequency
     */
    if (SB.wId == AUDIO_PRODUCT_SB) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 4000, 22050);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB15) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 22050);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB20) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        if (SB.wFormat & AUDIO_FORMAT_STEREO) {
            SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 22050);
        }
        else {
            SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
        }
        SB.wFormat &= ~AUDIO_FORMAT_16BITS;
    }
    else {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
    }
    if (SB.wId != AUDIO_PRODUCT_SB16) {
        SB.nSampleRate = 1000000L / (1000000L / SB.nSampleRate);
    }
    SB.nDmaChannel = (SB.wFormat & AUDIO_FORMAT_16BITS ?
        SB.nHighDmaChannel : SB.nLowDmaChannel);

    return AUDIO_ERROR_NONE;
}

static VOID DSPDoneAudio(VOID)
{
    DSPReset();
}


/*
 * Sound Blaster audio driver API interface
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_SB, "Sound Blaster",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08
    };
    static AUDIOCAPS Caps15 =
    {
        AUDIO_PRODUCT_SB15, "Sound Blaster 1.5",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08
    };
    static AUDIOCAPS Caps20 =
    {
        AUDIO_PRODUCT_SB20, "Sound Blaster 2.0",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08 |
        AUDIO_FORMAT_4M08
    };
    static AUDIOCAPS CapsPro =
    {
        AUDIO_PRODUCT_SBPRO, "Sound Blaster Pro",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_4M08
    };
    static AUDIOCAPS Caps16 =
    {
        AUDIO_PRODUCT_SB16, "Sound Blaster 16",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    switch (SB.wId) {
    case AUDIO_PRODUCT_SB16:
        memcpy(pCaps, &Caps16, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SBPRO:
        memcpy(pCaps, &CapsPro, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SB20:
        memcpy(pCaps, &Caps20, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SB15:
        memcpy(pCaps, &Caps15, sizeof(AUDIOCAPS));
        break;
    default:
        memcpy(pCaps, &Caps, sizeof(AUDIOCAPS));
        break;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    PSZ pszBlaster;
    UINT nChar;

    SB.wId = AUDIO_PRODUCT_SB;
    SB.wPort = 0x220;
    SB.nIrqLine = 7;
    SB.nLowDmaChannel = 1;
    SB.nHighDmaChannel = 5;
    if ((pszBlaster = DosGetEnvironment("BLASTER")) != NULL) {
        nChar = DosParseString(pszBlaster, TOKEN_CHAR);
        while (nChar != 0) {
            switch (nChar) {
            case 'A':
            case 'a':
                SB.wPort = DosParseString(NULL, TOKEN_HEX);
                break;
            case 'I':
            case 'i':
                SB.nIrqLine = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'D':
            case 'd':
                SB.nLowDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'H':
            case 'h':
                SB.nHighDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'T':
            case 't':
                switch (DosParseString(NULL, TOKEN_DEC)) {
                case 1: SB.wId = AUDIO_PRODUCT_SB; break;
                case 2: SB.wId = AUDIO_PRODUCT_SBPRO; break;
                case 3: SB.wId = AUDIO_PRODUCT_SB20; break;
                case 4: SB.wId = AUDIO_PRODUCT_SBPRO; break;
                case 5: SB.wId = AUDIO_PRODUCT_SBPRO; break;
                case 6: SB.wId = AUDIO_PRODUCT_SB16; break;
                }
                break;
            }
            nChar = DosParseString(NULL, TOKEN_CHAR);
        }
        return DSPProbe();
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    ULONG dwBytesPerSecond;

    /*
     * Initialize the SB audio driver for playback
     */
    memset(&SB, 0, sizeof(SB));
    SB.wFormat = pInfo->wFormat;
    SB.nSampleRate = pInfo->nSampleRate;
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;
    if (DSPInitAudio())
        return AUDIO_ERROR_NODEVICE;

    /*
     * Allocate DMA channel buffer and start playback transfers
     */
    dwBytesPerSecond = SB.nSampleRate;
    if (SB.wFormat & AUDIO_FORMAT_16BITS)
        dwBytesPerSecond <<= 1;
    if (SB.wFormat & AUDIO_FORMAT_STEREO)
        dwBytesPerSecond <<= 1;
    SB.nBufferSize = dwBytesPerSecond / (1000 / BUFFERSIZE);
    SB.nBufferSize = (SB.nBufferSize + BUFFRAGSIZE) & -BUFFRAGSIZE;
    if (DosAllocChannel(SB.nDmaChannel, SB.nBufferSize)) {
        DSPDoneAudio();
        return AUDIO_ERROR_NOMEMORY;
    }
    if ((SB.pBuffer = DosLockChannel(SB.nDmaChannel)) != NULL) {
        memset(SB.pBuffer, SB.wFormat & AUDIO_FORMAT_16BITS ?
            0x00 : 0x80, SB.nBufferSize);
        DosUnlockChannel(SB.nDmaChannel);
    }
    DSPStartPlayback();

    /* refresh caller's encoding format and sampling frequency */
    pInfo->wFormat = SB.wFormat;
    pInfo->nSampleRate = SB.nSampleRate;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    /*
     * Stop DMA playback transfers and reset the DSP processor
     */
    DSPStopPlayback();
    DosFreeChannel(SB.nDmaChannel);
    DSPDoneAudio();
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    int nBlockSize, nSize;

    if ((SB.pBuffer = DosLockChannel(SB.nDmaChannel)) != NULL) {
        nBlockSize = SB.nBufferSize - SB.nPosition -
            DosGetChannelCount(SB.nDmaChannel);
        if (nBlockSize < 0)
            nBlockSize += SB.nBufferSize;
        nBlockSize &= -BUFFRAGSIZE;
        while (nBlockSize != 0) {
            nSize = SB.nBufferSize - SB.nPosition;
            if (nSize > nBlockSize)
                nSize = nBlockSize;
            if (SB.pfnAudioWave != NULL) {
                SB.pfnAudioWave(&SB.pBuffer[SB.nPosition], nSize);
            }
            else {
                memset(&SB.pBuffer[SB.nPosition],
                    SB.wFormat & AUDIO_FORMAT_16BITS ?
                    0x00 : 0x80, nSize);
            }
            if ((SB.nPosition += nSize) >= SB.nBufferSize)
                SB.nPosition -= SB.nBufferSize;
            nBlockSize -= nSize;
        }
        DosUnlockChannel(SB.nDmaChannel);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    SB.pfnAudioWave = pfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * Sound Blaster drivers public interface
 */
AUDIOWAVEDRIVER SoundBlasterWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER SoundBlasterDriver =
{
    &SoundBlasterWaveDriver, NULL
};
