#!/usr/bin/tclsh

proc openser { n } {
	set ser [open COM$n: RDWR]
	fconfigure $ser -mode 19200,e,7,1 -handshake none -translation binary
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
