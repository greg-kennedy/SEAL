@echo off

if "%1"=="bcl" goto bcl
if "%1"=="wcl" goto wcl
if "%1"=="bcx" goto bcx
if "%1"=="wcf" goto wcf
if "%1"=="djf" goto djf
if "%1"=="w16bc" goto w16bc
if "%1"=="w16wc" goto w16wc
if "%1"=="w32bc" goto w32bc
if "%1"=="w32wc" goto w32wc
if "%1"=="w32vc" goto w32vc
if "%1"=="w32bp" goto w32bp

:usage
echo **** usage: build [target]
echo ****
echo **** 16-bit DOS real mode, large memory model:
echo ****   bcl - Borland C++ 3.1 real mode
echo ****   wcl - Watcom C/C++16 10.0 real mode
echo ****
echo **** 16-bit DOS protected mode, large memory model:
echo ****   bcx - Borland C++ 4.5 protected mode (DPMI16 PowerPack)
echo ****
echo **** 32-bit DOS protected mode, flat memory model:
echo ****   wcf - Watcom C/C++32 10.0 protected mode (DOS4GW Extender)
echo ****   djf - DJGPP V 2.0 protected mode (GO32/DPMI32 Extender)
echo ****
echo **** 16-bit Windows 3.x protected mode, large memory model:
echo ****   w16bc - Borland C++ 3.1 protected mode (Win16)
echo ****   w16wc - Watcom C/C++16 10.0 protected mode (Win16)
echo ****
echo **** 32-bit Windows 95/NT protected node, flat memory model:
echo ****   w32bc - Borland C++ 4.5 protected mode (Win32)
echo ****   w32wc - Watcom C/C++16 10.0 protected mode (Win32)
echo ****   w32vc - Microsoft Visual C++ 4.1 protected mode (Win32)
echo ****   w32bp - Borland Delphi 2.0 protected mode (Win32)
echo ****
goto exit

:bcl
for f in (*.c) bcc -ml -I..\include ..\lib\dos\audiobcl.lib %f
goto exit

:wcl
for f in (*.c) wcl -ml -I..\include ..\lib\dos\audiowcl.lib %f
goto exit

:bcx
for f in (*.c) bcc -ml -WX -I..\include ..\lib\dpmi16\audiobcx.lib %f
goto exit

:wcf
for f in (*.c) wcl386 -I..\include ..\lib\dpmi32\audiowcf.lib %f
goto exit

:djf
for f in (1 2 3 4 5) gcc -o example%f.exe -I..\include example%f.c ..\lib\dpmi32\audiodjf.a
goto exit

:w16bc
for f in (*.c) bcc -ml -W -I..\include ..\lib\win16\audw16bc.lib %f
goto exit

:w16wc
for f in (*.c) wcl -ml -zw -I..\include ..\lib\win16\audw16wc.lib mmsystem.lib %f
goto exit

:w32bc
for f in (*.c) bcc32a -WC -I..\include ..\lib\win32\audw32bc.lib %f
goto exit

:w32wc
for f in (*.c) wcl386 -l=nt -I..\include ..\lib\win32\audw32wc.lib %f
goto exit

:w32vc
for f in (*.c) cl -D__FLAT__ -I..\include ..\lib\win32\audw32vc.lib %f
goto exit

:w32bp
dcc32 -CC -U..\include demo.pas
goto exit

:exit
if exist *.obj del *.obj > nul
if exist *.o del *.o > nul
