/*
 * $Id: _mixdrv.S 1.8 1996/08/15 02:33:23 chasan released $
 *                1.9 1998/10/25 15:46:38 chasan released (BeOS port)
 *
 * Low-level assembly resamplation and quantization routines.
 * Optimized for superscalar architecture of Pentium processors.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

        .file   "_mixdrv"

/* assembly linkage macros */
#ifdef __LINUX__
#define __ASSEMBLY__
#include <linux/linkage.h>
#else
#ifdef __BEOS__
#define ALIGN .align 4,0x90
#define SYMBOL_NAME(name) name
#define SYMBOL_NAME_LABEL(name) name##:
#define ENTRY(name) .globl SYMBOL_NAME(name); ALIGN; SYMBOL_NAME_LABEL(name)
#else
#define ALIGN .align 4,0x90
#define SYMBOL_NAME(name) _##name
#define SYMBOL_NAME_LABEL(name) _##name##:
#define ENTRY(name) .globl SYMBOL_NAME(name); ALIGN; SYMBOL_NAME_LABEL(name)
#endif
#endif

/* voice structure field offsets */
DATA            = 0x00                  /* data pointer */
ACCUM           = 0x04                  /* accumulator */
PITCH           = 0x08                  /* pitch shift */
LOOPSTART       = 0x0C                  /* loop start point */
LOOPEND         = 0x10                  /* loop end point */
VOLUME          = 0x14                  /* volume */
PANNING         = 0x15                  /* panning */
CONTROL         = 0x16                  /* control bits */
RESERVED        = 0x17                  /* filter sample data */

/* pitch shift accuracy */
ACCURACY        = 12

        .data

        .globl  SYMBOL_NAME(lpVolumeTable)
        .globl  SYMBOL_NAME(lpFilterTable)

        ALIGN
nCount:         .long   0
nStepLo:        .long   0
nStepHi:        .long   0

        .text

/*
 * VOID QuantAudioData08(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
 */
ENTRY(QuantAudioData08)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi
        movl    0x08(%ebp),%edi
        movl    0x0c(%ebp),%esi
        movl    0x10(%ebp),%ecx
        movl    $0x8000,%ebx
        movl    $0xffff,%edx
        decl    %edi
        ALIGN
1:      movl    (%esi),%eax             /* 5 cycles/sample */
        incl    %edi
        addl    %ebx,%eax
        jl      3f
        cmp     %edx,%eax
        jg      4f
2:      add     $4,%esi
        movb    %ah,(%edi)
        decl    %ecx
        jg      1b
        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret
        ALIGN
3:      xorl    %eax,%eax
        jmp     2b
        ALIGN
4:      movl    %edx,%eax
        jmp     2b

/*
 * VOID QuantAudioData16(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
 */
ENTRY(QuantAudioData16)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi
        movl    0x08(%ebp),%edi
        movl    0x0c(%ebp),%esi
        movl    0x10(%ebp),%ecx
        movl    $-32768,%ebx
        movl    $+32767,%edx
        subl    $2,%edi
        ALIGN
1:      movl    (%esi),%eax             /* 5 cycles/sample */
        addl    $2,%edi
        cmpl    %ebx,%eax
        jl      3f
        cmpl    %edx,%eax
        jg      4f
2:      addl    $4,%esi
        movw    %ax,(%edi)
        decl    %ecx
        jg      1b
        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret
        ALIGN
3:      movl    %ebx,%eax
        jmp     2b
        ALIGN
4:      movl    %edx,%eax
        jmp     2b


/*------------------------- 8 bit mixing routines --------------------------*/

/*
 * VOID MixAudioData08M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData08M)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x0C(%ebp),%ecx         /* get parameters */
        movl    0x08(%ebp),%edi
        movl    0x10(%ebp),%esi

__MixAudioData08M:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     (%esi),%bl              /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x40(%edi,%eax,4),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF(%edi),%edx;         \
        movl    (,%ebx,4),%eax;         \
        addl    %edx,%eax;              \
        addl    %ebp,%ecx;              \
        movb    (%esi),%bl;             \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* 4 cycles/sample */
