/*
 * $Id: msdos.c 1.5 1996/05/24 08:30:44 chasan released $
 *
 * MS-DOS hardware programming API interface.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "audio.h"
#include "msdos.h"


/*
 * MS-DOS direct memory access API routines
 */
static UCHAR aDmaSingle[8] =
{
    DMA1_SNGL, DMA1_SNGL, DMA1_SNGL, DMA1_SNGL,
    DMA2_SNGL, DMA2_SNGL, DMA2_SNGL, DMA2_SNGL
};

static UCHAR aDmaMode[8] =
{
    DMA1_MODE, DMA1_MODE, DMA1_MODE, DMA1_MODE,
    DMA2_MODE, DMA2_MODE, DMA2_MODE, DMA2_MODE
};

static UCHAR aDmaClear[8] =
{
    DMA1_CLRFF, DMA1_CLRFF, DMA1_CLRFF, DMA1_CLRFF,
    DMA2_CLRFF, DMA2_CLRFF, DMA2_CLRFF, DMA2_CLRFF
};

static UCHAR aDmaPage[8] =
{
    DMA0_PAGE, DMA1_PAGE, DMA2_PAGE, DMA3_PAGE,
    DMA4_PAGE, DMA5_PAGE, DMA6_PAGE, DMA7_PAGE
};

static UCHAR aDmaAddr[8] =
{
    DMA0_ADDR, DMA1_ADDR, DMA2_ADDR, DMA3_ADDR,
    DMA4_ADDR, DMA5_ADDR, DMA6_ADDR, DMA7_ADDR
};

static UCHAR aDmaCount[8] =
{
    DMA0_CNT, DMA1_CNT, DMA2_CNT, DMA3_CNT,
    DMA4_CNT, DMA5_CNT, DMA6_CNT, DMA7_CNT
};

static ULONG aDmaLinearAddress[8] =
{
    0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000
};

static USHORT aDmaBufferLength[8] =
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

VOID AIAPI DosSetupChannel(UINT nChannel, UCHAR nMode, USHORT nCount)
{
    USHORT wAddr;
    UCHAR nPage;

    if (nChannel <= 7) {
        /* check out the count parameter */
        if (nCount <= 0 || nCount >= aDmaBufferLength[nChannel])
            nCount = aDmaBufferLength[nChannel];

        /* get buffer physical address and length */
        if (nChannel >= 4) {
            wAddr = LOWORD(aDmaLinearAddress[nChannel] >> 1);
            nPage = (UCHAR) HIWORD(aDmaLinearAddress[nChannel]);
            nCount >>= 1;
        }
        else {
            wAddr = LOWORD(aDmaLinearAddress[nChannel]);
            nPage = (UCHAR) HIWORD(aDmaLinearAddress[nChannel]);
        }
        nCount--;

        /* disable DMA channel before setting parameters */
        OUTB(aDmaSingle[nChannel], (nChannel & 0x03) | DMA_DISABLE);

        /* set DMA channel transfer mode */
        OUTB(aDmaMode[nChannel], (nChannel & 0x03) | nMode);

        /* set DMA channel buffer physical address */
        OUTB(aDmaClear[nChannel], 0x00);
        OUTB(aDmaAddr[nChannel], LOBYTE(wAddr));
        OUTB(aDmaAddr[nChannel], HIBYTE(wAddr));
        OUTB(aDmaPage[nChannel], nPage);

        /* set DMA channel buffer length */
        OUTB(aDmaClear[nChannel], 0x00);
        OUTB(aDmaCount[nChannel], LOBYTE(nCount));
        OUTB(aDmaCount[nChannel], HIBYTE(nCount));

        /* enable DMA channel for transfers */
        OUTB(aDmaSingle[nChannel], (nChannel & 0x03) | DMA_ENABLE);
    }
}

VOID AIAPI DosDisableChannel(UINT nChannel)
{
    if (nChannel <= 7) {
        OUTB(aDmaSingle[nChannel], (nChannel & 0x03) | DMA_DISABLE);
    }
}

VOID AIAPI DosEnableChannel(UINT nChannel)
{
    if (nChannel <= 7) {
        OUTB(aDmaSingle[nChannel], (nChannel & 0x03) | DMA_ENABLE);
    }
}

