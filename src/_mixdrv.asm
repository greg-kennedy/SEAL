;*
;* $Id: _mixdrv.asm 1.8 1996/08/15 02:33:23 chasan released $
;*
;* Low-level assembly resampling and quantization routines.
;* Optimized for superscalar architecture of Pentium processors.
;*
;* Copyright (C) 1995-1999 Carlos Hasan
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU Lesser General Public License as published
;* by the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*

        .386p
        .model  flat

; voice structure field offsets
Voice   struc
LPDATA          dd      ?
ACCUM           dd      ?
PITCH           dd      ?
LOOPSTART       dd      ?
LOOPEND         dd      ?
VOLUME          db      ?
PANNING         db      ?
CONTROL         db      ?
RESERVED        db      ?
Voice   ends


; pitch shift accuracy
ACCURACY        equ     0ch

        .data

        extrn   _lpVolumeTable:dword
        extrn   _lpFilterTable:dword

        align   4
nCount          dd      ?
nStepLo         dd      ?
nStepHi         dd      ?

        .code

; Borland C++ 4.5 calling convention
        public  QuantAudioData08
        public  QuantAudioData16
        public  MixAudioData08M
        public  MixAudioData08S
        public  MixAudioData08MI
        public  MixAudioData08SI
        public  MixAudioData16M
        public  MixAudioData16S
        public  MixAudioData16MI
        public  MixAudioData16SI

; WATCOM C/C++ 10.0 calling convention
        public  _QuantAudioData08
        public  _QuantAudioData16
        public  _MixAudioData08M
        public  _MixAudioData08S
        public  _MixAudioData08MI
        public  _MixAudioData08SI
        public  _MixAudioData16M
        public  _MixAudioData16S
        public  _MixAudioData16MI
        public  _MixAudioData16SI

; Visual C++ 4.1 calling convention
        public  _QuantAudioData08@12
        public  _QuantAudioData16@12
        public  _MixAudioData08M@12
        public  _MixAudioData08S@12
        public  _MixAudioData08MI@12
        public  _MixAudioData08SI@12
        public  _MixAudioData16M@12
        public  _MixAudioData16S@12
        public  _MixAudioData16MI@12
        public  _MixAudioData16SI@12


