#
# Makefile for the Synthetic Audio Library (SEAL)
#
# Copyright (C) 1995-1999 Carlos Hasan. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

usage:
	@echo "Select the target architecture:"
	@echo ""
	@echo "32-bit Unix platforms:"
	@echo "   irix    - Silicon Graphics IRIX 4.0 platforms (untested)"
	@echo "   sunos   - SPARCstation SunOS 4.1.* platforms (untested)"
	@echo "   solaris - SPARCstation SunOS 5.* platforms (untested)"
	@echo "   linux   - GNU/Linux x86 platforms"
	@echo "   freebsd - FreeBSD x86 platforms (untested)"
	@echo "   beos    - BeOS Intel R3 or R4 platforms"
	@echo ""
	@echo "16-bit DOS real mode, large memory model:"
	@echo "   bcl - Borland C++ 3.1 real mode"
	@echo "   wcl - Watcom C/C++16 10.0 real mode"
	@echo ""
	@echo "16-bit DOS protected mode, large memory model:"
	@echo "   bcx - Borland C++ 4.5 protected mode (DPMI16 PowerPack)"
	@echo ""
	@echo "32-bit DOS protected mode, flat memory model:"
	@echo "   bcf - Borland C++ 4.5 protected mode (DPMI32 PowerPack)"
	@echo "   wcf - Watcom C/C++32 10.0 protected mode (DOS4GW Extender)"
	@echo "   djf - DJGPP V 2.0 protected mode (GO32/DPMI32 Extender)"
	@echo ""
	@echo "16-bit Windows 3.x protected mode, large memory model:"
	@echo "   w16bc - Borland C++ 3.1 protected mode (Win16)"
	@echo "   w16wc - Watcom C/C++16 10.0 protected mode (Win16)"
	@echo ""
	@echo "32-bit Windows 95/NT protected mode, flat memory model:"
	@echo "   w32bc - Borland C++ 4.5 protected mode (Win32)"
	@echo "   w32wc - Watcom C/C++32 10.0 protected mode (Win32)"
	@echo "   w32vc - Microsoft Visual C++ 4.1 protected mode (Win32)"
	@echo ""
	@echo "32-bit OS/2 MMPM protected mode, flat memory model:"
	@echo "   os2emx - EMX port of GCC (OS2)"
	@echo ""

irix:
	make -f Indigo/Makefile

sunos:
	make -f SunOS/Makefile

solaris:
	make -f Solaris/Makefile

linux:
	make -f Linux/Makefile

freebsd:
	make -f FreeBSD/Makefile

beos:
	make -f BeOS/Makefile

bcl:
	make -f DOS\Large\Borland\Makefile

wcl:
	wmake -f DOS\Large\Watcom\Makefile

bcx:
	make -f DOS\DPMI16\Borland\Makefile

bcf:
	make -f DOS\DPMI32\Borland\Makefile

wcf:
	wmake -f DOS\DPMI32\Watcom\Makefile

djf:
	make -f DOS\DPMI32\DJGPP\Makefile

w16bc:
	make -f Windows\Win16\Borland\Makefile

w16wc:
	wmake -f Windows\Win16\Watcom\Makefile

w32bc:
	make -f Windows\Win32\Borland\Makefile

w32wc:
	wmake -f Windows\Win32\Watcom\Makefile

w32vc:
	nmake -f Windows\Win32\VisualC\Makefile

os2emx:
	make -f OS2/Makefile
