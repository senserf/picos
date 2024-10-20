#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
if [ ! -f RTAGS ]
then
	echo "Must be called from PICOS root directory"
	exit 0
fi

rm -rf Linux Simnets

cp Docs/PicOS*.pdf zzz01.pdf
cp Docs/VNETI*.pdf zzz02.pdf
cp Docs/PinOps*.pdf zzz03.pdf
cp Docs/ADCSampler.pdf zzz04.pdf
rm -rf Docs
mkdir Docs
mv zzz01.pdf Docs/PicOS.pdf
mv zzz02.pdf Docs/VNETI.pdf
mv zzz03.pdf Docs/PinOps.pdf
mv zzz04.pdf Docs/ADCSampler.pdf

cd PicOS

rm -rf eCOG
cd Libs
rm -rf LibComms ATTIC
cd Lib
rm -rf phys_dm2200* phys_dm2100* phys_cc1000* phys_ether* phys_radio* phys_rf24*
cd ../..
rm -rf *cc1000* *dm2* *rf24* 
cd MSP430
rm -rf *cc1000* *dm2* *rf24* 
cd BOARDS
rm -rf GENESIS DM2100 VERSA* WARSAW SFU_HEART
cd ../../..
cd Apps
./cleanall
rm -rf ADC ATTIC GENESIS MISC M_AMRD Peg RTag* R_AMRD Radio Remote* Sniffer TESTS Tag VMesh2 VUEE
cd SFU
rm -rf HEART* BANDWIDTH* URF*
cd ../..
cd Scripts
rm strip.sh
cd ..
find . -name "CVS" -type d -exec rm -rf '{}' \; 2>&1 | sed -e "s/find: \(.*\):.*/\1/"