;
; VOID cdecl QuantAudioData08(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
;
        align   4

QuantAudioData08 proc
_QuantAudioData08:
_QuantAudioData08@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        mov     edi,[ebp+08h]           ; get parameters
        mov     esi,[ebp+0ch]
        mov     ecx,[ebp+10h]
        mov     ebx,08000h
        mov     edx,0ffffh
        dec     edi
        align   4
L1:     mov     eax,[esi]               ; 5 cycles/sample
        inc     edi
        add     eax,ebx
        jl      L3
        cmp     eax,edx
        jg      L4
L2:     add     esi,4
        mov     [edi],ah
        dec     ecx
        jg      L1
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret
        align   4
L3:     xor     eax,eax
        jmp     L2
        align   4
L4:     mov     eax,edx
        jmp     L2
QuantAudioData08 endp


;
; VOID cdecl QuantAudioData16(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
;
        align  4
QuantAudioData16 proc
_QuantAudioData16:
_QuantAudioData16@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
        mov     edi,[ebp+08h]           ; get parameters
        mov     esi,[ebp+0ch]
        mov     ecx,[ebp+10h]
        mov     ebx,-32768
        mov     edx,+32767
        sub     edi,2
        align   4
L5:     mov     eax,[esi]               ; 5 cycles/sample
        add     edi,2
        cmp     eax,ebx
        jl      L7
        cmp     eax,edx
        jg      L8
L6:     add     esi,4
        mov     [edi],ax
        dec     ecx
        jg      L5
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret
        align   4
L7:     mov     eax,ebx
        jmp     L6
        align   4
L8:     mov     eax,edx
        jmp     L6
QuantAudioData16 endp


;=========================== 8 bit mixing routines ===========================

;
; VOID cdecl MixAudioData08M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align   4
MixAudioData08M proc
_MixAudioData08M:
_MixAudioData08M@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

__MixAudioData08M:
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[esi]                ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*4+(-40h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL08M+eax*4]

RESAMPLE08M macro disp
        mov     edx,[edi+4*disp]        ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     eax,edx
        add     ecx,ebp
        mov     bl,[esi]
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     [edi+4*disp],eax
E08M&disp:
        endm

        align   4
L08M:   RESAMPLE08M 0
        RESAMPLE08M 1
        RESAMPLE08M 2
        RESAMPLE08M 3
        RESAMPLE08M 4
        RESAMPLE08M 5
        RESAMPLE08M 6
        RESAMPLE08M 7
        RESAMPLE08M 8
        RESAMPLE08M 9
        RESAMPLE08M 10
        RESAMPLE08M 11
        RESAMPLE08M 12
        RESAMPLE08M 13
        RESAMPLE08M 14
        RESAMPLE08M 15
        add     edi,40h
        dec     [nCount]
        jge     L08M

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL08M  dd      offset E08M15
        dd      offset E08M14
        dd      offset E08M13
        dd      offset E08M12
        dd      offset E08M11
        dd      offset E08M10
        dd      offset E08M9
        dd      offset E08M8
        dd      offset E08M7
        dd      offset E08M6
        dd      offset E08M5
        dd      offset E08M4
        dd      offset E08M3
        dd      offset E08M2
        dd      offset E08M1
        dd      offset E08M0
MixAudioData08M endp


;
; VOID cdecl MixAudioData08S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData08S proc
_MixAudioData08S:
_MixAudioData08S@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

__MixAudioData08S:
        mov     al,[esi+PANNING]        ; get panning
        cmp     al,10h
        jb      MixAudioLeft08S         ; full left?
        cmp     al,78h
        jb      MixAudioPan08S          ; left?
        cmp     al,88h
        jb      MixAudioMiddle08S       ; center?
        cmp     al,0f0h
        jae     MixAudioRight08S        ; full right?
MixAudioData08S endp

;
; 8-bit general stereo mixing routine
;
        align   4
MixAudioPan08S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get voice parameters
        mov     edx,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        movzx   ecx,[esi+PANNING]
        mov     esi,[esi+LPDATA]

        push    esi                     ; get volume table addresses
        imul    ecx,ebx
        shr     ecx,8
        sub     ebx,ecx
        shl     ebx,10
        shl     ecx,10
        add     ebx,[_lpVolumeTable]
        add     ecx,[_lpVolumeTable]
        shr     ebx,2
        shr     ecx,2

        mov     eax,ebp                 ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shl     ebp,32-ACCURACY
        add     esi,eax
        mov     eax,edx
        sar     edx,ACCURACY
        shl     eax,32-ACCURACY
        mov     [nStepLo],eax
        mov     [nStepHi],edx

        mov     bl,[esi]                ; get first sample and advance
        add     ebp,eax
        adc     esi,edx
        mov     cl,bl

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL08S+eax*4]

RESAMPLE08S macro disp
        mov     eax,[ebx*4]             ; 7 cycles/sample
        mov     edx,[edi+8*disp]
        add     eax,edx
        mov     edx,[edi+8*disp+4]
        mov     [edi+8*disp],eax
        mov     eax,[ecx*4]
        add     eax,edx
        mov     bl,[esi]
        mov     [edi+8*disp+4],eax
        mov     eax,[nStepLo]
        add     ebp,eax
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     cl,bl
E08S&disp:
        endm

        align   4
L08S:   RESAMPLE08S 0
        RESAMPLE08S 1
        RESAMPLE08S 2
        RESAMPLE08S 3
        RESAMPLE08S 4
        RESAMPLE08S 5
        RESAMPLE08S 6
        RESAMPLE08S 7
        RESAMPLE08S 8
        RESAMPLE08S 9
        RESAMPLE08S 10
        RESAMPLE08S 11
        RESAMPLE08S 12
        RESAMPLE08S 13
        RESAMPLE08S 14
        RESAMPLE08S 15
        add     edi,80h
        dec     [nCount]
        jge     L08S

        mov     eax,[nStepLo]           ; go back one step
        mov     edx,[nStepHi]
        sub     ebp,eax
        sbb     esi,edx
        pop     eax                     ; get and save accumulator
        sub     esi,eax
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL08S  dd      offset E08S15
        dd      offset E08S14
        dd      offset E08S13
        dd      offset E08S12
        dd      offset E08S11
        dd      offset E08S10
        dd      offset E08S9
        dd      offset E08S8
        dd      offset E08S7
        dd      offset E08S6
        dd      offset E08S5
        dd      offset E08S4
        dd      offset E08S3
        dd      offset E08S2
        dd      offset E08S1
        dd      offset E08S0
MixAudioPan08S endp

;
; 8-bit full right stereo mixing routine
;
        align   4
MixAudioRight08S proc
        add     edi,4
MixAudioRight08S endp

;
; 8-bit full left stereo mixing routine
;
        align   4
MixAudioLeft08S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[esi]                ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL08L+eax*4]

RESAMPLE08L macro disp
        mov     edx,[edi+8*disp]        ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     eax,edx
        add     ecx,ebp
        mov     bl,[esi]
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     [edi+8*disp],eax
E08L&disp:
        endm

        align   4
L08L:   RESAMPLE08L 0
        RESAMPLE08L 1
        RESAMPLE08L 2
        RESAMPLE08L 3
        RESAMPLE08L 4
        RESAMPLE08L 5
        RESAMPLE08L 6
        RESAMPLE08L 7
        RESAMPLE08L 8
        RESAMPLE08L 9
        RESAMPLE08L 10
        RESAMPLE08L 11
        RESAMPLE08L 12
        RESAMPLE08L 13
        RESAMPLE08L 14
        RESAMPLE08L 15
        add     edi,80h
        dec     [nCount]
        jge     L08L

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL08L  dd      offset E08L15
        dd      offset E08L14
        dd      offset E08L13
        dd      offset E08L12
        dd      offset E08L11
        dd      offset E08L10
        dd      offset E08L9
        dd      offset E08L8
        dd      offset E08L7
        dd      offset E08L6
        dd      offset E08L5
        dd      offset E08L4
        dd      offset E08L3
        dd      offset E08L2
        dd      offset E08L1
        dd      offset E08L0
