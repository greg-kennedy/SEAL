
Synthetic Audio Library (SEAL) Development Kit
Copyright (C) 1995, 1996 Carlos Hasan
All Rights Reserved


This 1.0.0 release of the Synthetic Audio Library (SEAL) lets you write
applications that conform to the 1.0 SEAL API which is able to play
digital audio waveforms and music modules on many platforms.


Features
--------
    - simple and powerful API interface
    - 32 simultaneous digital audio channels
    - smooth pitch shifting, volume and panning controllers per channel
    - smart digital filtering for improved sound quality
    - efficient digital audio waveform mixing algorithms combined with 
      uncompromising low-level optimizations
    - importing of wave and music file formats such as Protracker 1.1,
      Scream Tracker 3.0 and FastTracker 2.0 modules
    - performance tuned for the i486 and Pentium processors
    - native support in DOS for hardware wavetable accelerators such as 
      the Gravis Forte's GF1 synthesizer, and normal digital audio devices 
      such as the Sound Blaster 1.5/2.0/Pro/16, Pro Audio Spectrum 16, 
      Windows Sound System, Ensoniq Soundscape and Aria soundcards


Work in Progress
----------------
    - real-time binaural 3D audio engine for VR applications
    - portable object-oriented C++ version of the library
    - native support for other hardware wavetable chipsets such as
      the Sound Blaster AWE32 EMU8K synthesizer
    - interface to Windows 95 based hardware acceleration through
      DirectSound


Supported platforms
-------------------
The SEAL SDK is available for DOS, Extended DOS, Windows 95, Windows NT,
and Linux platforms. FreeBSD, SPARC SunOS, SPARC Solaris and SGI Indigo 
ports are in progress. The following compilers were used to build the 
library files:

    - Borland C++ 4.5 compiler for DOS and Win32s
    - WATCOM C/C++ 10.0 compiler for DOS and Windows
    - Microsoft Visual C++ 4.1 compiler for Windows 95
    - DJGPP 2.0 port of GNU C/C++ compiler for DOS
    - GNU C/C++ 2.7.0 for Linux (a.out and ELF)


The SEAL SDK contains
---------------------
    - SEAL API header and library files
    - SEAL audio library source code files (commercial license)
    - set of example source code files


---------------------------------------------------------------------
IMPORTANT: Please make sure you understand the Copyright and License
information (in the file named LICENSE.TXT) before using this release.
---------------------------------------------------------------------


Installing the Software
-----------------------
The SEAL SDK can be installed unpacking the distribution archive
in your hard disk. The SEAL SDK include and library directories
must be installed in your C programming environment modifying
the header and library directory paths.

You can check the included batch files in the examples directory
for details about using the SEAL SDK in your own programs.


Notes about 16-bit libraries
----------------------------
The SEAL Audio Library has been optimized for i486 and Pentium processors 
running in 32-bit protected mode environments such as DOS4GW, DPMI32, 
Windows 95, Windows NT or Linux.

However, the 16-bit versions of the library have been included in the 
release (these are VERY SLOW and should never be used for productions).


Licensing the Software
----------------------
Please read the file named ORDER.TXT for details about licensing the
SEAL Audio Library for use in shareware or commercial products.


Contacting the Author
---------------------
This file is the only documentation included in this release. If you
have questions, problems or comments you can contact the author at the 
following email address: chasan@dcc.uchile.cl.


Closing Words
-------------
I would like to thank all the beta testers and specially to Jon L. 
for the music module file used by the example programs, and to all
the IRC friends at the #coders channel.


