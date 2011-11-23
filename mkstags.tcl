#!/bin/sh
###################\
exec tclsh85 "$0" "$@"
#

##################################################################
# Copyright (C) Olsonet Communications, 2011 All Rights Reserved #
##################################################################

##
## This script performs the following actions:
##
## If called without arguments, it locates all system files used by the
## project (based on the combined contents of Makefiles in the current
## directory) and executes elvtags for all those files to produce a combined
## ctags file on the standard output.
##
## If called with -f, it just performs the first step and returns the sorted
## list of unique file paths on the standard output. PIP calls it this way,
## if it just wants to learn which system files the project uses.
##
## If called with -F followed by the CPU type (msp430, eCOG), it returns the
## list of all system files for the given CPU type, regardless of whether the
## project uses them or not (the Makefiles are irrelevant) on the standard
## output.
##

###############################################################################
# Determine the system type ###################################################
###############################################################################

if [catch { exec uname } ST(SYS)] {
	set ST(SYS) "W"
} elseif [regexp -nocase "linux" $ST(SYS)] {
	set ST(SYS) "L"
} elseif [regexp -nocase "cygwin" $ST(SYS)] {
	set ST(SYS) "C"
} else {
	set ST(SYS) "W"
}

## this is the full path to the project's directory, i.e., the current one
set WD [file normalize [pwd]]

## determine the preferred path format
if [regexp -nocase "^\[a-z\]:" $WD] {
	# DOS paths
	set ST(DP) 1
} else {
	# UNIX paths
	set ST(DP) 0
}

###############################################################################

set TagsCmd "elvtags"
set TagsArgs "-l -i -t -v -h --"
set PPCmd "picospath"
## zero-level directories other than ..cpu.. to scan with -F (lower case)
set PPDirs { libs kernel }

###############################################################################

proc xq { pgm { pargs "" } } {
#
# A flexible exec (or so I hope)
#
	set ef [auto_execok $pgm]
	if ![file executable $ef] {
		set ret [eval [list exec] [list sh] [list $ef] $pargs]
	} else {
		set ret [eval [list exec] [list $ef] $pargs]
	}
	return $ret
}

proc inside_project { f } {
#
# Checks if the specified path refers to something inside the project
#
	global WD

	if { [string first $WD $f] == 0 } {
		# OK
		return 1
	}
	return 0
}

proc scan_mkfile { mfn } {
#
	global ST FLIST

	if [catch { open $mfn "r" } fd] {
		return
	}

	if [catch { read $fd } mf] {
		catch { close $mf }
		return
	}

	catch { close $fd }

	set mf [split $mf "\n"]

	foreach ln $mf {
		# collect the list of file paths
		if { (  $ST(DP) &&
	            [regexp "^(\[IS\]\[0-9\]+)=(\[A-Z\]:.*)" $ln jk pf fn] ) ||
		     ( !$ST(DP) &&
		    [regexp "^(\[IS\]\[0-9\]+)U?=(/home/.*)" $ln jk pf fn] ) } {

			set FS($pf) [string trimright $fn]
			continue
		}
		# check for the special case of "current" directory
		if [regexp "^(\[IS\]\[0-9\]+)=\[.\]" $ln jk pf] {
			set FS($pf) "."
			continue
		}
		# scan for file reference
		while 1 {

			if ![regexp "\\$\\((\[IS\]\[0-9\]+)U?\\)/(\[^ \t\]+)" \
				$ln ma pf fn] {
					break
			}

			# remove the match from the string
			set ix [string first $ma $ln]
			if { $ix < 0 } {
				# impossible
				break
			}
			set ln "[string range $ln 0 [expr $ix-1]][string range \
				$ln [expr $ix + [string length $ma]] end]"

			# check if one of interesting files
			set ex [file extension $fn]
			if { $ex != ".cc" && $ex != ".c" && $ex != ".h" } {
				# ignore
				continue
			}
			if ![info exists FS($pf)] {
				# this is impossible, unless the makefile is
				# broken
				continue
			}

			set fn [file normalize [file join $FS($pf) $fn]]

			if [inside_project $fn] {
				# ignore project files, which are taken care of
				# internally
				continue
			}

			set FLIST($fn) ""
		}

	}
}

proc make_list_of_project_related_system_files { } {

	global FLIST

	if { [catch { glob "Makefile*" } ml] || $ml == "" } {
		return
	}

	foreach m $ml {
		scan_mkfile $m
	}

	set fl [lsort [array names FLIST]]

	array unset FLIST

	return $fl
}

proc make_list_of_all_system_files { cpu } {

	global PPCmd FLIST PDIR

	# PicOS path
	set PDIR [file join [xq $PPCmd ""] "PicOS"]

	set FLIST ""

	# the files will be PICOS/PicOS-relative
	traverse $cpu ""
}

proc traverse { c p } {
#
# Traverses recursively directories collecting all file names
#
	global FLIST PDIR PPDirs

	set wh [file join $PDIR $p]

	if [catch { glob -directory $wh -tails * } sdl] {
		return
	}

	foreach f $sdl {
		set ff [file join $wh $f]
		if [file isfile $ff] {
			lappend FLIST [file join $p $f]
			continue
		}
		if ![file isdirectory $ff] {
			continue
		} 
		if [catch { file lstat $ff vv } ] {
			continue
		}
		if { $vv(type) != "directory" } {
			continue
		}
		array unset vv
		if { $c != "" } {
			# zero level
			set f [string tolower $f]
			if { $f != $c && [lsearch $f $PPDirs] < 0 } {
				# ignore
				continue
			}
		}
		traverse "" [file join $p $f]
	}
}

###############################################################################

set FL [lsearch -exact $argv "-F"]
if { $FL >= 0 } {
	set cpu [lindex $argv [expr $FL + 1]]
	if { $cpu == "" } {
		set cpu "msp430"
	} else {
		# unify the case
		set cpu [string tolower $cpu]
	}
	make_list_of_all_system_files $cpu
	puts [lsort $FLIST]
	exit 0
}

set FL [make_list_of_project_related_system_files]

if { [lsearch -exact $argv "-f"] >= 0 } {
	puts $FL
	exit 0
}

if { [llength $FL] > 0 } {
	# make the tags
	puts [xq $TagsCmd [concat $TagsArgs $FL]]
}
