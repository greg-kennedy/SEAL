# Makefile for WATCOM C/C++32 10.0 (Win32 protected mode, flat memory model)

CC = wcl386
LD = wlink
AS = tasm
AR = wlib -b -n -q
CFLAGS = -zq -c -w9 -5r -s -oxt -DWIN32 -D__WINDOWS__ -D__ASM__
AFLAGS = -q -t -m -ml -zi
LFLAGS = -zq -l=nt

#AS = wasm
#AFLAGS = -zq -w9 -we -5r

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj mtmfile.obj &
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj windrv.obj dsdrv.obj

DLL = audiow32.dll
LIB = ..\lib\Win32\audw32wc.lib

all : mp.exe $(DLL) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) mp.obj $(LIB) $(LFLAGS)

$(LIB) : $(DLL)
	$(AR) $(LIB) @Windows\Win32\Watcom\audiow32.lbc

$(DLL) : $(OBJS)
	$(LD) @Windows\Win32\Watcom\audiow32.lnk

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean : .SYMBOLIC
	@if exist *.obj del *.obj > nul
