/*
 * $Id: gusdrv.c 1.11 1996/09/22 20:17:51 chasan released $
 *
 * Advanced Gravis UltraSound (GF1 and InterWave) audio driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define INTERWAVE

#include <string.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"

/*
 * Gravis Ultrasound register offsets
 */
/* MIDI-101 interface */
#define GUS_MIDI_CTRL           0x100   /* (3X0) MIDI control register */
#define GUS_MIDI_STAT           0x100   /* (3X0) MIDI status register */
#define GUS_MIDI_DATA           0x101   /* (3X1) MIDI data register */

/* Joystick interface */
#define GUS_JOYSTICK_TIMER      0x001   /* (2X1) joystick trigger timer */
#define GUS_JOYSTICK_DATA       0x001   /* (2X1) joystick read data */

/* GF1 synthesizer */
#define GUS_GF1_PAGE            0x102   /* (3X2) GF1 page register */
#define GUS_GF1_INDEX           0x103   /* (3X3) GF1 register select */
#define GUS_GF1_DATA_LOW        0x104   /* (3X4) GF1 data low byte */
#define GUS_GF1_DATA_HIGH       0x105   /* (3X5) GF1 data high byte */
#define GUS_GF1_IRQ_STAT        0x006   /* (2X6) GF1 IRQ status */
#define GUS_GF1_TIMER_CTRL      0x008   /* (2X8) GF1 timer control */
#define GUS_GF1_TIMER_DATA      0x009   /* (2X9) GF1 timer data */
#define GUS_GF1_DRAM_DATA       0x107   /* (3X7) GF1 DRAM I/O register */

/* Board only */
#define GUS_BOARD_MIX_CTRL      0x000   /* (2X0) mix control register */
#define GUS_BOARD_IRQ_CTRL      0x00B   /* (2XB) IRQ control register */
#define GUS_BOARD_DMA_CTRL      0x00B   /* (2XB) DMA control register */
#define GUS_BOARD_REG_CTRL      0x00F   /* (2XF) register controls (Rev 3.4) */
#define GUS_BOARD_IRQ_CLEAR     0x00B   /* (2XB) IRQ clear register (Rev 3.4) */
#define GUS_BOARD_JUMPERS       0x00B   /* (2XB) jumper register (Rev 3.4) */
#define GUS_BOARD_VERSION       0x506   /* (7X6) board version (Rev 3.7) */

/* ICS2101 mixer interface (Rev 3.7) */
#define GUS_ICS_MIXER_ADDR      0x506   /* (7X6) mixer control register */
#define GUS_ICS_MIXER_DATA      0x106   /* (3X6) mixer data register */

/* CS4231 interface (UltraMax) */
#define GUS_CODEC_BASE0         0x530   /* (530) Daughter Codec address */
#define GUS_CODEC_BASE1         0x604   /* (604) Daughter Codec address */
#define GUS_CODEC_BASE2         0xE80   /* (E80) Daughter Codec address */
#define GUS_CODEC_BASE3         0xF40   /* (F40) Daughter Codec address */
#define GUS_CODEC_BASE4         0x10C   /* (3XC) UltraMax Codec address */
#define GUS_CODEC_CTRL          0x106   /* (3X6) UltraMax Codec control */


/*
 * MIDI (6850 UART) control register bit fields (3X0)
 */
#define MIDI_RESET              0x03    /* MIDI master reset */
#define MIDI_ENABLE_TX_IRQ      0x20    /* MIDI transmit IRQ enabled */
#define MIDI_ENABLE_RX_IRQ      0x80    /* MIDI receive IRQ enabled */

/*
 * MIDI (6850 UART) status register bit fields (3X0)
 */
#define MIDI_RECV_FULL          0x01    /* recv register full */
#define MIDI_XMIT_EMPTY         0x02    /* xmit register empty */
#define MIDI_FRAME_ERROR        0x10    /* framing error */
#define MIDI_OVERRUN_ERROR      0x20    /* overrun error */
#define MIDI_IRQ_PENDING        0x80    /* interrupt pending */


/*
 * GF1 global indirect register indexes (3X3)
 */
#define GF1_DRAM_DMA_CTRL       0x41    /* DRAM DMA control register */
#define GF1_DMA_ADDR            0x42    /* DMA start address register */
#define GF1_DRAM_ADDR_LOW       0x43    /* DRAM I/O low address register */
#define GF1_DRAM_ADDR_HIGH      0x44    /* DRAM I/O high address register */
#define GF1_TIMER_CTRL          0x45    /* timer control register */
#define GF1_TIMER1_COUNT        0x46    /* timer 1 count register */
#define GF1_TIMER2_COUNT        0x47    /* timer 2 count register */
#define GF1_ADC_DMA_RATE        0x48    /* sampling frequency register */
#define GF1_ADC_DMA_CTRL        0x49    /* sampling control register */
#define GF1_JOYSTICK_DAC        0x4B    /* joystick trim DAC register */
#define GF1_MASTER_RESET        0x4C    /* master reset register */

/*
 * GF1 DRAM DMA control register bit fields (41)
 */
#define DRAM_DMA_ENABLE         0x01    /* enable DMA transfer */
#define DRAM_DMA_DISABLE        0x00    /* disable DMA transfer */
#define DRAM_DMA_READ           0x02    /* read DMA direction */
#define DRAM_DMA_WRITE          0x00    /* write DMA direction */
#define DRAM_DMA_WIDTH_16       0x04    /* 8/16 bit DMA channel */
#define DRAM_DMA_RATE0          0x00    /* DMA rate divisor (1) */
#define DRAM_DMA_RATE1          0x08    /* DMA rate divisor (2) */
#define DRAM_DMA_RATE2          0x10    /* DMA rate divisor (3) */
#define DRAM_DMA_RATE3          0x18    /* DMA rate divisor (4) */
#define DRAM_DMA_IRQ_ENABLE     0x20    /* DMA IRQ enable */
#define DRAM_DMA_IRQ_PENDING    0x40    /* DMA IRQ pending */
#define DRAM_DMA_DATA_16        0x40    /* 8/16 bit data */
#define DRAM_DMA_TWOS_COMP      0x80    /* invert MSB data */

/*
 * GF1 timer control register bit fields (45)
 */
#define TIMER1_IRQ_ENABLE       0x04    /* enable timer 1 interrupts */
#define TIMER2_IRQ_ENABLE       0x08    /* enable timer 2 interrupts */

/*
 * GF1 ADC DMA sampling control register bit fields (49)
 */
#define ADC_DMA_ENABLE          0x01    /* enable sampling */
#define ADC_DMA_DISABLE         0x00    /* disable sampling */
#define ADC_DMA_STEREO_MODE     0x02    /* select stereo mode */
#define ADC_DMA_MONO_MODE       0x00    /* select mono mode */
#define ADC_DMA_WIDTH_16        0x04    /* 8/16 bit DMA channel */
#define ADC_DMA_IRQ_ENABLE      0x20    /* DMA IRQ enable */
#define ADC_DMA_IRQ_PENDING     0x40    /* DMA IRQ pending */
#define ADC_DMA_TWOS_COMP       0x80    /* invert MSB data */

/*
 * GF1 master reset register bit fields (4C)
 */
#define MASTER_RESET            0x01    /* GF1 master reset register */
#define MASTER_ENABLE_DAC       0x02    /* master DAC output enable */
#define MASTER_ENABLE_IRQ       0x04    /* GF1 master IRQ enable */


/*
 * GF1 voice indirect register indexes (3X3)
 */
#define GF1_SET_VOICE_CTRL      0x00    /* voice control register */
#define GF1_SET_FREQ_CTRL       0x01    /* frequency control register */
#define GF1_SET_START_HIGH      0x02    /* high start address register */
#define GF1_SET_START_LOW       0x03    /* low start address register */
#define GF1_SET_END_HIGH        0x04    /* high end address register */
#define GF1_SET_END_LOW         0x05    /* low end address register */
#define GF1_SET_VOLUME_RATE     0x06    /* volume ramp rate register */
#define GF1_SET_VOLUME_START    0x07    /* volume ramp start register */
#define GF1_SET_VOLUME_END      0x08    /* volume ramp end register */
#define GF1_SET_VOLUME          0x09    /* current volume register */
#define GF1_SET_ACCUM_HIGH      0x0A    /* high accumulator register */
#define GF1_SET_ACCUM_LOW       0x0B    /* low accumulator register */
#define GF1_SET_PANNING         0x0C    /* pan position register */
#define GF1_SET_VOLUME_CTRL     0x0D    /* volume ramp control register */
#define GF1_SET_NUMVOICES       0x0E    /* active voices register */

