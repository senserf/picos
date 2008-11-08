#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for XRS protocol
#

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	puts stderr "Cannot locate file $f 'sourced' by the script."
	exit 99
}

msource oss_u.tcl
msource oss_u_ab.tcl

package require oss_u_ab 1.0

###############################################################################

proc uget { msg } {
#
# Receive a normal command response line from the board
#
	puts $msg
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

	set line [string trim $line]

	if { $line == "" } {
		# always quietly ignore empty lines
		return
	}

	if { $line == "!!" } {
		# previous command
		if { $ST(PCM) == "" } {
			puts "no previous rejected command"
			return
		}
		set line $ST(PCM)
		set ST(PCM) ""
	}

	if ![u_ab_ready] {
		puts "board busy"
		set ST(PCM) $line
		return
	}

	if { [string index $line 0] == "!" } {
		# not for the board
		if [icmd [string trimleft \
		    [string range $line 1 end]]] {
			# failed
			set ST(PCM) $line
		}
		return
	}

	# send it to the board
	u_ab_write $line
}

proc icmd { cmd } {
#
# A command addressed to us
#
	puts "ICMD commands not implemented"
	return 0
}

######### COM port ############################################################

set prt [lindex $argv 0]
set spd [lindex $argv 1]
set mpl [lindex $argv 2]

if [catch { expr $spd } spd] {
	set spd 9600
}

if [catch { expr $mpl } mpl] {
	set mpl 62
}

if [catch { u_start $prt $spd "" } err] {
	puts $err
	exit 99
}

u_ab_setif uget $mpl

fconfigure stdin -buffering line -blocking 0
fileevent stdin readable sget

# u_settrace 7 dump.txt

vwait None