MixAudioLeft08S endp

;
; 8-bit middle stereo mixing routine
;
        align   4
MixAudioMiddle08S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shr     ebx,1
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[esi]                ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL08C+eax*4]

RESAMPLE08C macro disp
        mov     edx,[edi+8*disp+0]      ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     edx,eax
        mov     bl,[esi]
        mov     [edi+8*disp+0],edx
        mov     edx,[edi+8*disp+4]
        add     edx,eax
        mov     eax,[nStepHi]
        mov     [edi+8*disp+4],edx
        add     ecx,ebp
        adc     esi,eax
E08C&disp:
        endm

        align   4
L08C:   RESAMPLE08C 0
        RESAMPLE08C 1
        RESAMPLE08C 2
        RESAMPLE08C 3
        RESAMPLE08C 4
        RESAMPLE08C 5
        RESAMPLE08C 6
        RESAMPLE08C 7
        RESAMPLE08C 8
        RESAMPLE08C 9
        RESAMPLE08C 10
        RESAMPLE08C 11
        RESAMPLE08C 12
        RESAMPLE08C 13
        RESAMPLE08C 14
        RESAMPLE08C 15
        add     edi,80h
        dec     [nCount]
        jge     L08C

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL08C  dd      offset E08C15
        dd      offset E08C14
        dd      offset E08C13
        dd      offset E08C12
        dd      offset E08C11
        dd      offset E08C10
        dd      offset E08C9
        dd      offset E08C8
        dd      offset E08C7
        dd      offset E08C6
        dd      offset E08C5
        dd      offset E08C4
        dd      offset E08C3
        dd      offset E08C2
        dd      offset E08C1
        dd      offset E08C0
MixAudioMiddle08S endp



;
; VOID cdecl MixAudioData08MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData08MI proc
_MixAudioData08MI:
_MixAudioData08MI@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

        mov     eax,[esi+PITCH]         ; select mixing routine
        cmp     eax,+(1 SHL ACCURACY)
        jge     __MixAudioData08M
        cmp     eax,-(1 SHL ACCURACY)
        jle     __MixAudioData08M

        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        push    esi

        shl     ebx,10                  ; get volume table address
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L10
        neg     edx
L10:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*4+(-40h)]
        shr     [nCount],4
        jmp     dword ptr [TBL08MI+eax*4]

RESAMPLE08MI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[esi]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+4*disp],eax
E08MI&disp:
        endm

L08MI:  RESAMPLE08MI 0                  ; 11 cycles/sample
        RESAMPLE08MI 1
        RESAMPLE08MI 2
        RESAMPLE08MI 3
        RESAMPLE08MI 4
        RESAMPLE08MI 5
        RESAMPLE08MI 6
        RESAMPLE08MI 7
        RESAMPLE08MI 8
        RESAMPLE08MI 9
        RESAMPLE08MI 10
        RESAMPLE08MI 11
        RESAMPLE08MI 12
        RESAMPLE08MI 13
        RESAMPLE08MI 14
        RESAMPLE08MI 15
        add     edi,40h
        dec     [nCount]
        jge     L08MI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL08MI dd      offset E08MI15
        dd      offset E08MI14
        dd      offset E08MI13
        dd      offset E08MI12
        dd      offset E08MI11
        dd      offset E08MI10
        dd      offset E08MI9
        dd      offset E08MI8
        dd      offset E08MI7
        dd      offset E08MI6
        dd      offset E08MI5
        dd      offset E08MI4
        dd      offset E08MI3
        dd      offset E08MI2
        dd      offset E08MI1
        dd      offset E08MI0
MixAudioData08MI endp

;
; VOID cdecl MixAudioData08SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData08SI proc
_MixAudioData08SI:
_MixAudioData08SI@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

        mov     eax,[esi+PITCH]         ; select mixing routine
        cmp     eax,+(1 SHL ACCURACY)
        jge     __MixAudioData08S
        cmp     eax,-(1 SHL ACCURACY)
        jle     __MixAudioData08S

        mov     al,[esi+PANNING]        ; select panning routine
        cmp     al,10h
        jb      MixAudioLeft08SI
        cmp     al,78h
        jb      MixAudioPan08SI
        cmp     al,88h
        jb      MixAudioMiddle08SI
        cmp     al,0f0h
        jae     MixAudioRight08SI
MixAudioData08SI endp


;
; 8-bit general stereo mixing routine
;
        align   4
MixAudioPan08SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get voice parameters
        mov     edx,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        movzx   ecx,[esi+PANNING]
        mov     al,[esi+RESERVED]
        mov     esi,[esi+LPDATA]

        push    esi                     ; get volume table addresses
        imul    ecx,ebx
        shr     ecx,8
        sub     ebx,ecx
        shl     ebx,10
        shl     ecx,10
        add     ebx,[_lpVolumeTable]
        add     ecx,[_lpVolumeTable]
        shr     ebx,2
        shr     ecx,2
        mov     bl,al                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     eax,edx                 ; convert pitch
        test    edx,edx
        jge     L11
        neg     edx