#define GF1_GET_VOICE_CTRL      0x80    /* voice control register */
#define GF1_GET_FREQ_CTRL       0x81    /* frequency control register */
#define GF1_GET_START_HIGH      0x82    /* high start address register */
#define GF1_GET_START_LOW       0x83    /* low start address register */
#define GF1_GET_END_HIGH        0x84    /* high end address register */
#define GF1_GET_END_LOW         0x85    /* low end address register */
#define GF1_GET_VOLUME_RATE     0x86    /* volume ramp rate register */
#define GF1_GET_VOLUME_START    0x87    /* volume ramp start register */
#define GF1_GET_VOLUME_END      0x88    /* volume ramp end register */
#define GF1_GET_VOLUME          0x89    /* current volume register */
#define GF1_GET_ACCUM_HIGH      0x8A    /* high accumulator register */
#define GF1_GET_ACCUM_LOW       0x8B    /* low accumulator register */
#define GF1_GET_PANNING         0x8C    /* pan position register */
#define GF1_GET_VOLUME_CTRL     0x8D    /* volume ramp control register */
#define GF1_GET_NUMVOICES       0x8E    /* active voices register */
#define GF1_GET_IRQ_STAT        0x8F    /* IRQ source register */

#ifdef INTERWAVE
/*
 * Interwave global indirect register indexes (3X3)
 */
#define IW_LDSAHI               0x50    /* LMC DMA start addr high register */
#define IW_LMCFI                0x52    /* local memory config register */

/*
 * InterWave voice indirect register indexes (3X3)
 */
#define IW_SROI_WR              0x0C    /* synth right offset register */
#define IW_SROI_RD              0x8C
#define IW_SLOI_WR              0x13    /* synth left offset register */
#define IW_SLOI_RD              0x93
#define IW_SUAI_WR              0x10    /* synth upper address register */
#define IW_SUAI_RD              0x90
#define IW_SMSI_WR              0x15    /* synth mode select register */
#define IW_SMSI_RD              0x95
#define IW_SGMI_WR              0x19    /* synth global mode register */
#define IW_SGMI_RD              0x99

/*
 * InterWave synth mode select register bit fields (15,95)
 */
#define IW_SMSI_VOICE_DISABLE   0x02    /* de-activate enhanced voice */
#define IW_SMSI_VOICE_OFFSET    0x20    /* enable smooth voice panning */

/* 
 * InterWave synth global mode register bit fields (19,99)
 */
#define IW_SGMI_ENHMODE         0x01    /* enable enhanced mode */
#define IW_SGMI_ENABLE_LFOS     0x02    /* enable LFOs */

#endif

/*
 * GF1 voice control register bit fields (00,80)
 */
#define VOICE_NORMAL            0x00    /* default voice bits */
#define VOICE_STOPPED           0x01    /* voice stopped */
#define VOICE_STOP              0x02    /* stop voice */
#define VOICE_DATA_16           0x04    /* 8/16 bit wave data */
#define VOICE_LOOP              0x08    /* loop enable */
#define VOICE_BIDILOOP          0x10    /* bidi-loop enable */
#define VOICE_ENABLE_IRQ        0x20    /* wavetable IRQ enable */
#define VOICE_DIRECT            0x40    /* playing direction */
#define VOICE_IRQ_PENDING       0x80    /* wavetable IRQ pending */

/*
 * GF1 voice volume control register bit fields (0D,8D)
 */
#define VOLUME_STOPPED          0x01    /* volume ramp stopped */
#define VOLUME_STOP             0x02    /* stop volume ramp */
#define VOLUME_ROLLOVER         0x04    /* rollover condition */
#define VOLUME_LOOP             0x08    /* loop enable */
#define VOLUME_BIDILOOP         0x10    /* bidi-loop enable */
#define VOLUME_ENABLE_IRQ       0x20    /* volume ramp IRQ enable */
#define VOLUME_DIRECT           0x40    /* volume ramp direction */
#define VOLUME_IRQ_PENDING      0x80    /* volume ramp IRQ pending */

/*
 * GF1 global interrupt source register bit fields (8F)
 */
#define STATUS_VOICE            0x1F    /* interrupting voice # mask */
#define STATUS_VOLUME_IRQ       0x40    /* volume ramp IRQ pending */
#define STATUS_WAVETABLE_IRQ    0x80    /* wavetable IRQ pending */


/*
 * GF1 interrupt status register bit fields (2X6)
 */
#define IRQ_STAT_MIDI_TX        0x01    /* MIDI transmit IRQ */
#define IRQ_STAT_MIDI_RX        0x02    /* MIDI receive IRQ */
#define IRQ_STAT_TIMER1         0x04    /* timer 1 IRQ */
#define IRQ_STAT_TIMER2         0x08    /* timer 2 IRQ */
#define IRQ_STAT_WAVETABLE      0x20    /* wavetable IRQ */
#define IRQ_STAT_VOLUME         0x40    /* volume ramp IRQ */
#define IRQ_STAT_DMA_TC         0x80    /* DRAM or ADC DMA IRQ */

/*
 * GF1 timer control register bit fields (2X8)
 */
#define TIMER_CTRL_SELECT       0x04    /* select timer stuff */
#define TIMER2_CTRL_EXPIRED     0x20    /* timer 2 has expired */
#define TIMER1_CTRL_EXPIRED     0x40    /* timer 1 has expired */

/*
 * GF1 timer data register bit fields (2X9)
 */
#define TIMER1_START_UP         0x01    /* timer 1 start */
#define TIMER2_START_UP         0x02    /* timer 2 start */
#define TIMER2_MASK_OFF         0x20    /* mask off timer 2 */
#define TIMER1_MASK_OFF         0x40    /* mask off timer 1 */
#define TIMER1_2_RESET          0x80    /* clear timer IRQ */

/*
 * GF1 board mix control register bit fields (2X0)
 */
#define MIXER_DISABLE_LINE_IN   0x01    /* disable line input */
#define MIXER_DISABLE_LINE_OUT  0x02    /* disable line output */
#define MIXER_ENABLE_MIC_IN     0x04    /* enable mic input */
#define MIXER_ENABLE_LATCHES    0x08    /* enable IRQ/DMA latches */
#define MIXER_COMBINE_IRQS      0x10    /* combine GF1/MIDI IRQ channels */
#define MIXER_ENABLE_MIDI_LOOP  0x20    /* enable MIDI loopback TxD to RxD */
#define MIXER_LATCH_DMA         0x00    /* select DMA control register */
#define MIXER_LATCH_IRQ         0x40    /* select IRQ control register */

/*
 * ICS2101 mixer indirect register indexes (7X6)
 */
#define ICS_MIXER_CHANNEL       0x38    /* channel register bit mask */
#define ICS_MIXER_CTRL_LEFT     0x00    /* select left control register */
#define ICS_MIXER_CTRL_RIGHT    0x01    /* select right control register */
#define ICS_MIXER_ATTN_LEFT     0x02    /* select left attenuation register */
#define ICS_MIXER_ATTN_RIGHT    0x03    /* select right attenuation register */
#define ICS_MIXER_PANNING       0x04    /* select panning register */
#define ICS_MIXER_MIC           0x00    /* select mic input channel */
#define ICS_MIXER_LINE          0x08    /* select line input channel */
#define ICS_MIXER_CD            0x10    /* select CD input channel */
#define ICS_MIXER_GF1           0x18    /* select GF1 output channel */
#define ICS_MIXER_MASTER        0x28    /* select master output channel */

/*
 * UltraMax (CS4231) control register bit fields (3X6)
 */
#define CODEC_ADDR_DECODE       0x0F    /* CS4231 codec address decode bits */
#define CODEC_CAPTURE_DMA       0x10    /* 8/16 bit capture DMA channel */
#define CODEC_PLAYBACK_DMA      0x20    /* 8/16 bit playback DMA channel */
#define CODEC_ENABLED           0x40    /* disable/enable CS4231 codec */

/*
 * GF1 driver DRAM memory manager defines
 */
#define DRAM_HEADER_SIZE        32      /* DRAM memory header size */
#define DRAM_BANK_SIZE          0x40000 /* DRAM memory bank size */
#define DMA_BUFFER_SIZE         2048    /* DRAM DMA buffer size */
#define DMA_TIMEOUT             1024    /* DMA-TC busy-waiting timeout */

/*
 * GF1 IRQ and DMA interface latch tables
 */
static BYTE aIrqLatchTable[] =
{
    0x00, 0x00, 0x01, 0x03, 0x00, 0x02, 0x00, 0x04,
    0x00, 0x01, 0x00, 0x05, 0x06, 0x00, 0x00, 0x07
};

static BYTE aDmaLatchTable[] =
{
    0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x04, 0x05
};

/*
 * GF1 sampling frequencies and linear volumes table
 */
static WORD aFrequencyTable[] =
{
    44100, 41160, 38587, 36317, 34300, 32494, 30870, 29400,
    28063, 26843, 25725, 24696, 23746, 22866, 22050, 21289,
    20580, 19916, 19293
};

