/*
 * $Id: awedrv.c 1.10 1996/09/13 17:00:08 chasan released $
 *               1.11 1998/10/24 18:20:53 chasan released (Mixer API)
 *
 * Sound Blaster AWE32 audio driver.
 *
 * Copyright (C) 1996-1999 Carlos Hasan
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

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif


/*
 * Macros to build a oscillator/register/port packed register
 */
#define AWEREG(nReg, nCnl, nPort) ((nCnl)|((nReg)<<5)|((nPort)<<8))
#define ALL 0

/*
 * Sound Blaster AWE32 I/O ports offsets
 */
#define AWE32_DATA0     0x400       /* read and write of dword data */
#define AWE32_DATA1     0x800       /* read and write of word and dword */
#define AWE32_DATA2     0x802       /* read and write of word data */
#define AWE32_DATA3     0xC00       /* read and write of word data */
#define AWE32_POINTER   0xC02       /* read and write of register pointer */

/*
 * Sound Blaster AWE32 voice indirect registers
 */
#define CPF     AWEREG(0, ALL, 0)   /* current pitch and fractional address */
#define PTRX    AWEREG(1, ALL, 0)   /* pitch target, reverb send, aux byte */
#define CVCF    AWEREG(2, ALL, 0)   /* current volume and filter cutoff */
#define VTFT    AWEREG(3, ALL, 0)   /* volume and filter cutoff targets */
#define PSST    AWEREG(6, ALL, 0)   /* pan send and loop start address */
#define CSL     AWEREG(7, ALL, 0)   /* chorus send and loop end address */
#define CCCA    AWEREG(0, ALL, 1)   /* Q, control bits, current address */
#define ENVVOL  AWEREG(4, ALL, 1)   /* volume envelope delay */
#define DCYSUSV AWEREG(5, ALL, 1)   /* volume envelope sustain and decay */
#define ENVVAL  AWEREG(6, ALL, 1)   /* modulation envelope delay */
#define DCYSUS  AWEREG(7, ALL, 1)   /* modulation envelope sustain and decay */
#define ATKHLDV AWEREG(4, ALL, 2)   /* volume envelope hold and attack */
#define LFO1VAL AWEREG(5, ALL, 2)   /* LFO#1 delay */
#define ATKHLD  AWEREG(6, ALL, 2)   /* modulation envelope hold and attack */
#define LFO2VAL AWEREG(7, ALL, 2)   /* LFO#2 delay */
#define IP      AWEREG(0, ALL, 3)   /* initial pitch */
#define IFATN   AWEREG(1, ALL, 3)   /* initial filter cutoff and attenuation */
#define PEFE    AWEREG(2, ALL, 3)   /* pitch and filter envelope heights */
#define FMMOD   AWEREG(3, ALL, 3)   /* vibrato and filter modulation from LFO#1 */
#define TREMFRQ AWEREG(4, ALL, 3)   /* LFO#1 tremolo amount and frequency */
#define FM2FRQ2 AWEREG(5, ALL, 3)   /* LFO#2 vibrato amount and frequency */
#define PROBE   AWEREG(7, ALL, 3)   /* dunno... not documented! */

/*
 * Sound Blaster AWE32 global indirect registers
 */
#define HWCF4   AWEREG(1, 9, 1)     /* configuration doubleword 4 */
#define HWCF5   AWEREG(1, 10, 1)    /* configuration doubleword 5 */
#define HWCF6   AWEREG(1, 13, 1)    /* configuration doubleword 6 */
#define SMALR   AWEREG(1, 20, 1)    /* SM address for left SM reads */
#define SMARR   AWEREG(1, 21, 1)    /* SM address for right SM reads */
#define SMALW   AWEREG(1, 22, 1)    /* SM address for left SM writes */
#define SMARW   AWEREG(1, 23, 1)    /* SM address for right SM writes */
#define SMLD    AWEREG(1, 26, 1)    /* sound memory left data */
#define SMRD    AWEREG(1, 26, 2)    /* sound memory right data */
#define WC      AWEREG(1, 27, 2)    /* sample counter */
#define HWCF1   AWEREG(1, 29, 1)    /* configuration word 1 */
#define HWCF2   AWEREG(1, 30, 1)    /* configuration word 2 */
#define HWCF3   AWEREG(1, 31, 1)    /* configuration word 3 */
#define INIT1   AWEREG(2, ALL, 1)   /* initialization array 1 */
#define INIT2   AWEREG(2, ALL, 2)   /* initialization array 2 */
#define INIT3   AWEREG(3, ALL, 1)   /* initialization array 3 */
#define INIT4   AWEREG(3, ALL, 2)   /* initialization array 4 */


/*
 * Sound Blaster AWE32 Sound Memory Manager Defines
 */
#define NODE_HEADER_SIZE 4
#define CLICKBUFSIZE     8

/*
 * Sound Blaster AWE32 Chorus/Reverb/FilterQ default settings
 */
#define CHORUS          0 /*21*/
#define REVERB          0 /*58*/
#define FILTERQ         0


/*
 * Sound Blaster AWE32 Initialization Arrays
 */
static WORD aInitTableA[4*32] = {
    0x03FF, 0x0030, 0x07FF, 0x0130, 0x0BFF, 0x0230, 0x0FFF, 0x0330,
    0x13FF, 0x0430, 0x17FF, 0x0530, 0x1BFF, 0x0630, 0x1FFF, 0x0730,
    0x23FF, 0x0830, 0x27FF, 0x0930, 0x2BFF, 0x0A30, 0x2FFF, 0x0B30,
    0x33FF, 0x0C30, 0x37FF, 0x0D30, 0x3BFF, 0x0E30, 0x3FFF, 0x0F30,

    0x43FF, 0x0030, 0x47FF, 0x0130, 0x4BFF, 0x0230, 0x4FFF, 0x0330,
    0x53FF, 0x0430, 0x57FF, 0x0530, 0x5BFF, 0x0630, 0x5FFF, 0x0730,
    0x63FF, 0x0830, 0x67FF, 0x0930, 0x6BFF, 0x0A30, 0x6FFF, 0x0B30,
    0x73FF, 0x0C30, 0x77FF, 0x0D30, 0x7BFF, 0x0E30, 0x7FFF, 0x0F30,

    0x83FF, 0x0030, 0x87FF, 0x0130, 0x8BFF, 0x0230, 0x8FFF, 0x0330,
    0x93FF, 0x0430, 0x97FF, 0x0530, 0x9BFF, 0x0630, 0x9FFF, 0x0730,
    0xA3FF, 0x0830, 0xA7FF, 0x0930, 0xABFF, 0x0A30, 0xAFFF, 0x0B30,
    0xB3FF, 0x0C30, 0xB7FF, 0x0D30, 0xBBFF, 0x0E30, 0xBFFF, 0x0F30,

    0xC3FF, 0x0030, 0xC7FF, 0x0130, 0xCBFF, 0x0230, 0xCFFF, 0x0330,
    0xD3FF, 0x0430, 0xD7FF, 0x0530, 0xDBFF, 0x0630, 0xDFFF, 0x0730,
    0xE3FF, 0x0830, 0xE7FF, 0x0930, 0xEBFF, 0x0A30, 0xEFFF, 0x0B30,
    0xF3FF, 0x0C30, 0xF7FF, 0x0D30, 0xFBFF, 0x0E30, 0xFFFF, 0x0F30
};

