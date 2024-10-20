#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
###########################\
exec tclsh "$0" "$@"

#########################
# Testing the XML package
#########################

##################################################################

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

msource xml.tcl

###############################################################################

while 1 {

	puts "file to parse:"

	set fn [string trim [gets stdin]]

	if { $fn == "" } {
		continue
	}

	if [catch { open $fn "r" } fd] {
		puts "cannot open: $fd"
		continue
	}

	if [catch { read $fd } sm] {
		puts "cannot read: $sm"
		catch { close $fd }
		continue
	}

	catch { close $fd }

	if [catch { sxml_parse sm } sm] {
		puts "parse error: $sm"
		continue
	}

	while 1 {

		puts "command:"
		set fn [string trim [gets stdin]]

		if { $fn == "q" } {
			break
		}

		if [catch { eval $fn } res] {
			puts "command error: $res"
			continue
		}

		puts "RESULT = $res"
	}
}