static WORD aVolumeTable[65] =
{
    0x0400,
    0x07FF, 0x0980, 0x0A40, 0x0AC0, 0x0B20, 0x0B60, 0x0BA0, 0x0BE0,
    0x0C10, 0x0C30, 0x0C50, 0x0C70, 0x0C90, 0x0CB0, 0x0CD0, 0x0CF0,
    0x0D08, 0x0D18, 0x0D28, 0x0D38, 0x0D48, 0x0D58, 0x0D68, 0x0D78,
    0x0D88, 0x0D98, 0x0DA8, 0x0DB8, 0x0DC8, 0x0DD8, 0x0DE8, 0x0DF8,
    0x0E04, 0x0E0C, 0x0E14, 0x0E1C, 0x0E24, 0x0E2C, 0x0E34, 0x0E3C,
    0x0E44, 0x0E4C, 0x0E54, 0x0E5C, 0x0E64, 0x0E6C, 0x0E74, 0x0E7C,
    0x0E84, 0x0E8C, 0x0E94, 0x0E9C, 0x0EA4, 0x0EAC, 0x0EB4, 0x0EBC,
    0x0EC4, 0x0ECC, 0x0ED4, 0x0EDC, 0x0EE4, 0x0EEC, 0x0EF4, 0x0EFC
};

#ifdef INTERWAVE
/*
 * InterWave panning table
 */
static WORD aPanningTable[128] =
{
    0xFFF0, 0x6FD0, 0x5FD0, 0x5670, 0x4FD0, 0x4AB0, 0x4670, 0x42E0,
    0x3FD0, 0x3D20, 0x3AB0, 0x3870, 0x3670, 0x34A0, 0x32E0, 0x3150,
    0x2FD0, 0x2E70, 0x2D20, 0x2BE0, 0x2AB0, 0x2990, 0x2870, 0x2770,
    0x2670, 0x2580, 0x24A0, 0x23C0, 0x22E0, 0x2210, 0x2150, 0x2090,
    0x1FD0, 0x1F20, 0x1E70, 0x1DC0, 0x1D20, 0x1C70, 0x1BE0, 0x1B40,
    0x1AB0, 0x1A20, 0x1990, 0x1900, 0x1870, 0x17F0, 0x1770, 0x16F0,
    0x1670, 0x1600, 0x1580, 0x1510, 0x14A0, 0x1430, 0x13C0, 0x1350,
    0x12E0, 0x1280, 0x1210, 0x11B0, 0x1150, 0x10F0, 0x1090, 0x1030,
    0x0FD0, 0x0F70, 0x0F20, 0x0EC0, 0x0E70, 0x0E10, 0x0DC0, 0x0D70,
    0x0D20, 0x0CD0, 0x0C70, 0x0C30, 0x0BE0, 0x0B90, 0x0B40, 0x0AF0,
    0x0AB0, 0x0A60, 0x0A20, 0x09D0, 0x0990, 0x0940, 0x0900, 0x08C0,
    0x0870, 0x0830, 0x07F0, 0x07B0, 0x0770, 0x0730, 0x06F0, 0x06B0,
    0x0670, 0x0640, 0x0600, 0x05C0, 0x0580, 0x0550, 0x0510, 0x04D0,
    0x04A0, 0x0460, 0x0430, 0x03F0, 0x03C0, 0x0380, 0x0350, 0x0320,
    0x02E0, 0x02B0, 0x0280, 0x0250, 0x0210, 0x01E0, 0x01B0, 0x0180,
    0x0150, 0x0120, 0x00F0, 0x00C0, 0x0090, 0x0060, 0x0030, 0x0000
};

/*
 * InterWave supported DRAM bank configurations
 */
static WORD aBankCfgTable[13][4] =
{
    {  256,    0,    0,    0 },     /* 0x00 -   256K */
    {  256,  256,    0,    0 },     /* 0x01 -   512K */
    {  256,  256,  256,  256 },     /* 0x02 -  1024K */
    {  256, 1024,    0,    0 },     /* 0x03 -  1280K */
    {  256, 1024, 1024, 1024 },     /* 0x04 -  3328K */
    {  256,  256, 1024,    0 },     /* 0x05 -  1536K */
    {  256,  256, 1024, 1024 },     /* 0x06 -  2560K */
    { 1024,    0,    0,    0 },     /* 0x07 -  1024K */
    { 1024, 1024,    0,    0 },     /* 0x08 -  2048K */
    { 1024, 1024, 1024, 1024 },     /* 0x09 -  4096K */
    { 4096,    0,    0,    0 },     /* 0x0A -  4096K */
    { 4096, 4096,    0,    0 },     /* 0x0B -  8192K */
    { 4096, 4096, 4096, 4096 }      /* 0x0C - 16384K */
};
#endif


/* GF1 configuration and shadow registers */
static struct {
#ifdef INTERWAVE
    WORD    wId;                        /* audio device identifier */
#endif
    WORD    wPort;                      /* GF1 base port address */
    BYTE    nIrqLine;                   /* GF1 interrupt line */
    BYTE    nMidiIrqLine;               /* MIDI interrupt line */
    BYTE    nDramDmaChannel;            /* DRAM DMA channel */
    BYTE    nAdcDmaChannel;             /* ADC DMA channel */
    DWORD   dwMemorySize;               /* GF1 DRAM total memory */
    LONG    dwFreqDivisor;              /* current frequency divisor */
    BYTE    bMixControl;                /* board mix control register */
    BYTE    bTimerControl;              /* timer control register */
    BYTE    bTimerMask;                 /* timer data shadow register */
    BYTE    bIrqControl;                /* board IRQ control register */
    BYTE    bDmaControl;                /* board DMA control register */
    BYTE    nVoices;                    /* active voices register */
    DWORD   aVoiceStart[32];            /* voice start address registers */
    DWORD   aVoiceLength[32];           /* voice length in samples */
    LONG    dwTimer1Accum;              /* timer #1 accumulator */
    LONG    dwTimer1Rate;               /* timer #1 rate */
    LONG    dwTimer2Accum;              /* timer #2 accumulator */
    LONG    dwTimer2Rate;               /* timer #2 rate */
    LPFNAUDIOTIMER lpfnTimer1Handler;   /* timer #1 callback */
    LPFNAUDIOTIMER lpfnTimer2Handler;   /* timer #2 callback */
#ifdef INTERWAVE
    DWORD   aBankSize[4];               /* Local Memory Bank Configs */
#endif
} GF1;

static volatile BOOL fDramDmaPending;


/*
 * GF1 basic programming routines
 */
static VOID GF1SelectVoice(BYTE nVoice)
{
    OUTB(GF1.wPort + GUS_GF1_PAGE, nVoice);
}

static VOID GF1PortB(BYTE nIndex, BYTE bData)
{
    OUTB(GF1.wPort + GUS_GF1_INDEX, nIndex);
    OUTB(GF1.wPort + GUS_GF1_DATA_HIGH, bData);
}

static VOID GF1PortW(BYTE nIndex, WORD wData)
{
    OUTB(GF1.wPort + GUS_GF1_INDEX, nIndex);
    OUTW(GF1.wPort + GUS_GF1_DATA_LOW, wData);
}

static BYTE GF1PortRB(BYTE nIndex)
{
    OUTB(GF1.wPort + GUS_GF1_INDEX, nIndex);
    return INB(GF1.wPort + GUS_GF1_DATA_HIGH);
}

static WORD GF1PortRW(BYTE nIndex)
{
    OUTB(GF1.wPort + GUS_GF1_INDEX, nIndex);
    return INW(GF1.wPort + GUS_GF1_DATA_LOW);
}

static BYTE GF1GetIrqStatus(VOID)
{
    return INB(GF1.wPort + GUS_GF1_IRQ_STAT);
}

static VOID GF1StartTimer(UINT nTimer)
{
    if (nTimer == 1) {
        GF1.bTimerControl |= TIMER1_IRQ_ENABLE;
        GF1.bTimerMask |= TIMER1_START_UP;
    }
    else {
        GF1.bTimerControl |= TIMER2_IRQ_ENABLE;
        GF1.bTimerMask |= TIMER2_START_UP;
    }
    GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl);
    OUTB(GF1.wPort + GUS_GF1_TIMER_CTRL, TIMER_CTRL_SELECT);
    OUTB(GF1.wPort + GUS_GF1_TIMER_DATA, GF1.bTimerMask);
}

static VOID GF1StopTimer(UINT nTimer)
{
    if (nTimer == 1) {
        GF1.bTimerControl &= ~TIMER1_IRQ_ENABLE;
        GF1.bTimerMask &= ~TIMER1_START_UP;
    }
    else {
        GF1.bTimerControl &= ~TIMER2_IRQ_ENABLE;
        GF1.bTimerMask &= ~TIMER2_START_UP;
    }
    GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl);
    OUTB(GF1.wPort + GUS_GF1_TIMER_CTRL, TIMER_CTRL_SELECT);
    OUTB(GF1.wPort + GUS_GF1_TIMER_DATA, GF1.bTimerMask);
}

static VOID GF1SetTimerRate(UINT nTimer, BYTE nCount)
{
    if (nTimer == 1) {
        GF1PortB(GF1_TIMER1_COUNT, 256 - nCount);
    }
    else {
        GF1PortB(GF1_TIMER2_COUNT, 256 - nCount);
    }
}


/*
 * GF1 board mix control programming routines
 */
static VOID GF1EnableLineIn(VOID)
{
    GF1.bMixControl &= ~MIXER_DISABLE_LINE_IN;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}

static VOID GF1DisableLineIn(VOID)
{
    GF1.bMixControl |= MIXER_DISABLE_LINE_IN;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}

