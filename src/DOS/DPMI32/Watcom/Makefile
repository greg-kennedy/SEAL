# Makefile for WATCOM C/C++32 10.0 (DOS4GW protected mode, flat memory model)

CC = wcl386
AS = tasm
AR = wlib -b -n -q
CFLAGS = -c -zq -zu -w9 -we -5r -s -oxt -D__DPMI__ -D__ASM__
AFLAGS = -t -m -q -ml -zi
LFLAGS = -l=dos4g -zq

#AS = wasm
#AFLAGS = -zq -w9 -we -5r

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj &
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj &
	msdos.obj sbdrv.obj pasdrv.obj wssdrv.obj gusdrv.obj ariadrv.obj

LIB = audiowcf.lib

all : mp.exe $(LIB) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) mp.obj $(LIB) $(LFLAGS)

$(LIB) : $(OBJS)
	*$(AR) $(LIB) $(OBJS)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean : .SYMBOLIC
	@if exist *.obj del *.obj > nul