2:      RESAMPLE(0x04)
3:      RESAMPLE(0x08)
4:      RESAMPLE(0x0C)
5:      RESAMPLE(0x10)
6:      RESAMPLE(0x14)
7:      RESAMPLE(0x18)
8:      RESAMPLE(0x1C)
9:      RESAMPLE(0x20)
10:     RESAMPLE(0x24)
11:     RESAMPLE(0x28)
12:     RESAMPLE(0x2C)
13:     RESAMPLE(0x30)
14:     RESAMPLE(0x34)
15:     RESAMPLE(0x38)
16:     RESAMPLE(0x3C)
17:     addl    $0x40,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData08S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData08S)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x08(%ebp),%edi         /* get parameters */
        movl    0x0C(%ebp),%ecx
        movl    0x10(%ebp),%esi

__MixAudioData08S:
        movb    PANNING(%esi),%al       /* select panning routine */
        cmpb    $0x10,%al
        jb      MixAudioLeft08S         /* full left panned? */
        cmpb    $0x78,%al
        jb      MixAudioPan08S          /* left panned? */
        cmpb    $0x88,%al
        jb      MixAudioMiddle08S       /* middle panned? */
        cmpb    $0xF0,%al
        jae     MixAudioRight08S        /* full right panned? */


/*
 * 8-bit general stereo mixing routine
 */
        ALIGN
MixAudioPan08S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%edx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movzbl  PANNING(%esi),%ecx      /* get panning */
        movl    DATA(%esi),%esi         /* get data pointer */

        pushl   %esi
        imull   %ebx,%ecx               /* get volume tables */
        shrl    $8,%ecx
        subl    %ecx,%ebx
        shll    $10,%ebx
        shll    $10,%ecx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ecx
        shrl    $2,%ebx
        shrl    $2,%ecx

        movl    %ebp,%eax               /* accum to 32.32 fixed point */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %edx,%eax               /* pitch to 32.32 fixed point */
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%eax
        movl    %eax,nStepLo
        movl    %edx,nStepHi

        movb    (%esi),%bl              /* get first sample, and */
        addl    %eax,%ebp               /* advance one step */
        adcl    %edx,%esi
        movb    %bl,%cl

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    (,%ebx,4),%eax;         \
        movl    OFF+0(%edi),%edx;       \
        addl    %edx,%eax;              \
        movl    OFF+4(%edi),%edx;       \
        movl    %eax,OFF+0(%edi);       \
        movl    (,%ecx,4),%eax;         \
        addl    %edx,%eax;              \
        movb    (%esi),%bl;             \
        movl    %eax,OFF+4(%edi);       \
        movl    nStepLo,%eax;           \
        addl    %eax,%ebp;              \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movb    %bl,%cl;

        ALIGN
1:      RESAMPLE(0x00)                  /* 7 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepLo,%eax            /* go back one step */
        movl    nStepHi,%edx
        subl    %eax,%ebp
        sbbl    %edx,%esi
        popl    %eax                    /* get accumulator */
        subl    %eax,%esi
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 8-bit full right stereo mixing routine
 */
        ALIGN
MixAudioRight08S:
        addl    $4,%edi

/*
 * 8-bit full left stereo mixing routine
 */
        ALIGN
MixAudioLeft08S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     (%esi),%bl              /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF(%edi),%edx;         \
        movl    (,%ebx,4),%eax;         \
        addl    %edx,%eax;              \
        addl    %ebp,%ecx;              \
        movb    (%esi),%bl;             \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* 4 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accum */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 8-bit middle stereo mixing routine
 */
        ALIGN
MixAudioMiddle08S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%ebx
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     (%esi),%bl              /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF+0(%edi),%edx;       \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,%edx;              \
        movb    (%esi),%bl;             \
        movl    %edx,OFF+0(%edi);       \
        movl    OFF+4(%edi),%edx;       \
        addl    %eax,%edx;              \
        movl    nStepHi,%eax;           \
        movl    %edx,OFF+4(%edi);       \
        addl    %ebp,%ecx;              \
        adcl    %eax,%esi;

        ALIGN
