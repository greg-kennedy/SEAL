# Makefile for Borland C++ 4.5 (DOS real mode, large memory model)

CC = bcc
AS = tasm
AR = tlib
CFLAGS = -ml -c -w9 -3 -O2 -D__DOS16__ -D__16BIT__
AFLAGS = -t -m -q -zi
LFLAGS = -ml

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj \
	iofile.obj modeng.obj nondrv.obj mixdrv.obj msdos.obj \
	sbdrv.obj awedrv.obj pasdrv.obj wssdrv.obj gusdrv.obj ariadrv.obj

LIB = ..\lib\DOS\audiobcl.lib

all : mp.exe $(LIB) clean
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) $(LFLAGS) mp.obj $(LIB)

$(LIB) : $(OBJS)
	@if exist $(LIB) del $(LIB) > nul
	$(AR) $(LIB) @DOS\Large\Borland\audiobcl.lnk

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