static WORD aInitTableB[4*32] = {
    0x03FF, 0x8030, 0x07FF, 0x8130, 0x0BFF, 0x8230, 0x0FFF, 0x8330,
    0x13FF, 0x8430, 0x17FF, 0x8530, 0x1BFF, 0x8630, 0x1FFF, 0x8730,
    0x23FF, 0x8830, 0x27FF, 0x8930, 0x2BFF, 0x8A30, 0x2FFF, 0x8B30,
    0x33FF, 0x8C30, 0x37FF, 0x8D30, 0x3BFF, 0x8E30, 0x3FFF, 0x8F30,

    0x43FF, 0x8030, 0x47FF, 0x8130, 0x4BFF, 0x8230, 0x4FFF, 0x8330,
    0x53FF, 0x8430, 0x57FF, 0x8530, 0x5BFF, 0x8630, 0x5FFF, 0x8730,
    0x63FF, 0x8830, 0x67FF, 0x8930, 0x6BFF, 0x8A30, 0x6FFF, 0x8B30,
    0x73FF, 0x8C30, 0x77FF, 0x8D30, 0x7BFF, 0x8E30, 0x7FFF, 0x8F30,

    0x83FF, 0x8030, 0x87FF, 0x8130, 0x8BFF, 0x8230, 0x8FFF, 0x8330,
    0x93FF, 0x8430, 0x97FF, 0x8530, 0x9BFF, 0x8630, 0x9FFF, 0x8730,
    0xA3FF, 0x8830, 0xA7FF, 0x8930, 0xABFF, 0x8A30, 0xAFFF, 0x8B30,
    0xB3FF, 0x8C30, 0xB7FF, 0x8D30, 0xBBFF, 0x8E30, 0xBFFF, 0x8F30,

    0xC3FF, 0x8030, 0xC7FF, 0x8130, 0xCBFF, 0x8230, 0xCFFF, 0x8330,
    0xD3FF, 0x8430, 0xD7FF, 0x8530, 0xDBFF, 0x8630, 0xDFFF, 0x8730,
    0xE3FF, 0x8830, 0xE7FF, 0x8930, 0xEBFF, 0x8A30, 0xEFFF, 0x8B30,
    0xF3FF, 0x8C30, 0xF7FF, 0x8D30, 0xFBFF, 0x8E30, 0xFFFF, 0x8F30
};

static WORD aInitTableC[4*32] = {
    0x0C10, 0x8470, 0x14FE, 0xB488, 0x167F, 0xA470, 0x18E7, 0x84B5,
    0x1B6E, 0x842A, 0x1F1D, 0x852A, 0x0DA3, 0x8F7C, 0x167E, 0xF254,
    0x0000, 0x842A, 0x0001, 0x852A, 0x18E6, 0x8BAA, 0x1B6D, 0xF234,
    0x229F, 0x8429, 0x2746, 0x8529, 0x1F1C, 0x86E7, 0x229E, 0xF224,

    0x0DA4, 0x8429, 0x2C29, 0x8529, 0x2745, 0x87F6, 0x2C28, 0xF254,
    0x383B, 0x8428, 0x320F, 0x8528, 0x320E, 0x8F02, 0x1341, 0xF264,
    0x3EB6, 0x8428, 0x3EB9, 0x8528, 0x383A, 0x8FA9, 0x3EB5, 0xF294,
    0x3EB7, 0x8474, 0x3EBA, 0x8575, 0x3EB8, 0xC4C3, 0x3EBB, 0xC5C3,

    0x0000, 0xA404, 0x0001, 0xA504, 0x141F, 0x8671, 0x14FD, 0x8287,
    0x3EBC, 0xE610, 0x3EC8, 0x8C7B, 0x031A, 0x87E6, 0x3EC8, 0x86F7,
    0x3EC0, 0x821E, 0x3EBE, 0xD208, 0x3EBD, 0x821F, 0x3ECA, 0x8386,
    0x3EC1, 0x8C03, 0x3EC9, 0x831E, 0x3ECA, 0x8C4C, 0x3EBF, 0x8C55,

    0x3EC9, 0xC208, 0x3EC4, 0xBC84, 0x3EC8, 0x8EAD, 0x3EC8, 0xD308,
    0x3EC2, 0x8F7E, 0x3ECB, 0x8219, 0x3ECB, 0xD26E, 0x3EC5, 0x831F,
    0x3EC6, 0xC308, 0x3EC3, 0xB2FF, 0x3EC9, 0x8265, 0x3EC9, 0x8319,
    0x1342, 0xD36E, 0x3EC7, 0xB3FF, 0x0000, 0x8365, 0x1420, 0x9570
};

static WORD aInitTableD[4*32] = {
    0x0C10, 0x8470, 0x14FE, 0xB488, 0x167F, 0xA470, 0x18E7, 0x84B5,
    0x1B6E, 0x842A, 0x1F1D, 0x852A, 0x0DA3, 0x0F7C, 0x167E, 0x7254,
    0x0000, 0x842A, 0x0001, 0x852A, 0x18E6, 0x0BAA, 0x1B6D, 0x7234,
    0x229F, 0x8429, 0x2746, 0x8529, 0x1F1C, 0x06E7, 0x229E, 0x7224,

    0x0DA4, 0x8429, 0x2C29, 0x8529, 0x2745, 0x07F6, 0x2C28, 0x7254,
    0x383B, 0x8428, 0x320F, 0x8528, 0x320E, 0x0F02, 0x1341, 0x7264,
    0x3EB6, 0x8428, 0x3EB9, 0x8528, 0x383A, 0x0FA9, 0x3EB5, 0x7294,
    0x3EB7, 0x8474, 0x3EBA, 0x8575, 0x3EB8, 0x44C3, 0x3EBB, 0x45C3,

    0x0000, 0xA404, 0x0001, 0xA504, 0x141F, 0x0671, 0x14FD, 0x0287,
    0x3EBC, 0xE610, 0x3EC8, 0x0C7B, 0x031A, 0x07E6, 0x3EC8, 0x86F7,
    0x3EC0, 0x821E, 0x3EBE, 0xD208, 0x3EBD, 0x021F, 0x3ECA, 0x0386,
    0x3EC1, 0x0C03, 0x3EC9, 0x031E, 0x3ECA, 0x8C4C, 0x3EBF, 0x0C55,

    0x3EC9, 0xC208, 0x3EC4, 0xBC84, 0x3EC8, 0x0EAD, 0x3EC8, 0xD308,
    0x3EC2, 0x8F7E, 0x3ECB, 0x0219, 0x3ECB, 0xD26E, 0x3EC5, 0x031F,
    0x3EC6, 0xC308, 0x3EC3, 0x32FF, 0x3EC9, 0x0265, 0x3EC9, 0x8319,
    0x1342, 0xD36E, 0x3EC7, 0x33FF, 0x0000, 0x8365, 0x1420, 0x9570
};

