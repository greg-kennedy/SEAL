# Makefile for WATCOM C/C++16 10.0 (Win16 protected mode, large memory model)

CC = wcl
AS = tasm
AR = wlib -b -n -q
CFLAGS = -zq -zw -ml -c -w9 -we -5 -s -oxt -D__WINDOWS__ -D__16BIT__
AFLAGS = -t -m -q -ml -zi
LFLAGS = -zq -zw -ml

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj &
	iofile.obj modeng.obj nondrv.obj mixdrv.obj windrv.obj

LIB = ..\lib\Win16\audw16wc.lib

all : mp.exe $(LIB) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) $(LFLAGS) mp.obj $(LIB) mmsystem.lib

$(LIB) : $(OBJS)
	*$(AR) $(LIB) $(OBJS)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :  .SYMBOLIC
	@if exist *.obj del *.obj > nul
