#!/bin/sh
TD="/cygdrive/c/Olimex/MSP430 Programmer"
cp $2 "${TD}"/image.hex
cd "$TD"
./mspprog-cli-v2.exe /p=USB /d=$1 /f=image.hex /ec /w < /dev/null
rm -f image.hex
