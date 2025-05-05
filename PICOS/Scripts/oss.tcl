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
exec wish "$0" "$@"

###############################################################################
# Do this to combine the app oss script with the starter:
#
# 1. remove the exec above (and the preceding comment)
# 2. insert the script into USPEC (see below)
# 3. set PM(CON) "r" (below)
# 4. run freewrap on the result
###############################################################################

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
if { $ST(SYS) != "L" } {
	# sanitize arguments
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}
	unset u
}

###############################################################################
# Determine the way devices are named; if running natively under Cygwin, use
# Linux style
###############################################################################

if [file isdirectory "/dev"] {
	set ST(DEV) "L"
} else {
	set ST(DEV) "W"
}

set ST(WSH) [info tclversion]

if { $ST(WSH) < 8.5 } {
	puts stderr "$argv0 requires Tcl/Tk 8.5 or newer!"
	exit 99
}

######################################################################
### this must be replaced by hand when wrapping up for standalone exec
######################################################################

proc xq { pgm { pargs "" } } {
#
# A flexible exec
#
	set ef [auto_execok $pgm]
	if ![file executable $ef] {
		set ret [eval [list exec] [list sh] [list $ef] $pargs]
	} else {
		set ret [eval [list exec] [list $ef] $pargs]
	}
	return $ret
}

if [catch { xq picospath } PicOSPath] {
	puts stderr "cannot locate PicOS path: $PicOSPath"
	exit 99
}

lappend auto_path [file join $PicOSPath "Scripts" "Packages"]

###############################################################################
# Standard packages ###########################################################
###############################################################################

package require uartpoll
package require unames
package require vuart
package require autoconnect
package require noss

###############################################################################
# OSS LIB, not a package, just a namespace ####################################
###############################################################################

