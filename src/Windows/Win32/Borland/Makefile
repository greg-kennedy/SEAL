# Makefile for Borland C++ 4.5 (Win32 protected mode, flat memory model)

CC = bcc32a
LD = tlink32
IMPLIB = implib
AS = tasm
AR = tlib
CFLAGS = -WC -c -w9 -5 -O2 -DWIN32 -D__WINDOWS__ -D__ASM__
AFLAGS = -t -m -q -ml -zi
LFLAGS = -WD
LDFLAGS = -x -Tpd
OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj \
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj windrv.obj \
	drdrv.obj audiow32.obj

DLL = audiow32.dll
LIB = ..\lib\Win32\audw32bc.lib

all : mp.exe $(DLL) $(LIB) clean

mp.exe : mp.obj $(LIB)
	$(CC) -WC mp.obj $(LIB)

$(DLL): $(OBJS)
	$(LD) $(LDFLAGS) @Windows\Win32\Borland\audiow32.lnk

$(LIB) : $(DLL)
	$(IMPLIB) $(LIB) $(DLL)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
