# Makefile for Watcom C/C++16 (DOS real mode, large memory model)

CC = wcl
AS = tasm
AR = wlib -n -b -q
CFLAGS = -zq -zu -ml -c -w9 -we -5 -s -oxt -D__DOS16__ -D__16BIT__
AFLAGS = -t -m -q -zi
LFLAGS = -zq -ml

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj &
	iofile.obj modeng.obj nondrv.obj mixdrv.obj msdos.obj &
	sbdrv.obj awedrv.obj pasdrv.obj wssdrv.obj gusdrv.obj ariadrv.obj

LIB = ..\lib\DOS\audiowcl.lib

all : mp.exe $(LIB) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) $(LFLAGS) mp.obj $(LIB)

$(LIB) : $(OBJS)
	*$(AR) $(LIB) $(OBJS)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean : .SYMBOLIC
	@if exist *.obj del *.obj > nul
