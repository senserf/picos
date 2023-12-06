#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
#####################\
exec tclsh "$0" "$@"

###############################################################################
# A generic script to modify data files line-by-line and globally
###############################################################################

proc edit_by_line { src } {

	# the default action
	return $src
}

proc edit_globally { src } {

	# the default action
	return $src
}

###############################################################################

proc main { } {

	global argv argv0

	set ifn [lindex $argv 0]
	set ofn [lindex $argv 1]

	if { $ifn == "" || $ofn == "" } {
		puts stderr "usage: $argv0 inputfile outputfile"
		exit 1
	}

	if { [catch { open $ifn "r" } ifd] } {
		puts stderr "cannot open '$ifn', $ifd"
		exit 1
	}

	if { [catch { open $ofn "w" } ofd] } {
		puts stderr "cannot open '$ofn', $ofd"
		exit 1
	}

	set ofile ""
	foreach line [split [edit_globally [read $ifd]] "\n"] {
		lappend ofile [edit_by_line $line]
	}

	puts -nonewline $ofd [join $ofile "\n"]
}

main
