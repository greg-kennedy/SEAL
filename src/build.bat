@echo off
if "%1"=="bcl" goto bcl
if "%1"=="wcl" goto wcl
if "%1"=="bcx" goto bcx
if "%1"=="bcf" goto bcf
if "%1"=="wcf" goto wcf
if "%1"=="djf" goto djf
if "%1"=="w16bc" goto w16bc
if "%1"=="w16wc" goto w16wc
if "%1"=="w32bc" goto w32bc
if "%1"=="w32wc" goto w32wc
if "%1"=="w32vc" goto w32vc
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
echo ****   bcf - Borland C++ 4.5 protected mode (DPMI32 PowerPack)
echo ****   wcf - Watcom C/C++32 10.0 protected mode (DOS4GW Extender)
echo ****   djf - DJGPP V 2.0 protected mode (GO32/DPMI32 Extender)
echo ****
echo **** 16-bit Windows 3.x protected mode, large memory model:
echo ****   w16bc - Borland C++ 3.1 protected mode (Win16)
echo ****   w16wc - Watcom C/C++16 10.0 protected mode (Win16)
echo ****
echo **** 32-bit Windows 95/NT protected node, flat memory model:
echo ****   w32bc - Borland C++ 4.5 protected mode (Win32)
echo ****   w32wc - Watcom C/C++32 10.0 protected mode (Win32)
echo ****   w32vc - Microsoft Visual C++ 4.1 protected mode (Win32)
echo ****
goto exit
:bcl
make -f DOS\Large\Borland\Makefile
goto exit
:wcl
wmake -f DOS\Large\Watcom\Makefile
goto exit
:bcx
make -f DOS\DPMI16\Borland\Makefile
goto exit
:bcf
make -f DOS\DPMI32\Borland\Makefile
goto exit
:wcf
wmake -f DOS\DPMI32\Watcom\Makefile
goto exit
:djf
make -f DOS\DPMI32\DJGPP\Makefile
goto exit
:w16bc
make -f Windows\Win16\Borland\Makefile
goto exit
:w16wc
wmake -f Windows\Win16\Watcom\Makefile
goto exit
:w32bc
make -f Windows\Win32\Borland\Makefile
goto exit
:w32wc
wmake -f Windows\Win32\Watcom\Makefile
goto exit
:w32vc
nmake -f Windows\Win32\VisualC\Makefile
goto exit
:exit
