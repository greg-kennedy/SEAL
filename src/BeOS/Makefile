# Makefile for BeOS Release 4 operating systems

CC = gcc
AR = ar 
AS = nasm
CFLAGS = -c -O3 -Wall -Wno-multichar -D__ASM__ -D__FLAT__
ARFLAGS = cr
LIBS = -L../lib/BeOS/ -laudio -lroot -lbe -lmedia

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o _mixdrv.o be3drv.o bedrv.o

LIB = ../lib/BeOS/libaudio.a

all : mp clean
	@echo done.

mp : mp.o $(LIB)
	$(CC) -o mp mp.o $(LIBS)

$(LIB) : $(OBJS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJS)

.SUFFIXES: .c .cpp .h .obj

.c.o:
	$(CC) $(CFLAGS) $<

.cpp.o:
	$(CC) $(CFLAGS) $<

.asm.o:
	$(AS) $<

clean :
	@rm -rf *.o