static VOID GF1EnableLineOut(VOID)
{
    GF1.bMixControl &= ~MIXER_DISABLE_LINE_OUT;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}

static VOID GF1DisableLineOut(VOID)
{
    GF1.bMixControl |= MIXER_DISABLE_LINE_OUT;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}

static VOID GF1EnableMicIn(VOID)
{
    GF1.bMixControl |= MIXER_ENABLE_MIC_IN;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}

static VOID GF1DisableMicIn(VOID)
{
    GF1.bMixControl &= ~MIXER_ENABLE_MIC_IN;
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);
}


/*
 * GF1 basic onboard DRAM programming routines
 */
static DWORD GF1ConvertTo16Bits(DWORD dwAddr)
{
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        return (dwAddr >> 1);
#endif
    return ((dwAddr >> 1) & 0x0001FFFFL) | (dwAddr & 0x000C0000L);
}

static DWORD GF1ConvertFrom16Bits(DWORD dwAddr)
{
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        return (dwAddr << 1);
#endif
    return ((dwAddr << 1) & 0x0003FFFFL) | (dwAddr & 0x000C0000L);
}

static BYTE GF1PeekB(DWORD dwAddr)
{
    GF1PortW(GF1_DRAM_ADDR_LOW, LOWORD(dwAddr));
    GF1PortB(GF1_DRAM_ADDR_HIGH, (BYTE) HIWORD(dwAddr));
    return INB(GF1.wPort + GUS_GF1_DRAM_DATA);
}

static VOID GF1PokeB(DWORD dwAddr, BYTE bData)
{
    GF1PortW(GF1_DRAM_ADDR_LOW, LOWORD(dwAddr));
    GF1PortB(GF1_DRAM_ADDR_HIGH, (BYTE) HIWORD(dwAddr));
    OUTB(GF1.wPort + GUS_GF1_DRAM_DATA, bData);
}

static WORD GF1PeekW(DWORD dwAddr)
{
    WORD wLoData, wHiData;

    wLoData = GF1PeekB(dwAddr + 0);
    wHiData = GF1PeekB(dwAddr + 1);
    return MAKEWORD(wLoData, wHiData);
}

static VOID GF1PokeW(DWORD dwAddr, WORD wData)
{
    GF1PokeB(dwAddr + 0, LOBYTE(wData));
    GF1PokeB(dwAddr + 1, HIBYTE(wData));
}

static DWORD GF1PeekDW(DWORD dwAddr)
{
    DWORD dwLoData, dwHiData;

    dwLoData = GF1PeekW(dwAddr + 0);
    dwHiData = GF1PeekW(dwAddr + 2);
    return MAKELONG(dwLoData, dwHiData);
}

static VOID GF1PokeDW(DWORD dwAddr, DWORD dwData)
{
    GF1PokeW(dwAddr + 0, LOWORD(dwData));
    GF1PokeW(dwAddr + 2, HIWORD(dwData));
}

#ifdef USEPOKE

static VOID GF1PokeBytes(DWORD dwAddr, LPBYTE lpBuffer, UINT nSize)
{
    DWORD dwChunkSize;

    while (nSize != 0) {
        dwChunkSize = 0x10000L - LOWORD(dwAddr);
        if (dwChunkSize > nSize)
            dwChunkSize = nSize;
        nSize -= dwChunkSize;
        GF1PortB(GF1_DRAM_ADDR_HIGH, (BYTE) HIWORD(dwAddr));
        GF1PortW(GF1_DRAM_ADDR_LOW, LOWORD(dwAddr));
        while (dwChunkSize--) {
            OUTW(GF1.wPort + GUS_GF1_DATA_LOW, LOWORD(dwAddr));
            OUTB(GF1.wPort + GUS_GF1_DRAM_DATA, *lpBuffer++);
            dwAddr++;
        }
    }
}

#else

static VOID GF1UploadData(WORD wFormat, DWORD dwAddr,
			  LPBYTE lpBuffer, UINT nSize)
{
    LPBYTE lpDmaBuffer;
    BYTE bControl;
    UINT nChannelCount, nTimeOut, nCount;

    /*
     * The DRAM address must be 32-byte aligned, so we need to use
     * direct I/O to poke the unaligned bytes into DRAM memory.
     */
    while ((dwAddr & 0x1F) != 0x00 && nSize != 0) {
        GF1PokeB(dwAddr++, *lpBuffer++);
        nSize--;
    }

    /* setup the DRAM DMA control bit fields */
    bControl = DRAM_DMA_ENABLE | DRAM_DMA_IRQ_ENABLE |
        DRAM_DMA_WRITE | DRAM_DMA_RATE0;
    if (GF1.nDramDmaChannel >= 4)
        bControl |= DRAM_DMA_WIDTH_16;
    if (wFormat & AUDIO_FORMAT_16BITS)
        bControl |= DRAM_DMA_DATA_16;

    while (nSize != 0) {
        /* setup the DMA controller and the DMA buffer data */
        if ((nCount = nSize) > DMA_BUFFER_SIZE)
            nCount = DMA_BUFFER_SIZE;

        /*
         * We should fill the DMA buffer before setting the DMA controller
         * to start the transfer. This fixes some problems when running
         * under Windows 95 or any other operating system that provides
         * virtualized DMA controllers.
         */
        if ((lpDmaBuffer = DosLockChannel(GF1.nDramDmaChannel)) != NULL) {
            memcpy(lpDmaBuffer, lpBuffer, nCount);
            DosUnlockChannel(GF1.nDramDmaChannel);
        }
        DosSetupChannel(GF1.nDramDmaChannel, DMA_WRITE, nCount);
        fDramDmaPending = 0;

        /* setup the starting DRAM address in paragraphs */
        GF1PortW(GF1_DMA_ADDR, (bControl & DRAM_DMA_WIDTH_16 ?
				GF1ConvertTo16Bits(dwAddr) : dwAddr) >> 4);

#ifdef INTERWAVE
        if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
            DWORD dwLDSA;
            dwLDSA = bControl & DRAM_DMA_WIDTH_16 ?
                GF1ConvertTo16Bits(dwAddr) : dwAddr;
            /* set DRAM address bits 23:20 and 3:0 */
            GF1PortB(IW_LDSAHI, 
		     ((dwLDSA >> 16) & 0x00F0) | (dwLDSA & 0x000F));
        }
#endif

        /* start DMA DRAM transfer */
        GF1PortB(GF1_DRAM_DMA_CTRL, bControl);

        /* wait until the DMA transfer finishes */
        for (nTimeOut = nChannelCount = 0; nTimeOut < DMA_TIMEOUT &&
		 !fDramDmaPending; nTimeOut++) {
            if (DosGetChannelCount(GF1.nDramDmaChannel) != nChannelCount) {
                nChannelCount = DosGetChannelCount(GF1.nDramDmaChannel);
                nTimeOut = 0;
            }
        }

        /* stop DMA DRAM transfer */
        GF1PortB(GF1_DRAM_DMA_CTRL,
		 GF1PortRB(GF1_DRAM_DMA_CTRL) & ~DRAM_DMA_ENABLE);
        DosDisableChannel(GF1.nDramDmaChannel);

        /* advance the pointers to the next chunk */
        lpBuffer += nCount;
        dwAddr += nCount;
        nSize -= nCount;
    }

}

#endif


/*
 * GF1 basic synthesizer programming routines
 */
static VOID GF1Delay(VOID)
{
    UINT n;

    for (n = 0; n < 8; n++) {
        INB(GF1.wPort + GUS_GF1_DRAM_DATA);
    }
}

