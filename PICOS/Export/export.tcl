#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
########\
exec tclsh "$0" "$@"

if { [llength $argv] != 1 } {
	puts stderr "usage: $argv0 exportname"
	exit 1
}

# sanity checks
if { [file tail [pwd]] != "Export" || ![file isdirectory "../PicOS"] } {
	puts stderr "must be called in Export of the PICOS tree"
	exit 1
}

set ename [lindex $argv 0]
set eroot [file rootname $ename]
set eextn [file extension $ename]
set etail [file tail $eroot]

if { $etail != $eroot } {
	puts stderr "the export name must be a simple file name in Export"
	exit 1
}

if { $eextn == "" } {
	set ename "${etail}.exp"
} elseif { $eextn != ".exp" } {
	puts stderr "illegal file extension, must be .exp"
	exit 1
}

set tardir [file normalize "../../__PICOS_EXPORTS__/$eroot"]

puts "Storing the export in $tardir"

file delete -force $tardir
file mkdir $tardir

###############################################################################

cd ../Apps
puts "cleaning up PICOS"
# exec ./cleanup -p
exec ./cleanup
cd ../..

# container of PICOS and stuff
set sd [pwd]



cd $tardir
file mkdir PICOS/PicOS
cd $sd

puts "Trimming PICOS"
exec PICOS/Scripts/pxport PICOS/Export/$ename PICOS $tardir/PICOS

puts "Repacking PIP"
exec tar cf - PIP | gzip -9 > /tmp/__pack.tgz
cd $tardir
exec zcat /tmp/__pack.tgz | tar -xf -
file delete -force /tmp/__pack.tgz
cd PIP
file delete -force .git

puts "Repacking SIDE"
cd $sd
cd SIDE
exec ./cleanup
cd ..
exec tar cf - SIDE | gzip -9 > /tmp/__pack.tgz
cd $tardir
exec zcat /tmp/__pack.tgz | tar -xf -
file delete /tmp/__pack.tgz
cd SIDE
file delete -force .git

puts "Repacking VUEE"
cd $sd
cd VUEE
exec ./cleanup
cd ..
exec tar cf - VUEE | gzip -9 > /tmp/__pack.tgz
cd $tardir
exec zcat /tmp/__pack.tgz | tar -xf -
file delete /tmp/__pack.tgz
cd VUEE
file delete -force .git

puts "done"