/*
 * Sound Blaster AWE32 Linear Volume Table
 */
static BYTE aVolumeTable[65] = {
    0xFF,
    0x3E, 0x3D, 0x3C, 0x3B, 0x3A, 0x39, 0x38, 0x37, 
    0x36, 0x35, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 
    0x2F, 0x2E, 0x2D, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 
    0x29, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24, 0x23, 
    0x23, 0x22, 0x21, 0x21, 0x20, 0x1F, 0x1F, 0x1E, 
    0x1D, 0x1D, 0x1C, 0x1B, 0x1B, 0x1A, 0x19, 0x19, 
    0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 
    0x13, 0x13, 0x12, 0x12, 0x11, 0x11, 0x10, 0x10
};


/*
 * Sound Blaster AWE32 configuration state
 */
static struct {
    WORD    wPort;
    BYTE    nIrqLine;
    BYTE    nDmaChannel;
    WORD    wBasePort;
    WORD    wPointerPort;
    WORD    wDataPort[4];
    DWORD   dwTopOfMemory;
    UINT    nVoices;
    DWORD   aCurrentAddress[32];
    DWORD   aStartAddress[32];
    DWORD   aLoopStart[32];
    DWORD   aLoopEnd[32];
    DWORD   aFrequencyTable[32];
    BYTE    aVolumeTable[32];
    BYTE    aPanningTable[32];
    LPFNAUDIOTIMER lpfnAudioTimer;
} AWE32;

static volatile BYTE nEmuIndex;


/*
 * Low-level Sound Blaster AWE32 programming routines
 */
static VOID AWEPortW(WORD wIndex, WORD wData)
{
    nEmuIndex = LOBYTE(wIndex);
    /* get oscillator number (0-31) and register number (0-7) */
    OUTW(AWE32.wPointerPort, LOBYTE(wIndex));
    /* write the word data into the specified I/O data port (0-3) */
    OUTW(AWE32.wDataPort[HIBYTE(wIndex)], wData);
}

static WORD AWEPortRW(WORD wIndex)
{
    nEmuIndex = LOBYTE(wIndex);
    OUTW(AWE32.wPointerPort, LOBYTE(wIndex));
    return INW(AWE32.wDataPort[HIBYTE(wIndex)]);
}

static VOID AWEPortDW(WORD wIndex, DWORD dwData)
{
    nEmuIndex = LOBYTE(wIndex);
    /* get oscillator number (0-31) and register number (0-7) */
    OUTW(AWE32.wPointerPort, LOBYTE(wIndex));
    /* write the doubleword data into the specified I/O port (0-1) */
    OUTW(AWE32.wDataPort[HIBYTE(wIndex)], LOWORD(dwData));
    OUTW(AWE32.wDataPort[HIBYTE(wIndex)] + 2, HIWORD(dwData));
}

static DWORD AWEPortRDW(WORD wIndex)
{
    DWORD dwData;
    nEmuIndex = LOBYTE(wIndex);
    OUTW(AWE32.wPointerPort, LOBYTE(wIndex));
    dwData = (DWORD) INW(AWE32.wDataPort[HIBYTE(wIndex)]);
    dwData |= ((DWORD) INW(AWE32.wDataPort[HIBYTE(wIndex)] + 2)) << 16;
    return dwData;
}

static VOID AWEWait(UINT nTicks)
{
    UINT nTarget, nTimeOut;
    nTarget = (WORD) (AWEPortRW(WC) + nTicks);
    for (nTimeOut = 0; nTimeOut < 0xC000; nTimeOut++)
        if (AWEPortRW(WC) == nTarget) break;
}

static BOOL AWEProbe(WORD wBasePort)
{
    /* set up AWE32 base I/O port addresses */
    if (wBasePort > 0x300) wBasePort -= AWE32_DATA0;
    AWE32.wDataPort[0] = wBasePort + AWE32_DATA0;
    AWE32.wDataPort[1] = wBasePort + AWE32_DATA1;
    AWE32.wDataPort[2] = wBasePort + AWE32_DATA2;
    AWE32.wDataPort[3] = wBasePort + AWE32_DATA3;
    AWE32.wPointerPort = wBasePort + AWE32_POINTER;

    /* probe if a EMU8000 chip is there */
    AWEPortRW(PROBE);
    return ((AWEPortRW(PROBE) & 0x000f) == 0x000c &&
            (AWEPortRW(HWCF1) & 0x007e) == 0x0058 &&
            (AWEPortRW(HWCF2) & 0x0003) == 0x0003);
}

