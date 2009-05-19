#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for straightforward communication
#

###############################################################################

proc u_cdevl { pi } {
#
# Returns the candidate list of devices to open based on the port identifier
#
	if { [regexp "^\[0-9\]+$" $pi] && ![catch { expr $pi } pn] } {
		# looks like a number
		if { $pn < 10 } {
			# use internal Tcl COM id, which is faster
			set wd "COM${pn}:"
		} else {
			set wd "\\\\.\\COM$pn"
		}
		return [list $wd "/dev/ttyUSB$pn" "/dev/tty$pn"]
	}

	# not a number
	return [list $pi "\\\\.\\$pi" "/dev/$pi" "/dev/tty$pi"]
}

proc u_start { udev speed dfun } {
#
# Initialize UART
#
	global ST

	set wfp "\\\\.\\"

	if { $udev == "" } {
		global argv
		# take from arguments
		set udev [lindex $argv 0]
	}

	if { $udev == "" } {
		set devlist ""
		for { set udev 0 } { $udev < 10 } { incr udev } {
			# On Windows, this is limited to COM1-COM9 (i.e.,
			# the ports hardwired into Tcl); using a full device
			# path takes a lot of time
			set devlist [concat $devlist [u_cdevl $udev]]
		}
	} else {
		set devlist [u_cdevl $udev]
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
		-buffering line -blocking 0 -eofchar ""

	fileevent $ST(SFD) readable u_rdline
}

proc u_rdline { } {

	global ST

	if [catch { gets $ST(SFD) line } nc] {
		puts "UART read error"
		return
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
		puts "UART write error"
	}
}

######### COM port ############################################################

set prt [lindex $argv 0]
set spd [lindex $argv 1]

if [catch { expr $spd } spd] {
	set spd 9600
}

if [catch { u_start $prt $spd "" } err] {
	puts $err
	exit 99
}

fconfigure stdin -buffering line -blocking 0 -eofchar ""
fileevent stdin readable sget

# u_settrace 7 dump.txt

vwait None