static VOID GF1Reset(UINT nVoices)
{
    DWORD dwData;
    UINT n;

    /* number of voices must be between 14 and 32 */
    if (nVoices < 14)
        nVoices = 14;
    else if (nVoices > 32)
        nVoices = 32;

    /* reset GF1 shadow registers */
    GF1.nVoices = nVoices;
    GF1.dwFreqDivisor = aFrequencyTable[nVoices - 14];
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        /* InterWave is always at 44100 Hz */
        GF1.dwFreqDivisor = 44100;
#endif
    GF1.bMixControl = MIXER_DISABLE_LINE_IN |
        MIXER_DISABLE_LINE_OUT | MIXER_ENABLE_LATCHES;
    GF1.bTimerControl = 0x00;
    GF1.bTimerMask = 0x00;
    GF1.bIrqControl = 0x00;
    GF1.bDmaControl = 0x00;

    /* pull a reset on the GF1 synthesizer */
    dwData = GF1PeekDW(0L);
    GF1PokeDW(0L, 0L);
    GF1PortB(GF1_MASTER_RESET, 0x00);
    for (n = 0; n < 16; n++)
        GF1Delay();

    /* release reset and wait */
    GF1PortB(GF1_MASTER_RESET, MASTER_RESET);
    for (n = 0; n < 16; n++)
        GF1Delay();
    GF1PokeDW(0L, dwData);

    /* reset MIDI ports */
    OUTB(GF1.wPort + GUS_MIDI_CTRL, MIDI_RESET);
    for (n = 0; n < 16; n++)
        GF1Delay();
    OUTB(GF1.wPort + GUS_MIDI_CTRL, 0x00);

#ifdef INTERWAVE
    /* force InterWave native node */
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        GF1PortB(IW_SGMI_WR, GF1PortRB(IW_SGMI_RD) | IW_SGMI_ENHMODE);
#endif

    /* clear all the GF1 interrupts */
    GF1PortB(GF1_DRAM_DMA_CTRL, 0x00);
    GF1PortB(GF1_TIMER_CTRL, 0x00);
    GF1PortB(GF1_ADC_DMA_CTRL, 0x00);

    /* set number of active voices */
    GF1PortB(GF1_SET_NUMVOICES, (nVoices - 1) | 0xC0);

    /* clear interrupt on voices */
    GF1GetIrqStatus();
    GF1PortRB(GF1_DRAM_DMA_CTRL);
    GF1PortRB(GF1_ADC_DMA_CTRL);
    GF1PortRB(GF1_GET_IRQ_STAT);

    for (n = 0; n < nVoices; n++) {
        /* select proper voice */
        GF1SelectVoice(n);

        /* stop voice and volume */
        GF1PortB(GF1_SET_VOICE_CTRL, VOICE_STOPPED | VOICE_STOP);
        GF1PortB(GF1_SET_VOLUME_CTRL, VOLUME_STOPPED | VOLUME_STOP);
        GF1Delay();

        /* initialize voice specific registers */
        GF1PortW(GF1_SET_FREQ_CTRL, 0x400);
        GF1PortB(GF1_SET_PANNING, 0x07);
        GF1PortW(GF1_SET_START_HIGH, 0x0000);
        GF1PortW(GF1_SET_START_LOW, 0x0000);
        GF1PortW(GF1_SET_END_HIGH, 0x0000);
        GF1PortW(GF1_SET_END_LOW, 0x0000);
        GF1PortB(GF1_SET_VOLUME_RATE, 0x3F);
        GF1PortB(GF1_SET_VOLUME_START, 0x40);
        GF1PortB(GF1_SET_VOLUME_END, 0x40);
        GF1PortW(GF1_SET_VOLUME, 0x04000);
        GF1PortW(GF1_SET_ACCUM_HIGH, 0x0000);
        GF1PortW(GF1_SET_ACCUM_LOW, 0x0000);
#ifdef INTERWAVE
        if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
            /* active voice smooth panning */
            GF1PortB(IW_SMSI_WR, IW_SMSI_VOICE_OFFSET);
            /* InterWave synth upper address */
            GF1PortB(IW_SUAI_WR, 0x00);
            /* InterWave synth left offset */
            GF1PortW(IW_SLOI_WR, aPanningTable[64]);
            /* InterWave synth right offset */
            GF1PortW(IW_SROI_WR, aPanningTable[64]);
        }
#endif
    }

    /* clear interrupt on voices (again) */
    GF1GetIrqStatus();
    GF1PortRB(GF1_DRAM_DMA_CTRL);
    GF1PortRB(GF1_ADC_DMA_CTRL);
    GF1PortRB(GF1_GET_IRQ_STAT);

    /* setup GF1 synth for interrupts and enable DACs */
    GF1PortB(GF1_MASTER_RESET, MASTER_RESET |
	     MASTER_ENABLE_DAC | MASTER_ENABLE_IRQ);
}

static VOID GF1SetInterface(VOID)
{
    /* set GF1/MIDI IRQ latches */
    GF1.bIrqControl |= aIrqLatchTable[GF1.nIrqLine];
    if (!GF1.nIrqLine || (GF1.nIrqLine != GF1.nMidiIrqLine))
        GF1.bIrqControl |= aIrqLatchTable[GF1.nMidiIrqLine] << 3;
    else
        GF1.bIrqControl |= 0x40;

    /* set DRAM/ADC DMA latches */
    GF1.bDmaControl |= aDmaLatchTable[GF1.nDramDmaChannel];
    if (!GF1.nDramDmaChannel || (GF1.nDramDmaChannel != GF1.nAdcDmaChannel))
        GF1.bDmaControl |= aDmaLatchTable[GF1.nAdcDmaChannel] << 3;
    else
        GF1.bDmaControl |= 0x40;

    /* setup for digital ASIC */
    OUTB(GF1.wPort + GUS_BOARD_REG_CTRL, 0x05);
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl | MIXER_LATCH_DMA);
    OUTB(GF1.wPort + GUS_BOARD_IRQ_CLEAR, 0x00);
    OUTB(GF1.wPort + GUS_BOARD_REG_CTRL, 0x00);

    /* first do DMA control register */
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl | MIXER_LATCH_DMA);
    OUTB(GF1.wPort + GUS_BOARD_DMA_CTRL, GF1.bDmaControl | 0x80);

    /* now do IRQ control register */
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl | MIXER_LATCH_IRQ);
    OUTB(GF1.wPort + GUS_BOARD_IRQ_CTRL, GF1.bIrqControl);

    /* first do DMA control register */
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl | MIXER_LATCH_DMA);
    OUTB(GF1.wPort + GUS_BOARD_DMA_CTRL, GF1.bDmaControl);

    /* now do IRQ control register */
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl | MIXER_LATCH_IRQ);
    OUTB(GF1.wPort + GUS_BOARD_IRQ_CTRL, GF1.bIrqControl);

    /* lock out writes to IRQ/DMA control registers */
    OUTB(GF1.wPort + GUS_GF1_PAGE, 0x00);

    /* enable output and interrupts, disable line & mic input */
    GF1.bMixControl |= (MIXER_DISABLE_LINE_IN | MIXER_ENABLE_LATCHES);
    OUTB(GF1.wPort + GUS_BOARD_MIX_CTRL, GF1.bMixControl);

    /* lock out writes to IRQ/DMA control registers */
    OUTB(GF1.wPort + GUS_GF1_PAGE, 0x00);
}

static VOID AIAPI GF1InterruptHandler(VOID)
{
    BYTE bIrqStatus, nPage, nRegister;

    nPage = INB(GF1.wPort + GUS_GF1_PAGE);
    nRegister = INB(GF1.wPort + GUS_GF1_INDEX);
    while ((bIrqStatus = GF1GetIrqStatus()) != 0x00) {
        if (bIrqStatus & IRQ_STAT_TIMER1) {
            GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl & ~TIMER1_IRQ_ENABLE);
            GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl);
            if (GF1.lpfnTimer1Handler != NULL) {
                GF1.dwTimer1Accum += GF1.dwTimer1Rate;
                while (GF1.dwTimer1Accum >= 0x10000L) {
                    GF1.dwTimer1Accum -= 0x10000L;
                    GF1.lpfnTimer1Handler();
                }
            }
        }
        if (bIrqStatus & IRQ_STAT_TIMER2) {
            GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl & ~TIMER2_IRQ_ENABLE);
            GF1PortB(GF1_TIMER_CTRL, GF1.bTimerControl);
            if (GF1.lpfnTimer2Handler != NULL) {
                GF1.dwTimer2Accum += GF1.dwTimer2Rate;
                while (GF1.dwTimer2Accum >= 0x10000L) {
                    GF1.dwTimer2Accum -= 0x10000L;
                    GF1.lpfnTimer2Handler();
                }
            }
        }
        if (bIrqStatus & IRQ_STAT_DMA_TC) {
            if (GF1PortRB(GF1_DRAM_DMA_CTRL) & DRAM_DMA_IRQ_PENDING) {
                fDramDmaPending++;
            }
        }
    }
    OUTB(GF1.wPort + GUS_GF1_INDEX, nRegister);
    OUTB(GF1.wPort + GUS_GF1_PAGE, nPage);
}


static BOOL GF1Probe(VOID)
{
    WORD wData, wMyData;

    /* pull a reset on the GF1 */
    GF1PortB(GF1_MASTER_RESET, 0x00);
    GF1Delay();
    GF1Delay();

    /* release reset on the GF1 */
    GF1PortB(GF1_MASTER_RESET, MASTER_RESET);
    GF1Delay();
    GF1Delay();

    /* probe on board DRAM memory */
    wData = GF1PeekW(0L);
    GF1PokeW(0L, 0x55AA);
    wMyData = GF1PeekW(0L);
    GF1PokeW(0L, wData);
    return (wMyData != 0x55AA);
}

/*
 * GF1 onboard DRAM memory manager routines
 */
