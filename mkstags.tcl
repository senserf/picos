#!/bin/sh
###################\
exec tclsh "$0" "$@"
#
# Creates ctags from the system files used by the project
#

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
		    [regexp "^(\[IS\]\[0-9\]+)U=(/home/.*)" $ln jk pf fn] ) } {

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

proc make_file_list { } {

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

proc make_ctags { f } {

	global TagsCmd TagsArgs

	puts [xq $TagsCmd [concat $TagsArgs $f]]
}

set FL [make_file_list]

if { [llength $FL] > 0 } {
	make_ctags $FL
}