L11:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]
        push    eax
        shl     eax,32-ACCURACY
        mov     [nStepLo],eax
        pop     eax
        sar     eax,ACCURACY
        mov     [nStepHi],eax

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL08PI+eax*4]

RESAMPLE08PI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[esi]
        mov     dl,[edx]
        mov     eax,[nStepLo]
        add     bl,dl
        add     ebp,eax
        mov     cl,bl
        mov     eax,[nStepHi]
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
        mov     eax,[ecx*4]
        add     [edi+8*disp+4],eax
E08PI&disp:
        endm

        align   4
L08PI:  RESAMPLE08PI 0                  ; 15 cycles/sample
        RESAMPLE08PI 1
        RESAMPLE08PI 2
        RESAMPLE08PI 3
        RESAMPLE08PI 4
        RESAMPLE08PI 5
        RESAMPLE08PI 6
        RESAMPLE08PI 7
        RESAMPLE08PI 8
        RESAMPLE08PI 9
        RESAMPLE08PI 10
        RESAMPLE08PI 11
        RESAMPLE08PI 12
        RESAMPLE08PI 13
        RESAMPLE08PI 14
        RESAMPLE08PI 15
        add     edi,80h
        dec     [nCount]
        jge     L08PI

        pop     eax                     ; get and save accumulator
        sub     esi,eax
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL08PI dd      offset E08PI15
        dd      offset E08PI14
        dd      offset E08PI13
        dd      offset E08PI12
        dd      offset E08PI11
        dd      offset E08PI10
        dd      offset E08PI9
        dd      offset E08PI8
        dd      offset E08PI7
        dd      offset E08PI6
        dd      offset E08PI5
        dd      offset E08PI4
        dd      offset E08PI3
        dd      offset E08PI2
        dd      offset E08PI1
        dd      offset E08PI0
MixAudioPan08SI endp

;
; 8-bit full right stereo mixing routine
;
        align   4
MixAudioRight08SI proc
        add     edi,4
MixAudioRight08SI endp

;
; 8-bit full left stereo mixing routine
;
        align  4
MixAudioLeft08SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        push    esi

        shl     ebx,10                  ; get volume table address
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L12
        neg     edx
L12:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL08LI+eax*4]

RESAMPLE08LI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[esi]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
E08LI&disp:
        endm

L08LI:  RESAMPLE08LI 0                  ; 11 cycles/sample
        RESAMPLE08LI 1
        RESAMPLE08LI 2
        RESAMPLE08LI 3
        RESAMPLE08LI 4
        RESAMPLE08LI 5
        RESAMPLE08LI 6
        RESAMPLE08LI 7
        RESAMPLE08LI 8
        RESAMPLE08LI 9
        RESAMPLE08LI 10
        RESAMPLE08LI 11
        RESAMPLE08LI 12
        RESAMPLE08LI 13
        RESAMPLE08LI 14
        RESAMPLE08LI 15
        add     edi,80h
        dec     [nCount]
        jge     L08LI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL08LI dd      offset E08LI15
        dd      offset E08LI14
        dd      offset E08LI13
        dd      offset E08LI12
        dd      offset E08LI11
        dd      offset E08LI10
        dd      offset E08LI9
        dd      offset E08LI8
        dd      offset E08LI7
        dd      offset E08LI6
        dd      offset E08LI5
        dd      offset E08LI4
        dd      offset E08LI3
        dd      offset E08LI2
        dd      offset E08LI1
        dd      offset E08LI0
MixAudioLeft08SI endp


;
; 8-bit middle stereo mixing routine
;
        align  4
MixAudioMiddle08SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        push    esi

        shr     ebx,1                   ; get volume table address
        shl     ebx,10
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L13
        neg     edx
L13:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL08CI+eax*4]

RESAMPLE08CI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[esi]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
        add     [edi+8*disp+4],eax
E08CI&disp:
        endm

L08CI:  RESAMPLE08CI 0                  ; 14 cycles/sample
        RESAMPLE08CI 1
        RESAMPLE08CI 2
        RESAMPLE08CI 3
        RESAMPLE08CI 4
        RESAMPLE08CI 5
        RESAMPLE08CI 6
        RESAMPLE08CI 7
        RESAMPLE08CI 8
        RESAMPLE08CI 9
        RESAMPLE08CI 10
        RESAMPLE08CI 11
        RESAMPLE08CI 12
        RESAMPLE08CI 13
        RESAMPLE08CI 14
        RESAMPLE08CI 15
        add     edi,80h
        dec     [nCount]
        jge     L08CI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL08CI dd      offset E08CI15
        dd      offset E08CI14
        dd      offset E08CI13
        dd      offset E08CI12
        dd      offset E08CI11
        dd      offset E08CI10
        dd      offset E08CI9
        dd      offset E08CI8
        dd      offset E08CI7
        dd      offset E08CI6
        dd      offset E08CI5
        dd      offset E08CI4
        dd      offset E08CI3
        dd      offset E08CI2
        dd      offset E08CI1
        dd      offset E08CI0
