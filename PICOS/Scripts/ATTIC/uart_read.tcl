#!/usr/bin/tclsh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#

proc openser { n } {
	set ser [open COM$n: RDWR]
	fconfigure $ser -mode 19200,e,7,1 -handshake none -translation binary -eofchar ""
		# -blocking 0
	return $ser
}

proc main { } {

	set ser [openser 2]

	while 1 {
		if ![catch { set f [read $ser 1] } err] {
			puts -nonewline $f 
			flush stdout
		}
	}
}

main