static DWORD GF1MemorySize(VOID)
{
    DWORD dwAddr;

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
        UINT nBank, nBankCfg;

        /* force InterWave enhanced mode */
        GF1PortB(IW_SGMI_WR, GF1PortRB(IW_SGMI_RD) | IW_SGMI_ENHMODE);

        /* enable full access to 16MB */
        GF1PortW(IW_LMCFI, (GF1PortRW(IW_LMCFI) & 0xFFF0) | 0x0C);

        /* clean every RAM step location */
        for (dwAddr = 0x0000000L; dwAddr < 0x1000000L; dwAddr += 0x10000L)
            GF1PokeDW(dwAddr, 0x12345678L);

        /* determine amount of DRAM in each bank */
        for (nBank = 0; nBank < 4; nBank++) {
            GF1.aBankSize[nBank] = 0;
            dwAddr = ((DWORD) nBank) << 22;
            if (GF1PeekDW(dwAddr) == 0x12345678L) {
                GF1PokeDW(dwAddr, 0x55AA55AAL);
                if (GF1PeekDW(dwAddr) == 0x55AA55AAL) {
                    /* each bank can have up to 4MB of memory */
                    while ((GF1.aBankSize[nBank] += 64) < 4096) {
                        dwAddr += 0x10000L;
                        if (GF1PeekDW(dwAddr) == 0x55AA55AAL) break;
                    }
                }
            }
        }

        for (nBankCfg = 0; nBankCfg < 13; nBankCfg++) {
            if (aBankCfgTable[nBankCfg][0] == GF1.aBankSize[0] &&
                aBankCfgTable[nBankCfg][1] == GF1.aBankSize[1] &&
                aBankCfgTable[nBankCfg][2] == GF1.aBankSize[2] &&
                aBankCfgTable[nBankCfg][3] == GF1.aBankSize[3]) {
                /* set up contiguous access to local memory banks */
                GF1PortW(IW_LMCFI, (GF1PortRW(IW_LMCFI) & 0xFFF0) | nBankCfg);
                GF1.aBankSize[0] += 
                    GF1.aBankSize[1] + GF1.aBankSize[2] + GF1.aBankSize[3];
                GF1.aBankSize[1] = GF1.aBankSize[2] = GF1.aBankSize[3] = 0;
                return (DWORD) GF1.aBankSize[0] << 10;
            }
        }

        /* enable non-contiguous access to local memory banks */
        GF1PortW(IW_LMCFI, (GF1PortRW(IW_LMCFI) & 0xFFF0) | 0x0C);
        return (DWORD)(GF1.aBankSize[0] + GF1.aBankSize[1] +
		       GF1.aBankSize[2] + GF1.aBankSize[3]) << 10;
    }
#endif
    dwAddr = 0x00000L;
    GF1PokeDW(dwAddr, 0x12345678L);
    if (GF1PeekDW(dwAddr) == 0x12345678L) {
        GF1PokeDW(dwAddr, 0x00000000L);
        while ((dwAddr += 0x00400L) < 0x100000L) {
            GF1PokeDW(dwAddr, 0x12345678L);
            if (GF1PeekDW(dwAddr) != 0x12345678L)
                break;
            if (GF1PeekDW(0x00000L) != 0x00000000L)
                break;
        }
    }
    return dwAddr;
}

static LONG GF1MemAvail(VOID)
{
    DWORD dwPrevNode, dwNode, dwMemSize;

    dwMemSize = 0L;
    dwPrevNode = 0L;
    while ((dwNode = GF1PeekDW(dwPrevNode + 0)) != 0L) {
        dwMemSize += GF1PeekDW(dwNode + 4);
        dwPrevNode = dwNode;
    }
    return dwMemSize;
}

static VOID GF1MemInit(VOID)
{
    DWORD dwNode, dwNextNode, dwNodeSize;

    /* get amount of GF1 DRAM memory in bytes */
    GF1.dwMemorySize = GF1MemorySize();

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
        DWORD nBank, dwTopMemory;

        dwNode = 0L;
        dwNodeSize = DRAM_HEADER_SIZE;
        dwNextNode = dwNode + dwNodeSize;
        GF1PokeDW(dwNode + 0, dwNextNode);
        GF1PokeDW(dwNode + 4, dwNodeSize);

        /* split DRAM banks in blocks of 256KB */
        /* WARNING: the first bank must have memory */
        nBank = 0;
        dwTopMemory = (GF1.aBankSize[nBank] << 10);
        while ((dwNode = dwNextNode) != 0L) {
            dwNextNode = (dwNode + DRAM_BANK_SIZE) & -DRAM_BANK_SIZE;
            dwNodeSize = (dwNextNode - dwNode);
            if (dwNextNode >= dwTopMemory) {
                dwNodeSize = (dwTopMemory - dwNode);
                while (++nBank < 4 && !GF1.aBankSize[nBank])
                    continue;
                if (nBank < 4) {
                    dwNextNode = ((DWORD)nBank << 22);
                    dwTopMemory = dwNextNode + (GF1.aBankSize[nBank] << 10);
                }
                else {
                    dwNextNode = 0x00000L;
                }
            }
            GF1PokeDW(dwNode + 0, dwNextNode);
            GF1PokeDW(dwNode + 4, dwNodeSize);
        }
        return;
    }
#endif

    /* setup first DRAM memory block */
    dwNode = 0x00000L;
    dwNodeSize = DRAM_HEADER_SIZE;
    dwNextNode = dwNode + dwNodeSize;
    GF1PokeDW(dwNode + 0, dwNextNode);
    GF1PokeDW(dwNode + 4, dwNodeSize);

    /* split DRAM memory in blocks of 256 kilobytes */
    while ((dwNode = dwNextNode) != 0L) {
        dwNextNode = (dwNode + DRAM_BANK_SIZE) & -DRAM_BANK_SIZE;
        dwNodeSize = (dwNextNode - dwNode);
        if (dwNextNode >= GF1.dwMemorySize) {
            dwNextNode = 0x00000L;
            dwNodeSize = (GF1.dwMemorySize - dwNode);
        }
        GF1PokeDW(dwNode + 0, dwNextNode);
        GF1PokeDW(dwNode + 4, dwNodeSize);
    }
}


static DWORD GF1MemAlloc(DWORD dwSize)
{
    DWORD dwNode, dwPrevNode, dwNodeSize;

    if ((dwSize += DRAM_HEADER_SIZE) != 0) {
        dwSize = (dwSize + DRAM_HEADER_SIZE - 1) & -DRAM_HEADER_SIZE;
        dwPrevNode = 0L;
        while ((dwNode = GF1PeekDW(dwPrevNode + 0)) != 0L) {
            if ((dwNodeSize = GF1PeekDW(dwNode + 4)) >= dwSize) {
                if (dwNodeSize == dwSize) {
                    GF1PokeDW(dwPrevNode + 0, GF1PeekDW(dwNode + 0L));
                }
                else {
                    dwNodeSize -= dwSize;
                    GF1PokeDW(dwNode + 4, dwNodeSize);
                    dwNode += dwNodeSize;
                    GF1PokeDW(dwNode + 4, dwSize);
                }
                GF1PokeDW(dwNode + 0, ~dwSize);
                return (dwNode + DRAM_HEADER_SIZE);
            }
            dwPrevNode = dwNode;
        }
    }
    return 0L;
}

static VOID GF1MemFree(DWORD dwAddr)
{
    DWORD dwNode, dwNextNode, dwNodeSize;

    if ((dwAddr -= DRAM_HEADER_SIZE) < (16L << 20) &&
        GF1PeekDW(dwAddr + 0) == ~GF1PeekDW(dwAddr + 4)) {
        dwNode = 0L;
        while (dwNode != (16L << 20)) {
            if ((dwNextNode = GF1PeekDW(dwNode + 0L)) == 0L)
                dwNextNode = (16L << 20);
            if (dwAddr > dwNode && dwAddr < dwNextNode)
                break;
            dwNode = dwNextNode;
        }
        if (dwNode != (16L << 20)) {
            dwNodeSize = GF1PeekDW(dwAddr + 4L);
            if (dwAddr + dwNodeSize == dwNextNode &&
                (dwNextNode & (DRAM_BANK_SIZE-1)) != 0L) {
                GF1PokeDW(dwAddr + 4, dwNodeSize + GF1PeekDW(dwNextNode + 4));
                GF1PokeDW(dwAddr + 0, GF1PeekDW(dwNextNode + 0));
            }
            else {
                GF1PokeDW(dwAddr + 0, dwNextNode);
            }
            dwNodeSize = GF1PeekDW(dwNode + 4);
            if (dwNode + dwNodeSize == dwAddr &&
                (dwAddr & (DRAM_BANK_SIZE-1)) != 0L) {
                GF1PokeDW(dwNode + 4, dwNodeSize + GF1PeekDW(dwAddr + 4));
                GF1PokeDW(dwNode + 0, GF1PeekDW(dwAddr + 0));
            }
            else {
                GF1PokeDW(dwNode + 0, dwAddr);
            }
        }
    }
}



/*
 * GF1 low level voice programming routines
 */