static BOOL AWEInitialize(VOID)
{
    UINT n;

    /* check if we have an EMU8000 chip */
    if (!AWEProbe(AWE32.wBasePort))
        return TRUE;

    /* reset the hardware configuration */
    AWEPortW(HWCF1, 0x0059);
    AWEPortW(HWCF2, 0x0020);

    /* initialize all the channels */
    for (n = 0; n < 32; n++) {
        /* turn off envelope engine */
        AWEPortW(DCYSUSV + n, 0x0080);
    }
    for (n = 0; n < 32; n++) {
        /* initialize all envelope and sound engine registers */
        AWEPortW(ENVVOL  + n, 0x0000);
        AWEPortW(ENVVAL  + n, 0x0000);
        AWEPortW(DCYSUS  + n, 0x0000);
        AWEPortW(ATKHLDV + n, 0x0000);
        AWEPortW(LFO1VAL + n, 0x0000);
        AWEPortW(ATKHLD  + n, 0x0000);
        AWEPortW(LFO2VAL + n, 0x0000);
        AWEPortW(IP      + n, 0x0000);
        AWEPortW(IFATN   + n, 0x0000);
        AWEPortW(PEFE    + n, 0x0000);
        AWEPortW(FMMOD   + n, 0x0000);
        AWEPortW(TREMFRQ + n, 0x0000);
        AWEPortW(FM2FRQ2 + n, 0x0000);
        AWEPortDW(PTRX   + n, 0x00000000L);
        AWEPortDW(VTFT   + n, 0x00000000L);
        AWEPortDW(PSST   + n, 0x00000000L);
        AWEPortDW(CSL    + n, 0x00000000L);
        AWEPortDW(CCCA   + n, 0x00000000L);
    }
    for (n = 0; n < 32; n++) {
        /* now initialize the "current" registers */
        AWEPortDW(CPF    + n, 0x00000000L);
        AWEPortDW(CVCF   + n, 0x00000000L);
    }

    /* initialize the sound memory DMA registers */
    AWEPortW(SMALR, 0x0000);
    AWEPortW(SMARR, 0x0000);
    AWEPortW(SMALW, 0x0000);
    AWEPortW(SMARW, 0x0000);

    /* set up the initialization arrays */

    /* 1. copy the first set of initialization arrays */
    for (n = 0; n < 32; n++)
        AWEPortW(INIT1 + n, aInitTableA[0x00 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT2 + n, aInitTableA[0x20 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT3 + n, aInitTableA[0x40 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT4 + n, aInitTableA[0x60 + n]);

    /* 2. wait 1024 sample periods (24 msec) */
    AWEWait(1024);

    /* 3. copy second set of initialization arrays */
    for (n = 0; n < 32; n++)
        AWEPortW(INIT1 + n, aInitTableB[0x00 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT2 + n, aInitTableB[0x20 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT3 + n, aInitTableB[0x40 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT4 + n, aInitTableB[0x60 + n]);

    /* 4. copy third set of initialization arrays */
    for (n = 0; n < 32; n++)
        AWEPortW(INIT1 + n, aInitTableC[0x00 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT2 + n, aInitTableC[0x20 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT3 + n, aInitTableC[0x40 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT4 + n, aInitTableC[0x60 + n]);

    /* 5. set the harware configuration registers */
    AWEPortDW(HWCF4, 0x00000000L);
    AWEPortDW(HWCF5, 0x00000083L);
    AWEPortDW(HWCF6, 0x00008000L);

    /* 6. copy fourth set of initialization arrays */
    for (n = 0; n < 32; n++)
        AWEPortW(INIT1 + n, aInitTableD[0x00 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT2 + n, aInitTableD[0x20 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT3 + n, aInitTableD[0x40 + n]);
    for (n = 0; n < 32; n++)
        AWEPortW(INIT4 + n, aInitTableD[0x60 + n]);

    /* enable audio output */
    AWEPortW(HWCF3, 0x0004);

    return FALSE;
}

static DWORD AWEDetectMemory(VOID)
{
    DWORD dwAddr;
    UINT nChannel, nTimeOut;

    /* allocate 15/15 channels in left read/write DMA mode */
    for (nChannel = 0; nChannel < 30; nChannel++) {
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT + nChannel, 0x00000000L);
        AWEPortDW(CVCF + nChannel, 0x00000000L);
        AWEPortDW(PTRX + nChannel, 0x40000000L);
        AWEPortDW(CPF  + nChannel, 0x40000000L);
        AWEPortDW(PSST + nChannel, 0x00000000L);
        AWEPortDW(CSL  + nChannel, 0x00000000L);
        /* select left read/write and enter in DMA mode */
        AWEPortDW(CCCA + nChannel, nChannel & 1 ? 0x06000000L : 0x04000000L);
    }

    /* wait until the empty and full bits of the DMA streams are clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALR) & 0x80000000L) &&
            !(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* write pattern at start of sound memory address space */
    AWEPortDW(SMALW, 0x00200000L);
    AWEPortW(SMLD, 0xAAAA);
    AWEPortW(SMLD, 0x5555);

    /* read it back to check if we have sound memory */
    AWEPortDW(SMALR, 0x00200000L);
    AWEPortRW(SMLD);
    if (AWEPortRW(SMLD) == 0xAAAA &&
        AWEPortRW(SMLD) == 0x5555) {

        /* check how much sound memory we have (up to 28MB) */
        for (dwAddr = 0x00220000L; dwAddr < 0xFE0000L; dwAddr += 0x10000L) {

            /* write pattern at sound memory step location */
            AWEPortDW(SMALW, dwAddr);
            AWEPortW(SMLD, 0x1234);
            AWEPortW(SMLD, 0x5678);

            /* check first sound memory location for mirrorring */
            AWEPortDW(SMALR, 0x00200000L);
            AWEPortRW(SMLD);
            if (AWEPortRW(SMLD) != 0xAAAA) break;

            /* now, check if the pattern was actually written */
            AWEPortDW(SMALR, dwAddr);
            AWEPortDW(SMALR, dwAddr);
            AWEPortRW(SMLD);
            if (AWEPortRW(SMLD) != 0x1234 ||
                AWEPortRW(SMLD) != 0x5678) break;
        }
        dwAddr -= 0x00200000L;
    }
    else {
        /* we do not have sound memory */
        dwAddr = 0L;
    }

    /* wait until the empty and full bits of the DMA streams are clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALR) & 0x80000000L) &&
            !(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* deallocate 30 channels */
    for (nChannel = 0; nChannel < 30; nChannel++) {
        AWEPortDW(CCCA + nChannel, 0x00000000L);
        AWEPortW(DCYSUSV + nChannel, 0x807F);
    }

    return dwAddr;
}

static VOID AWEUploadData(BYTE nFirstChannel, DWORD dwAddress,
			  LPWORD lpData, UINT nCount)
{
    UINT nTimeOut, nChannel;

    /* allocate channels in left write DMA mode */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT + nChannel, 0x00000000L);
        AWEPortDW(CVCF + nChannel, 0x00000000L);
        AWEPortDW(PTRX + nChannel, 0x40000000L);
        AWEPortDW(CPF  + nChannel, 0x40000000L);
        AWEPortDW(PSST + nChannel, 0x00000000L);
        AWEPortDW(CSL  + nChannel, 0x00000000L);
        AWEPortDW(CCCA + nChannel, 0x06000000L);
    }

    /* wait until the full bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* set start address for writting data */
    AWEPortDW(SMALW, dwAddress);

    /* upload 16-bit signed words to sound memory */
    while (nCount--) {
        AWEPortW(SMLD, *lpData++);
    }

    /* wait until the full bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* deallocate channels */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortDW(CCCA + nChannel, 0x00000000L);
        AWEPortW(DCYSUSV + nChannel, 0x807F);
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT   + nChannel, 0x00000000L);
        AWEPortDW(CVCF   + nChannel, 0x00000000L);
        AWEPortDW(PTRX   + nChannel, 0x00000000L);
        AWEPortDW(CPF    + nChannel, 0x00000000L);
    }
}

static VOID AWEUploadData8(BYTE nFirstChannel, DWORD dwAddress,
			   LPBYTE lpData, UINT nCount)
{
    UINT nTimeOut, nChannel;

    /* allocate channels in left write DMA mode */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT + nChannel, 0x00000000L);
        AWEPortDW(CVCF + nChannel, 0x00000000L);
        AWEPortDW(PTRX + nChannel, 0x40000000L);
        AWEPortDW(CPF  + nChannel, 0x40000000L);
        AWEPortDW(PSST + nChannel, 0x00000000L);
        AWEPortDW(CSL  + nChannel, 0x00000000L);
        AWEPortDW(CCCA + nChannel, 0x06000000L);
    }

    /* wait until the full bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* set start address for writting data */
    AWEPortDW(SMALW, dwAddress);

    /* upload 8-bit signed bytes to sound memory */
    while (nCount--) {
        AWEPortW(SMLD, *lpData++ << 8);
    }

    /* wait until the full bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALW) & 0x80000000L)) break;

    /* deallocate channel */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortDW(CCCA + nChannel, 0x00000000L);
        AWEPortW(DCYSUSV + nChannel, 0x807F);
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT   + nChannel, 0x00000000L);
        AWEPortDW(CVCF   + nChannel, 0x00000000L);
        AWEPortDW(PTRX   + nChannel, 0x00000000L);
        AWEPortDW(CPF    + nChannel, 0x00000000L);
    }
}

static VOID AWEDownloadData(BYTE nFirstChannel, DWORD dwAddress,
			    LPWORD lpData, UINT nCount)
{
    UINT nTimeOut, nChannel;

    /* allocate channels in left read DMA mode */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT + nChannel, 0x00000000L);
        AWEPortDW(CVCF + nChannel, 0x00000000L);
        AWEPortDW(PTRX + nChannel, 0x40000000L);
        AWEPortDW(CPF  + nChannel, 0x40000000L);
        AWEPortDW(PSST + nChannel, 0x00000000L);
        AWEPortDW(CSL  + nChannel, 0x00000000L);
        AWEPortDW(CCCA + nChannel, 0x04000000L);
    }

    /* wait until the empty bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALR) & 0x80000000L)) break;

    /* set start address for reading data */
    AWEPortDW(SMALR, dwAddress);
    AWEPortRW(SMLD);

    /* download 16-bit signed words from sound memory */
    while (nCount--) {
        *lpData++ = AWEPortRW(SMLD);
    }

    /* wait until the empty bit of the DMA stream is clear */
    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(AWEPortRDW(SMALR) & 0x80000000L)) break;

    /* deallocate channel */
    for (nChannel = nFirstChannel; nChannel < 32; nChannel++) {
        AWEPortDW(CCCA + nChannel, 0x00000000L);
        AWEPortW(DCYSUSV + nChannel, 0x807F);
        AWEPortW(DCYSUSV + nChannel, 0x0080);
        AWEPortDW(VTFT   + nChannel, 0x00000000L);
        AWEPortDW(CVCF   + nChannel, 0x00000000L);
        AWEPortDW(PTRX   + nChannel, 0x00000000L);
        AWEPortDW(CPF    + nChannel, 0x00000000L);
    }
}