UINT AIAPI DosGetChannelCount(UINT nChannel)
{
    INT nTimeOut, nCount, nDelta, nLoData, nHiData;

    if (nChannel <= 7) {
        /* clear DMA channel flip-flop register */
        OUTB(aDmaClear[nChannel], 0x00);

        for (nTimeOut = 0; nTimeOut < 1024; nTimeOut++) {
            /* poll DMA channel count register */
            nLoData = INB(aDmaCount[nChannel]);
            nHiData = INB(aDmaCount[nChannel]);
            nCount = MAKEWORD(nLoData, nHiData);
            nLoData = INB(aDmaCount[nChannel]);
            nHiData = INB(aDmaCount[nChannel]);
            nDelta = MAKEWORD(nLoData, nHiData);
            if ((nDelta -= nCount) > -16 && nDelta < +16)
                break;
        }

        /* adjust counter for 16 bit DMA channels */
        if (nChannel >= 4)
            nCount <<= 1;
        return nCount + 1;
    }
    return 0;
}


/*
 * 16-bit real mode custom DMA programming API routines
 */

#ifdef __DOS16__

static PVOID aDmaBufferAddr[8] =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

UINT AIAPI DosAllocChannel(UINT nChannel, USHORT nCount)
{
    ULONG dwAddress;

    /*
     * Allocate DOS memory using standard memory allocation
     * routines for Borland C++ 3.1 in real mode.
     */
    if (nChannel <= 7 &&
        (aDmaBufferAddr[nChannel] = malloc(nCount << 1)) != NULL) {
        dwAddress = ((ULONG) FP_SEG(aDmaBufferAddr[nChannel]) << 4) +
            FP_OFF(aDmaBufferAddr[nChannel]);
        if ((dwAddress & 0xFFFFL) + nCount > 0x10000L)
            dwAddress += nCount;
        aDmaLinearAddress[nChannel] = dwAddress;
        aDmaBufferLength[nChannel] = nCount;
        return 0;
    }
    return 1;
}

VOID AIAPI DosFreeChannel(UINT nChannel)
{
    if (nChannel <= 7 && aDmaBufferAddr[nChannel]) {
        free(aDmaBufferAddr[nChannel]);
        aDmaBufferAddr[nChannel] = NULL;
    }
}

PVOID AIAPI DosLockChannel(UINT nChannel)
{
    ULONG dwAddress;

    if (nChannel <= 7) {
        dwAddress = aDmaLinearAddress[nChannel];
        return MK_FP((unsigned) (dwAddress >> 4),
            (unsigned) (dwAddress & 0x000F));
    }
    return NULL;
}

VOID AIAPI DosUnlockChannel(UINT nChannel)
{
    if (nChannel <= 7) {
    }
}

#endif


/*
 * 16/32-bit protected mode DPMI custom DMA programming API routines
 */

#if defined(__DPMI__) && !defined(__DJGPP__)

static USHORT aDmaBufferSelector[8] =
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

UINT AIAPI DosAllocChannel(UINT nChannel, USHORT nCount)
{
    static DOSREGS r;
    ULONG dwAddress;

    /*
     * Allocate DOS memory using a DPMI system call, works with DOS/4GW
     * and PMODE/W 32-bit DOS extenders, Borland's DPMI16 and DPMI32,
     * and with DJGPP V2 32-bit DPMI host.
     */
    if (nChannel <= 7) {
        r.ax = 0x0100;
        r.bx = ((nCount << 1) + 15) >> 4;
        DosIntVector(0x31, &r);
        if (!r.cflag) {
            dwAddress = (ULONG) r.ax << 4;
            if ((dwAddress & 0xFFFF) + nCount > 0x10000L)
                dwAddress += nCount;
            aDmaBufferSelector[nChannel] = r.dx;
            aDmaLinearAddress[nChannel] = dwAddress;
            aDmaBufferLength[nChannel] = nCount;
            return 0;
        }
    }
    return 1;
}

VOID AIAPI DosFreeChannel(UINT nChannel)
{
    static DOSREGS r;

    if (nChannel <= 7 && aDmaBufferSelector[nChannel]) {
        r.ax = 0x0101;
        r.dx = aDmaBufferSelector[nChannel];
        DosIntVector(0x31, &r);
        aDmaBufferSelector[nChannel] = 0x0000;
    }
}

PVOID AIAPI DosLockChannel(UINT nChannel)
{
    if (nChannel <= 7) {
#ifdef __FLAT__
        /* TODO: This only works in DOS4GW and PMODEW extenders. */
        return (PVOID) aDmaLinearAddress[nChannel];
#else
        return MK_FP(aDmaBufferSelector[nChannel], 0x0000);
#endif
    }
    return NULL;
}

VOID AIAPI DosUnlockChannel(UINT nChannel)
{
    if (nChannel <= 7) {
    }
}

#endif