1:      RESAMPLE(0x00)                  /* 6 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accum */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData08MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData08MI)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x0C(%ebp),%ecx         /* get parameters */
        movl    0x08(%ebp),%edi
        movl    0x10(%ebp),%esi

        movl    PITCH(%esi),%eax        /* select mixing routine */
        cmpl    $(+1 << ACCURACY),%eax
        jge     __MixAudioData08M
        cmpl    $(-1 << ACCURACY),%eax
        jle     __MixAudioData08M

        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        pushl   %esi

        shll    $10,%ebx                /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        shll    $(32-ACCURACY),%ecx
        sarl    $ACCURACY,%eax
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x40(%edi,%eax,4),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    (%esi),%dl;             \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~11 cycles/sample */
2:      RESAMPLE(0x04)
3:      RESAMPLE(0x08)
4:      RESAMPLE(0x0C)
5:      RESAMPLE(0x10)
6:      RESAMPLE(0x14)
7:      RESAMPLE(0x18)
8:      RESAMPLE(0x1C)
9:      RESAMPLE(0x20)
10:     RESAMPLE(0x24)
11:     RESAMPLE(0x28)
12:     RESAMPLE(0x2C)
13:     RESAMPLE(0x30)
14:     RESAMPLE(0x34)
15:     RESAMPLE(0x38)
16:     RESAMPLE(0x3C)
17:     addl    $0x40,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData08SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData08SI)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x08(%ebp),%edi         /* get parameters */
        movl    0x0C(%ebp),%ecx
        movl    0x10(%ebp),%esi

        movl    PITCH(%esi),%eax        /* select mixing routine */
        cmpl    $(+1 << ACCURACY),%eax
        jge     __MixAudioData08S
        cmpl    $(-1 << ACCURACY),%eax
        jle     __MixAudioData08S

        movb    PANNING(%esi),%al       /* select panning routine */
        cmpb    $0x10,%al
        jb      MixAudioLeft08SI        /* full left panned? */
        cmpb    $0x78,%al
        jb      MixAudioPan08SI         /* left panned? */
        cmpb    $0x88,%al
        jb      MixAudioMiddle08SI      /* middle panned? */
        cmpb    $0xF0,%al
        jae     MixAudioRight08SI       /* full right panned? */


/*
 * 8-bit general stereo mixing routine
 */
        ALIGN
MixAudioPan08SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%edx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movzbl  PANNING(%esi),%ecx      /* get panning */
        movb    RESERVED(%esi),%al      /* get last sample */
        movl    DATA(%esi),%esi         /* get data pointer */

        pushl   %esi
        imull   %ebx,%ecx               /* get volume tables */
        shrl    $8,%ecx
        subl    %ecx,%ebx
        shll    $10,%ebx
        shll    $10,%ecx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ecx
        shrl    $2,%ebx
        shrl    $2,%ecx
        movb    %al,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* accum to 32.32 fixed point */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %edx,%eax               /* pitch to 32.32 fixed point */
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx
        pushl   %eax
        shll    $(32-ACCURACY),%eax
        movl    %eax,nStepLo
        popl    %eax
        sarl    $ACCURACY,%eax
        movl    %eax,nStepHi

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    (%esi),%dl;             \
        movb    (%edx),%dl;             \
        movl    nStepLo,%eax;           \
        addb    %dl,%bl;                \
        addl    %eax,%ebp;              \
        movb    %bl,%cl;                \
        movl    nStepHi,%eax;           \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);         \
        movl    (,%ecx,4),%eax;         \
        addl    %eax,OFF+4(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~15 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax                    /* get accumulator */
        subl    %eax,%esi
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 8-bit full right stereo mixing routine
 */
        ALIGN
MixAudioRight08SI:
        addl    $4,%edi

/*
 * 8-bit full left stereo mixing routine
 */
        ALIGN
MixAudioLeft08SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        pushl   %esi

        shll    $10,%ebx                /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ecx
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    (%esi),%dl;             \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~11 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 8-bit middle stereo mixing routine
 */
        ALIGN
MixAudioMiddle08SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        pushl   %esi

        shrl    $1,%ebx                 /* get volume table address */
        shll    $10,%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ecx
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    (%esi),%dl;             \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);         \
        addl    %eax,OFF+4(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~14 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*---------------------- fake 16 bit mixing routines -----------------------*/

/*
 * VOID MixAudioData16M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData16M)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x0C(%ebp),%ecx         /* get parameters */
        movl    0x08(%ebp),%edi
        movl    0x10(%ebp),%esi

__MixAudioData16M:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     1(,%esi,2),%bl          /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x40(%edi,%eax,4),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF(%edi),%edx;         \
        movl    (,%ebx,4),%eax;         \
        addl    %edx,%eax;              \
        addl    %ebp,%ecx;              \
        movb    1(,%esi,2),%bl;         \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* 4 cycles/sample */
2:      RESAMPLE(0x04)
3:      RESAMPLE(0x08)
4:      RESAMPLE(0x0C)
5:      RESAMPLE(0x10)
6:      RESAMPLE(0x14)
7:      RESAMPLE(0x18)
8:      RESAMPLE(0x1C)
9:      RESAMPLE(0x20)
10:     RESAMPLE(0x24)
11:     RESAMPLE(0x28)
12:     RESAMPLE(0x2C)
13:     RESAMPLE(0x30)
14:     RESAMPLE(0x34)
15:     RESAMPLE(0x38)
16:     RESAMPLE(0x3C)
17:     addl    $0x40,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData16S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData16S)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x08(%ebp),%edi         /* get parameters */
        movl    0x0C(%ebp),%ecx
        movl    0x10(%ebp),%esi

