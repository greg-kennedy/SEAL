# Makefile for Linux/386 and Voxware 2.90 operating system

CC = gcc
AR = ar
RANLIB = ranlib
STRIP = strip
CFLAGS = -c -Wall -O2 -fomit-frame-pointer -D__LINUX__ -D__ASM__
LFLAGS = -L. -laudio

OBJS = audio.o wavfile.o xmfile.o s3mfile.o modfile.o iofile.o \
	modeng.o nondrv.o mixdrv.o _mixdrv.o lnxdrv.o

ifeq ($(TARGET),static)
LIB = libaudio.a
else
CFLAGS += -fPIC
LIBNAME = libaudio.so
LIBMAJOR = 1
LIBMINOR = 0
LIBPATCH = 0
LIB = $(LIBNAME).$(LIBMAJOR).$(LIBMINOR).$(LIBPATCH)
endif

all : mp clean
	@echo done.

mp : mp.o $(LIB)
	$(CC) -o mp mp.o $(LFLAGS)
	$(STRIP) mp

ifeq ($(TARGET),static)
$(LIB) : $(OBJS)
	$(AR) rc $(LIB) $(OBJS)
	$(RANLIB) $(LIB)
else
$(LIB) : $(OBJS)
	$(CC) -shared -Wl,-soname,$(LIBNAME).$(LIBMAJOR) -o $(LIB) $(OBJS)
	ln -sf $(LIB) $(LIBNAME).$(LIBMAJOR)
endif

.c.o:
	$(CC) $(CFLAGS) $<

.s.o .S.o :
	$(CC) $(CFLAGS) $<

clean :
	@for f in *.o; do rm -f $$f; done