static VOID AWEPrimeNote(BYTE nChannel, WORD wPitch,
			 BYTE nVolume, BYTE nPanning, BYTE nChorusSend, BYTE nFilterQ,
			 DWORD dwStartAddress, DWORD dwLoopStart, DWORD dwLoopEnd)
{
    /* first the channel must be silent and idle */
    AWEPortW(DCYSUSV + nChannel, 0x0080);
    AWEPortDW(VTFT   + nChannel, 0x00000000L);
    AWEPortDW(CVCF   + nChannel, 0x00000000L);
    AWEPortDW(PTRX   + nChannel, 0x00000000L);
    AWEPortDW(CPF    + nChannel, 0x00000000L);

    /* now set the envelope engine parameters */
    AWEPortW(ENVVOL  + nChannel, 0x8000);
    AWEPortW(ENVVAL  + nChannel, 0x8000);
    AWEPortW(DCYSUS  + nChannel, 0x7F7F);
    AWEPortW(ATKHLDV + nChannel, 0x7F7F);
    AWEPortW(LFO1VAL + nChannel, 0x8000);
    AWEPortW(ATKHLD  + nChannel, 0x7F7F);
    AWEPortW(LFO2VAL + nChannel, 0x8000);
    AWEPortW(IP      + nChannel, wPitch);
    AWEPortW(IFATN   + nChannel, 0xFF00 | nVolume);
    AWEPortW(PEFE    + nChannel, 0x0000);
    AWEPortW(FMMOD   + nChannel, 0x0000);
    AWEPortW(TREMFRQ + nChannel, 0x0010);
    AWEPortW(FM2FRQ2 + nChannel, 0x0010);

    /* decrease memory addresses (due to interpolator offset) */
    dwStartAddress--;
    dwLoopStart--;
    dwLoopEnd--;

    /* set the loop start and loop end addresses */
    AWEPortDW(PSST   + nChannel, ((DWORD) nPanning << 24) | dwLoopStart);
    AWEPortDW(CSL    + nChannel, ((DWORD) nChorusSend << 24) | dwLoopEnd);

    /* set filter Q of channel and audio start address */
    AWEPortDW(CCCA   + nChannel, ((DWORD) nFilterQ << 28) | dwStartAddress);
}

static VOID AWEStartNote(BYTE nChannel)
{
    /* start the note (do it all as close to simultaneously as possible) */
    AWEPortDW(VTFT + nChannel, 0x0000FFFFL);
    AWEPortDW(CVCF + nChannel, 0x0000FFFFL);
    AWEPortW(DCYSUSV + nChannel, 0x7F7F);
    AWEPortDW(PTRX + nChannel, 0x40000000L | (REVERB << 8));
    AWEPortDW(CPF + nChannel, 0x40000000L);
}

static VOID AWEStopNote(BYTE nChannel)
{
    /* turn off the envolope engine and take the volume to zero */
    AWEPortW(DCYSUSV + nChannel, 0x0080);
    AWEPortDW(VTFT + nChannel, 0x0000FFFFL);
    AWEPortDW(CVCF + nChannel, 0x0000FFFFL);
}

static VOID AWEReleaseNote(BYTE nChannel, BYTE nReleaseRate)
{
    /* enter the envelope in release mode */
    AWEPortW(DCYSUSV + nChannel, 0x8000 | nReleaseRate);
}

static VOID AWESetNoteOffset(BYTE nChannel, DWORD dwStartAddress, BYTE nFilterQ)
{
    /* set filter Q of channel and audio start address */
    AWEPortDW(CCCA + nChannel, ((DWORD) nFilterQ << 28) | dwStartAddress);
}

static DWORD AWEGetNoteOffset(BYTE nChannel)
{
    DWORD dwCCCA;
    dwCCCA = AWEPortRDW(CCCA + nChannel);
    return (dwCCCA & 0x00FFFFFFL);
}

static VOID AWESetNotePitch(BYTE nChannel, WORD wPitch)
{
    /* change the initial pitch register on the fly */
    AWEPortW(IP + nChannel, wPitch);
}

static VOID AWESetNoteVolume(BYTE nChannel, BYTE nVolume)
{
    /* change the volume bit field of IFATN and set filter cutoff to 8 kHz */
    AWEPortW(IFATN + nChannel, 0xFF00 | nVolume);
}