MixAudioMiddle08SI endp

;========================= fake 16 bit mixing routines ========================;

;
; VOID cdecl MixAudioData16M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align   4
MixAudioData16M proc
_MixAudioData16M:
_MixAudioData16M@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

__MixAudioData16M:
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shr     esi,1
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[2*esi+1]            ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*4+(-40h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL16M+eax*4]

RESAMPLE16M macro disp
        mov     edx,[edi+4*disp]        ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     eax,edx
        add     ecx,ebp
        mov     bl,[2*esi+1]
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     [edi+4*disp],eax
E16M&disp:
        endm

        align   4
L16M:   RESAMPLE16M 0
        RESAMPLE16M 1
        RESAMPLE16M 2
        RESAMPLE16M 3
        RESAMPLE16M 4
        RESAMPLE16M 5
        RESAMPLE16M 6
        RESAMPLE16M 7
        RESAMPLE16M 8
        RESAMPLE16M 9
        RESAMPLE16M 10
        RESAMPLE16M 11
        RESAMPLE16M 12
        RESAMPLE16M 13
        RESAMPLE16M 14
        RESAMPLE16M 15
        add     edi,40h
        dec     [nCount]
        jge     L16M

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL16M  dd      offset E16M15
        dd      offset E16M14
        dd      offset E16M13
        dd      offset E16M12
        dd      offset E16M11
        dd      offset E16M10
        dd      offset E16M9
        dd      offset E16M8
        dd      offset E16M7
        dd      offset E16M6
        dd      offset E16M5
        dd      offset E16M4
        dd      offset E16M3
        dd      offset E16M2
        dd      offset E16M1
        dd      offset E16M0
MixAudioData16M endp


;
; VOID cdecl MixAudioData16S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData16S proc
_MixAudioData16S:
_MixAudioData16S@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

__MixAudioData16S:
        mov     al,[esi+PANNING]        ; get panning
        cmp     al,10h
        jb      MixAudioLeft16S         ; full left?
        cmp     al,78h
        jb      MixAudioPan16S          ; left?
        cmp     al,88h
        jb      MixAudioMiddle16S       ; center?
        cmp     al,0f0h
        jae     MixAudioRight16S        ; full right?
MixAudioData16S endp

;
; 16-bit general stereo mixing routine
;
        align   4
MixAudioPan16S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get voice parameters
        mov     edx,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        movzx   ecx,[esi+PANNING]
        mov     esi,[esi+LPDATA]
        shr     esi,1

        push    esi                     ; get volume table addresses
        imul    ecx,ebx
        shr     ecx,8
        sub     ebx,ecx
        shl     ebx,10
        shl     ecx,10
        add     ebx,[_lpVolumeTable]
        add     ecx,[_lpVolumeTable]
        shr     ebx,2
        shr     ecx,2

        mov     eax,ebp                 ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shl     ebp,32-ACCURACY
        add     esi,eax
        mov     eax,edx
        sar     edx,ACCURACY
        shl     eax,32-ACCURACY
        mov     [nStepLo],eax
        mov     [nStepHi],edx

        mov     bl,[2*esi+1]            ; get first sample and advance
        add     ebp,eax
        adc     esi,edx
        mov     cl,bl

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL16S+eax*4]

RESAMPLE16S macro disp
        mov     eax,[ebx*4]             ; 7 cycles/sample
        mov     edx,[edi+8*disp]
        add     eax,edx
        mov     edx,[edi+8*disp+4]
        mov     [edi+8*disp],eax
        mov     eax,[ecx*4]
        add     eax,edx
        mov     bl,[2*esi+1]
        mov     [edi+8*disp+4],eax
        mov     eax,[nStepLo]
        add     ebp,eax
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     cl,bl
E16S&disp:
        endm

        align   4
L16S:   RESAMPLE16S 0
        RESAMPLE16S 1
        RESAMPLE16S 2
        RESAMPLE16S 3
        RESAMPLE16S 4
        RESAMPLE16S 5
        RESAMPLE16S 6
        RESAMPLE16S 7
        RESAMPLE16S 8
        RESAMPLE16S 9
        RESAMPLE16S 10
        RESAMPLE16S 11
        RESAMPLE16S 12
        RESAMPLE16S 13
        RESAMPLE16S 14
        RESAMPLE16S 15
        add     edi,80h
        dec     [nCount]
        jge     L16S

        mov     eax,[nStepLo]           ; go back one step
        mov     edx,[nStepHi]
        sub     ebp,eax
        sbb     esi,edx
        pop     eax                     ; get and save accumulator
        sub     esi,eax
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL16S  dd      offset E16S15
        dd      offset E16S14
        dd      offset E16S13
        dd      offset E16S12
        dd      offset E16S11
        dd      offset E16S10
        dd      offset E16S9
        dd      offset E16S8
        dd      offset E16S7
        dd      offset E16S6
        dd      offset E16S5
        dd      offset E16S4
        dd      offset E16S3
        dd      offset E16S2
        dd      offset E16S1
        dd      offset E16S0