__MixAudioData16S:
        movb    PANNING(%esi),%al       /* select panning routine */
        cmpb    $0x10,%al
        jb      MixAudioLeft16S         /* full left panned? */
        cmpb    $0x78,%al
        jb      MixAudioPan16S          /* left panned? */
        cmpb    $0x88,%al
        jb      MixAudioMiddle16S       /* middle panned? */
        cmpb    $0xF0,%al
        jae     MixAudioRight16S        /* full right panned? */


/*
 * 16-bit general stereo mixing routine
 */
        ALIGN
MixAudioPan16S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%edx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movzbl  PANNING(%esi),%ecx      /* get panning */
        movl    DATA(%esi),%esi         /* get data pointer */
        shrl    $1,%esi

        pushl   %esi
        imull   %ebx,%ecx               /* get volume tables */
        shrl    $8,%ecx
        subl    %ecx,%ebx
        shll    $10,%ebx
        shll    $10,%ecx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ecx
        shrl    $2,%ebx
        shrl    $2,%ecx

        movl    %ebp,%eax               /* accum to 32.32 fixed point */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %edx,%eax               /* pitch to 32.32 fixed point */
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%eax
        movl    %eax,nStepLo
        movl    %edx,nStepHi

        movb    1(,%esi,2),%bl          /* get first sample, and */
        addl    %eax,%ebp               /* advance one step */
        adcl    %edx,%esi
        movb    %bl,%cl

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    (,%ebx,4),%eax;         \
        movl    OFF+0(%edi),%edx;       \
        addl    %edx,%eax;              \
        movl    OFF+4(%edi),%edx;       \
        movl    %eax,OFF+0(%edi);       \
        movl    (,%ecx,4),%eax;         \
        addl    %edx,%eax;              \
        movb    1(,%esi,2),%bl;         \
        movl    %eax,OFF+4(%edi);       \
        movl    nStepLo,%eax;           \
        addl    %eax,%ebp;              \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movb    %bl,%cl;

        ALIGN
1:      RESAMPLE(0x00)                  /* 7 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepLo,%eax            /* go back one step */
        movl    nStepHi,%edx
        subl    %eax,%ebp
        sbbl    %edx,%esi
        popl    %eax                    /* get accumulator */
        subl    %eax,%esi
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 16-bit full right stereo mixing routine
 */
        ALIGN
MixAudioRight16S:
        addl    $4,%edi

/*
 * 16-bit full left stereo mixing routine
 */
        ALIGN
MixAudioLeft16S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     1(,%esi,2),%bl          /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF(%edi),%edx;         \
        movl    (,%ebx,4),%eax;         \
        addl    %edx,%eax;              \
        addl    %ebp,%ecx;              \
        movb    1(,%esi,2),%bl;         \
        movl    nStepHi,%edx;           \
        adcl    %edx,%esi;              \
        movl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* 4 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accum */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 16-bit middle stereo mixing routine
 */
        ALIGN
MixAudioMiddle16S:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ecx        /* get accumulator */
        movl    PITCH(%esi),%ebp        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        shrl    $1,%ebx
        shll    $10,%ebx                /* get table offset */
        pushl   %esi
        movl    %ecx,%eax               /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx /* accum to 32.32 fixed-point */
        sarl    $ACCURACY,%eax          /* pitch to 32.32 fixed-point */
        shrl    $2,%ebx
        shll    $(32-ACCURACY),%ecx
        movl    %ebp,%edx
        addl    %eax,%esi
        sarl    $ACCURACY,%edx
        shll    $(32-ACCURACY),%ebp
        mov     %edx,nStepHi

        mov     1(,%esi,2),%bl          /* get first sample, and */
        movl    nStepHi,%edx            /* advance one step */
        addl    %ebp,%ecx
        adcl    %edx,%esi

        movl    nCount,%eax             /* jump into the inner */
        movl    %eax,%edx               /* of the unrolled loop */
        andl    $0x0F,%eax
        shrl    $4,%edx
        leal    -0x80(%edi,%eax,8),%edi
        movl    %edx,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movl    OFF+0(%edi),%edx;       \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,%edx;              \
        movb    1(,%esi,2),%bl;         \
        movl    %edx,OFF+0(%edi);       \
        movl    OFF+4(%edi),%edx;       \
        addl    %eax,%edx;              \
        movl    nStepHi,%eax;           \
        movl    %edx,OFF+4(%edi);       \
        addl    %ebp,%ecx;              \
        adcl    %eax,%esi;

        ALIGN
