# Makefile for Borland C++ 4.5 (Win16 protected mode, large memory model)

CC = bcc
AS = tasm
AR = tlib
CFLAGS = -ml -W -c -w9 -3 -O2 -D__WINDOWS__ -D__16BIT__
AFLAGS = -t -m -q -zi
LFLAGS = -ml -W

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj \
	iofile.obj modeng.obj nondrv.obj mixdrv.obj windrv.obj

LIB = ..\lib\Win16\audw16bc.lib

all : mp.exe $(LIB) clean
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) $(LFLAGS) mp.obj $(LIB)

$(LIB) : $(OBJS)
	@if exist $(LIB) del $(LIB) > nul
	$(AR) $(LIB) @Windows\Win16\Borland\audw16bc.lnk

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
