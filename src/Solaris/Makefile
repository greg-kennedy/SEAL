# Makefile for SPARC 10 Solaris 2.x with dbri or AMD ulaw audio device

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -O2 -D__SOLARIS__ -D__BIGENDIAN__
LFLAGS = -L../lib/Solaris -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o sundrv.o

LIB = ../lib/Solaris/libaudio.a

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
