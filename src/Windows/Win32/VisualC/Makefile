# Makefile for Visual C++ 4.1 - 5.2 (Win32 protected mode, flat memory model)

!if "$(MSVCDIR)"==""
MSVCDIR = C:\VC
!endif

!if "$(DXSDKROOT)"==""
DXSDKROOT = $(MSVCDIR)
!endif

CC = cl
LD = link
AS = tasm
AR = lib
CFLAGS = -nologo -G5 -Gr -MD -LD -W2 -GD -Ox -Zp4 -c -DWIN32 -D__WINDOWS__ -D__FLAT__ -D__ASM__ -D__MSC__ -I"$(DXSDKROOT)\INCLUDE" -I"$(MSVCDIR)\INCLUDE" 
AFLAGS = -t -m -q -ml -zi
OBJS = audiow32.obj audio.obj wavfile.obj xmfile.obj s3mfile.obj mtmfile.obj \
	modfile.obj iofile.obj modeng.obj nondrv.obj mixdrv.obj \
	_mixdrv.obj windrv.obj dsdrv.obj dsdrv2.obj

DLL = audiow32.dll
LIB = ..\lib\Win32\audw32vc.lib

LIBS =  "$(MSVCDIR)\LIB\msvcrt.lib" \
	"$(MSVCDIR)\LIB\kernel32.lib" "$(MSVCDIR)\LIB\winmm.lib" \
	"$(MSVCDIR)\LIB\ole32.lib" "$(MSVCDIR)\LIB\user32.lib" \
	"$(DXSDKROOT)\LIB\dsound.lib"

LFLAGS = -NOLOGO -DLL -MACHINE:I386 -VERSION:1.07 \
	-DEF:Windows\Win32\VisualC\audiow32.def -OUT:$(DLL) -IMPLIB:$(LIB)

all : mp.exe $(DLL) $(LIB) clean

mp.exe : mp.obj $(LIB)
	@$(LD) $(LIBS) -NOLOGO -SUBSYSTEM:CONSOLE -MACHINE:I386 -OUT:mp.exe mp.obj $(LIB)

$(DLL) $(LIB) : $(OBJS)
	$(LD) $(LIBS) $(LFLAGS) $(OBJS)

.c.obj :
	$(CC) $(CFLAGS) $<

.asm.obj :
	$(AS) $(AFLAGS) $<

clean :
	@if exist *.obj del *.obj > nul
	@if exist *.exp del *.exp > nul