/*
 * DJGPP V2 custom DMA programming routines
 */

#ifdef __DJGPP__

static USHORT aDmaBufferSelector[8] =
{
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000
};

UINT AIAPI DosAllocChannel(UINT nChannel, USHORT nCount)
{
    ULONG dwAddress;
    UINT nSelector;

    /*
     * Allocate DOS memory using a DPMI system call
     * for DJGPP V2 in 32-bit protected mode.
     */
    if (nChannel <= 7) {
        if (__dpmi_allocate_dos_memory(((nCount << 1) + 15) >> 4, &nSelector) != -1) {
            __dpmi_get_segment_base_address(nSelector, &dwAddress);
            if ((dwAddress & 0xFFFFL) + nCount > 0x10000L)
                dwAddress += nCount;
            aDmaBufferSelector[nChannel] = nSelector;
            aDmaLinearAddress[nChannel] = dwAddress;
            aDmaBufferLength[nChannel] = nCount;
            return 0;
        }
    }
    return 1;
}

VOID AIAPI DosFreeChannel(UINT nChannel)
{
    if (nChannel <= 7 && aDmaBufferSelector[nChannel]) {
        __dpmi_free_dos_memory(aDmaBufferSelector[nChannel]);
        aDmaBufferSelector[nChannel] = 0x0000;
    }
}

PVOID AIAPI DosLockChannel(UINT nChannel)
{
    if (nChannel <= 7 && ((_crt0_startup_flags & _CRT0_FLAG_NEARPTR) ||
        __djgpp_nearptr_enable())) {
        return (PVOID) (aDmaLinearAddress[nChannel] +
            __djgpp_conventional_base);
    }
    return NULL;
}

VOID AIAPI DosUnlockChannel(UINT nChannel)
{
    if (nChannel <= 7 && !(_crt0_startup_flags & _CRT0_FLAG_NEARPTR)) {
        __djgpp_nearptr_disable();
    }
}
#endif



/*
 * MS-DOS interrupt programming API routines
 */

#ifdef __BORLANDC__
typedef VOID __interrupt (*DOSHARDWAREVECTOR)(VOID);
#endif

#ifdef __WATCOMC__
typedef VOID (__interrupt* DOSHARDWAREVECTOR)(VOID);
#endif

#ifdef __DJGPP__
typedef _go32_dpmi_seginfo DOSHARDWAREVECTOR;
#define __interrupt
#endif

static UCHAR aIrqSlotNumber[16] =
{
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77
};

static DOSUSERVECTOR aUserVector[16] =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static DOSHARDWAREVECTOR aHardwareVector[16];


static VOID MasterWrapVector(UINT nIrqLine)
{
    if (aUserVector[nIrqLine] != NULL)
        (*aUserVector[nIrqLine])();
    if (nIrqLine >= 8)
        OUTB(0xA0, 0x20);
    OUTB(0x20, 0x20);
}

#define WRAPVECTOR(name, num) \
static VOID __interrupt name (VOID) { MasterWrapVector(num); }

WRAPVECTOR(Wrap0Vector, 0x00)
WRAPVECTOR(Wrap1Vector, 0x01)
WRAPVECTOR(Wrap2Vector, 0x02)
WRAPVECTOR(Wrap3Vector, 0x03)
WRAPVECTOR(Wrap4Vector, 0x04)
WRAPVECTOR(Wrap5Vector, 0x05)
WRAPVECTOR(Wrap6Vector, 0x06)
WRAPVECTOR(Wrap7Vector, 0x07)
WRAPVECTOR(Wrap8Vector, 0x08)
WRAPVECTOR(Wrap9Vector, 0x09)
WRAPVECTOR(WrapAVector, 0x0A)
WRAPVECTOR(WrapBVector, 0x0B)
WRAPVECTOR(WrapCVector, 0x0C)
WRAPVECTOR(WrapDVector, 0x0D)
WRAPVECTOR(WrapEVector, 0x0E)
WRAPVECTOR(WrapFVector, 0x0F)

#ifdef __DJGPP__
static DOSUSERVECTOR aWrapVector[16] =
#else
static DOSHARDWAREVECTOR aWrapVector[16] =
#endif
{
    Wrap0Vector, Wrap1Vector, Wrap2Vector, Wrap3Vector,
    Wrap4Vector, Wrap5Vector, Wrap6Vector, Wrap7Vector,
    Wrap8Vector, Wrap9Vector, WrapAVector, WrapBVector,
    WrapCVector, WrapDVector, WrapEVector, WrapFVector
};