MixAudioPan16S endp

;
; 16-bit full right stereo mixing routine
;
        align   4
MixAudioRight16S proc
        add     edi,4
MixAudioRight16S endp

;
; 16-bit full left stereo mixing routine
;
        align   4
MixAudioLeft16S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shr     esi,1
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[2*esi+1]            ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL16L+eax*4]

RESAMPLE16L macro disp
        mov     edx,[edi+8*disp]        ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     eax,edx
        add     ecx,ebp
        mov     bl,[2*esi+1]
        mov     edx,[nStepHi]
        adc     esi,edx
        mov     [edi+8*disp],eax
E16L&disp:
        endm

        align   4
L16L:   RESAMPLE16L 0
        RESAMPLE16L 1
        RESAMPLE16L 2
        RESAMPLE16L 3
        RESAMPLE16L 4
        RESAMPLE16L 5
        RESAMPLE16L 6
        RESAMPLE16L 7
        RESAMPLE16L 8
        RESAMPLE16L 9
        RESAMPLE16L 10
        RESAMPLE16L 11
        RESAMPLE16L 12
        RESAMPLE16L 13
        RESAMPLE16L 14
        RESAMPLE16L 15
        add     edi,80h
        dec     [nCount]
        jge     L16L

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL16L  dd      offset E16L15
        dd      offset E16L14
        dd      offset E16L13
        dd      offset E16L12
        dd      offset E16L11
        dd      offset E16L10
        dd      offset E16L9
        dd      offset E16L8
        dd      offset E16L7
        dd      offset E16L6
        dd      offset E16L5
        dd      offset E16L4
        dd      offset E16L3
        dd      offset E16L2
        dd      offset E16L1
        dd      offset E16L0
MixAudioLeft16S endp

;
; 16-bit middle stereo mixing routine
;
        align   4
MixAudioMiddle16S proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ecx,[esi+ACCUM]         ; get voice parameters
        mov     ebp,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        mov     esi,[esi+LPDATA]
        shr     esi,1
        shr     ebx,1
        shl     ebx,10                  ; get volume table address
        push    esi
        mov     eax,ecx
        add     ebx,[_lpVolumeTable]    ; convert accumulator and pitch
        sar     eax,ACCURACY            ; to 32.32 fixed point
        shr     ebx,2
        shl     ecx,32-ACCURACY
        mov     edx,ebp
        add     esi,eax
        sar     edx,ACCURACY
        shl     ebp,32-ACCURACY
        mov     [nStepHi],edx

        mov     bl,[2*esi+1]            ; get first sample and advance
        add     ecx,ebp
        adc     esi,edx

        mov     eax,[nCount]            ; jump inside of the unrolled loop
        mov     edx,eax
        and     eax,0fh
        shr     edx,4
        lea     edi,[edi+eax*8+(-80h)]
        mov     [nCount],edx
        jmp     dword ptr [TBL16C+eax*4]

RESAMPLE16C macro disp
        mov     edx,[edi+8*disp+0]      ; 4 cycles/sample
        mov     eax,[ebx*4]
        add     edx,eax
        mov     bl,[2*esi+1]
        mov     [edi+8*disp+0],edx
        mov     edx,[edi+8*disp+4]
        add     edx,eax
        mov     eax,[nStepHi]
        mov     [edi+8*disp+4],edx
        add     ecx,ebp
        adc     esi,eax
E16C&disp:
        endm

        align   4
L16C:   RESAMPLE16C 0
        RESAMPLE16C 1
        RESAMPLE16C 2
        RESAMPLE16C 3
        RESAMPLE16C 4
        RESAMPLE16C 5
        RESAMPLE16C 6
        RESAMPLE16C 7
        RESAMPLE16C 8
        RESAMPLE16C 9
        RESAMPLE16C 10
        RESAMPLE16C 11
        RESAMPLE16C 12
        RESAMPLE16C 13
        RESAMPLE16C 14
        RESAMPLE16C 15
        add     edi,80h
        dec     [nCount]
        jge     L16C

        mov     eax,[nStepHi]           ; go back one step
        sub     ecx,ebp
        sbb     esi,eax
        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ecx,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ecx
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align   4
TBL16C  dd      offset E16C15
        dd      offset E16C14
        dd      offset E16C13
        dd      offset E16C12
        dd      offset E16C11
        dd      offset E16C10
        dd      offset E16C9
        dd      offset E16C8
        dd      offset E16C7
        dd      offset E16C6
        dd      offset E16C5
        dd      offset E16C4
        dd      offset E16C3
        dd      offset E16C2
        dd      offset E16C1
        dd      offset E16C0
MixAudioMiddle16S endp



;
; VOID cdecl MixAudioData16MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData16MI proc
_MixAudioData16MI:
_MixAudioData16MI@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

        mov     eax,[esi+PITCH]         ; select mixing routine
        cmp     eax,+(1 SHL ACCURACY)
        jge     __MixAudioData16M
        cmp     eax,-(1 SHL ACCURACY)
        jle     __MixAudioData16M

        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        shr     esi,1
        push    esi

        shl     ebx,10                  ; get volume table address
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L20
        neg     edx
