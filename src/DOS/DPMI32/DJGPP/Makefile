# Makefile for DJGPP V2.0 (GO32/DPMI32 protected mode, flat memory model)

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
COFF2EXE = stubify
CFLAGS = -c -Wall -mpentium -O2 -D__DPMI__ -D__ASM__
LFLAGS =

#OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o mtmfile.o \
#       iofile.o modeng.o nondrv.o mixdrv.o _mixdrv.o msdos.o \
#       sbdrv.o awedrv.o pasdrv.o wssdrv.o gusdrv.o ariadrv.o

OBJS = audiodj.o _mixdrv.o

LIB = ../lib/DOS/libaudio.a

all : mp.exe $(LIB) clean
	@echo done.

mp.exe : mp.o $(LIB)
	$(CC) -o mp mp.o $(LIB) $(LFLAGS)
	$(STRIP) mp
	$(COFF2EXE) mp
	@del mp > nul

$(LIB) : $(OBJS)
	@$(AR) rc $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

.c.o :
	$(CC) $(CFLAGS) $<

.s.o :
	$(CC) $(CFLAGS) $*.S

clean :
	@if exist *.o del *.o > nul