static VOID GF1PrimeVoice(DWORD dwStart, DWORD dwLoopStart,
			  DWORD dwLoopEnd, BYTE bControl)
{
    DWORD dwTemp;

    bControl |= (VOICE_STOPPED | VOICE_STOP);

    if (dwLoopStart > dwLoopEnd) {
        dwTemp = dwLoopStart;
        dwLoopStart = dwLoopEnd;
        dwLoopEnd = dwTemp;
        bControl |= VOICE_DIRECT;
    }

    if (bControl & VOICE_DATA_16) {
        dwStart = GF1ConvertTo16Bits(dwStart << 1);
        dwLoopStart = GF1ConvertTo16Bits(dwLoopStart << 1);
        dwLoopEnd = GF1ConvertTo16Bits(dwLoopEnd << 1);
    }

    GF1PortB(GF1_SET_VOICE_CTRL, bControl);
    GF1Delay();
    GF1PortB(GF1_SET_VOICE_CTRL, bControl);

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        /* select InterWave local memory bank */
        GF1PortB(IW_SUAI_WR, (dwStart >> 22) & 0x03);
#endif

    GF1PortW(GF1_SET_ACCUM_HIGH, dwStart >> 7);
    GF1PortW(GF1_SET_ACCUM_LOW, dwStart << 9);

    GF1PortW(GF1_SET_START_HIGH, dwLoopStart >> 7);
    GF1PortW(GF1_SET_START_LOW, dwLoopStart << 9);

    GF1PortW(GF1_SET_END_HIGH, dwLoopEnd >> 7);
    GF1PortW(GF1_SET_END_LOW, dwLoopEnd << 9);
}

static VOID GF1StartVoice(VOID)
{
    BYTE bControl;

    bControl = GF1PortRB(GF1_GET_VOICE_CTRL) & ~VOICE_ENABLE_IRQ;
    bControl &= ~(VOICE_STOPPED | VOICE_STOP);

    GF1PortB(GF1_SET_VOICE_CTRL, bControl);
    GF1Delay();
    GF1PortB(GF1_SET_VOICE_CTRL, bControl);

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        /* activate voice */
        GF1PortB(IW_SMSI_WR, GF1PortRB(IW_SMSI_RD) & ~IW_SMSI_VOICE_DISABLE);
#endif
}

static VOID GF1StopVoice(VOID)
{
    BYTE bControl;

    bControl = GF1PortRB(GF1_GET_VOICE_CTRL) & ~VOICE_ENABLE_IRQ;
    bControl |= (VOICE_STOPPED | VOICE_STOP);

    GF1PortB(GF1_SET_VOICE_CTRL, bControl);
    GF1Delay();
    GF1PortB(GF1_SET_VOICE_CTRL, bControl);

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
        /* de-activate voice */
        GF1PortB(IW_SMSI_WR, GF1PortRB(IW_SMSI_RD) | IW_SMSI_VOICE_DISABLE);
    }
#endif
}

static BOOL GF1VoiceStopped(VOID)
{
    return (GF1PortRB(GF1_GET_VOICE_CTRL) & (VOICE_STOPPED | VOICE_STOP)) != 0;
}

static VOID GF1SetVoicePosition(DWORD dwAddr)
{
    BYTE bControl;

    bControl = GF1PortRB(GF1_GET_VOICE_CTRL) & ~VOICE_ENABLE_IRQ;
    if (bControl & VOICE_DATA_16)
        dwAddr = GF1ConvertTo16Bits(dwAddr << 1);

    GF1PortB(GF1_SET_VOICE_CTRL, bControl | VOICE_STOP | VOICE_STOPPED);
    GF1Delay();
    GF1PortB(GF1_SET_VOICE_CTRL, bControl | VOICE_STOP | VOICE_STOPPED);

#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        /* select InterWave local memory bank */
        GF1PortB(IW_SUAI_WR, (dwAddr >> 22) & 0x03);
#endif

    GF1PortW(GF1_SET_ACCUM_HIGH, dwAddr >> 7);
    GF1PortW(GF1_SET_ACCUM_LOW, dwAddr << 9);

    GF1PortB(GF1_SET_VOICE_CTRL, bControl);
    GF1Delay();
    GF1PortB(GF1_SET_VOICE_CTRL, bControl);
}

static DWORD GF1GetVoicePosition(VOID)
{
    DWORD dwAddr;
    UINT nTimeOut;

    for (nTimeOut = 0; nTimeOut < 4; nTimeOut++) {
        dwAddr = GF1PortRW(GF1_GET_ACCUM_HIGH);
        dwAddr = (dwAddr << 7) & 0xFFF80L;
        dwAddr |= (GF1PortRW(GF1_GET_ACCUM_LOW) >> 9) & 0x0007FL;
#ifdef INTERWAVE
        if (GF1.wId == AUDIO_PRODUCT_IWAVE)
            /* retrieve InterWave local memory bank */
            dwAddr |= ((DWORD) GF1PortRB(IW_SUAI_RD) & 0x03L) << 22;
#endif
        if (!(((dwAddr >> 7) ^ GF1PortRW(GF1_GET_ACCUM_HIGH)) & 0x01FFFL))
            break;
    }

    return (GF1PortRB(GF1_GET_VOICE_CTRL) & VOICE_DATA_16) ?
        (GF1ConvertFrom16Bits(dwAddr) >> 1) : dwAddr;
}

static VOID GF1SetFrequency(LONG dwFrequency)
{
    DWORD FC;

    FC = ((dwFrequency << 9) + (GF1.dwFreqDivisor >> 1)) / GF1.dwFreqDivisor;
    GF1PortW(GF1_SET_FREQ_CTRL, FC << 1);
}

static LONG GF1GetFrequency(VOID)
{
    DWORD FC;

    FC = GF1PortRW(GF1_GET_FREQ_CTRL);
    return ((FC >> 1) * GF1.dwFreqDivisor) >> 9;
}

static VOID GF1SetPanning(BYTE nPanning)
{
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
        nPanning >>= 1;
        /* InterWave synth right offset */
        GF1PortW(IW_SROI_WR, aPanningTable[nPanning]);
        /* InterWave synth left offset */
        GF1PortW(IW_SLOI_WR, aPanningTable[0x7F - nPanning]);
    }
    else
#endif
	GF1PortB(GF1_SET_PANNING, nPanning >> 4);
}

static BYTE GF1GetPanning(VOID)
{
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE) {
        WORD wPanValue, nPanning;
        wPanValue = GF1PortRW(IW_SROI_RD);
        for (nPanning = 0x00; nPanning < 0x7F; nPanning++) {
            if (wPanValue <= aPanningTable[nPanning])
                break;
        }
        return (nPanning << 1);
    }
#endif
    return GF1PortRB(GF1_GET_PANNING) << 4;
}

/*
 * static VOID GF1SetVolume(WORD nVolume)
 * {
 *     GF1PortW(GF1_SET_VOLUME, nVolume << 4);
 * }
 */

static WORD GF1GetVolume(VOID)
{
    return GF1PortRW(GF1_GET_VOLUME) >> 4;
}

static VOID GF1RampVolume(WORD nStartVolume, WORD nEndVolume,
			  BYTE nVolumeRate, BYTE bControl)
{
    WORD nBeginVolume;

    bControl &= ~(VOLUME_IRQ_PENDING | VOLUME_ROLLOVER |
		  VOLUME_STOP | VOLUME_STOPPED);

    if ((nBeginVolume = nStartVolume) > nEndVolume) {
        nStartVolume = nEndVolume;
        nEndVolume = nBeginVolume;
        bControl |= VOLUME_DIRECT;
    }
    if (nStartVolume < 0x040)
        nStartVolume = 0x040;
    if (nEndVolume > 0xFC0)
        nEndVolume = 0xFC0;

    GF1PortB(GF1_SET_VOLUME_RATE, nVolumeRate);
    GF1PortB(GF1_SET_VOLUME_START, nStartVolume >> 4);
    GF1PortB(GF1_SET_VOLUME_END, nEndVolume >> 4);
    GF1PortW(GF1_SET_VOLUME, nBeginVolume << 4);

    GF1PortB(GF1_SET_VOLUME_CTRL, bControl);
    GF1Delay();
    GF1PortB(GF1_SET_VOLUME_CTRL, bControl);
}


/*
 * Ultrasound GF1 driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_GUS, "Ultrasound",
        AUDIO_FORMAT_4S16
    };
#ifdef INTERWAVE
    static AUDIOCAPS IWCaps =
    {
        AUDIO_PRODUCT_IWAVE, "Ultrasound PnP (InterWave)",
        AUDIO_FORMAT_4S16
    };

    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        memcpy(lpCaps, &IWCaps, sizeof(AUDIOCAPS));
    else
#endif
	memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    LPSTR lpszUltrasnd;
    UINT wPort, nDramDma, nAdcDma, nIrqLine, nMidiIrqLine;

    if ((lpszUltrasnd = DosGetEnvironment("ULTRASND")) != NULL) {
        if ((wPort = DosParseString(lpszUltrasnd, TOKEN_HEX)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nDramDma = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nAdcDma = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nIrqLine = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN &&
            (DosParseString(NULL, TOKEN_CHAR) == ',') &&
            (nMidiIrqLine = DosParseString(NULL, TOKEN_DEC)) != BAD_TOKEN) {
            GF1.wPort = wPort;
            GF1.nDramDmaChannel = nDramDma;
            GF1.nAdcDmaChannel = nAdcDma;
            GF1.nIrqLine = nIrqLine;
            GF1.nMidiIrqLine = nMidiIrqLine;
#ifdef INTERWAVE
            GF1.wId = AUDIO_PRODUCT_GUS;
            if (DosGetEnvironment("INTERWAVE") != NULL)
                GF1.wId = AUDIO_PRODUCT_IWAVE;
#endif
            return GF1Probe();
        }
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    /*
     * Initialize GF1 driver configuration structure
     * and check if the hardware is present.
     */
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;
    lpInfo->wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    lpInfo->nSampleRate = 44100;

    /*
     * Allocate an small DMA buffer for DRAM uploading
     */