static VOID AWESetNotePanning(BYTE nChannel, BYTE nPanning, DWORD dwLoopStart)
{
    /* change the voice panning position and loop start address */
    AWEPortDW(PSST + nChannel, ((DWORD) nPanning << 24) | dwLoopStart);
}

static WORD AWEGetPitchShift(DWORD dwFrequency)
{
    UINT n, nPitch;

    /* convert frequency to logarithmic pitch shift */

    if (dwFrequency <= (44100 >> 14))
        return 0x0000;
    if (dwFrequency >= (44100 << 2))
        return 0xFFFF;

    dwFrequency <<= 14;
    dwFrequency /= 44100;

    nPitch = 0x0000;
    for (n = 16; n--; ) {
        if (dwFrequency >= (1 << n)) {
            nPitch = n << 12;
            dwFrequency <<= 14;
            dwFrequency >>= n;
            break;
        }
    }
    for (n = 12; n--; ) {
        dwFrequency = (dwFrequency * dwFrequency) >> 14;
        if (dwFrequency >= (2 << 14)) {
            nPitch += 1 << n;
            dwFrequency >>= 1;
        }
    }
    return (WORD) nPitch;
}


/*
 * Sound Blaster AWE32 Sound Memory Manager
 */
static VOID AWEPokeDW(DWORD dwAddress, DWORD dwValue)
{
    AWEUploadData(AWE32.nVoices, dwAddress, (LPWORD) &dwValue, 2);
}

static DWORD AWEPeekDW(DWORD dwAddress)
{
    DWORD dwValue;
    AWEDownloadData(AWE32.nVoices, dwAddress, (LPWORD) &dwValue, 2);
    return dwValue;
}

static BOOL AWEInitMemory(VOID)
{
    DWORD dwMemorySize;

    /* get amount of AWE32 memory size (16-bit words) */
    if ((dwMemorySize = AWEDetectMemory()) <= 2*NODE_HEADER_SIZE)
        return TRUE;

    /* set up heap memory blocks */
    AWE32.dwTopOfMemory = 0x200000L + dwMemorySize;
    AWEPokeDW(0x200000L + 0, 0x200000L + NODE_HEADER_SIZE);
    AWEPokeDW(0x200000L + 2, NODE_HEADER_SIZE);
    AWEPokeDW(0x200000L + NODE_HEADER_SIZE + 0, 0x000000L);
    AWEPokeDW(0x200000L + NODE_HEADER_SIZE + 2, dwMemorySize - NODE_HEADER_SIZE);

    return FALSE;
}

static DWORD AWEMemAvail(VOID)
{
    DWORD dwNode, dwPrevNode, dwSize;

    dwSize = 0;
    dwPrevNode = 0x200000L;
    while ((dwNode = AWEPeekDW(dwPrevNode + 0)) != 0x000000L) {
        dwSize += AWEPeekDW(dwNode + 2);
        dwPrevNode = dwNode;
    }

    /* return number of 8-bit bytes availables */
    return dwSize << 1;
}

static DWORD AWEMemAlloc(DWORD dwSize)
{
    DWORD dwNode, dwPrevNode, dwNodeSize;

    if ((dwSize += NODE_HEADER_SIZE) != 0L) {
        dwSize = (dwSize + NODE_HEADER_SIZE - 1) & -NODE_HEADER_SIZE;
        dwPrevNode = 0x200000L;
        while ((dwNode = AWEPeekDW(dwPrevNode + 0)) != 0L) {
            if ((dwNodeSize = AWEPeekDW(dwNode + 2)) >= dwSize) {
                if (dwNodeSize == dwSize) {
                    AWEPokeDW(dwPrevNode + 0, AWEPeekDW(dwNode + 0));
                }
                else {
                    dwNodeSize -= dwSize;
                    AWEPokeDW(dwNode + 2, dwNodeSize);
                    dwNode += dwNodeSize;
                    AWEPokeDW(dwNode + 2, dwSize);
                }
                AWEPokeDW(dwNode + 0, ~dwSize);
                return (dwNode + NODE_HEADER_SIZE);
            }
            dwPrevNode = dwNode;
        }
    }
    return 0L;
}

static VOID AWEMemFree(DWORD dwAddr)
{
    DWORD dwNode, dwNextNode, dwNodeSize;

    if ((dwAddr -= NODE_HEADER_SIZE) < AWE32.dwTopOfMemory &&
        AWEPeekDW(dwAddr + 0) == ~AWEPeekDW(dwAddr + 2)) {
        dwNode = 0x200000L;
        while (dwNode != AWE32.dwTopOfMemory) {
            if ((dwNextNode = AWEPeekDW(dwNode + 0)) == 0x000000L)
                dwNextNode = AWE32.dwTopOfMemory;
            if (dwAddr > dwNode && dwAddr < dwNextNode)
                break;
            dwNode = dwNextNode;
        }
        if (dwNode != AWE32.dwTopOfMemory) {
            if (dwNextNode == AWE32.dwTopOfMemory)
                dwNextNode = 0x00000L;
            dwNodeSize = AWEPeekDW(dwAddr + 2);
            if (dwAddr + dwNodeSize == dwNextNode) {
                AWEPokeDW(dwAddr + 2, dwNodeSize + AWEPeekDW(dwNextNode + 2));
                AWEPokeDW(dwAddr + 0, AWEPeekDW(dwNextNode + 0));
            }
            else {
                AWEPokeDW(dwAddr + 0, dwNextNode);
            }
            dwNodeSize = AWEPeekDW(dwNode + 2);
            if (dwNode + dwNodeSize == dwAddr) {
                AWEPokeDW(dwNode + 2, dwNodeSize + AWEPeekDW(dwAddr + 2));
                AWEPokeDW(dwNode + 0, AWEPeekDW(dwAddr + 0));
            }
            else {
                AWEPokeDW(dwNode + 0, dwAddr);
            }
        }
    }
}


/*
 * Sound Blaster AWE32 DSP-based timer routines
 */
static VOID SB16PortB(BYTE bData)
{
    UINT nTimeOut;

    for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
        if (!(INB(AWE32.wPort + 0x0C) & 0x80))
            break;
    OUTB(AWE32.wPort + 0x0C, bData);
}

