# Makefile for Borland C++ 4.5 (DPMI32 protected mode, flat memory model)

CC = bcc32a
AS = tasm
AR = tlib
CFLAGS = -WX -c -w9 -5 -O2 -D__DPMI__ -D__ASM__
AFLAGS = -t -m -q -ml -zi
LFLAGS = -WX

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj \
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj msdos.obj \
	sbdrv.obj awedrv.obj pasdrv.obj wssdrv.obj gusdrv.obj ariadrv.obj

LIB = ..\lib\DOS\audiobcf.lib

all : mp.exe $(LIB) clean
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) $(LFLAGS) mp.obj $(LIB)

$(LIB) : $(OBJS)
	@if exist $(LIB) del $(LIB) > nul
	$(AR) $(LIB) @DOS\DPMI32\Borland\audiobcf.lnk

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
