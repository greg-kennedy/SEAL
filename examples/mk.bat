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
echo **** 32-bit Windows 95/NT protected mode, flat memory model:
echo ****   w32bc - Borland C++ 4.5 protected mode (Win32)
echo ****   w32wc - Watcom C/C++16 10.0 protected mode (Win32)
echo ****   w32vc - Microsoft Visual C++ 4.1 protected mode (Win32)
echo ****   w32bp - Borland Delphi 2.0 protected mode (Win32)
echo ****
echo **** NOTE: 16-bit libraries are not available in this release.
goto exit

:bcl
for %%f in (*.c) do bcc -ml -I..\include ..\lib\dos\audiobcl.lib %%f
goto exit

:wcl
for %%f in (*.c) do wcl -ml -I..\include ..\lib\dos\audiowcl.lib %%f
goto exit

:bcx
for %%f in (*.c) do bcc -ml -WX -I..\include ..\lib\dos\audiobcx.lib %%f
goto exit

:wcf
for %%f in (*.c) do wcl386 -zq -I..\include ..\lib\dos\audiowcf.lib %%f
goto exit

:djf
for %%f in (1 2 3 4) do gcc -o example%%f.exe -I..\include example%%f.c -L..\lib\dos -laudio
goto exit

:w16bc
for %%f in (*.c) do bcc -ml -W -I..\include ..\lib\win16\audw16bc.lib %%f
goto exit

:w16wc
for %%f in (*.c) do wcl -ml -zw -I..\include ..\lib\win16\audw16wc.lib mmsystem.lib %%f
goto exit

:w32bc
for %%f in (*.c) do bcc32a -WC -DWIN32 -I..\include ..\lib\win32\audw32bc.lib %%f
goto exit

:w32wc
for %%f in (*.c) do wcl386 -l=nt -DWIN32 -I..\include ..\lib\win32\audw32wc.lib %%f
goto exit

:w32vc
for %%f in (*.c) do cl -DWIN32 -I..\include ..\lib\win32\audw32vc.lib %%f
goto exit

:w32bp
dcc32 -CC -U..\include demo.pas
goto exit

:exit
if exist *.obj del *.obj > nul
if exist *.o del *.o > nul