L20:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*4+(-40h)]
        shr     [nCount],4
        jmp     dword ptr [TBL16MI+eax*4]

RESAMPLE16MI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[2*esi+1]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+4*disp],eax
E16MI&disp:
        endm

L16MI:  RESAMPLE16MI 0                  ; 11 cycles/sample
        RESAMPLE16MI 1
        RESAMPLE16MI 2
        RESAMPLE16MI 3
        RESAMPLE16MI 4
        RESAMPLE16MI 5
        RESAMPLE16MI 6
        RESAMPLE16MI 7
        RESAMPLE16MI 8
        RESAMPLE16MI 9
        RESAMPLE16MI 10
        RESAMPLE16MI 11
        RESAMPLE16MI 12
        RESAMPLE16MI 13
        RESAMPLE16MI 14
        RESAMPLE16MI 15
        add     edi,40h
        dec     [nCount]
        jge     L16MI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL16MI dd      offset E16MI15
        dd      offset E16MI14
        dd      offset E16MI13
        dd      offset E16MI12
        dd      offset E16MI11
        dd      offset E16MI10
        dd      offset E16MI9
        dd      offset E16MI8
        dd      offset E16MI7
        dd      offset E16MI6
        dd      offset E16MI5
        dd      offset E16MI4
        dd      offset E16MI3
        dd      offset E16MI2
        dd      offset E16MI1
        dd      offset E16MI0
MixAudioData16MI endp

;
; VOID cdecl MixAudioData16SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
;
        align  4
MixAudioData16SI proc
_MixAudioData16SI:
_MixAudioData16SI@12:
        push    ebp
        mov     ebp,esp
        push    eax
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi

        mov     edi,[ebp+08h]           ; get parameters
        mov     ecx,[ebp+0ch]
        mov     esi,[ebp+10h]

        mov     eax,[esi+PITCH]         ; select mixing routine
        cmp     eax,+(1 SHL ACCURACY)
        jge     __MixAudioData16S
        cmp     eax,-(1 SHL ACCURACY)
        jle     __MixAudioData16S

        mov     al,[esi+PANNING]        ; select panning routine
        cmp     al,10h
        jb      MixAudioLeft16SI
        cmp     al,78h
        jb      MixAudioPan16SI
        cmp     al,88h
        jb      MixAudioMiddle16SI
        cmp     al,0f0h
        jae     MixAudioRight16SI
MixAudioData16SI endp


;
; 16-bit general stereo mixing routine
;
        align   4
MixAudioPan16SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get voice parameters
        mov     edx,[esi+PITCH]
        movzx   ebx,[esi+VOLUME]
        movzx   ecx,[esi+PANNING]
        mov     al,[esi+RESERVED]
        mov     esi,[esi+LPDATA]
        shr     esi,1

        push    esi                     ; get volume table addresses
        imul    ecx,ebx
        shr     ecx,8
        sub     ebx,ecx
        shl     ebx,10
        shl     ecx,10
        add     ebx,[_lpVolumeTable]
        add     ecx,[_lpVolumeTable]
        shr     ebx,2
        shr     ecx,2
        mov     bl,al                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     eax,edx                 ; convert pitch
        test    edx,edx
        jge     L21
        neg     edx
L21:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]
        push    eax
        shl     eax,32-ACCURACY
        mov     [nStepLo],eax
        pop     eax
        sar     eax,ACCURACY
        mov     [nStepHi],eax

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL16PI+eax*4]

RESAMPLE16PI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[2*esi+1]
        mov     dl,[edx]
        mov     eax,[nStepLo]
        add     bl,dl
        add     ebp,eax
        mov     cl,bl
        mov     eax,[nStepHi]
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
        mov     eax,[ecx*4]
        add     [edi+8*disp+4],eax
E16PI&disp:
        endm

        align   4
L16PI:  RESAMPLE16PI 0                  ; 15 cycles/sample
        RESAMPLE16PI 1
        RESAMPLE16PI 2
        RESAMPLE16PI 3
        RESAMPLE16PI 4
        RESAMPLE16PI 5
        RESAMPLE16PI 6
        RESAMPLE16PI 7
        RESAMPLE16PI 8
        RESAMPLE16PI 9
        RESAMPLE16PI 10
        RESAMPLE16PI 11
        RESAMPLE16PI 12
        RESAMPLE16PI 13
        RESAMPLE16PI 14
        RESAMPLE16PI 15
        add     edi,80h
        dec     [nCount]
        jge     L16PI

        pop     eax                     ; get and save accumulator
        sub     esi,eax
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL16PI dd      offset E16PI15
        dd      offset E16PI14
        dd      offset E16PI13
        dd      offset E16PI12
        dd      offset E16PI11
        dd      offset E16PI10
        dd      offset E16PI9
        dd      offset E16PI8
        dd      offset E16PI7
        dd      offset E16PI6
        dd      offset E16PI5
        dd      offset E16PI4
        dd      offset E16PI3
        dd      offset E16PI2
        dd      offset E16PI1
        dd      offset E16PI0
MixAudioPan16SI endp

