#!/bin/sh

case `uname -sr` in
  IRIX\ 4.0.*)
     LIB=../lib/Indigo ;;
  SunOS\ 4.1.*)
     LIB=../lib/SunOS ;;
  SunOS\ 5.*)
     LIB=../lib/Solaris ;;
  Linux*)
     # install ELF shared object archive (you must be logged as root)
     if [ ! -f /usr/lib/libaudio.so.1.0.0 ]; then
        cp ../lib/Linux/ELF/libaudio.so.1.0.0 /usr/lib
        ln -s /usr/lib/libaudio.so.1.0.0 /usr/lib/libaudio.so.1
     fi
     LIB=. ;;
  FreeBSD*)
     LIB=../lib/FreeBSD ;;
  *) 
     echo "This program has not been tested on your machine!"
     exit
esac

for f in 1 2 3 4 5; do
    echo "compiling example$f.c"
    gcc -I../include -o example$f example$f.c -L$LIB -laudio
    strip example$f
done

