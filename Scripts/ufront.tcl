#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for straightforward communication
#

###############################################################################

proc u_start { udev speed dfun } {
#
# Initialize UART
#
	global ST

	if { $udev == "" } {
		global argv
		# take from arguments
		set udev [lindex $argv 0]
	}

	if { $udev == "" } {
		set devlist ""
		for { set udev 0 } { $udev < 8 } { incr udev } {
			lappend devlist "COM${udev}:"
			lappend devlist "/dev/ttyUSB$udev"
			lappend devlist "/dev/tty$udev"
		}
	} else {
		if [catch { expr $udev } cn] {
			# must be a complete device
			set devlist \
			    [list $udev ${udev}: "/dev/$udev" "/dev/tty$udev"]
		} else {
			# com number or tty number
			set devlist [list "COM${udev}:" "/dev/ttyUSB$udev" \
				"/dev/tty$udev"]
		}
	}

	set fail 1
	foreach udev $devlist {
		if ![catch { open $udev "r+" } ST(SFD)] {
			set fail 0
			break
		}

	}

	if $fail {
		error "u_start: cannot open UART, device(s) $devlist"
	}

	fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
		-buffering line -blocking 0

	fileevent $ST(SFD) readable u_rdline
}

proc u_rdline { } {

	global ST

	if [catch { gets $ST(SFD) line } nc] {
		error "device has been closed"
	}

	if { $nc >= 0 } {
		puts $line
		flush stdout
	}
}

proc sget { } {
#
# STDIN becomes readable
#
	global ST

	if [catch { gets stdin line } stat] {
		# ignore any errors (can they happen at all?)
		return
	}

	if { $stat < 0 } {
		# end of file
		exit 0
	}

	# send it to the board
	if [catch { puts $ST(SFD) $line } ] {
		error "deice has been closed"
	}
}

######### COM port ############################################################

set prt [lindex $argv 0]
set spd [lindex $argv 1]
set mpl [lindex $argv 2]

if [catch { expr $spd } spd] {
	set spd 9600
}

if [catch { expr $mpl } mpl] {
	set mpl 256
}

if [catch { u_start $prt $spd "" } err] {
	puts $err
	exit 99
}

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

# u_settrace 7 dump.txt

vwait None
