# Makefile for WATCOM C/C++32 10.0 (Win32 protected mode, flat memory model)

CC = wcl386
LD = wlink
AS = tasm
AR = wlib -b -n -q
CFLAGS = -zq -c -w9 -we -5r -s -oxt -D__WINDOWS__ -D__ASM__
AFLAGS = -q -t -m -ml -zi
LFLAGS = -zq -l=nt

#AS = wasm
#AFLAGS = -zq -w9 -we -5r

OBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj &
	iofile.obj modeng.obj nondrv.obj mixdrv.obj _mixdrv.obj windrv.obj

DLL = audiow32.dll
LIB = audw32wc.lib

all : mp.exe $(DLL) clean .SYMBOLIC
	@echo done.

mp.exe : mp.obj $(LIB)
	$(CC) mp.obj $(LIB) $(LFLAGS)

$(LIB) : $(DLL)
	$(AR) +$(DLL)

$(DLL) : $(OBJS)
	$(LD) @Windows\Win32\Watcom\audiow32.lnk

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean : .SYMBOLIC
	@if exist *.obj del *.obj > nul