1:      RESAMPLE(0x00)                  /* 6 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        movl    nStepHi,%eax            /* go back one step */
        subl    %ebp,%ecx
        sbbl    %eax,%esi
        popl    %eax
        subl    %eax,%esi               /* get accumulator */
        shrdl   $(32-ACCURACY),%esi,%ecx
        popl    %esi
        movl    %ecx,ACCUM(%esi)        /* save accum */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData16MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData16MI)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x0C(%ebp),%ecx         /* get parameters */
        movl    0x08(%ebp),%edi
        movl    0x10(%ebp),%esi

        movl    PITCH(%esi),%eax        /* select mixing routine */
        cmpl    $(+1 << ACCURACY),%eax
        jge     __MixAudioData16M
        cmpl    $(-1 << ACCURACY),%eax
        jle     __MixAudioData16M

        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        pushl   %esi

        shll    $10,%ebx                /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        shll    $(32-ACCURACY),%ecx
        sarl    $ACCURACY,%eax
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x40(%edi,%eax,4),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    1(,%esi,2),%dl;         \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~11 cycles/sample */
2:      RESAMPLE(0x04)
3:      RESAMPLE(0x08)
4:      RESAMPLE(0x0C)
5:      RESAMPLE(0x10)
6:      RESAMPLE(0x14)
7:      RESAMPLE(0x18)
8:      RESAMPLE(0x1C)
9:      RESAMPLE(0x20)
10:     RESAMPLE(0x24)
11:     RESAMPLE(0x28)
12:     RESAMPLE(0x2C)
13:     RESAMPLE(0x30)
14:     RESAMPLE(0x34)
15:     RESAMPLE(0x38)
16:     RESAMPLE(0x3C)
17:     addl    $0x40,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b


/*
 * VOID MixAudioData16SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
 */
ENTRY(MixAudioData16SI)
        pushl   %ebp
        movl    %esp,%ebp
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi

        movl    0x08(%ebp),%edi         /* get parameters */
        movl    0x0C(%ebp),%ecx
        movl    0x10(%ebp),%esi

        movl    PITCH(%esi),%eax        /* select mixing routine */
        cmpl    $(+1 << ACCURACY),%eax
        jge     __MixAudioData16S
        cmpl    $(-1 << ACCURACY),%eax
        jle     __MixAudioData16S

        movb    PANNING(%esi),%al       /* select panning routine */
        cmpb    $0x10,%al
        jb      MixAudioLeft16SI        /* full left panned? */
        cmpb    $0x78,%al
        jb      MixAudioPan16SI         /* left panned? */
        cmpb    $0x88,%al
        jb      MixAudioMiddle16SI      /* middle panned? */
        cmpb    $0xF0,%al
        jae     MixAudioRight16SI       /* full right panned? */


/*
 * 16-bit general stereo mixing routine
 */
        ALIGN
MixAudioPan16SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%edx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movzbl  PANNING(%esi),%ecx      /* get panning */
        movb    RESERVED(%esi),%al      /* get last sample */
        movl    DATA(%esi),%esi         /* get data pointer */
        shrl    $1,%esi

        pushl   %esi
        imull   %ebx,%ecx               /* get volume tables */
        shrl    $8,%ecx
        subl    %ecx,%ebx
        shll    $10,%ebx
        shll    $10,%ecx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ecx
        shrl    $2,%ebx
        shrl    $2,%ecx
        movb    %al,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* accum to 32.32 fixed point */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %edx,%eax               /* pitch to 32.32 fixed point */
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx
        pushl   %eax
        shll    $(32-ACCURACY),%eax
        movl    %eax,nStepLo
        popl    %eax
        sarl    $ACCURACY,%eax
        movl    %eax,nStepHi

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    1(,%esi,2),%dl;         \
        movb    (%edx),%dl;             \
        movl    nStepLo,%eax;           \
        addb    %dl,%bl;                \
        addl    %eax,%ebp;              \
        movb    %bl,%cl;                \
        movl    nStepHi,%eax;           \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);         \
        movl    (,%ecx,4),%eax;         \
        addl    %eax,OFF+4(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~15 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax                    /* get accumulator */
        subl    %eax,%esi
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)        /* save accumulator */
        movb    %bl,RESERVED(%esi)      /* save last sample */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 16-bit full right stereo mixing routine
 */
        ALIGN
