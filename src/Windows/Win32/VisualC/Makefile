# Makefile for Visual C++ 4.1 (Win32 protected mode, flat memory model)

!if "$(MSTOOLS)"==""
MSTOOLS = \vc41\msdev
!endif

CC = cl
LD = link
AS = tasm
AR = lib
CFLAGS = -nologo -G5 -Gr -MT -W2 -GD -Ox -Zp4 -c -D__WINDOWS__ -D__FLAT__ -D__ASM__ -D__MSC__ -I$(MSTOOLS)\include
AFLAGS = -t -m -q -ml -zi
OBJS = audiow32.obj audio.obj wavfile.obj xmfile.obj s3mfile.obj \
	modfile.obj iofile.obj modeng.obj nondrv.obj mixdrv.obj \
	_mixdrv.obj windrv.obj

DLL = audiow32.dll
LIB = audw32vc.lib

LIBDIR = $(MSTOOLS)\lib
LIBS = $(LIBDIR)\libcmt.lib $(LIBDIR)\oldnames.lib \
	$(LIBDIR)\kernel32.lib $(LIBDIR)\winmm.lib

LFLAGS = -NOLOGO -SUBSYSTEM:WINDOWS -DLL -MACHINE:I386 -VERSION:1.0 \
	-DEF:Windows\Win32\VisualC\audiow32.def -OUT:$(DLL) -IMPLIB:$(LIB)

all : mp.exe $(DLL) $(LIB) clean

mp.exe : mp.obj $(LIB)
	@$(LD) $(LIBS) -NOLOGO -SUBSYSTEM:CONSOLE -MACHINE:I386 -OUT:mp.exe mp.obj $(LIB)

$(DLL) $(LIB) : $(OBJS)
	@$(LD) $(LIBS) $(LFLAGS) $(OBJS)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
	@if exist *.exp del *.exp > nul
