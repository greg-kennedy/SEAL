#!/bin/sh

case `uname -sr` in
  IRIX\ 4.0.*)
     make -f Indigo/Makefile ;;
  SunOS\ 4.1.*)
     make -f SunOS/Makefile ;;
  SunOS\ 5.*)
     make -f Solaris/Makefile ;;
  Linux*)
     make -f Linux/Makefile ;;
  FreeBSD*)
     make -f FreeBSD/Makefile ;;
  *) 
     echo "This program has not been tested on your machine!"
     exit
esac