MixAudioRight16SI:
        addl    $4,%edi

/*
 * 16-bit full left stereo mixing routine
 */
        ALIGN
MixAudioLeft16SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        pushl   %esi

        shll    $10,%ebx                /* get volume table address */
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ecx
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    1(,%esi,2),%dl;         \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~11 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

/*
 * 16-bit middle stereo mixing routine
 */
        ALIGN
MixAudioMiddle16SI:
        movl    %ecx,nCount             /* save counter */
        pushl   %esi
        movl    ACCUM(%esi),%ebp        /* get accumulator */
        movl    PITCH(%esi),%ecx        /* get pitch */
        movzbl  VOLUME(%esi),%ebx       /* get volume */
        movb    RESERVED(%esi),%dl      /* get last sample */
        movl    DATA(%esi),%esi         /* get data address */
        shrl    $1,%esi
        pushl   %esi

        shrl    $1,%ebx                 /* get volume table address */
        shll    $10,%ebx
        addl    SYMBOL_NAME(lpVolumeTable),%ebx
        shrl    $2,%ebx
        movb    %dl,%bl                 /* save sample for filtering */

        movl    %ebp,%eax               /* convert accumulator */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ebp
        addl    %eax,%esi

        movl    %ecx,%edx               /* get filter table address */
        movl    %ecx,%eax               /* and convert pitch */
        sarl    $ACCURACY,%eax
        shll    $(32-ACCURACY),%ecx
        movl    %eax,nStepHi
        testl   %edx,%edx
        jge     1f
        neg     %edx
1:      shrl    $(ACCURACY-5),%edx
        shll    $8,%edx
        addl    SYMBOL_NAME(lpFilterTable),%edx

        movl    nCount,%eax             /* jump inside of the loop */
        andl    $0x0F,%eax
        leal    -0x80(%edi,%eax,8),%edi
        shrl    $4,nCount
        jmpl    *18f(,%eax,4)

#define RESAMPLE(OFF)                   \
        movb    %bl,%dl;                \
        subb    (%edx),%bl;             \
        movb    1(,%esi,2),%dl;         \
        movb    (%edx),%dl;             \
        movl    nStepHi,%eax;           \
        addb    %dl,%bl;                \
        addl    %ecx,%ebp;              \
        adcl    %eax,%esi;              \
        movl    (,%ebx,4),%eax;         \
        addl    %eax,OFF(%edi);         \
        addl    %eax,OFF+4(%edi);

        ALIGN
1:      RESAMPLE(0x00)                  /* ~14 cycles/sample */
2:      RESAMPLE(0x08)
3:      RESAMPLE(0x10)
4:      RESAMPLE(0x18)
5:      RESAMPLE(0x20)
6:      RESAMPLE(0x28)
7:      RESAMPLE(0x30)
8:      RESAMPLE(0x38)
9:      RESAMPLE(0x40)
10:     RESAMPLE(0x48)
11:     RESAMPLE(0x50)
12:     RESAMPLE(0x58)
13:     RESAMPLE(0x60)
14:     RESAMPLE(0x68)
15:     RESAMPLE(0x70)
16:     RESAMPLE(0x78)
17:     addl    $0x80,%edi
        decl    nCount
        jge     1b

#undef RESAMPLE

        popl    %eax
        subl    %eax,%esi               /* get and save accumulator */
        shrdl   $(32-ACCURACY),%esi,%ebp
        popl    %esi
        movl    %ebp,ACCUM(%esi)
        movb    %bl,RESERVED(%esi)      /* save sample for filtering */

        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        popl    %ebp
        ret

        ALIGN
18:     .long   17b
        .long   16b
        .long   15b
        .long   14b
        .long   13b
        .long   12b
        .long   11b
        .long   10b
        .long   9b
        .long   8b
        .long   7b
        .long   6b
        .long   5b
        .long   4b
        .long   3b
        .long   2b

