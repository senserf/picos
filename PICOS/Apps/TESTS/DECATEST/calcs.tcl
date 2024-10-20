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

set TU [expr { 1.0 / (128 * 499.2 * 1000000.0) }]
set CC 299700000.0
set TF [expr { $TU * $CC }]

proc diff { a b } {
#
# Calculates difference between time stamps expressed as 40-bit unsigned
# numbers
#
	if { $a < $b } {
		set a [expr { $a + 0x100000000 }]
	}

	return [expr { $a - $b }]
}
	
proc calc { smp } {

	global TF

	lassign $smp TSP TRR TSF TRP TSR TRF

	set da1 [diff $TRR $TSP]
	set da2 [diff $TSR $TRP]

	set db1 [diff $TRF $TSR]
	set db2 [diff $TSF $TRR]

	set TOFA [expr { ($da1 - $da2) / 2.0 }]
	set TOFB [expr { ($db1 - $db2) / 2.0 }]

	set TOF [expr { ($TOFA + $TOFB) / 2.0 }]

	set DST [expr { $TOF * $TF }]
	set DSA [expr { $TOFA * $TF }]
	set DSB [expr { $TOFB * $TF }]

	return [list $DST $DSA $DSB $TOF $TOFA $TOFB]
}

proc do_calib { line } {

	global OFFSET

	if ![regexp {^c[^[:space:]]*[[:space:]]+([^[:space:]]+)} $line jnk of] {
		return
	}

	if [catch { expr { $of } } of] {
		return
	}

	set OFFSET $of

	puts "OFFSET = $OFFSET"
}

proc main { } {

	global OFFSET

	set ave 0.0
	set tot 0.0
	set cnt 0

	while 1 {

		set samples ""

		while 1 {

			set rd [gets stdin line]

			if { $rd < 0 } {
				exit
			}

			set line [string trim $line]
			if { $line == "" } {
				continue
			}

			if { [lindex $line 0] == "c" } {
				# calibration
				do_calib $line
				set ave 0.0
				set tot 0.0
				set cnt 0
				continue
			}

			if { [lindex $line 0] == "z" } {
				set ave 0.0
				set tot 0.0
				set cnt 0
			}

			set num "0x$line"

			if [catch { expr $num } num] {
				continue
			}

			lappend samples $num

			if { [llength $samples] == 6 } {
				break
			}
		}

		set res [calc $samples]
		set dis [expr { [lindex $res 0] - $OFFSET }]
		set tot [expr { $tot + $dis }]
		incr cnt
		set ave [expr { $tot / $cnt }]

		puts "Number:        $cnt"
		puts "Distance:      [format %1.3f $dis]"
		puts "Distance A:    [format %1.3f [lindex $res 1]]"
		puts "Distance B:    [format %1.3f [lindex $res 2]]"
		puts "ATOF:          [format %1.3f [lindex $res 3]]"
		puts "ATOFA:         [format %1.3f [lindex $res 4]]"
		puts "ATOFB:         [format %1.3f [lindex $res 5]]"
		puts "Average:       [format %1.3f $ave]\n"
	}
}

set OFFSET 0.0

main