namespace eval OSS {

variable OSSP
variable OSST
variable OSSCN
variable OSSCC
variable OSSMN
variable OSSMC

set OSSP(OPTIONS) { match number skip string start save restore then return
			subst checkpoint time }

set OSSI(OPTIONS) { id speed length parser connection }

set OSSI(SPEEDS) { 1200 2400 4800 9600 14400 19200 28800 38400 76800 115200
			230400 256000 }

# Current (working) parsed line + position of last action
set OSSP(CUR) ""
set OSSP(POS) 0
set OSSP(THN) 0

set OSST(word) 	[list 2 "su" 0 65535]
set OSST(sint) 	[list 2 "s" -32768 32767]
set OSST(lword) [list 4 "iu" 0 4294967295]
set OSST(lint) 	[list 4 "i" -2147483648 2147483647]
set OSST(byte) 	[list 1 "cu" 0 255]
set OSST(char) 	[list 1 "c" -128 127]
set OSST(blob) 	[list 2 ""]

# To keep track of errors in specification
set OSSI(ERRORS) 	""

###############################################################################
###############################################################################

proc oss_getwin { { separate 0 } } {
#
# Acquires a user window for private GUI
#
	global WI

	if $separate {
		set wi .user_$WI(USR)
		toplevel $wi
		incr WI(USR)
		return $wi
	}

	# top frame of standard window
	return ".user"
}

proc oss_isconnected { } {

	global ST

	if { $ST(UCS) != "" && $ST(HSK) } {
		return 1
	}

	return 0
}

proc oss_exit { } {

	sy_exit
}

###############################################################################

proc oss_keymatch { key klist } {
#
# Finds the closest match of key to keys in klist
#
	set res ""
	foreach k $klist {
		if { [string first $key $k] == 0 } {
			lappend res $k
		}
	}

	if { $res == "" } {
		error "$key not found"
	}

	if { [llength $res] > 1 } {
		error "multiple matches for $key: [join $res]"
	}

	return [lindex $res 0]
}

proc oss_isalnum { txt } {

	return [regexp {^[[:alpha:]_][[:alnum:]_]*$} $txt]
}

proc oss_valint { n { min "" } { max "" } } {
#
# Validate an integer value
#
	set n [string tolower [string trim $n]]
	if { $n == "" } {
		error "empty string"
	}

	if { [string first "." $n] >= 0 || [string first "e" $n] >= 0 } {
		error "not an integer number"
	}

	if [catch { expr $n } n] {
		error "not a number"
	}

	if { $min != "" && $n < $min } {
		error "must not be less than $min"
	}

	if { $max != "" && $n > $max } {
		error "must not be greater than $max"
	}

	return $n
}

proc oss_blobtovalues { blob } {
#
# Convert a blob to a list of numerical values
#
	if { [string length $blob] < 2 } {
		error "blob missing"
	}

	binary scan $blob su size

	if { $size == 0 } {
		return ""
	}

	set blob [string range $blob 2 [expr $size + 1]]
	if { [string length $blob] != $size } {
		error "blob data too short, $size bytes expected"
	}

	set res ""

	for { set i 0 } { $i < $size } { incr i } {
		binary scan [string index $blob $i] cu val
		lappend res "0x[format %02X $val]"
	}

	return $res
}

proc oss_blobtostring { blob } {
#
# Convert a blob to a string
#
	if { [string length $blob] < 2 } {
		error "blob missing"
	}

	binary scan $blob su size

	if { $size == 0 } {
		return ""
	}

	set blob [string range $blob 2 [expr $size + 1]]
	if { [string length $blob] != $size } {
		error "blob data too short, $size bytes expected"
	}

	# remove the sentinel(s) on the right
	return [string trimright $blob [binary format c 0]]
}

proc oss_valuestoblob { vals } {
#
# Convert numerical values to a blob
#
	set cnt [llength $vals]

	if { $cnt > 65535 } {
		set cnt 65535
		set vals [lrange $vals 0 65534]
	}

	set res "[binary format s $cnt][oss_bytestobin $vals]"

	return $res
}

proc oss_stringtoblob { vals } {

	set zer [binary format c 0]
	if { [string index $vals end] != $zer } {
		append vals $zer
	}

	set len [string length $vals]
	if { $len > 65535 } {
		set len 65535
		set vals "[string range $vals 0 65533]$zer"
	}

	return "[binary format s $len]$vals"
}

proc oss_bintobytes { block } {

	set line ""
	set i 0
	set l [string length $block]

	for { set i 0 } { $i < $l } { incr i } {
		binary scan [string index $block $i] cu val
		append line " 0x[format %02x $val]"
	}

	return $line
}

proc oss_bytestobin { bytes } {

	set res ""

	for { set i 0 } { $i < [llength $bytes] } { incr i } {
		set vv [lindex $bytes $i]
		if [catch { expr $vv & 0xFF } val] {
			error "illegal value in list of bytes, $vv"
		}
		append res [binary format c $val]
	}

	return $res
}

proc parse_subst { a r l p } {
#
# Does variable substitution in the command
#
	upvar $r res
	upvar $l line
	upvar $p ptr

	set line [oss_evalscript "subst { $line }"]
	# trc "SUBST: $line"
	lappend res $line
	set ptr 0

	return 0
}

proc parse_skip { a r l p } {
#
# Skip spaces (or the indicated characters), returns the first non-skipped
# character
#
	upvar $a args
	upvar $r res
	upvar $l line
	upvar $p ptr

	set na [lindex $args 0]

	if { $na != "" && [string index $na 0] != "-" } {
		# use as the set of characters
		set args [lrange $args 1 end]
		set line [string trimleft $line $na]
	} else {
		set line [string trimleft $line]
	}

	lappend res [string index $line 0]

	# pointer always at the beginning
	set ptr 0

	# success; this one always succeeds
	return 0
}

proc parse_match { a r l p } {

	upvar $a args
	upvar $r res
	upvar $l line
	upvar $p ptr

	set pat [lindex $args 0]
	set args [lrange $args 1 end]

	if { $pat == "" } {
		error_oss_parse "illegal empty pattern for -match"
	}

	if [catch { regexp -inline -indices -- $pat $line } mat] {
		error_oss_parse "illegal pattern for -match, $mat"
	}

	if { $mat == "" } {
		# no match at all, failure
		return 1
	}

	set ix [lindex [lindex $mat 0] 0]
	set iy [lindex [lindex $mat 0] 1]

	foreach m $mat {
		set fr [lindex $m 0]
		set to [lindex $m 1]
		lappend res [string range $line $fr $to]
	}

	set line [string replace $line $ix $iy]
	set ptr $ix

	return 0
}

proc parse_number { a r l p } {
#
# Parses something that should amount to a number, which can be an expression
#
	upvar $a args
	upvar $r res
	upvar $l line
	upvar $p ptr

	if [regexp {^[[:space:]]*"} $line] {
		# trc "PNUM: STRING!"
		# a special check for a string which fares fine as an
		# expression, but we don't want it; if it doesn't open the
		# expression, but occurs inside, that's fine
		return 1
	}

	set ll [string length $line]
	set ix $ll
	# trc "PNUM: $ix <$line>"

	while 1 {

		incr ix -1
		if { $ix < 0 } {
			# failure
			return 1
		}

		if ![catch { expr [string range $line 0 $ix] } val] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			lappend res $val
			set ptr 0
			return 0
		}
	}
}

proc parse_time { a r l p } {
#
# Parses a time string than can be handled by clock scan
#
	upvar $a args
	upvar $r res
	upvar $l line
	upvar $p ptr

	set ll [string length $line]
	set ix $ll

	while 1 {

		incr ix -1
		if { $ix < 0 } {
			# failure
			return 1
		}

		if ![catch { clock scan [string range $line 0 $ix] } val] {
			# found, remove the match
			set line [string range $line [expr $ix + 1] end]
			lappend res $val
			set ptr 0
			return 0
		}
	}
}

proc ishex { c } {
	return [regexp -nocase "\[0-9a-f\]" $c]
}

proc isoct { c } {
	return [regexp -nocase "\[0-7\]" $c]
}

proc parse_string { a r l p } {
#
# Parses a string
#
	upvar $a args
	upvar $r res
	upvar $l line
	upvar $p ptr

	set par [lindex $args 0]

	if { $par != "" && [string index $par 0] != "-" } {
		# this is our parameter, string length
		set args [lrange $args 1 end]
		if [catch { oss_valint $par 0 1024 } par] {
			error_oss_parse "illegal parameter for -string, $par,\
				must be a number between 0 and 1024"
		}
	} else {
		# apply delimiters
		set par ""
	}

	if { $par == "" } {
		set nc [string index $line 0]
		if { $nc != "\"" } {
			# failure, must start with "
			return 1
		}
		set mline [string range $line 1 end]
	} else {
		set mline $line
	}

	set vals ""
	set nchs 0

	while 1 {

		if { $par != "" && $par != 0 && $nchs == $par } {
			# we have the required number of characters
			break
		}

		set nc [string index $mline 0]

		if { $nc == "" } {
			if { $par != "" } {
				# this is OK
				break
			}
			# assume no match
			return 1
		}

		set mline [string range $mline 1 end]

		if { $par == "" && $nc == "\"" } {
			# done 
			break
		}

		if { $nc == "\\" } {
			# escapes
			set c [string index $mline 0]
			if { $c == "" } {
				# delimiter error, will be diagnosed at next
				# turn
				continue
			}
			if { $c == "x" } {
				# get hex digits
				set mline [string range $mline 1 end]
				while 1 {
					set d [string index $mline 0]
					if ![ishex $d] {
						break
					}
					append c $d
					set mline [string range $mline 1 end]
				}
				if [catch { expr 0$c % 256 } val] {
					error "illegal escape in -string 0$c"
				}
				lappend vals $val
				incr nchs
				continue
			}
			if [isoct $c] {
				if { $c != 0 } {
					set c "0$c"
				}
				# get octal digits
				set mline [string range $mline 1 end]
				while 1 {
					set d [string index $mline 0]
					if ![isoct $d] {
						break
					}
					append c $d
					set mline [string range $mline 1 end]
				}
				if [catch { expr $c % 256 } val] {
					error "illegal escape in -string $c"
				}
				lappend vals $val
				incr nchs
				continue
			}
			set mline [string range $mline 1 end]
			set nc $c
		}
		scan $nc %c val
		lappend vals [expr $val % 256]
		incr nchs
	}

	lappend res $vals
	set ptr 0
	set line $mline
	return 0
}

proc truncmt { m } {

	if { [string length $m] > 13 } {
		return "[string range $m 0 9]..."
	}

	return $m
}

proc error_oss_parse { msg } {

	error "oss_parse error, $msg"
}

proc oss_parse { args } {
#
# The parser
#
	variable OSSP
	variable OSSS

	set res ""
	# checkpoint
	set chk ""

	while { $args != "" } {

		set what [lindex $args 0]
		set args [lrange $args 1 end]

		if { [string index $what 0] != "-" } {
			error_oss_parse "selector $what doesn't start with '-'"
		}

		set what [string range $what 1 end]

		if [catch { oss_keymatch $what $OSSP(OPTIONS) } w] {
			error_oss_parse $w
		}
		
		# those that do not return values are serviced directly
		switch $w {

			"start" {

				set OSSP(CUR) [lindex $args 0]
				set args [lrange $args 1 end]
				set OSSP(POS) 0
				set OSSP(THN) 0
				# remove any saves
				array unset OSSS
				continue
			}

			"restore" {

				set p [lindex $args 0]
				if { $p != "" && [string index $p 0] != "-" } {
					if ![oss_isalnum $p] {
						error_oss_parse "illegal\
						  -restore tag, must be\
						  alphanumeric"
					}
					set args [lrange $args 1 end]
					
				} else {
					set p "+"
				}
				if ![info exists OSSS($p)] {
					error_oss_parse "-restore tag not found"
				}
				set OSSP(CUR) [lindex $OSSS($p) 0]
				set OSSP(POS) [lindex $OSSS($p) 1]
				set OSSP(THN) [lindex $OSSS($p) 2]
				continue
			}

			"save" {

				set p [lindex $args 0]
				if { $p != "" && [string index $p 0] != "-" } {
					if ![oss_isalnum $p] {
						error_oss_parse "illegal\
						  -save tag, must be\
						  alphanumeric"
					}
					set args [lrange $args 1 end]
					
				} else {
					set p "+"
				}
				set OSSS($p) [list $OSSP(CUR) $OSSP(POS) \
					$OSSP(THN)]
				continue
			}

			"then" {
				set OSSP(THN) 1
				continue
			}

			"return" {
				set na [lindex $args 0]
				if { $na != "" &&
				    [string index $na 0] != "-" } {
					if [catch { oss_valint $na 0 1024 } \
					    na] {
						error_oss_parse "illegal\
						    argument of -return, $na"
					}
					set res [lindex $res $na]
				}
				return $res
			}

			"checkpoint" {
				set chk [list $OSSP(CUR) $OSSP(POS) $OSSP(THN)]
				continue
			}
		}

		set ptr 0
		if $OSSP(THN) {
			# after the current pointer
			set line [string range $OSSP(CUR) $OSSP(POS) end]
		} else {
			set line $OSSP(CUR)
		}

		if [parse_$w args res line ptr] {
			# failure
			if { $chk != "" } {
				set OSSP(CUR) [lindex $chk 0]
				set OSSP(POS) [lindex $chk 1]
				set OSSP(THN) [lindex $chk 2]
			} else {
				set OSSP(THN) 0
			}
			return ""
		}

		if $OSSP(THN) {
			set OSSP(CUR) "[string range $OSSP(CUR) 0 \
				[expr $OSSP(POS) - 1]]$line"
			incr OSSP(POS) $ptr
		} else {
			set OSSP(POS) $ptr
			set OSSP(CUR) $line
		}
		set OSSP(THN) 0
	}

	return $res
}

###############################################################################
###############################################################################

proc process_struct { struct } {
#
# Parses the structure (command or message)
#
	variable OSST

	set len 0
	set stl ""
	set blof 0

	# remove comments
	regsub -line -all {^[[:blank:]]*#.*} $struct "" struct

	while 1 {

		set struct [string trimleft $struct]
		if { $struct == "" } {
			break
		}

		if ![regexp {^([[:alpha:]]+)[[:space:]]+} $struct mat tp] {
			error "illegal attribute syntax: [truncmt $struct]"
		}

		if ![info exists OSST($tp)] {
			error "unknown type, must be one of: [array names OSST]"
		}

		# remove the match
		set struct [string range $struct [string length $mat] end]

		if { $tp == "blob" } {
			if $blof {
				error "you cannot have more than one \
						blob attribute"
			}
			set blof 1
		} else {
			if $blof {
				error "a blob attribute can only occur\
					as the last one within the structure"
			}
		}

		# size
		set tis [lindex $OSST($tp) 0]

		while { $tis > 1 && [expr { $len % $tis }] } {
			# alignment
			incr len
		}

		# parse the name
		if ![regexp {^([[:alpha:]_][[:alnum:]_]+)} $struct nm] {
			error "illegal attribute syntax, name expected\
				after $tp: [truncmt $struct]"
		}

		if [info exists nms($nm)] {
			error "duplicate attribute name: $nm"
		}

		# remove the match
		set struct [string range $struct [string length $nm] end]

		set nms($nm) ""

		set struct [string trimleft $struct]
		set cc [string index $struct 0]

		if { $cc == "\[" } {
			if { $tp == "blob" } {
				error "blob cannot be dimensioned"
			}
			if ![regexp {^.([^;\[]+)\]} $struct mat ex] {
				error "illegal dimension for $nm:\
					[truncmt $struct]"
			}
			# remove the match
			set struct [string range $struct [string length $mat] \
				end]

			if { [catch { expr $ex } dim] || \
			    [catch { oss_valint $dim 1 256 } dim] } {
				error "illegal dimension for $nm, $dim"
			}

			set struct [string trimleft $struct]
			set cc [string index $struct 0]
			set cnt $dim
		} else {
			set cnt 1
			set dim 0
		}

		if { $cc != ";" } {
			error "semicolon missing at the end of $nm\
				specification"
		}

		set struct [string range $struct 1 end]

		# offset length count type name
		lappend stl [list $tp $nm $tis $dim $len]
		incr len [expr $tis * $cnt]
	}

	return [list $stl $len]
}

###############################################################################
###############################################################################

proc oss_ierr { msg } {
#
# Handles an error while parsing the spec file; we keep going until the
# error cound exceeds the max, then we throw an exception to stop the parser
#
	variable OSSI

	lappend OSSI(ERRORS) $msg

	if { [llength $OSSI(ERRORS)] > 11 } {
		error "Too many errors"
	}
}

proc oss_errors { } {

	variable OSSI

	return $OSSI(ERRORS)
}

proc oss_verify { } {

	variable OSSI
	variable OSSCN
	variable OSSMN

	set prs $::PM(PRS)
	set pfi [lindex $prs 0]
	set pse [lindex $prs 1]

	if { $pfi != "" && [info procs [sy_localize $pfi "USER"]] == "" } {
		lappend OSSI(ERRORS) "The specified command parse function $pfi\
			is not implemented"
	}

	if { $pse != "" && [info procs [sy_localize $pse "USER"]] == "" } {
		lappend OSSI(ERRORS) "The specified message parse function $pse\
			is not implemented"
	}

	if { [array names OSSCN] == "" } {
		lappend OSSI(ERRORS) "No commands have been defined"
	}

	if { [array names OSSMN] == "" } {
		lappend OSSI(ERRORS) "No messages have been defined"
	}

	if { $OSSI(ERRORS) != "" } {
		error [join [oss_errors] "\n"]
	}
}

proc oss_interface { args } {
#
# Declares the interface
#
	variable OSSI

	while { $args != "" } {

		set what [lindex $args 0]
		set arg [lindex $args 1]
		set args [lrange $args 2 end]

		if { [string index $what 0] != "-" } {

			oss_ierr "oss_interface, $illegal selector, must\
				start with -"
			return
		}

		set what [string range $what 1 end]

		if [catch { oss_keymatch $what $OSSI(OPTIONS) } key] {
			oss_ierr "oss_interface, $key"
			continue
		}

		if [info exists dupl($key)] {
			oss_ierr "oss_interface, duplicate option -$key"
			continue
		}

		set dupl($key) ""

		###############################################################

		switch $key {

		"id" {

			if [catch { oss_valint $arg 0 0xFFFFFFFF } v] {
				oss_ierr "oss_interface, illegal -id, $v"
				continue
			}

			set ::PM(PXI) $v
		}

		"speed" {

			if { [catch { oss_valint $arg 1200 256000 } v] ||
			    [lsearch -exact $OSSI(SPEEDS) $v] < 0 } {
				oss_ierr "oss_interface, illegal -speed, must\
				    be one of [join $OSSI(SPEEDS)]"
				continue
			}

			set ::PM(USP) $v
		}

		"length" {

			if { [catch { oss_valint $arg 12 252 } v] || 
			    [expr $v & 1] } {
				oss_ierr "oss_interface, illegal -length, must\
				    be an even number between 12 and 252"
				continue
			}

			set ::PM(MPL) $v
		}

		"parser" {

			set cpd [lindex $arg 0]
			set mps [lindex $arg 1]
			set ini [lindex $arg 2]

			if { $cpd == "" && $mps == "" || $cpd != "" &&
			  ![oss_isalnum $cpd] || $mps != "" && 
			  ![oss_isalnum $mps] } {
				oss_ierr "oss_interface, illegal -parser\
				    argument, must be one or two (a list) \
				    function names"
				continue
			}

			set ::PM(PRS) $arg
		}

		"connection" {

			if $::PM(CVR) {
				# overriden by call parameters
				break
			}
				
			set cty [string tolower [lindex $arg 0]]

			set err [sy_valvp [lrange $arg 1 end]]
			if { $err != "" } {
				oss_ierr "oss_interface, $err"
			}

			set conn ""

			if { [string first "v" $cty] >= 0 } {
				append conn "v"
			}
			if { [string first "r" $cty] >= 0 } {
				append conn "r"
			}
			if { $conn == "v" && [string first "*" $cty] >= 0 } {
				append conn "*"
			}
			if { $conn == "" } {
				oss_ierr "oss_interface, illegal -connection\
					argument, must be vuee, vuee*, real, or\
					vuee+real"
			}

			set ::PM(CON) $conn
		}

		}
		###############################################################
	}
}

proc oss_command { name code struct } {
#
# Declares a command layout
#
	global PM
	variable OSSCC
	variable OSSCN

	if ![oss_isalnum $name] {
		oss_ierr "oss_command name ($name) should be alphanumeric"
		return
	}

	if [info exists OSSCN($name)] {
		oss_ierr "oss_command, duplicate command name $name"
		return
	}

	if [catch { oss_valint $code 1 255 } cc] {
		oss_ierr "oss_command $name, invalid code, $cc"
		return
	}

	if [info exists OSSCC($cc)] {
		oss_ierr "oss_command $name, duplicate code $code"
	}

	if [catch { process_struct $struct } str] {
		oss_ierr "oss_command $name, $str"
		return
	}

	set OSSCN($name) [list $name $str $code]

	# check if the length is within limits
	set len [lindex $str 1]

	# effective length of the command body (struct)
	set efl [expr $PM(MPL) - 4]

	if { $len > $efl } {
		oss_ierr "oss_command $name, command structure too long: $len,\
			the limit (implied by the -length attribute of\
			oss_interface) is $efl"
	}
			
	set OSSCC($cc) $OSSCN($name)
}

proc oss_message { name code struct } {
#
# Declares a message layout
#
	global PM
	variable OSSMC
	variable OSSMN

	if ![oss_isalnum $name] {
		oss_ierr "oss_message name ($name) should be alphanumeric"
		return
		
	}

	if [info exists OSSMN($name)] {
		oss_ierr "oss_message, duplicate message name $name"
		return
	}

	if [catch { oss_valint $code 1 255 } cc] {
		oss_ierr "oss_message $name, invalid code, $cc"
		return
	}

	if [info exists OSSMC($cc)] {
		oss_ierr "oss_message $name, duplicate code $code"
	}

	if [catch { process_struct $struct } str] {
		oss_ierr "oss_message $name, $str"
		return
	}

	# check if the length is within limits
	set len [lindex $str 1]

	# effective length of the message body (struct)
	set efl [expr $PM(MPL) - 2]

	if { $len > $efl } {
		oss_ierr "oss_message $name, message structure too long: $len,\
			the limit (implied by the -length attribute of\
			oss_interface) is $efl"
	}
	
	set OSSMN($name) [list $name $str $code]
	set OSSMC($cc) $OSSMN($name)
}

proc oss_getcmdstruct { nc { nam "" } { cod "" } { len "" } } {
#
# Get the command structure by name or code
#
	variable OSSCC
	variable OSSCN

	if [info exists OSSCN($nc)] {
		set c $OSSCN($nc)
	} elseif [info exists OSSCC($nc)] {
		set c $OSSCC($nc)
	} else {
		return ""
	}

	if { $nam != "" } {
		upvar $nam n
		set n [lindex $c 0]
	}

	if { $cod != "" } {
		upvar $cod k
		set k [lindex $c 2]
	}

	set c [lindex $c 1]

	if { $len != "" } {
		upvar $len l
		set l [lindex $c 1]
	}

	return [lindex $c 0]
}

proc oss_getmsgstruct { nc { nam "" } { cod "" } { len "" } } {
#
# Get the message structure by name or code
#
	variable OSSMC
	variable OSSMN

	if [info exists OSSMN($nc)] {
		set c $OSSMN($nc)
	} elseif [info exists OSSMC($nc)] {
		set c $OSSMC($nc)
	} else {
		return ""
	}

	if { $nam != "" } {
		upvar $nam n
		set n [lindex $c 0]
	}

	if { $cod != "" } {
		upvar $cod k
		set k [lindex $c 2]
	}

	set c [lindex $c 1]

	if { $len != "" } {
		upvar $len l
		set l [lindex $c 1]
	}

	return [lindex $c 0]
}

proc oss_getvalues { blk nc { bstr 0 } } {
#
# Retrieve values from binary blk according to message nc, bstr means that
# a blob is to be treated as a string
#
	variable OSST

	set res ""

	set str [oss_getmsgstruct $nc]
	if { $str == "" } {
		error "message $nc not found"
	}

	while 1 {

		if { $str == "" } {
			return $res
		}

		foreach { tp nm ts di of } [lindex $str 0] { }
		set str [lrange $str 1 end]

		if { $tp == "blob" } {
			# this one is a bit special
			set b [string range $blk $of end]
			if $bstr {
				lappend res [oss_blobtostring $b]
			} else {
				lappend res [oss_blobtovalues $b]
			}
			return $res
		}

		# select the format based on type
		set fm [lindex $OSST($tp) 1]

		if { $di == 0 } {
			# a single item
			set b [string range $blk $of [expr $of + $ts - 1]]
			if { [string length $b] < $ts } {
				# truncated
				error "message shorter than structure,\
					attribute $nm missing"
			}
			binary scan $b $fm d
			lappend res $d
			continue
		}

		# an array
		set tre ""
		set siz [expr $di * $ts]
		set b [string range $blk $of [expr $of + $siz - 1]]
		if { [string length $b] < $siz } {
			# truncated
			error "message shorter than structure, attribute\
				$nm missing"
		}

		while { $di } {

			binary scan $b $fm d
			lappend tre $d
			set b [string range $b $ts end]
			incr di -1
		}

		lappend res $tre
	}
}

proc oss_setvalues { vals nc { bstr 0 } } {
#
# Constructs a command block from vals according to structure str, bstr as above
#
	variable OSST

	set res ""
	set fil 0
	set zer [binary format c 0]

	set stru [oss_getcmdstruct $nc]

	if { $stru == "" } {
		error "command $nc not found"
	}

	while 1 {

		if { $vals == "" } {
			if { $stru != "" } {
				error "oss_setvalues, more structure attributes\
					than values"
			}
			break
		}

		if { $stru == "" } {
			error "oss_setvalues, more values than structure\
				attributes"
		}

		set val [lindex $vals 0]
		set str [lindex $stru 0]
		set vals [lrange $vals 1 end]
		set stru [lrange $stru 1 end]

		foreach { tp nm ts di of } $str { }

		while { $fil < $of } {
			# align as needed
			append res $zer
			incr fil
		}

		if { $tp == "blob" } {
			if $bstr {
				append res [oss_stringtoblob $val]
			} else {
				append res [oss_valuestoblob $val]
			}
			break
		}

		set fm [string index [lindex $OSST($tp) 1] 0]

		if $di {
			set dy $di
			while { $di } {
				if [catch { expr [lindex $val 0] } v] {
					error "illegal item in value list for\
					oss_setvalues: $val (should be a list\
					of $dy numbers)"
				}
				append res [binary format $fm $v]
				set val [lrange $val 1 end]
				incr di -1
				incr fil $ts
			}
		} else {
			if [catch { expr $val } v] {
				error "illegal item in value list for\
					oss_setvalues: $val (should be a\
					number)"
			}
			append res [binary format $fm $v]
			incr fil $ts
		}
	}

	return $res
}

proc oss_defparse { line { bstr 0 } } {
#
# Default command parser: bstr = parse blob as string
#
	variable OSSCN
	variable OSST

	oss_parse -start [string trim $line]
	set cmd [oss_parse -match {^[[:alpha:]_][[:alnum:]_]*} -return 0]

	if { $cmd == "" } {
		error "a command must start with an alphanumeric keyword"
	}

	# locate the command

	if ![info exists OSSCN($cmd)] {
		error "command $cmd not found"
	}

	set vals ""


	foreach st [lindex [lindex $OSSCN($cmd) 1] 0] {

		set cc [oss_parse -skip " \t," -return 0]

		foreach { tp nm ti di le } $st { }

		if { $tp == "blob" } {

			if { $cc == "\"" } {
				set bstr 1
			}

			if $bstr {
				# treat as a string
				if { $cc == "\"" } {
					set bb [oss_parse -string -return 0]
				} else {
					set bb [oss_parse -string 0 -return 0]
				}
				set bb [oss_bytestobin $bb]
				set zer [binary format c 0]
				if { [string index $bb end] != $zer } {
					append bb $zer
				}
			} else {
				# treat as a bunch of values
				set bb ""
				while 1 {
					set val [oss_parse -skip " \t," \
						-number -return 1]
					if { $val == "" } {
						break
					}
					lappend bb $val
				}
			}
			lappend vals $bb
			# this must be the last item
			break
		}

		set fr [lindex $OSST($tp) 2]
		set up [lindex $OSST($tp) 3]

		set res ""

		if $di {
			set nc $di
		} else {
			set nc 1
		}

		for { set i 0 } { $i < $nc } { incr i } {
			set val [oss_parse -number -skip " \t," -return 0]
			if { $val == "" } {
				error "argument $nm, expected number"
			}
			if [catch { oss_valint $val $fr $up } val] {
				error "argument $nm, the number is out of\
					range, $val"
			}
			lappend res $val
		}

		if $di {
			lappend vals [lindex $res 0]
		} else {
			lappend vals $res
		}
	}

	if { [oss_parse -skip -return 0] != "" } {
		error "supefluous arguments [oss_parse -match ".*" -return 0]"
	}

	oss_issuecommand [lindex $OSSCN($cmd) 2] \
		[oss_setvalues $vals $cmd $bstr]
}

proc oss_defshow { code opref block { bstr 0 } } {
#
# Default message "show-er", bstr as above
#
	variable OSSMC

	if { $code == 0 } {
		# pure ACK
		if { [strlen $block] < 2 } {
			return
		}
		binary scan $block su stat
		oss_ttyout "ACK [format %02X $opref], [format %04X $stat]"
		return
	}

	if ![info exists OSSMC($code)] {
		error "no layout found"
	}

	set vals [oss_getvalues $block $code $bstr]

	set res "[lindex $OSSMC($code) 0] <$opref>:"

	foreach st [lindex [lindex $OSSMC($code) 1] 0] {

		foreach { tp nm ti di le } $st { }
		set val [lindex $vals 0]
		set vals [lrange $vals 1 end]
		append res " $nm="

		if { $tp == "blob" } {
			if $bstr {
				# the value is a string already
				append res "\"$val\""
			} else {
				append res "([join $val " "])"
			}
			break
		}

		if $di {
			# an array
			append res "\["
			for { set i 0 } { $i < $di } { incr i } {
				append res "[lindex $val $i] "
			}
			set res "[string trimright $res]\]"
		} else {
			append res $val
		}
	}

	oss_ttyout $res
}

proc oss_dump { args } {
#
# Sets the dump flag
#
	global PM

	while { $args != "" } {

		set what [lindex $args 0]
		set args [lrange $args 1 end]

		if { [string index $what 0] != "-" } {
			error "selector for oss_dump doesn't start with '-'"
		}

		set what [string range $what 1 end]

		if [catch { oss_keymatch $what { "incoming" "outgoing" "off" 
		    "none" } } w] {
			error "oss_dump, $w"
		}

		if { $w == "incoming" } {
			set PM(DMP) [expr { $PM(DMP) | 0x01 }]
		} elseif { $w == "outgoing" } {
			set PM(DMP) [expr { $PM(DMP) | 0x02 }]
		} else {
			set pm(DMP) 0
		}
	}
}

proc oss_genheader { } {

	global PM
	variable OSSCC
	variable OSSMC

###########################
###########################

	set res "#ifndef  __ossi_h_pg__\n#define  __ossi_h_pg__\n"
	append res "#include \"sysio.h\""
	append res {
// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

}
	append res "#define\tOSS_PRAXIS_ID\t\t$PM(PXI)\n"
	append res "#define\tOSS_UART_RATE\t\t$PM(USP)\n"
	append res "#define\tOSS_PACKET_LENGTH\t$PM(MPL)\n"

	append res {
typedef	struct {
	word size;
	byte content [];
} blob;

typedef	struct {
	byte code, ref;
} oss_hdr_t;

}

###########################
###########################

	append res "// ==================\n"
	append res "// Command structures\n"
	append res "// ==================\n\n"

	set lcmd [lsort [array names OSSCC]]

	foreach c $lcmd {

		set s $OSSCC($c)
		set nm [lindex $s 0]
		set st [lindex [lindex $s 1] 0]

		append res "#define\tcommand_${nm}_code\t$c\n"
		append res "typedef struct {\n"

		foreach t $st {
			append res "\t[lindex $t 0]\t[lindex $t 1]"
			set dim [lindex $t 3]
			if $dim {
				append res " \[$dim\]"
			}
			append res ";\n"
		}

		append res "} command_${nm}_t;\n\n"
	}

	append res "// ==================\n"
	append res "// Message structures\n"
	append res "// ==================\n\n"

	set lmsg [lsort [array names OSSMC]]

	foreach c $lmsg {

		set s $OSSMC($c)
		set nm [lindex $s 0]
		set st [lindex [lindex $s 1] 0]

		append res "#define\tmessage_${nm}_code\t$c\n"
		append res "typedef struct {\n"

		foreach t $st {
			append res "\t[lindex $t 0]\t[lindex $t 1]"
			set dim [lindex $t 3]
			if $dim {
				append res " \[$dim\]"
			}
			append res ";\n"
		}

		append res "} message_${nm}_t;\n\n"
	}

	append res {
// ===================================
// End of automatically generated code 
// ===================================
}

	append res "#endif\n"

###########################
###########################

	return $res
}

proc oss_ttyout { ln } {
#
# writes a line to the terminal window
#
	sy_dspline $ln

	if { $::ST(SFS) != "" } {
		catch { puts $::ST(SFS) $ln }
	}
}

proc oss_issuecommand { code block } {

	global PM ST

	set msg "[binary format cc $code $ST(OPREF)]$block"

	set cl [string length $msg]
	if { $cl > $PM(MPL) } {
		error "command packet too long: $cl, the max is $PM(MPL)"
	}

	incr ST(OPREF)

	if { $ST(OPREF) >= 256 } {
		set ST(OPREF) 1
	}

	if { [expr { $PM(DMP) & 0x01 } ] != 0 } {
		oss_ttyout "OUT: [oss_bintobytes $msg]"
	}

	noss_send $msg
}

proc oss_evalscript { s } {

	uplevel #0 "namespace eval USER { $s }"
}

namespace export oss_*

}

namespace import ::OSS::*

###############################################################################
# END OF OSS LIB NAMESPACE ####################################################
###############################################################################

# Default name of the specification file
set PM(DSF)	"ossi.tcl"

# Dump flag
set PM(DMP)	0

# Current directory
set PM(PWD)	[pwd]

# Default praxis ID
set PM(PXI)	0xFFFFFFFF

# Default UART rate
set PM(USP)	9600

# Default max packet length; this is the exact equivalent of the argument of
# phys_uart; it covers the complete payload sans CRC (the [unused] Network ID
# is covered, too); the max is 252
set PM(MPL)	82

# Command parser functions
set PM(PRS)	""

# connection type
set PM(CON)	"rv"

# connection type override flag
set PM(CVR)	0

# device list for UART connection
set PM(DVL)	""

# VUEE socket connection timeout
set PM(VUT)	[expr 6 * 1000]

# Max lines off term window
set PM(TLC)	1024

# Device to which we are connected
set ST(UCS)	""

# Flag: handshake established
set ST(HSK)	0

# Recursive exit prevention flag
set WI(REX)	0

# UART/socket file descriptor
set ST(SFD)	""

# Save file
set ST(SFS)	""

# Log input as well
set ST(SFB)	0

# Last log directory
set WI(LLD)	$PM(PWD)

# Last directory where the header file was stored
set WI(LHD)	$WI(LLD)

# Last VUEE selection for connect
set WI(VUS)	0
set WI(HID)	0
set WI(WUH)	"localhost"
set WI(WUP)	4443
set WI(WUN)	0

# User window number
set WI(USR)	0

# OPREF
set ST(OPREF)	1

###############################################################################
###############################################################################

proc sy_dmp { bytes hdr } {

	puts "$hdr[oss_bintobytes $bytes]"
}

proc trc { msg } {

	puts $msg
	flush stdout
}

proc sy_alert { msg } {

	tk_dialog .alert "Attention!" "${msg}!" "" 0 "OK"
}

proc sy_exit { } {

	global WI MO

	if $WI(REX) { return }

	set WI(REX) 1

	if [info exists MO(WIN)] {
		# a modal window exists, destroy it first
		catch { destroy $MO(WIN) }
		catch { unset MO(WIN) }
	}

	for { set i 0 } { $i < $WI(USR) } { incr i } {
		# user window
		catch { destroy .user_$WI(USR) }
	}

	exit 0
}

proc sy_abort { hdr { msg "" } } {

	global ST

	if { $msg == "" } {
		# single message, simple abort
		tk_dialog .abert "Abort!" "Fatal error: $hdr!" "" 0 "OK"
		exit 1
	}

	sy_dspline $msg
	tk_dialog .abert "Abort!" "$hdr, see the term window!" "" 0 "OK"
	tkwait variable WI(REX)
	exit 1
}

proc sy_localize { ref ns } {

	if { $ref != "" && [string range $ref 0 1] != "::" } {
		set ref "::${ns}::$ref"
	}

	return $ref
}

proc sy_varvar { v } {
#
# Returns the value of a variable whose name is stored in a variable
#
	upvar $v vv
	return $vv
}

proc sy_valaddr { addr } {
#
# Check if the address pattern appears to make sense; return:
#
#     0 - valid domain (at least two components)
#     1 - valid address
#     2 - formally incorrect
#     3 - illegal character (domain or e-mail)
#
	# rewrote this code, but used the same valid characters
	# (more or less) as in the original code

	if [regexp {^[^@[:space:]]+[@][^.@[:space:]]+(?:[.][^.@[:space:]]+)+$} \
	    $addr] {
		# in the form of an e-mail address
		if [regexp {^[@*='`/0-9a-z+._~-]+$} $addr] {
			# contains valid characters
			return 1
		}
		return 3
	} elseif [regexp {^[^.@[:space:]]+(?:[.][^.@[:space:]]+)+$} $addr] {
		# in the form of a domain
		if [regexp {^[*0-9a-z._~-]+$} $addr] {
			# contains valid characters
			return 0
		}
		return 3
	} else {
		# in the form of junk
		return 2
	}
}

###############################################################################

proc sy_addtext { w txt } {

	$w configure -state normal
	$w insert end "$txt"
	$w configure -state disabled
	$w yview -pickplace end
}

proc sy_endline { w } {

	global PM

	$w configure -state normal
	$w insert end "\n"

	while 1 {
		set ix [$w index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= $PM(TLC) } {
			break
		}
		# delete the topmost line if above limit
		$w delete 1.0 2.0
	}

	$w configure -state disabled
	# make sure the last line is displayed
	$w yview -pickplace end
}

proc sy_dspline { ln } {
#
# Write a line to the terminal; the tricky bit is that we cannot assume that
# the line doesn't contain newlines
#
	append ln "\r\n"
	while { [regexp "\[\r\n\]" $ln m] } {
		# eol delimiter
		set el [string first $m $ln]
		if { $el == 0 } {
			# first character, check the second one
			set n [string index $ln 1]
			if { $m == "\r" && $n == "\n" || \
			     $m == "\n" && $n == "\r"    } {
				# two-character EOL
				set ln [string range $ln 2 end]
			} else {
				set ln [string range $ln 1 end]
			}
			# complete previous line
			sy_endline .t
			continue
		}
		# send the preceding string to the terminal
		sy_addtext .t [string range $ln 0 [expr $el - 1]]
		set ln [string range $ln $el end]
	}
}

proc sy_updtitle { } {

	global ST

	if { $ST(UCS) == "" } {
		set hd "disconnected"
	} else {
		if $ST(HSK) {
			set hd "connected to"
		} else {
			set hd "connecting to"
		}
		append hd " $ST(UCS)"
	}

	wm title . "OSS ZZ000000A: $hd"
}

proc sy_terminput { } {
#
# Input line from the terminal
#
	global WI ST

	if { $ST(SFD) == "" } {
		sy_dspline "No connection!!!"
		return
	}

	set tx ""
	# extract the line
	regexp "\[^\r\n\]+" [.stat.u get 0.0 end] tx
	# remove it from the input field
	.stat.u delete 0.0 end

	# current input line (a clumsy way to send the line to sy_sgett to be
	# compatible with the command-line-version callback)

	sy_handle_input_line $tx
}

proc sy_setsblab { } {
#
# Sets the label on the "save" button
#
	global ST WI

	if { $ST(SFS) != "" } {
		set lab "Stop"
	} else {
		set lab "Save"
	}

	$WI(SFS) configure -text $lab
}

proc sy_startlog { } {
#
# Issues the initial log message
#
	global ST

	set msg "\n################################################\n"
	append msg "### Logging started on: "
	append msg [clock format [clock seconds] -format \
		"%y/%m/%d at %H:%M:%S ###"]
	append msg "\n################################################\n"

	catch { puts $ST(SFS) $msg }
}

proc sy_fndpfx { } {
#
# Produces the date/time prefix for a file name
#
	return [clock format [clock seconds] -format %y%m%d_%H%M%S]
}

proc sy_savefile { } {
#
# Toggle logging
#
	global ST WI

	if { $ST(SFS) != "" } {
		# close the current log
		catch { close $ST(SFS) }
		set ST(SFS) ""
		sy_setsblab
		return
	}

	while 1 {

		set fn [tk_getSaveFile \
			-defaultextension ".txt" \
			-parent "." \
			-title "Log file name" \
			-initialfile "[sy_fndpfx]_oss_log.txt" \
			-initialdir $WI(LLD)]

		if { $fn == "" } {
			# cancelled
			return
		}

		if [catch { open $fn "a" } std] {
			if [tk_dialog .alert "Attention!" "File $fn cannot be\
				opened for writing!" "" 0 "Try another file" \
				"Cancel"] {
					return
			}
			continue
		}

		fconfigure $std -buffering line -translation lf

		set ST(SFS) $std

		sy_startlog

		# preserve the directory for future opens
		set WI(LLD) [file dirname $fn]
		sy_setsblab
		return
	}
}

if 0 {

proc sy_genheader { } {

	global WI

	# generate the header
	if [catch { oss_genheader } hdr] {
		sy_alert "Cannot generate header, $hdr"
		return
	}

	while 1 {

		set fn [tk_getSaveFile \
			-defaultextension ".h" \
			-parent "." \
			-title "Header file name" \
			-initialfile "ossi.h" \
			-initialdir $WI(LHD)]

		if { $fn == "" } {
			# cancelled
			return
		}

		if [catch { open $fn "w" } std] {
			if [tk_dialog .alert "Attention!" "File $fn cannot be\
				opened for writing!" "" 0 "Try another file" \
				"Cancel"] {
					return
			}
			continue
		}

		if [catch { puts -nonewline $std $hdr } err] {
			catch { close $std }
			if [tk_dialog .alert "Attention!" "Cannot write to file\
				$fn, $err!"  "" 0 "Try another file" \
				"Cancel"] {
					return
			}
			continue
		}

		catch { close $std }
		# preserve the directory for future opens
		set WI(LHD) [file dirname $fn]
		return
	}
}

}

proc sy_mkterm { } {

	global ST WI

	sy_updtitle

	frame .user
	pack .user -expand yes -fill both

	text .t \
		-yscrollcommand ".scroly set" \
		-setgrid true \
        	-width 80 -height 24 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	.t delete 1.0 end
	scrollbar .scroly -command ".t yview"
	pack .scroly -side right -fill y
	pack .t -expand yes -fill both
	
	frame .stat -borderwidth 2
	pack .stat -expand no -fill x

	set WI(INP) [text .stat.u -height 1 -font {-family courier -size 10} \
		-state normal -width 10]

	pack .stat.u -side left -expand yes -fill x

	bind .stat.u <Return> "sy_terminput"

	frame .stat.fs -borderwidth 0
	pack .stat.fs -side right -expand no

	set WI(CON) [button .stat.fs.rb -command sy_reconnect -text "Connect" \
		-width 10]
	pack .stat.fs.rb -side right

if 0 { 
	# this is now handled by -H
	button .stat.fs.hb -command sy_genheader -text Hdr
	pack .stat.fs.hb -side right
}

	set WI(SVA) [checkbutton .stat.fs.sa -state normal -variable ST(SFB)]
	pack .stat.fs.sa -side right
	label .stat.fs.sl -text " All:"
	pack .stat.fs.sl -side right

	set WI(SFS) [button .stat.fs.sf -command "sy_savefile"]
	pack $WI(SFS) -side right

	sy_setsblab

	.t configure -state disabled
	bind . <Destroy> "sy_exit"
	bind .t <ButtonRelease-3> "sy_cut_copy_paste %W %X %Y c"
	bind .stat.u <ButtonRelease-3> "sy_cut_copy_paste %W %X %Y c"
}

proc sy_cut_copy_paste { w x y { c "" } } {
#
# Handles windows-style cut-copy-paste from a text widget; invoked in response
# to right click in a text widget
#
	if [catch { $w get sel.first sel.last } sel] {
		# selection absent -> empty
		set sel ""
	}

	# determine the state, i.e., are we allowed to paste into the widget?
	set sta [$w cget -state]
	if { [string first "normal" $sta] >= 0 } {
		set sta "normal"
	} else {
		set sta "disabled"
	}

	set r $w._rcm

	catch { destroy $r }

	set m [menu $r -tearoff 0]

	if { $sel != "" && $sta == "normal" } {
		# cut allowed
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Cut" -command "tk_textCut $w" -state $st

	if { $sel != "" } {
		# copy allowed
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Copy" -command "tk_textCopy $w" -state $st

	if [catch { clipboard get -displayof $w } cs] {
		set cs ""
	}
	if { $sta == "normal" && $cs != "" } {
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Paste" -command "tk_textPaste $w" -state $st

	if { $c != "" } {
		$m add separator
		if [$w compare 1.0 < "end - 1 chars"] {
			set st "normal"
		} else {
			set st "disabled"
		}
		$m add command -label "Clear" -command "sy_clear_txt $w" \
			-state $st
	}

	tk_popup $m $x $y
}

proc sy_clear_txt { w } {

	set sta [$w cget -state]
	if { [string first "normal" $sta] >= 0 } {
		set sta "normal"
	} else {
		set sta "disabled"
	}

	if [catch { $w configure -state normal }] {
		return
	}

	$w delete 1.0 end

	$w configure -state $sta
}

proc sy_term_enable { level } {

	global WI

	if { $level == 0 } {
		# disable all widgets (except for the main text area)
		foreach w { INP CON SVA SFS } {
			$WI($w) configure -state disabled
		}
		return
	}

	if { $level == 1 } {
		# enable all except the input area
		foreach w { CON SVA SFS } {
			$WI($w) configure -state normal
		}
		$WI(INP) configure -state disabled
		return
	}

	# enable all
	foreach w { INP CON SVA SFS } {
		$WI($w) configure -state normal
	}
}

proc sy_valvp { vp } {
#
# Validate VUEE params
#
	global WI

	set err ""

	if { [llength $vp] >= 3 } {
		# host name
		set WI(WUH) [lindex $vp 0]
		if { $WI(WUH) != "localhost" && [sy_valaddr $WI(WUH)] != 0 } {
			lappend err "illegal host name $WI(WUH)"
		}
		set vp [lrange $vp 1 2]
	}

	if { [llength $vp] == 2 } {
		# port number
		set val [lindex $vp 0]
		if [catch { oss_valint $val 1 65535 } WI(WUP)] {
			lappend err "illegal port number $val, $WI(WUP)"
		}
		set vp [lrange $vp 1 end]
	}

	if { [llength $vp] == 1 } {
		set val [lindex $vp 0]
		if { [string tolower [string index $val 0]] == "h" } {
			set WI(HID) 1
			set val [string range $val 1 end]
		}
		if [catch { oss_valint $val 0 65535 } vam] {
			lappend err "illegal node number $val, $vam"
		} else {
			set WI(WUN) $vam
		}
	}

	if { $err != "" } {
		set err [join $err ", "]
	}

	return $err
}

proc sy_reconnect { } {
#
# Responds to the Connect button, i.e., connects or disconnects
#
	global ST WI MO PM

	set st [$WI(CON) cget -text]

	if { $st != "Connect" } {
		# connected or connecting
		autocn_stop
		$WI(CON) configure -text "Connect"
		sy_updtitle
		return
	}

	# connect

	if { [string first "v" $PM(CON)] < 0 } {
		# real only, do not show the window
		sy_start_uart
		$WI(CON) configure -text "Disconnect"
		sy_updtitle
		return
	}

	if { [string first "*" $PM(CON)] >= 0 } {
		# VUEE with preset parameters, no need for the window
		sy_start_vuee
		$WI(CON) configure -text "Disconnect"
		sy_updtitle
		return
	}

	set w .params
	set MO(WIN) $w

	toplevel $w
	wm title $w "Connect to:"

	# make it modal
	catch { grab $w }

	#######################################################################

	set f [frame $w.tf -padx 4 -pady 4]
	pack $f -side top -expand y -fill x

	button $f.c -text "Cancel" -command "set MO(GOF) 0"
	pack $f.c -side left -expand n

	button $f.p -text "Proceed" -command "set MO(GOF) 1"
	pack $f.p -side right -expand n

	set f [labelframe $w.bf -padx 4 -pady 4 -text "VUEE"]
	pack $f -side top -expand y -fill x

	if { [string first "r" $PM(CON)] >= 0 } {
		# choice between VUEE and real
		set WI(VUS) 0
		set c [checkbutton $f.vs -state normal -variable WI(VUS) \
			-command "set MO(GOF) -2"]
		pack $c -side top -anchor w -expand no
	} else {
		set WI(VUS) 1
	}

	set f [frame $f.b]
	pack $f -side top -expand y -fill both

	#########################################

	label $f.hl -text "Host:  " -anchor w
	grid $f.hl -column 0 -row 0 -sticky nws

	label $f.pl -text "Port:  " -anchor w
	grid $f.pl -column 0 -row 1 -sticky nws

	label $f.nl -text "Node:  " -anchor w
	grid $f.nl -column 0 -row 2 -sticky nws

	label $f.il -text "HID:  " -anchor w
	grid $f.il -column 0 -row 3 -sticky nws

	#########################################

	# these need verification, so we can't use the stored defaults
	set MO(WUH) $WI(WUH)
	set MO(WUP) $WI(WUP)
	set MO(WUN) $WI(WUN)

	set hen [entry $f.he -width 16 -textvariable MO(WUH) -bg gray]
	grid $hen -column 1 -row 0 -sticky news
	set pen [entry $f.pe -width 16 -textvariable MO(WUP) -bg gray]
	grid $pen -column 1 -row 1 -sticky news
	set nen [entry $f.ne -width 16 -textvariable MO(WUN) -bg gray]
	grid $nen -column 1 -row 2 -sticky news

	set ien [checkbutton $f.ie -state normal -variable WI(HID)]
	grid $ien -column 1 -row 3 -sticky nws

	#########################################

	bind $w <Destroy> "set MO(GOF) 0"

	set MO(GOF) -1

	raise $w

	while 1 {

		# enable or disable the VUEE widgets
		if $WI(VUS) {
			set st "normal"
		} else {
			set st "disabled"
		}

		foreach v { hen pen nen ien } {
			[sy_varvar $v] configure -state $st
		}

		tkwait variable MO(GOF)

		if { $MO(GOF) < 0 } {
			continue
		}

		if { $MO(GOF) == 0 } {
			# cancel
			break
		}

		# proceed, validate arguments
		set err ""

		if { $MO(WUH) != "localhost" && [sy_valaddr $MO(WUH)] != 0 } {
			lappend err "host address is invalid"
		} else {
			set WI(WUH) $MO(WUH)
		}

		if [catch { oss_valint $MO(WUP) 1 65535 } fai] {
			lappend err "port number is invalid: $fai"
		} else {
			set WI(WUP) $fai
		}

		if [catch { oss_valint $MO(WUN) 0 65535 } fai] {
			lappend err "node number is invalid: $fai"
		} else {
			set WI(WUN) $fai
		}

		if { $err != "" } {
			set err [join $err ", "]
			sy_alert "Bad parameters: $err"
			continue
		}

		if { $WI(VUS) == 0 } {
			# UART autoconnect
			sy_start_uart
		} else {
			sy_start_vuee
		}

		$WI(CON) configure -text "Disconnect"
		break
	}

	catch { destroy $w }
	array unset MO
	sy_updtitle
}

proc sy_start_uart { } {

	global PM

	autocn_start \
		sy_uart_open \
		noss_close \
		sy_send_handshake \
		sy_handshake_ok \
		sy_connected \
		sy_poll \
		$PM(DVL)
}

proc sy_start_vuee { } {

	autocn_start \
		sy_socket_open \
		noss_close \
		sy_send_handshake \
		sy_handshake_ok \
		sy_connected \
		sy_poll \
		{ dummy }
}

proc sy_uart_open { udev } {

	global PM ST

	set emu [uartpoll_interval $ST(SYS) $ST(DEV)]

	if { $ST(SYS) == "L" } {
		set accs { RDWR NOCTTY NONBLOCK }
	} else {
		set accs "r+"
	}

	set ST(UCS) $udev
	sy_updtitle
	set d [unames_unesc $udev]

	if [catch { open $d $accs } ST(SFD)] {
		set ST(SFD) ""
		return 0
	}

	if [catch { fconfigure $ST(SFD) -mode "$PM(USP),n,8,1" -handshake none \
	    -blocking 0 -eofchar "" -ttycontrol { RTS 0 } } err] {
		catch { close $ST(SFD) }
		set ST(SFD) ""
		return 0
	}

	set ST(HSK) 0

	# configure the protocol
	noss_init $ST(SFD) $PM(MPL) sy_uart_read oss_ttyout sy_uart_close $emu

	sy_term_enable 2

	return 1
}

proc sy_socket_open { udev } {

	global PM ST WI

	set ST(ABV) 0
	set ST(VUC) [after $PM(VUT) "incr ::ST(ABV)"]
	set ST(UCS) "VUEE ($WI(WUH):$WI(WUP)/$WI(WUN) \["
	if $WI(HID) {
		append ST(UCS) "h"
	} else {
		append ST(UCS) "s"
	}
	append ST(UCS) "\]"
	sy_updtitle

	if { [catch {
		vuart_conn $WI(WUH) $WI(WUP) $WI(WUN) ::ST(ABV) $WI(HID)
					} ST(SFD)] || $ST(ABV) } {
		catch { after cancel $ST(VUC) }
		set ST(SFD) ""
		unset ST(ABV)
		unset ST(VUC)
		return 0
	}

	catch { after cancel $ST(VUC) }
	unset ST(ABV)
	unset ST(VUC)
	set ST(HSK) 0
	noss_init $ST(SFD) $PM(MPL) sy_uart_read oss_ttyout sy_uart_close 0

	sy_term_enable 2

	return 1
}

proc sy_uart_close { { err "" } } {

	global ST

	catch { close $ST(SFD) }
	set ST(SFD) ""
	set ST(HSK) 0
	set ST(UCS) ""

	sy_updtitle
	sy_term_enable 1
}

proc sy_send_handshake { } {

	global PM

	set msg [binary format cci 0x00 0x00 $PM(PXI)]

	# twice to make it more reliable
	noss_send $msg
	noss_send $msg
}

proc sy_poll { } {

	global PM

	set msg [binary format cci 0x00 0x00 $PM(PXI)]
	noss_send $msg
}

proc sy_handshake_ok { } {

	global ST
	return $ST(HSK)
}

proc sy_connected { } {

	global ST
	return [expr { $ST(SFD) != "" }]
}

proc sy_uart_read { msg } {
#
# Handles data from the node
#
	global PM ST

	autocn_heartbeat

	set len [string length $msg]
	if { $len < 2 } {
		# no need to even look at it
		return
	}

	binary scan $msg cucu code opref
	set mes [string range $msg 2 end]

	if { $code == 0 && $opref == 0 } {
		# heartbeat/autoconnect
		if { $len < 4 } {
			# ignore
			return
		}
		binary scan $mes su pxi
		if { $pxi == [expr ($PM(PXI) ^ ($PM(PXI) >> 16)) & 0xFFFF] } {
			# handshake OK
			if !$ST(HSK) {
				set ST(HSK) 1
				sy_updtitle
			}
		}
		return
	}

	set shw [lindex $PM(PRS) 1]
	if { $shw == "" } {
		set shw "oss_defshow"
	} else {
		set shw [sy_localize $shw "USER"]
	}

	if [catch { $shw $code $opref $mes } out] {
		set line "Message "
		append line [format "\[%02X %02X\], " $code $opref]
		append line "$out\n"
		append line "Content:[oss_bintobytes $mes]"
		oss_ttyout $line
		return
	}

	if { [expr { $PM(DMP) & 0x02 } ] != 0 } {
		oss_ttyout "INC: [oss_bintobytes $msg]"
	}
}

proc sy_handle_input_line { line } {
#
# Handles user input
#
	global PM

	# internal command intercept to be added here

	set prs [lindex $PM(PRS) 0]
	if { $prs == "" } {
		set prs "oss_defparse"
	} else {
		set prs [sy_localize $prs "USER"]
	}

	if [catch { $prs $line } err] {
		oss_ttyout $err
	}
}

###############################################################################
# Place to insert the inline spec file: make it the value of USPEC ############
###############################################################################

set USPEC {}

###############################################################################
###############################################################################
###############################################################################

###############################################################################
#
# Usage:
#
#	-I interface file (also -F)
#	-V host port [h]node
#	-V port [h]node
#	-V [h]node
#	-V
#	-U dev ... dev (also -R)
#	-U
#	-H header file
#	-H
#
###############################################################################

proc sy_args { } {

	global argv USPEC PM WI

	while { $argv != "" } {

		set arg [lindex $argv 0]
		set argv [lrange $argv 1 end]

		if { $arg == "-F" || $arg == "-I" } {
			# interface file
			if { $USPEC != "" } {
				sy_abort "(usage) -F illegal with compiled-in\
					specification"
			}
			if [info exists A(f)] {
				sy_abort "(usage) duplicate argument $arg"
			}
			set PM(DSF) [lindex $argv 0]
			if { $PM(DSF) == "" } {
				sy_abort "(usage) interface file required\
					following $arg"
			}
			set argv [lrange $argv 1 end]
			set A(f) ""
			continue
		}

		if { $arg == "-V" } {
			if [info exists A(u)] {
				sy_abort "(usage) -V and -U cannot be mixed"
			}
			if [info exists A(v)] {
				sy_abort "(usage) duplicate argument -V"
			}
			set vp ""
			for { set i 0 } { $i < 3 } { incr i } {
				set arg [lindex $argv 0]
				if { $arg == "" ||
					       [string index $arg 0] == "-" } {
					break
				}
				lappend vp $arg
				set argv [lrange $argv 1 end]
			}

			if { $vp == "" } {
				set PM(CON) "v"
			} else {
				set err [sy_valvp $vp]
				if { $err != "" } {
					sy_abort "(usage) $err"
				}
				set PM(CON) "v*"
			}
			set PM(CVR) 1
			set A(v) ""
			continue
		}

		if { $arg == "-U" || $arg == "-R" } {
			if [info exists A(v)] {
				sy_abort "(usage) -V and -U cannot be mixed"
			}
			if [info exists A(u)] {
				sy_abort "(usage) duplicate argument -U"
			}
			# extract the device list
			while 1 {
				set arg [lindex $argv 0]
				if { $arg == "" ||
					       [string index $arg 0] == "-" } {
					break
				}
				lappend PM(DVL) $arg
				set argv [lrange $argv 1 end]
			}
			set A(u) ""
			set PM(CON) "r"
			set PM(CVR) 1
			continue
		}

		if { $arg == "-H" } {
			# header file request
			if [info exists A(h)] {
				sy_abort "(usage) duplicate argument -H"
			}
			set A(h) ""
			set arg [lindex $argv 0]
			if { $arg == "" || [string index $arg 0] == "-" } {
				set PM(HEF) ""
			} else {
				set PM(HEF) $arg
				set argv [lrange $argv 1 end]
			}
			continue
		}

		sy_abort "(usage) illegal argument $arg"
	}
}
				
proc sy_init { } {

	global PM WI USPEC ST

	catch { close stdin }

	unames_init $ST(DEV) $ST(SYS)
	# start by creating the window (in disabled state); we will use it
	# to display any error messages
	sy_mkterm

	# disable all widgets
	sy_term_enable 0

	sy_args

	if { $USPEC == "" } {
		if [catch { open $PM(DSF) "r" } ifd] {
			sy_abort "Cannot open specification file $PM(DSF), $ifd"
		}
		if [catch { read $ifd } USPEC] {
			sy_abort "Cannot read specification file $PM(DSF),\
				$USPEC"
		}
		catch { close $ifd }
	}

	if [catch { oss_evalscript $USPEC } sts] {

		set errmsg [oss_errors]
		# include the last one
		lappend errmsg $sts
		sy_abort "Error(s) in specification file" [join $errmsg "\n"]
	}

	if [catch { oss_verify } sts] {
		sy_abort "Error(s) in specification file" $sts
	}

	# we seem to be in the clear, so let us enable connections; there
	# should be an option (settable in the specification file) to make
	# the connection completely automatic (no need to hit any buttons)

	if [info exists PM(HEF)] {
		# write the header file
		if [catch { oss_genheader } hdr] {
			sy_abort "Cannot generate header, $hdr"
		}
		if { $PM(HEF) == "" } {
			set fd "stdout"
		} elseif [catch { open $PM(HEF) "w" } fd] {
			sy_abort "Cannot open $PM(HEF), $fd"
		}

		if [catch { puts -nonewline $fd $hdr } err] {
			sy_abort "Cannot write to $PM(HEF), $err"
		}

		catch { close $fd }
		unset PM(HEF)
		exit 0
	}

	sy_term_enable 1

	# user init function
	set ini [lindex $PM(PRS) 2]
	
	if { $ini != "" } {
		set fun [sy_localize $ini "USER"]
		if [catch { $fun } err] {
			sy_abort "user init function failed, $err"
		}
	}

	while 1 {

		tkwait variable WI(REX)
	}
}

sy_init