VOID AIAPI DosEnableVectorHandler(UINT nIrqLine)
{
    USHORT nIrqMask;

    if (nIrqLine <= 15) {
        nIrqMask = MAKEWORD(INB(0x21), INB(0xA1)) & ~(1 << nIrqLine);
        OUTB(0x21, LOBYTE(nIrqMask));
        OUTB(0xA1, HIBYTE(nIrqMask));
    }
}

VOID AIAPI DosDisableVectorHandler(UINT nIrqLine)
{
    USHORT nIrqMask;

    if (nIrqLine <= 15) {
        nIrqMask = MAKEWORD(INB(0x21), INB(0xA1)) | (1 << nIrqLine);
        OUTB(0x21, LOBYTE(nIrqMask));
        OUTB(0xA1, HIBYTE(nIrqMask));
    }
}

VOID AIAPI DosSetVectorHandler(UINT nIrqLine, DOSUSERVECTOR pfnUserVector)
{
#ifdef __DJGPP__
    _go32_dpmi_seginfo wrapper;

#endif
    UINT nIntr;

    if (nIrqLine <= 15) {
        nIntr = aIrqSlotNumber[nIrqLine];
        if (pfnUserVector != NULL) {
            if (!aUserVector[nIrqLine]) {
#ifdef __DJGPP__
                _go32_dpmi_get_protected_mode_interrupt_vector(nIntr, &aHardwareVector[nIrqLine]);
                wrapper.pm_offset = (ULONG) aWrapVector[nIrqLine];
                wrapper.pm_selector = _go32_my_cs();
                _go32_dpmi_allocate_iret_wrapper(&wrapper);
                _go32_dpmi_set_protected_mode_interrupt_vector(nIntr, &wrapper);
#else
                aHardwareVector[nIrqLine] = _dos_getvect(nIntr);
                _dos_setvect(nIntr, aWrapVector[nIrqLine]);
#endif
                DosEnableVectorHandler(nIrqLine);
            }
        }
        else {
            if (aUserVector[nIrqLine]) {
                DosDisableVectorHandler(nIrqLine);
#ifdef __DJGPP__
                _go32_dpmi_get_protected_mode_interrupt_vector(nIntr, &wrapper);
                _go32_dpmi_set_protected_mode_interrupt_vector(nIntr, &aHardwareVector[nIrqLine]);
                _go32_dpmi_free_iret_wrapper(&wrapper);
#else
                _dos_setvect(nIntr, aHardwareVector[nIrqLine]);
#endif
            }
        }
        aUserVector[nIrqLine] = pfnUserVector;
    }
}

VOID AIAPI DosIntVector(UINT nIntr, PDOSREGS pRegs)
{
    static union REGS r;

    memset(&r, 0, sizeof(r));
#ifdef __FLAT__
    r.w.ax = pRegs->ax;
    r.w.bx = pRegs->bx;
    r.w.cx = pRegs->cx;
    r.w.dx = pRegs->dx;
    r.w.si = pRegs->si;
    r.w.di = pRegs->di;
    int386(nIntr, &r, &r);
    pRegs->ax = r.w.ax;
    pRegs->bx = r.w.bx;
    pRegs->cx = r.w.cx;
    pRegs->dx = r.w.dx;
    pRegs->si = r.w.si;
    pRegs->di = r.w.di;
    pRegs->cflag = r.x.cflag;
#else
    r.x.ax = pRegs->ax;
    r.x.bx = pRegs->bx;
    r.x.cx = pRegs->cx;
    r.x.dx = pRegs->dx;
    r.x.si = pRegs->si;
    r.x.di = pRegs->di;
    int86(nIntr, &r, &r);
    pRegs->ax = r.x.ax;
    pRegs->bx = r.x.bx;
    pRegs->cx = r.x.cx;
    pRegs->dx = r.x.dx;
    pRegs->si = r.x.si;
    pRegs->di = r.x.di;
    pRegs->cflag = r.x.cflag;
#endif
}


PSZ AIAPI DosGetEnvironment(PSZ pszKeyName)
{
    return getenv(pszKeyName);
}

UINT AIAPI DosParseString(PSZ pszText, UINT nToken)
{
    static PSZ pszString = NULL;

    if (pszText != NULL)
        pszString = pszText;

    if (pszString != NULL) {
       if (nToken == TOKEN_CHAR) {
           return *pszString++;
       }
       if (nToken == TOKEN_DEC) {
           return strtol(pszString, &pszString, 10);
       }
       if (nToken == TOKEN_HEX) {
           return strtol(pszString, &pszString, 0x10);
       }
    }
    return BAD_TOKEN;
}
