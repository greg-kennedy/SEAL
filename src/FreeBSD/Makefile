# Makefile for FreeBSD/386 and VoxWare 2.90 or later

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -O2 -fomit-frame-pointer -D__FREEBSD__ -D__ASM__
LFLAGS = -L../lib/FreeBSD -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o _mixdrv.o lnxdrv.o

LIB = ../lib/FreeBSD/libaudio.a

all : mp $(LIB) clean
	@echo done.

mp : mp.o $(LIB)
	$(CC) -o mp mp.o $(LFLAGS)
	$(STRIP) mp

$(LIB) : $(OBJS)
	$(AR) rc $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

.c.o .s.o .S.o :
	$(CC) $(CFLAGS) $<

clean :
	@for f in *.o; do rm -f $$f; done
