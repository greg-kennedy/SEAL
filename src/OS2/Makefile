# Makefile for OS/2 MMPM with EMX and GCC
#
# To build an OS/2 DLL and the ModPlayer executable,
# just set the MMBASE environment variable to point to
# your MMOS2 base directory, change to the src directory
# and type make -f os2/Makefile.  That's all there is
# to it!

MMBASE=f:\mmos2

CC = gcc
CPP = cpp
IMPLIB = implib
CFLAGS = -c -Wall -O3 -Zsys -Zmts -fomit-frame-pointer -D__OS2__ -D__ASM__ -D __32BIT__ -D __FLAT__

DLL = audio.dll
LIB = ../lib/OS2/audio.lib

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o _mixdrv.o mixdrv.o os2drv.o

OMFOBJS = audio.obj wavfile.obj xmfile.obj s3mfile.obj modfile.obj \
	mtmfile.obj iofile.obj modeng.obj nondrv.obj _mixdrv.obj \
	mixdrv.obj os2drv.obj

all : mp.exe
	@echo done.

$(DLL) : $(OBJS) mdm.lib
	$(CC) -Zsys -Zomf -Zmts -Zso -Zdll -o $(DLL) os2/audio.def -L. -lmdm $(OMFOBJS)

mdm.lib: $(MMBASE)/dll/mdm.dll
	$(IMPLIB) mdm.lib $(MMBASE)/dll/mdm.dll

$(LIB): $(DLL)
	$(IMPLIB) $(LIB) $(DLL)

mp.exe : $(OBJS) mp.o $(LIB)
	emxomf mp.o
	$(CC) -Zsys -Zomf -Zmts -Zlinker /STACK:32768 -Zlinker /PMTYPE:VIO -o mp mp.obj -L../lib/OS2 -laudio -lmdm

.c.o:
	$(CC) $(CFLAGS) $<
	emxomf $*.o

_mixdrv.o: _mixdrv.S
	$(CPP) _mixdrv.S | $(AS) -o _mixdrv.o
	emxomf _mixdrv.o

clean :
	@rm -rf *.obj *.o

