# Makefile for Silicon Graphics Indigo IRIX 4.x with audio ports

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -O2 -D__SILICON__ -D__BIGENDIAN__
LFLAGS = -laudio -L../lib/Indigo -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
	iofile.o modeng.o nondrv.o mixdrv.o sgidrv.o

LIB = ../lib/Indigo/libaudio.a

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
