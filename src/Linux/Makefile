# Makefile for Linux/386 and Voxware 2.90 operating system

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -mpentium -O3 -D__LINUX__ -D__ASM__
LFLAGS = -L../lib/Linux -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o lnxdrv.o _mixdrv.o

LIB = ../lib/Linux/libaudio.a

all : mp clean
	@echo done.

mp : mp.o $(LIB)
	$(CC) -o mp mp.o $(LFLAGS)
	$(STRIP) mp

$(LIB) : $(OBJS)
	$(AR) rc $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) $(CFLAGS) $<

.s.o .S.o :
	$(CC) $(CFLAGS) $<

clean :
	@rm -rf *.o
