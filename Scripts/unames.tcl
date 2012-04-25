package provide unames 1.0
##########################################################################
# This is a package for handling the various names under which COM ports #
# may appear in our messy setup.                                         #
# Copyright (C) 2012 Olsonet Communications Corporation.                 #
##########################################################################

namespace eval UNAMES {

variable Dev

proc unames_init { stype } {

	variable Dev

	set Dev(SYS) $stype
	unames_defnames

	if { $Dev(SYS) == "L" } {
		# determine the root of virtual ttys
		if [file isdirectory "/dev/pts"] {
			# BSD style
			set Dev(PRT) "/dev/pts/"
		} else {
			set Dev(PRT) "/dev/pty"
		}
	}
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

	if { $Dev(SYS) == "L" } {
		# Linux style
		for { set i 0 } { $i < 32 } { incr i } {
			lappend Dev(COM) "ttyS$i (COM[expr $i + 1])"
		}
		for { set i 0 } { $i < 9 } { incr i } {
			lappend Dev(VCM) "pty$i (virt)"
		}
	} else {
		for { set i 1 } { $i <= 32 } { incr i } {
			lappend Dev(COM) "COM$i:"
		}
		for { set i 0 } { $i < 4 } { incr i } {
			lappend Dev(VCM) "CNCA$i"
			lappend Dev(VCM) "CNCB$i"
		}
	}
}

proc unames_ntodev { n } {
#
# Proposes a device list for a number
#
	variable Dev

	if { $Dev(SYS) == "L" } {
		return "/dev/ttyS$n /dev/ttyUSB$n /dev/tty$n"
	} else {
		if { $n < 1 } {
			return ""
		}
		return "COM$n:"
	}
}

proc unames_ntovdev { n } {
#
# Proposes a virtual device list for a number
#
	variable Dev

	if { $Dev(SYS) == "L" } {
		return "$Dev(PRT)$n"
	} else {
		if { $n > 3 } {
			return ""
		}
		return "CNCA$n CNCB$n"
	}
}

proc unames_unesc { dn } {
#
# Escapes the device name so it can be used as an argument to open
#
	variable Dev

	if { $Dev(SYS) == "L" } {
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
		if { $dl == "" } {
			continue
		}
		foreach d $dl {
			if [catch { open [unames_unesc $d] "r" } fd] {
				continue
			}
			catch { close $fd }
			lappend Dev(COM) $d
		}
	}

	# virtual devices
	for { set i 0 } { $i < 32 } { incr i } {
		set dl [unames_ntovdev $i]
		if { $dl == "" } {
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

### end of UNAMES namespace ####################################################
}

namespace import ::UNAMES::*