static UINT SB16Reset(VOID)
{
    UINT n, nTimeOut;

    for (n = 0; n < 8; n++) {
        /* first reset the DSP processor */
        OUTB(AWE32.wPort + 0x06, 0x01);
        for (nTimeOut = 0; nTimeOut < 8; nTimeOut++)
            INB(AWE32.wPort + 0x06);
        OUTB(AWE32.wPort + 0x06, 0x00);

        /* now wait to get back the DSP acknowledge */
        for (nTimeOut = 0; nTimeOut < 0x8000; nTimeOut++)
            if (INB(AWE32.wPort + 0x0E) & 0x80)
                break;
        if (INB(AWE32.wPort + 0x0A) == 0xAA)
            return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NODEVICE;
}

static VOID AWEInterruptHandler(VOID)
{
    BYTE nSaveIndex;

    if (AWE32.lpfnAudioTimer != NULL) {
        nSaveIndex = nEmuIndex;
        AWE32.lpfnAudioTimer();
        nEmuIndex = nSaveIndex;
        OUTW(AWE32.wPointerPort, nEmuIndex);
    }
    INB(AWE32.wPort + 0x0E);
}

static UINT AWEInitTimer(VOID)
{
    LPBYTE lpBuffer;

    /* reset the DSP processor */
    if (SB16Reset())
        return AUDIO_ERROR_NODEVICE;

    /* turn off the DSP speaker */
    SB16PortB(0xD3);

    /* set output sample rate to 44100 Hz */
    SB16PortB(0x41);
    SB16PortB(HIBYTE(44100));
    SB16PortB(LOBYTE(44100));

    /* allocate and clear small DMA buffer */
    if (DosAllocChannel(AWE32.nDmaChannel, 4))
        return AUDIO_ERROR_NOMEMORY;

    if ((lpBuffer = DosLockChannel(AWE32.nDmaChannel)) != NULL) {
        memset(lpBuffer, 0x80, 4);
        DosUnlockChannel(AWE32.nDmaChannel);
    }

    /* setup the DMA channel parameters */
    DosSetupChannel(AWE32.nDmaChannel, DMA_WRITE | DMA_AUTOINIT, 0);

    /* setup our IRQ interrupt handler */
    DosSetVectorHandler(AWE32.nIrqLine, AWEInterruptHandler);

    return AUDIO_ERROR_NONE;
}

static VOID AWEDoneTimer(VOID)
{
    /* reset the DSP processor */
    SB16Reset();

    /* turn off DSP speaker */
    SB16PortB(0xD3);

    /* restore the interrupt handler */
    DosSetVectorHandler(AWE32.nIrqLine, NULL);

    /* reset and release DMA buffer */
    DosDisableChannel(AWE32.nDmaChannel);
    DosFreeChannel(AWE32.nDmaChannel);
}

static VOID AWESetTimerRate(UINT nTicks)
{
    /* start 8-bit mono high-speed autoinit DMA transfer */
    SB16PortB(0xC6);
    SB16PortB(0x00);
    nTicks--;
    SB16PortB(LOBYTE(nTicks));
    SB16PortB(HIBYTE(nTicks));
}


/*
 * Sound Blaster AWE32 driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps = {
        AUDIO_PRODUCT_AWE32, "Sound Blaster AWE32",
        AUDIO_FORMAT_4S16,
    };
    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    LPSTR lpszBlaster;
    UINT n;

    if ((lpszBlaster = DosGetEnvironment("BLASTER")) != NULL) {
        AWE32.wPort = 0x220;
        AWE32.nIrqLine = 5;
        AWE32.nDmaChannel = 1;
        AWE32.wBasePort = 0x620;
        n = DosParseString(lpszBlaster, TOKEN_CHAR);
        while (n != 0) {
            switch (n) {
            case 'A':
            case 'a':
                AWE32.wPort = DosParseString(NULL, TOKEN_HEX);
                break;
            case 'I':
            case 'i':
                AWE32.nIrqLine = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'D':
            case 'd':
                AWE32.nDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'E':
            case 'e':
                AWE32.wBasePort = DosParseString(NULL, TOKEN_HEX);
                break;
            }
            n = DosParseString(NULL, TOKEN_CHAR);
        }
        if (AWEProbe(AWE32.wBasePort) && AWEDetectMemory())
            return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    /* initialize the AWE32 driver for playback */
    memset(&AWE32, 0, sizeof(AWE32));
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;
    if (AWEInitialize())
        return AUDIO_ERROR_NODEVICE;
    if (AWEInitMemory())
        return AUDIO_ERROR_NODRAMMEMORY;
    if (AWEInitTimer())
        return AUDIO_ERROR_NODEVICE;

    /* set caller playback format fields */
    lpInfo->wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    lpInfo->nSampleRate = 44100;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    UINT nVoice;

    /* reset and release AWE32 timer resources */
    AWEDoneTimer();

    /* reset EMU8000 synthesizer and exit */
    for (nVoice = 0; nVoice < 31; nVoice++)
        AWEStopNote(nVoice);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioMixerValue(UINT nChannel, UINT nValue)
{
    if (nChannel != AUDIO_MIXER_MASTER_VOLUME &&
        nChannel != AUDIO_MIXER_TREBLE &&
        nChannel != AUDIO_MIXER_BASS &&
        nChannel != AUDIO_MIXER_CHORUS &&
        nChannel != AUDIO_MIXER_REVERB)
        return AUDIO_ERROR_NOTSUPPORTED;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    if (nVoices < AUDIO_MAX_VOICES && nVoices < 31) {
        AWE32.nVoices = nVoices;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    UINT nVoice;
    for (AWE32.nVoices = nVoice = 0; nVoice < 31; nVoice++)
        AWEStopNote(nVoice);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    AWE32.lpfnAudioTimer = lpfnAudioTimer;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nBPM)
{
    UINT nTicks;

    if (nBPM >= 0x20 && nBPM <= 0xFF) {
        nTicks = (44100 * 5) / (2 * nBPM);
        AWESetTimerRate(nTicks);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static LONG AIAPI GetAudioDataAvail(VOID)
{
    return AWEMemAvail();
}

static UINT AIAPI CreateAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL) {
        if (lpWave->wFormat & AUDIO_FORMAT_16BITS)
            lpWave->dwHandle = AWEMemAlloc((lpWave->dwLength >> 1) + CLICKBUFSIZE);
        else
            lpWave->dwHandle = AWEMemAlloc(lpWave->dwLength + CLICKBUFSIZE);
        return lpWave->dwHandle ? AUDIO_ERROR_NONE : AUDIO_ERROR_NODRAMMEMORY;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI DestroyAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL && lpWave->dwHandle != 0) {
        AWEMemFree(lpWave->dwHandle);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI WriteAudioData(LPAUDIOWAVE lpWave, 
				 DWORD dwOffset, UINT nCount)
{
    static WORD aClickBuffer[CLICKBUFSIZE];

    if (lpWave != NULL && lpWave->dwHandle != 0) {
        if (dwOffset + nCount <= lpWave->dwLength) {
            if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
                AWEUploadData(AWE32.nVoices, lpWave->dwHandle + (dwOffset >> 1),
			      (LPWORD) (lpWave->lpData + dwOffset), nCount >> 1);
            }
            else {
                AWEUploadData8(AWE32.nVoices, lpWave->dwHandle + dwOffset,
			       lpWave->lpData + dwOffset, nCount);
            }

            if (lpWave->wFormat & (AUDIO_FORMAT_LOOP | AUDIO_FORMAT_BIDILOOP)) {
                if (dwOffset <= lpWave->dwLoopEnd &&
                    dwOffset + nCount >= lpWave->dwLoopEnd) {
                    if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
                        AWEDownloadData(AWE32.nVoices,
					lpWave->dwHandle + (lpWave->dwLoopStart >> 1),
					aClickBuffer, CLICKBUFSIZE);
                        AWEUploadData(AWE32.nVoices,
				      lpWave->dwHandle + (lpWave->dwLoopEnd >> 1),
				      aClickBuffer, CLICKBUFSIZE);
                    }
                    else {
                        AWEDownloadData(AWE32.nVoices,
					lpWave->dwHandle + lpWave->dwLoopStart,
					aClickBuffer, CLICKBUFSIZE);
                        AWEUploadData(AWE32.nVoices,
				      lpWave->dwHandle + lpWave->dwLoopEnd,
				      aClickBuffer, CLICKBUFSIZE);
                    }
                }
            }
            else if (dwOffset + nCount >= lpWave->dwLength) {
                memset(aClickBuffer, 0, sizeof(aClickBuffer));
                if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
                    AWEUploadData(AWE32.nVoices,
				  lpWave->dwHandle + (lpWave->dwLength >> 1),
				  aClickBuffer, CLICKBUFSIZE);
                }
                else {
                    AWEUploadData(AWE32.nVoices,
				  lpWave->dwHandle + lpWave->dwLength,
				  aClickBuffer, CLICKBUFSIZE);
                }
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, LPAUDIOWAVE lpWave)
{
    DWORD dwLength, dwLoopStart, dwLoopEnd;

    if (nVoice < 31 && lpWave != NULL && lpWave->dwHandle != 0) {
        AWE32.aStartAddress[nVoice] =
	    AWE32.aCurrentAddress[nVoice] = lpWave->dwHandle;
        dwLength = lpWave->dwLength;
        dwLoopStart = lpWave->dwLoopStart;
        dwLoopEnd = lpWave->dwLoopEnd;
        if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
            dwLength >>= 1;
            dwLoopStart >>= 1;
            dwLoopEnd >>= 1;
        }
        if (lpWave->wFormat & (AUDIO_FORMAT_LOOP | AUDIO_FORMAT_BIDILOOP)) {
            AWE32.aLoopStart[nVoice] = dwLoopStart;
            AWE32.aLoopEnd[nVoice] = dwLoopEnd;
        }
        else {
            AWE32.aLoopStart[nVoice] = dwLength + 2;
            AWE32.aLoopEnd[nVoice] = dwLength + CLICKBUFSIZE - 2;
        }

        AWEPrimeNote(nVoice, 
		     AWEGetPitchShift(AWE32.aFrequencyTable[nVoice]),
		     aVolumeTable[AWE32.aVolumeTable[nVoice]],
		     0xFF - AWE32.aPanningTable[nVoice],
		     CHORUS, FILTERQ,
		     AWE32.aStartAddress[nVoice],
		     AWE32.aStartAddress[nVoice] + AWE32.aLoopStart[nVoice],
		     AWE32.aStartAddress[nVoice] + AWE32.aLoopEnd[nVoice]);

        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    if (nVoice < 31) {
        if (AWE32.aCurrentAddress[nVoice]) {
            AWESetNoteOffset(nVoice, AWE32.aCurrentAddress[nVoice], FILTERQ);
            AWEStartNote(nVoice);
            AWE32.aCurrentAddress[nVoice] = 0;
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < 31) {
        /* release the note at 240 usec/dB (faster rate) */
        if (!AWE32.aCurrentAddress[nVoice]) {
            AWE32.aCurrentAddress[nVoice] = AWEGetNoteOffset(nVoice);
            AWEReleaseNote(nVoice, 0x7F);
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < 31) {
        if (dwPosition >= 0 && dwPosition <= AWE32.aLoopEnd[nVoice]) {
            AWESetNoteOffset(nVoice, 
			     AWE32.aStartAddress[nVoice] + dwPosition, 0);
            if (AWE32.aCurrentAddress[nVoice])
                AWE32.aCurrentAddress[nVoice] = AWEGetNoteOffset(nVoice);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceFrequency(UINT nVoice, LONG dwFrequency)
{
    if (nVoice < 31) {
        if (dwFrequency >= AUDIO_MIN_FREQUENCY && 
            dwFrequency <= AUDIO_MAX_FREQUENCY) {
            AWE32.aFrequencyTable[nVoice] = dwFrequency;
            AWESetNotePitch(nVoice, AWEGetPitchShift(dwFrequency));
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceVolume(UINT nVoice, UINT nVolume)
{
    if (nVoice < 31) {
        if (nVolume <= AUDIO_MAX_VOLUME) {
            AWE32.aVolumeTable[nVoice] = nVolume;
            AWESetNoteVolume(nVoice, aVolumeTable[nVolume]);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePanning(UINT nVoice, UINT nPanning)
{
    if (nVoice < 31) {
        if (nPanning <= AUDIO_MAX_PANNING) {
            AWE32.aPanningTable[nVoice] = nPanning;
            AWESetNotePanning(nVoice, 0xFF - nPanning, 
			      AWE32.aStartAddress[nVoice] + AWE32.aLoopStart[nVoice]);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, LPLONG lpdwPosition)
{
    if (nVoice < 31) {
        if (lpdwPosition != NULL) {
            *lpdwPosition = AWEGetNoteOffset(nVoice) - 
                AWE32.aStartAddress[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, LPLONG lpdwFrequency)
{
    if (nVoice < 31) {
        if (lpdwFrequency != NULL) {
            *lpdwFrequency = AWE32.aFrequencyTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, LPUINT lpnVolume)
{
    if (nVoice < 31) {
        if (lpnVolume != NULL) {
            *lpnVolume = AWE32.aVolumeTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, LPUINT lpnPanning)
{
    if (nVoice < 31) {
        if (lpnPanning != NULL) {
            *lpnPanning = AWE32.aPanningTable[nVoice];
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, LPBOOL lpbStatus)
{
    if (nVoice < 31) {
        if (lpbStatus != NULL) {
            *lpbStatus = (AWE32.aLoopStart[nVoice] + CLICKBUFSIZE - 4 == AWE32.aLoopEnd[nVoice]) &&
                (AWEGetNoteOffset(nVoice) >=
		 AWE32.aStartAddress[nVoice] + AWE32.aLoopStart[nVoice]);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}


/*
 * Sound Blaster AWE32 driver public interface
 */
AUDIOSYNTHDRIVER SoundBlaster32SynthDriver =
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

AUDIODRIVER SoundBlaster32Driver =
{
    NULL, &SoundBlaster32SynthDriver
};
