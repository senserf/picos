package provide unames 1.0
##########################################################################
# This is a package for handling the various names under which COM ports #
# may appear in our messy setup.                                         #
# Copyright (C) 2012 Olsonet Communications Corporation.                 #
##########################################################################

namespace eval UNAMES {

variable Dev

proc unames_init { dtype { stype "" } } {

	variable Dev

	# device layout type: "L" (Linux), other "Windows"
	set Dev(DEV) $dtype
	# system type: "L" (Linux), other "Windows/Cygwin"
	set Dev(SYS) $stype

	if { $Dev(DEV) == "L" } {
		# determine the root of virtual ttys
		if [file isdirectory "/dev/pts"] {
			# BSD style
			set Dev(PRT) "/dev/pts/%"
		} else {
			set Dev(PRT) "/dev/pty%"
		}
		# number bounds (inclusive) for virtual devices
		set Dev(PRB) { 0 8 }
		# real ttys
		if { $Dev(SYS) == "L" } {
			# actual Linux
			set Dev(RRT) "/dev/ttyUSB%"
		} else {
			# Cygwin
			set Dev(RRT) "/dev/ttyS%"
		}
		# and their bounds
		set Dev(RRB) { 0 31 }
	} else {
		set Dev(PRT) { "CNCA%" "CNCB%" }
		set Dev(PRB) { 0 3 }
		set Dev(RRT) "COM%:"
		set Dev(RRB) { 1 32 }
	}

	unames_defnames
}

proc unames_defnames { } {
#
# Generate the list of default names
#
	variable Dev

	# flag == default names, not real devices
	set Dev(DEF) 1
	# true devices
	set Dev(COM) ""
	# virtual devices
	set Dev(VCM) ""

	set rf [lindex $Dev(RRB) 0]
	set rt [lindex $Dev(RRB) 1]
	set pf [lindex $Dev(PRB) 0]
	set pt [lindex $Dev(PRB) 1]

	while { $rf <= $rt } {
		foreach d $Dev(RRT) {
			regsub "%" $d $rf d
			lappend Dev(COM) $d
		}
		incr rf
	}

	while { $pf <= $pt } {
		foreach d $Dev(PRT) {
			regsub "%" $d $pf d
			lappend Dev(VCM) $d
		}
		incr pf
	}
}

proc unames_ntodev { n } {
#
# Proposes a device list for a number
#
	variable Dev

	regsub -all "%" $Dev(RRT) $n d
	return $d
}

proc unames_ntovdev { n } {
#
# Proposes a virtual device list for a number
#
	variable Dev

	regsub -all "%" $Dev(PRT) $n d

	if { $Dev(DEV) == "L" && ![file exists $d] } {
		# this is supposed to be authoritative
		return ""
	}
	return $d
}

proc unames_unesc { dn } {
#
# Escapes the device name so it can be used as an argument to open
#
	variable Dev

	if { $Dev(DEV) == "L" } {
		# no need to do anything
		return $dn
	}

	if [regexp -nocase "^com(\[0-9\]+):$" $dn jk pn] {
		set dn "\\\\.\\COM$pn"
	} else {
		set dn "\\\\.\\$dn"
	}

	return $dn
}

proc unames_scan { } {
#
# Scan actual devices
#
	variable Dev

	set Dev(DEF) 0
	set Dev(COM) ""
	set Dev(VCM) ""

	# real devices
	for { set i 0 } { $i < 256 } { incr i } {
		set dl [unames_ntodev $i]
		foreach d $dl {
			if [catch { open [unames_unesc $d] "r" } fd] {
				continue
			}
			catch { close $fd }
			lappend Dev(COM) $d
		}
	}

	for { set i 0 } { $i < 32 } { incr i } {
		set dl [unames_ntovdev $i]
		if { $dl == "" } {
			continue
		}
		if { $Dev(DEV) == "L" } {
			# don't try to open them; unames_ntovdev is
			# authoritative, and opening those representing
			# terminals may mess them up
			foreach d $dl {
				lappend Dev(VCM) $d
			}
			continue
		}
		foreach d $dl {
			if [catch { open [unames_unesc $d] "r" } fd] {
				continue
			}
			catch { close $fd }
			lappend Dev(VCM) $d
		}
	}
}

proc unames_fnlist { fn } {
#
# Returns the list of filenames to try to open, given an element from one of
# the lists; if not on the list, assume a direct name (to be escaped, however)
#
	variable Dev

	if [regexp "^\[0-9\]+$" $fn] {
		# just a number
		return [unames_ntodev $fn]
	}

	if { [lsearch -exact $Dev(COM) $fn] >= 0 } {
		if !$Dev(DEF) {
			# this is an actual device
			return $fn
		}
		# get a number and convert to a list
		if ![regexp "\[0-9\]+" $fn n] {
			return ""
		}
		return [unames_ntodev $n]
	}
	if { [lsearch -exact $Dev(VCM) $fn] >= 0 } {
		if !$Dev(DEF) {
			return $fn
		}
		if ![regexp "\[0-9\]+" $fn n] {
			return ""
		}
		return [unames_ntovdev $n]
	}
	# return as is
	return $fn
}

proc unames_choice { } {

	variable Dev

	return [list $Dev(COM) $Dev(VCM)]
}

namespace export unames_*

### end of UNAMES namespace ###################################################
}

namespace import ::UNAMES::*