#ifndef USEPOKE
    if (DosAllocChannel(GF1.nDramDmaChannel, DMA_BUFFER_SIZE))
        return AUDIO_ERROR_NOMEMORY;
#endif

    /*
     * Initialize GF1 DRAM memory manager and interrupt handler
     */
    GF1MemInit();
    DosSetVectorHandler(GF1.nIrqLine, GF1InterruptHandler);

    /*
     * Open the GF1 synthesizer for playback
     */
    GF1Reset(14);
    GF1SetInterface();
    GF1EnableLineOut();
    GF1DisableLineIn();
    GF1DisableMicIn();
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    /*
     * Reset the GF1 synthesizer, restore the interrupt handler
     * and releases the DRAM/DMA buffer used for uploading.
     */
    GF1DisableLineOut();
    GF1EnableLineIn();
    GF1EnableMicIn();
    GF1Reset(14);
#ifdef INTERWAVE
    if (GF1.wId == AUDIO_PRODUCT_IWAVE)
        /* force GF1 compatible mode */
        GF1PortB(IW_SGMI_WR, GF1PortRB(IW_SGMI_RD) & ~IW_SGMI_ENHMODE);
#endif
    DosSetVectorHandler(GF1.nIrqLine, NULL);
#ifndef USEPOKE
    DosFreeChannel(GF1.nDramDmaChannel);
#endif
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioMixerValue(UINT nChannel, UINT nValue)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    /*
     * Open the GF1 synthesizer for playback
     */
    if (nVoices < AUDIO_MAX_VOICES) {
        GF1Reset(nVoices);
        GF1EnableLineOut();
        GF1DisableLineIn();
        GF1DisableMicIn();
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    /*
     * Reset the GF1 synthesizer and restore the interrupt handler
     */
    GF1DisableLineOut();
    GF1EnableLineIn();
    GF1EnableMicIn();
    GF1Reset(14);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    if ((GF1.lpfnTimer2Handler = lpfnAudioTimer) != NULL) {
        GF1StartTimer(2);
    }
    else {
        GF1StopTimer(2);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nBPM)
{
    UINT nTicks;

    if (nBPM >= 0x20 && nBPM <= 0xFF) {
        nTicks = 15625 / (nBPM << 1);
        GF1.dwTimer2Rate = (0x20000L * nBPM * nTicks) / 15625;
        GF1SetTimerRate(2, nTicks);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static LONG AIAPI GetAudioDataAvail(VOID)
{
    return GF1MemAvail();
}

static UINT AIAPI CreateAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL) {
        if ((lpWave->dwHandle = GF1MemAlloc(lpWave->dwLength + 4)) != 0)
            return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NODRAMMEMORY;
}

static UINT AIAPI DestroyAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL && lpWave->dwHandle != 0) {
        GF1MemFree(lpWave->dwHandle);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI WriteAudioData(LPAUDIOWAVE lpWave, DWORD dwOffset, UINT nCount)
{
    if (lpWave != NULL && lpWave->dwHandle != 0) {
        if (dwOffset + nCount <= lpWave->dwLength) {
#ifndef USEPOKE
            GF1UploadData(lpWave->wFormat, lpWave->dwHandle + dwOffset,
			  lpWave->lpData + dwOffset, nCount);
#else
            GF1PokeBytes(lpWave->dwHandle + dwOffset,
			 lpWave->lpData + dwOffset, nCount);
#endif
            /* anticlick removal workaround */
            if (lpWave->wFormat & AUDIO_FORMAT_LOOP) {
                if (dwOffset <= lpWave->dwLoopEnd &&
                    dwOffset + nCount >= lpWave->dwLoopEnd) {
                    GF1PokeDW(lpWave->dwHandle + lpWave->dwLoopEnd,
			      GF1PeekDW(lpWave->dwHandle + lpWave->dwLoopStart));
                }
            }
            else if (dwOffset + nCount >= lpWave->dwLength) {
                GF1PokeDW(lpWave->dwHandle + lpWave->dwLength, 0);
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, LPAUDIOWAVE lpWave)
{
    DWORD dwStart, dwLoopStart, dwLoopEnd;
    BYTE bControl;

    if (nVoice < GF1.nVoices && lpWave != NULL && lpWave->dwHandle != 0) {
        dwStart = lpWave->dwHandle;
        bControl = VOICE_STOP;
        if (lpWave->wFormat & (AUDIO_FORMAT_LOOP | AUDIO_FORMAT_BIDILOOP)) {
            dwLoopStart = lpWave->dwLoopStart;
            dwLoopEnd = lpWave->dwLoopEnd;
            bControl |= VOICE_LOOP;
            if (lpWave->wFormat & AUDIO_FORMAT_BIDILOOP)
                bControl |= VOICE_BIDILOOP;
        }
        else {
            dwLoopStart = dwLoopEnd = lpWave->dwLength;
            bControl |= VOICE_NORMAL;
        }
        if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
            dwStart >>= 1;
            dwLoopStart >>= 1;
            dwLoopEnd >>= 1;
            bControl |= VOICE_DATA_16;
        }
        GF1.aVoiceStart[nVoice] = dwStart;
        GF1.aVoiceLength[nVoice] = dwLoopEnd;
        GF1SelectVoice(nVoice);
        GF1PrimeVoice(dwStart, dwStart + dwLoopStart,
		      dwStart + dwLoopEnd, bControl);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    if (nVoice < GF1.nVoices) {
        GF1SelectVoice(nVoice);
        GF1StartVoice();
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < GF1.nVoices) {
        GF1SelectVoice(nVoice);
        GF1StopVoice();
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < GF1.nVoices) {
        if (dwPosition >= 0 && dwPosition <= GF1.aVoiceLength[nVoice]) {
            GF1SelectVoice(nVoice);
            GF1SetVoicePosition(GF1.aVoiceStart[nVoice] + dwPosition);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceFrequency(UINT nVoice, LONG dwFrequency)
{
    if (nVoice < GF1.nVoices) {
        if (dwFrequency >= AUDIO_MIN_FREQUENCY &&
            dwFrequency <= AUDIO_MAX_FREQUENCY) {
            GF1SelectVoice(nVoice);
            GF1SetFrequency(dwFrequency);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceVolume(UINT nVoice, UINT nVolume)
{
    if (nVoice < GF1.nVoices) {
        if (nVolume <= AUDIO_MAX_VOLUME) {
            GF1SelectVoice(nVoice);
            GF1RampVolume(GF1GetVolume(), aVolumeTable[nVolume], 0x3F, 0x00);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePanning(UINT nVoice, UINT nPanning)
{
    if (nVoice < GF1.nVoices) {
        if (nPanning <= AUDIO_MAX_PANNING) {
            GF1SelectVoice(nVoice);
            GF1SetPanning(nPanning);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, LPLONG lpdwPosition)
{
    if (nVoice < GF1.nVoices) {
        if (lpdwPosition != NULL) {
            GF1SelectVoice(nVoice);
            *lpdwPosition = GF1GetVoicePosition() - GF1.aVoiceStart[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, LPLONG lpdwFrequency)
{
    if (nVoice < GF1.nVoices) {
        if (lpdwFrequency != NULL) {
            GF1SelectVoice(nVoice);
            *lpdwFrequency = GF1GetFrequency();
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, LPUINT lpnVolume)
{
    UINT nVolume, nLogVolume;

    if (nVoice < GF1.nVoices) {
        if (lpnVolume != NULL) {
            GF1SelectVoice(nVoice);
            nLogVolume = GF1GetVolume();
            for (nVolume = 0; nVolume < AUDIO_MAX_VOLUME; nVolume++)
                if (nLogVolume <= aVolumeTable[nVolume])
                    break;
            *lpnVolume = nVolume;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, LPUINT lpnPanning)
{
    if (nVoice < GF1.nVoices) {
        if (lpnPanning != NULL) {
            GF1SelectVoice(nVoice);
            *lpnPanning = GF1GetPanning();
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, LPBOOL lpnStatus)
{
    if (nVoice < GF1.nVoices) {
        if (lpnStatus != NULL) {
            GF1SelectVoice(nVoice);
            *lpnStatus = GF1VoiceStopped();
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}


/*
 * Gravis Ultrasound driver public interface
 */
AUDIOSYNTHDRIVER UltraSoundSynthDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate, SetAudioMixerValue,
    GetAudioDataAvail, CreateAudioData, DestroyAudioData,
    WriteAudioData, PrimeVoice, StartVoice, StopVoice,
    SetVoicePosition, SetVoiceFrequency, SetVoiceVolume,
    SetVoicePanning, GetVoicePosition, GetVoiceFrequency,
    GetVoiceVolume, GetVoicePanning, GetVoiceStatus
};

AUDIODRIVER UltraSoundDriver =
{
    NULL, &UltraSoundSynthDriver
};
