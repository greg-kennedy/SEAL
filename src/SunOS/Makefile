# Makefile for SPARC 1/2/IPX/... SunOS 4.1.x with AMD ulaw audio device

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -O2 -D__SPARC__ -D__BIGENDIAN__
LFLAGS = -L../lib/SunOS -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o sundrv.o

LIB = ../lib/SunOS/libaudio.a

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
	@rm -rf *.o
