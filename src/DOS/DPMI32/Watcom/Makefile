# Makefile for WATCOM C/C++32 10.0 (DOS4GW protected mode, flat memory model)

CC = wcc386
LD = wcl386
AS = tasm
AR = wlib -s -t -q -n
CFLAGS = -zq -zu -w9 -we -5r -s -oxt -D__DPMI__ -D__ASM__
AFLAGS = -t -m -q -ml -zi
LFLAGS = -l=dos4g -zq

#AS = wasm
#AFLAGS = -zq -w9 -we -5r

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj \
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj msdos.obj \
	sbdrv.obj awedrv.obj pasdrv.obj wssdrv.obj gusdrv.obj ariadrv.obj

LIB = "..\lib\DOS\audiowcf.lib"

all : mp.exe $(LIB) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(LD) mp.obj $(LIB) $(LFLAGS)

$(LIB) : $(OBJS)
	@echo $(OBJS) > audiowcf.lnk
	$(AR) $(LIB) @audiowcf.lnk
	@del audiowcf.lnk
	
.SUFFIXES: .c .asm .obj

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean : .SYMBOLIC
	@if exist *.obj del *.obj > nul
