/*
 * $Id: msdos.h 1.5 1996/08/05 18:51:19 chasan released $
 *
 * MS-DOS hardware programming API interface.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MSDOS_H
#define __MSDOS_H

#ifdef __BORLANDC__
#include <dos.h>
#include <conio.h>
#endif

#ifdef __WATCOMC__
#include <i86.h>
#include <dos.h>
#include <conio.h>
#endif

#ifdef __DJGPP__
#include <sys/nearptr.h>
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#include <dos.h>
#include <crt0.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I/O address space macro routines
 */
#ifdef __BORLANDC__
#define INB     inportb
#define INW     inport
#define OUTB    outportb
#define OUTW    outport
#endif

#ifdef __WATCOMC__
#define INB     inp
#define INW     inpw
#define OUTB    outp
#define OUTW    outpw
#endif

#ifdef __DJGPP__
#define INB     inportb
#define INW     inportw
#define OUTB    outportb
#define OUTW    outportw
#endif

/*
 * 8-bit DMA controller register ports
 */
#define DMA1_STAT       0x08
#define DMA1_WCMD       0x08
#define DMA1_WREQ       0x09
#define DMA1_SNGL       0x0A
#define DMA1_MODE       0x0B
#define DMA1_CLRFF      0x0C
#define DMA1_MCLR       0x0D
#define DMA1_CLRM       0x0E
#define DMA1_WRTALL     0x0F

/*
 * 16-bit DMA controller register ports
 */
#define DMA2_STAT       0xD0
#define DMA2_WCMD       0xD0
#define DMA2_WREQ       0xD2
#define DMA2_SNGL       0xD4
#define DMA2_MODE       0xD6
#define DMA2_CLRFF      0xD8
#define DMA2_MCLR       0xDA
#define DMA2_CLRM       0xDC
#define DMA2_WRTALL     0xDE

/*
 * DMA controllers base address and count registers
 */
#define DMA0_ADDR       0x00
#define DMA0_CNT        0x01
#define DMA1_ADDR       0x02
#define DMA1_CNT        0x03
#define DMA2_ADDR       0x04
#define DMA2_CNT        0x05
#define DMA3_ADDR       0x06
#define DMA3_CNT        0x07
#define DMA4_ADDR       0xC0
#define DMA4_CNT        0xC2
#define DMA5_ADDR       0xC4
#define DMA5_CNT        0xC6
#define DMA6_ADDR       0xC8
#define DMA6_CNT        0xCA
#define DMA7_ADDR       0xCC
#define DMA7_CNT        0xCE

#define DMA0_PAGE       0x87
#define DMA1_PAGE       0x83
#define DMA2_PAGE       0x81
#define DMA3_PAGE       0x82
#define DMA4_PAGE       0x8F
#define DMA5_PAGE       0x8B
#define DMA6_PAGE       0x89
#define DMA7_PAGE       0x8A

/*
 * DMA controller mode bit fields
 */
#define DMA_ENABLE      0x00
#define DMA_DISABLE     0x04
#define DMA_AUTOINIT    0x10
#define DMA_READ        0x44
#define DMA_WRITE       0x48

/*
 * PIC hardware interrupt callback routine
 */
    typedef VOID (AIAPI* LPFNUSERVECTOR)(VOID);

    typedef struct {
	WORD    ax;
	WORD    bx;
	WORD    cx;
	WORD    dx;
	WORD    si;
	WORD    di;
	WORD    cflag;
    } DOSREGS, *LPDOSREGS;


/*
 * DOS environment parsing token types
 */
#define TOKEN_CHAR      0x00
#define TOKEN_DEC       0x01
#define TOKEN_HEX       0x02
#define BAD_TOKEN       0xFFFFU


/*
 * MS-DOS hardware programming API interface
 */
    VOID AIAPI DosSetVectorHandler(UINT nIrqLine, LPFNUSERVECTOR lpfnUserVector);
    VOID AIAPI DosEnableVectorHandler(UINT nIrqLine);
    VOID AIAPI DosDisableVectorHandler(UINT nIrqLine);

    VOID AIAPI DosSetupChannel(UINT nChannel, BYTE nMode, WORD nCount);
    VOID AIAPI DosDisableChannel(UINT nChannel);
    VOID AIAPI DosEnableChannel(UINT nChannel);
    UINT AIAPI DosGetChannelCount(UINT nChannel);
    UINT AIAPI DosAllocChannel(UINT nChannel, WORD nCount);
    VOID AIAPI DosFreeChannel(UINT nChannel);
    LPVOID AIAPI DosLockChannel(UINT nChannel);
    VOID AIAPI DosUnlockChannel(UINT nChannel);

    VOID AIAPI DosIntVector(UINT nIntr, LPDOSREGS lpRegs);

    LPSTR AIAPI DosGetEnvironment(LPSTR lpszKeyName);
    UINT AIAPI DosParseString(LPSTR lpszText, UINT nToken);

#ifdef __cplusplus
};
#endif

#endif