;
; 16-bit full right stereo mixing routine
;
        align   4
MixAudioRight16SI proc
        add     edi,4
MixAudioRight16SI endp

;
; 16-bit full left stereo mixing routine
;
        align  4
MixAudioLeft16SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        shr     esi,1
        push    esi

        shl     ebx,10                  ; get volume table address
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L22
        neg     edx
L22:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL16LI+eax*4]

RESAMPLE16LI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[2*esi+1]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
E16LI&disp:
        endm

L16LI:  RESAMPLE16LI 0                  ; 11 cycles/sample
        RESAMPLE16LI 1
        RESAMPLE16LI 2
        RESAMPLE16LI 3
        RESAMPLE16LI 4
        RESAMPLE16LI 5
        RESAMPLE16LI 6
        RESAMPLE16LI 7
        RESAMPLE16LI 8
        RESAMPLE16LI 9
        RESAMPLE16LI 10
        RESAMPLE16LI 11
        RESAMPLE16LI 12
        RESAMPLE16LI 13
        RESAMPLE16LI 14
        RESAMPLE16LI 15
        add     edi,80h
        dec     [nCount]
        jge     L16LI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL16LI dd      offset E16LI15
        dd      offset E16LI14
        dd      offset E16LI13
        dd      offset E16LI12
        dd      offset E16LI11
        dd      offset E16LI10
        dd      offset E16LI9
        dd      offset E16LI8
        dd      offset E16LI7
        dd      offset E16LI6
        dd      offset E16LI5
        dd      offset E16LI4
        dd      offset E16LI3
        dd      offset E16LI2
        dd      offset E16LI1
        dd      offset E16LI0
MixAudioLeft16SI endp


;
; 16-bit middle stereo mixing routine
;
        align  4
MixAudioMiddle16SI proc
        mov     [nCount],ecx            ; save counter
        push    esi
        mov     ebp,[esi+ACCUM]         ; get accumulator
        mov     ecx,[esi+PITCH]         ; get pitch
        movzx   ebx,[esi+VOLUME]        ; get volume
        mov     dl,[esi+RESERVED]       ; get last sample
        mov     esi,[esi+LPDATA]        ; get data address
        shr     esi,1
        push    esi

        shr     ebx,1                   ; get volume table address
        shl     ebx,10
        add     ebx,[_lpVolumeTable]
        shr     ebx,2
        mov     bl,dl                   ; save sample for filtering

        mov     eax,ebp                 ; convert accumulator
        sar     eax,ACCURACY
        shl     ebp,32-ACCURACY
        add     esi,eax

        mov     edx,ecx                 ; get filter table address
        mov     eax,ecx
        shl     ecx,32-ACCURACY
        sar     eax,ACCURACY            ; convert pitch
        mov     [nStepHi],eax
        test    edx,edx
        jge     L23
        neg     edx
L23:    shr     edx,ACCURACY-5
        shl     edx,8
        add     edx,[_lpFilterTable]

        mov     eax,[nCount]            ; jump inside of the loop
        and     eax,0fh
        lea     edi,[edi+eax*8+(-80h)]
        shr     [nCount],4
        jmp     dword ptr [TBL16CI+eax*4]

RESAMPLE16CI macro disp
        mov     dl,bl
        sub     bl,[edx]
        mov     dl,[2*esi+1]
        mov     dl,[edx]
        mov     eax,[nStepHi]
        add     bl,dl
        add     ebp,ecx
        adc     esi,eax
        mov     eax,[ebx*4]
        add     [edi+8*disp],eax
        add     [edi+8*disp+4],eax
E16CI&disp:
        endm

L16CI:  RESAMPLE16CI 0                  ; 14 cycles/sample
        RESAMPLE16CI 1
        RESAMPLE16CI 2
        RESAMPLE16CI 3
        RESAMPLE16CI 4
        RESAMPLE16CI 5
        RESAMPLE16CI 6
        RESAMPLE16CI 7
        RESAMPLE16CI 8
        RESAMPLE16CI 9
        RESAMPLE16CI 10
        RESAMPLE16CI 11
        RESAMPLE16CI 12
        RESAMPLE16CI 13
        RESAMPLE16CI 14
        RESAMPLE16CI 15
        add     edi,80h
        dec     [nCount]
        jge     L16CI

        pop     eax
        sub     esi,eax                 ; get and save accumulator
        shrd    ebp,esi,32-ACCURACY
        pop     esi
        mov     [esi+ACCUM],ebp
        mov     [esi+RESERVED],bl

        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     ebp
        ret

        align  4
TBL16CI dd      offset E16CI15
        dd      offset E16CI14
        dd      offset E16CI13
        dd      offset E16CI12
        dd      offset E16CI11
        dd      offset E16CI10
        dd      offset E16CI9
        dd      offset E16CI8
        dd      offset E16CI7
        dd      offset E16CI6
        dd      offset E16CI5
        dd      offset E16CI4
        dd      offset E16CI3
        dd      offset E16CI2
        dd      offset E16CI1
        dd      offset E16CI0
MixAudioMiddle16SI endp

        end
