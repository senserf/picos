#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
########\
exec tclsh "$0" "$@"

package require Tk
package require Ttk

set ST(VER) 1.6

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

# no DOS path type
set ST(DP) 0

if { $ST(SYS) != "L" } {
	# sanitize arguments; here you have a sample of the magnitude of
	# nonsense I have to fight when glueing together Windows and Cygwin
	# stuff; the last argument (sometimes!) has a CR character appended
	# at the end, and you wouldn't believe how much havoc that can cause
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}

	# Not Linux: issue a dummy reference to a file path to eliminate the
	# DOS-path warning triggered at the first reference after Cygwin
	# startup

	set u [file normalize [pwd]]
	catch { exec ls $u }

	if [regexp -nocase "^\[a-z\]:" $u] {
		# DOS paths
		set ST(DP) 1
	}
	unset u
}

if { [lsearch -exact $argv "-D"] >= 0 } {
	set ST(DEB) 1
} else {
	set ST(DEB) 0
}

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

proc fpnorm { fn } {
#
# Normalizes the file-name path, accounts for the Cygwin/Windows duality
#
	global ST

	if { $ST(DP) && [string index $fn 0] == "/" } {
		# this is the only place where we may have a problem: we have
		# a full path from Cygwin, while the script needs DOS
		if ![catch { xq "cygpath" [list -w $fn] } fm] {
			# log "Path (L->D): $fn -> $fm"
			set fn $fm
		}
	} elseif { !$ST(DP) && $ST(SYS) != "L" &&
	    [regexp "^\[A-Za-z\]:\[/\\\\\]" $fn] } {
		if ![catch { xq "cygpath" [list $fn] } fm] {
			# log "Path (D->L): $fn -> $fm"
			set fn $fm
		}
	}
	if [catch { file normalize $fn } fm] {
		# this may fail on Cygwin when trying to expand fancy file
		# names, like ones including ~
		return $fn
	} else {
		return $fm
	}
}

# 250416: moved here, so we can discover the package directory

if [catch { xq picospath } PicOSPath] {
	puts stderr "cannot locate PicOS path: $PicOSPath"
	exit 1
}

set PicOSPath [fpnorm $PicOSPath]

# make sure our packages can be accessed
lappend auto_path [file join $PicOSPath "Scripts" "Packages"]

###############################################################################

set DefProjDir	""

set EditCommand "elvis -f ram -m -G x11"

## Delay after opening a new elvis session and before the first command to it
## can be issued; I am not sure this is needed, because I only had problems
## with this (or rather with something that seems to be circumvented by this)
## on my laptop's Linux X server, which behaves weirdly with many other
## programs
set NewCmdDelay 300
set TagsCmd "elvtags"
set PTagsArgs "-l -i -t -v -h -s --"
set STagsArgs "-l -i -t -v -h --"
## -s helps VUEE tags, because otherwise global functions don't seem to get
## tagged
set VTagsArgs "-l -i -t -v -h -s --"

###############################################################################
###############################################################################

# Flash loaders by arch; we remove the "known" loaders and replace them with
# multiple loaders defineable by the user; I haven't used Elpro for a while,
# then, the kludge to pass parameters to it was extremely clumsy and problem
# prone. MPSDebug, while still usable, can be handled by a user-settable 
# command or (preferably) a list of commands.

# Generic items for loaders; to be turned into options, they have to be
# prefixed by arch; we change the prefix to arch_ to automatically invalidate
# any old config settings
# 	LDSEL		- current loader selection
#	LDLIST		- the list of loaders
#
set CFLoadItemsGen { "LDSEL" "" "LDLIST" "" }

###############################################################################
###############################################################################

if { $ST(SYS) == "L" } {
	# check for dolphin and nautilus with dolphin being preferred
	set EXCmd [auto_execok "dolphin"]
	if { $EXCmd == "" } {
		set EXCmd [auto_execok "nautilus"]
	}
	# loaders
	set SIDENAME "side"
} else {
	set EXCmd "explorer"
	set SIDENAME "side.exe"
}

set PiterCmd "piter"
set SACmd "sa"
set DefTerm "xterm"
set DefDTerm [list xterm -e %f]
set DefAEdit ""

## File types to be listed in the Files view:
## header label, file qualifying patterns, filetypes [for tk_getSaveFile]
set LFTypes {
	{ Headers { "\\.h$" "\\.ch$" } { Header { ".h" ".ch" } } }
	{ Sources { "\\.cc?$" "\\.asm$" } { Source { ".cc" } } }
	{ Options { "^options\[_a-zA-Z0-9\]*\\.sys$" "^options\\.vuee?$" }
		{ Options { ".sys" ".vue" ".vuee" } } }
	{ XMLData { "\\.xml$" "\\.geo$" } { XMLData { ".xml" ".geo" } } }
	{ Scripts { "\\.tcl$" } { Scripts { ".tcl" } } }
}

## Directory names to be ignored in the project's directory:
## strict names, patterns (case ignored)
set IGDirs { "^cvs$" "^vuee_tmp$" "^ktmp" "junk" "attic" "ossi" "\\~\\$" 
		"\[ \t.\]" }

## List of directories for soft cleaning; perhaps the Cyan ones should not be
## there
set SoftCleanDirs { "out" "tmp" "KTMP" }
set VueeCleanFiles [list "VUEE_TMP" $SIDENAME]

###############################################################################

## Dictionary of configuration items (to be searched for in config.prj) + their
## default values:
##
##	ARCH - architecture
##	MB   - multiple boards (0 or 1) for multiprogram praxes
##	BO   - list of boards (per program, P(PL) is the corresponding list of
##	       labels
##	LM   - library mode (indexed as BO), 0 - no, 1 - YES
##	LD   - disabled programs
##
set CFBoardItems {
			"ARCH" 		""
			"MB" 		0
			"BO" 		""
			"LM"		""
			"LD"		""
			"CCF"		"compile.xml"
}

set CFVueeItems {
			"VDISABLE"	0
			"CMPIS"		0
			"THRD"		0
			"DPBC"		0
			"PFAC"		"Default"
			"UDON"		0
			"UDTM"		0
			"YCDN"		0
			"UDPL"		""
			"OSON"		0
			"OSNN"		0
			"OSNH"		0
			"VUDF"		""
			"VUOF"		""
			"VUSM"		1.0
			"VURI"		500
			"EARG"		""
			"EBRG"		""
}

## Extra defines for the compiler
set CFEDefines {
			"XDEFS"		""
}

## Options: the toolchain prefix, the xterm program, the debugger command, the
## max number of lines for the console window + permissions for accessing
## system files; need to evaluate
set CFOptItems [list \
		 "OPTTRMCMND" $DefTerm \
		 "OPTSHWCMND" $DefDTerm \
		 "OPTATECMND" "" \
		 "OPTERMLINES" 1000 \
		 "OPSYSFILES" 1 \
		 "OPVUEEFILES" 0 \
		 "OPCOMMENT" ""]

set CFOptSFModes { "Never" "Tags, R/O" "Always, R/O" "Always, R/W" }

## Saved parameters of the search window: the string searched for, the string
## type (RE = regular expression), search system files (too), maximum number
## of returned lines, maximum number of returned cases, surrounding lines per
## case
set CFSearchItems {
			"SESTRING"	""
			"SESTYPE"	"RE"
			"SESFILES"	"None"
			"SESFILESV"	0
			"SESMAXL"	1000
			"SESMAXC"	256
			"SESBRACKET"	5
			"SESFQNEG"	0
			"SESFQUAL"	""
			"SESCASE"	1
			"SESCOHD"	"#FF9A35"
			"SESCOTA"	"#F4FB7B"
		}

## Option tags for easy reference from the search window
set CFSearchTags { "s" "m" "x" "v" "l" "c" "b" "g" "f" "k" "h" "n" }
set CFSearchModes { "RE" "ST" "WD" }
set CFSearchSFiles { "None" "Proj" "All" "Only" }

## Exec items: last program executed in console
set CFXecItems {
			"XELPGM"	""
		}

## Default console line number limit
set TermLines 1000

set CFItems 	[concat $CFBoardItems \
			$CFEDefines \
			$CFVueeItems \
			$CFOptItems \
			$CFSearchItems \
			$CFXecItems]

# loader options (by arch) will be appended by initialize

###############################################################################

## List of last projects
set LProjects ""

## List of file types for Elvis scheme assignment
set ESFTypes { Project PicOS VUEE External All }

## List of Elvis schemes defined by the user
set ESchemes ""

## Assignment of schemes to file types
set ESchemesA ""

## The default (mandatory) scheme (as generated by elvissettings, with some
## formatting adjustments)
set ESchemesD {
		{normal {{} #00FF00 {} #000000}}
		{idle {normal {} {} {}}}
		{bottom {normal {} {} {}}}
		{lnum {normal {} {} {}}}
		{showmode {normal {} {} {}}}
		{ruler {normal {} {} {}}}
		{selection {{} {} {} #D2B48c}}
		{hlsearch {{} {} {} {} boxed}}
		{cursor {{} #FF0000 {} #FFFF00}}
		{tool {{} #000000 {} #BFBFBF}}
		{toolbar {{} #FFFFFF {} #666666}}
		{scroll {tool {} {} {}}}
		{scrollbar {toolbar {} {} {}}}
		{status {tool {} {} {}}}
		{statusbar {toolbar {} {} {}}}
		{comment {{} #006400 #90EE90 {} italic}}
		{string {{} #8B5A2B #FFA54F {}}}
		{keyword {{} {} {} {} bold}}
		{function {{} #8B0000 #FFC0CB {}}}
		{number {{} #00008B #ADD8E6 {}}}
		{prep {number {} {} {} bold}}
		{prepquote {string {} {} {}}}
		{other {keyword {} {} {}}}
		{variable {{} #262626 #EEE8AA {}}}
		{fixed {{} #595959 #CCCCCC {}}}
		{libt {keyword {} {} {} italic}}
		{argument {{} #00FF00 #006400 {} bold}}
		{hexheading {{} #B3B3B3 {} {}}}
		{linenumber {{} #BEBEBE {} {}}}
		{formatted {normal {} {} {}}}
		{link {{} #0000FF #ADD8E6 {} underlined}}
		{spell {{} {} {} #FFC0CB}}
		{font default}
		{commands {}}
	}

## Number of rows in the schemes array shown in the configuration windows
set NSESchemes	6

## Fixed font for the terminal, entry boxes, and so on
set FFont {-family courier -size 10}
set SFont {-family courier -size 9}

###############################################################################

##
## Status of external programs
##

## Pipe fd of the program running in term
set TCMD(FD) ""

## Pipe fd of the program running the spectrum analyzer
set TCMD(SA) ""

## Flag: command needs input
set TCMD(SH) 0

## Accumulated input chunk arriving from the program running in term
set TCMD(BF) ""

## BOL flag: 1 if line started but not yet completed
set TCMD(BL) 0

## Extra action to be carried out after the (successful) term command
set TCMD(EA) ""

## Extra action to be carried out after the command is aborted
set TCMD(AA) ""

## Callback (after) to visualize that something is running in term
set TCMD(CB) ""

## Counter used by the callback
set TCMD(CL) 0

## File descriptor of the udaemon pipe (!= "" -> udaemon running)
set TCMD(FU) ""

## File descriptor of the oss script pipe
set TCMD(FO) ""

## File descriptor of the genimage pipe
set TCMD(FG) ""

## Flash loader status: 0 - not running, 1 - single process running in FL,
## 2 - single process running in FD, 3 - two processes, one running in FL,
### the other in FD
set TCMD(FY) 0

## Callback for delaying debugger startup with proxy, the TCMD(FY) = 3 case
set TCMD(FY,CB) ""

## Argument for the second program
set TCMD(FY,AR) ""

## Process ID of FET loader (!= "" -> FET loader is running) + callback
## to monitor its disappearance + signal to kill + action to be performed
## after kill; on Cygwin, a periodic callback seems to be the only way to
## learn that a background process has disappeared
set TCMD(FL) ""
set TCMD(FL,CB) ""
set TCMD(FL,SI) "INT"
set TCMD(FL,AC) "upload_action"

## Slots for up to 4 instances of piter, CPITERS counts them, so we can order
## them dynamically
set TCMD(NPITERS) 4
set TCMD(CPITERS) 0
for { set p 0 } { $p < $TCMD(NPITERS) } { incr p } {
	set TCMD(PI$p) ""
	set TCMD(PI$p,SN) 0
}

## Running status (tex variable) of term + running "value" (number of seconds)
set P(SSL) ""
set P(SSV) ""

## Search window and status
set P(SWN) ""
set P(SST) 0
set P(SSR) 0

## double exit avoidance flag
set DEAF 0

###############################################################################

package require xml

###############################################################################

proc log { m } {

	global ST

	if $ST(DEB) {
		puts $m
	}
}

###############################################################################

proc isspace { c } {
	return [regexp "\[ \t\n\r\]" $c]
}

proc mreplace { l ix it } {
#
# Single-element replace preserving the list order, i.e., short lists are
# expanded to the right number of elements
#
	while { [llength $l] <= $ix } {
		# make sure we have that many elements
		lappend l ""
	}

	return [lreplace $l $ix $ix $it]
}

proc valfname { fn t } {
#
# Checks if the file/directory name doesn't include illegal (from our point of
# view) characters
#
	if { [string tolower [string index $t 0]] == "d" } {
		set t "directory"
	} else {
		set t "file"
	}

	if [regexp "\[\\\\ \t\r\n;\]" $fn] {
		alert "Illegal character(s) in $t name $fn; names used in\
			projects (or anywhere inside PICOS)\
			must not include spaces, tabs, semicolons,\
			or backslashes"
		return 0
	}
	return 1
}

proc valnum { n { min "" } { max "" } } {

	set n [string tolower [string trim $n]]
	if { $n == "" } {
		error "empty string"
	}

	if { [string first "." $n] >= 0 || [string first "e" $n] >= 0 } {
		error "string is not an integer number"
	}

	if [catch { expr $n } n] {
		error "string is not a number"
	}

	if { $min != "" && $n < $min } {
		error "number must not be less than $min"
	}

	if { $max != "" && $n > $max } {
		error "number must not be greater than $max"
	}

	return $n
}

proc valport { n } {

	return [valnum $n 1 65535]
}

proc blindex { lst ix } {
#
# Forced boolean interpretation of a list element
#
	set v [lindex $lst $ix]

	if { ![regexp {^[[:digit:]]+$} $v] || $v == 0 } {
		return 0
	}
	return 1
}
	
proc valcol { c } {
#
# Validates a color
#
	set c [string toupper [string trim $c]]
	if { [string length $c] != 7 || ![regexp "^#\[0-9A-F\]+$" $c] } {
		error "illegal color $c"
	}
	return $c
}

###############################################################################

proc delay { msec } {
#
# A variant of "after" admitting events while waiting
#
	global P

	if { [info exists P(DEL)] && $P(DEL) != "" } {
		catch { after cancel $P(DEL) }
	}

	set P(DEL) [after $msec delay_trigger]
	vwait P(DEL)
	unset P(DEL)
}

proc delay_trigger { } {

	global P

	if ![info exists P(DEL)] {
		return
	}

	set P(DEL) ""
}

###############################################################################

proc fix_exec { cmd } {
#
# This is my last attempt to try to make sense of this file path conversion
# mess. We should STOP using Cygwin!!!
#
	global ST

	set cmd [auto_execok $cmd]
	# Seem to need to do this only for windows Tcl under cygwin (DOS paths)
	if { $ST(DP) && $cmd != "" } {
		if [file executable $cmd] {
			set cmd "[list $cmd]"
		} else {
			set cmd "[list sh] [list $cmd]"
		}
	}
	return $cmd
}

###############################################################################

proc dospath { fn } {
#
# Converts the file path to DOS for the occassion of passing it to some Windows
# program that doesn't understand Cygwin
#
	global ST

	if { $ST(SYS) == "L" } {
		alert "DOS path conversion requested from Linux (file name:\
			$fn). This looks like some configuration problem"
		return $fn
	}

	set fn [fpnorm $fn]

	if $ST(DP) {
		# preferred path format is DOS, just make sure it is a native
		# name
		set fn [file nativename $fn]
	} else {
		# have to convert, -w will make it native
		if ![catch { xq "cygpath" [list -w $fn] } fm] {
			log "DOS path: $fn -> $fm"
			set fn $fm
		} else {
			log "cygpath failed: $fn, $fm"
		}
	}

	return $fn
}

proc unipath { fn } {
#
# Converts the file path to UNIX for the occasion of passing it to a program 
# that we know requires UNIX paths
#
	global ST

	set fn [fpnorm $fn]
	if { $ST(SYS) == "L" || !$ST(DP) } {
		return $fn
	}
	if ![catch { xq "cygpath" [list $fn] } fm] {
		log "Path (D->L): $fn -> $fm"
		return $fm
	}
	log "cygpath failed: $fn, $fm"
	return $fn
}

proc isfullpath { fn } {

	global ST

	if { $ST(SYS) == "L" || !$ST(DP) } {
		if { [string index $fn 0] == "/" } {
			return 1
		} else {
			return 0
		}
	}

	if [regexp -nocase {^[a-z]:[/\\]} $fn] {
		return 1
	} else {
		return 0
	}
}

proc cw { } {
#
# Returns the window currently in focus or null if this is the root window
#
	set w [focus]
	if { $w == "." } {
		set w ""
	}

	return $w
}

proc alert { msg } {

	reset_all_menus 1
	tk_dialog [cw].alert "Attention!" "${msg}!" "" 0 "OK"
	reset_all_menus
}

proc confirm { msg } {

	reset_all_menus 1
	set w [tk_dialog [cw].confirm "Warning!" $msg "" 0 "NO" "YES"]
	reset_all_menus
	return $w
}

proc trunc_fname { fn { n 64 } } {
#
# Truncate a file name to be displayed
#
	if { $n < 16 } {
		# cannot be too short
		set n 16
	}

	set ln [string length $fn]
	if { $ln > $n } {
		# how many characters to remove
		set d [expr { $ln - $n + 3 }]
		set lf [expr { $ln / 2 - $d / 2 }]
		set lt [expr { $lf + $d + 1 }]
		set fn "[string range $fn 0 $lf]...[string range $fn $lt end]"
	}
	return $fn
}

proc term_clean { } {

	global Term

	$Term configure -state normal
	$Term delete 1.0 end
	$Term configure -state disabled
}

proc term_addtxt { txt } {

	global Term ST

	$Term configure -state normal
	$Term insert end $txt
	$Term configure -state disabled
	$Term yview -pickplace end

	if { $ST(CF) != "" } {
		catch { puts -nonewline $ST(CF) $txt }
	}
}

proc term_endline { } {

	global TCMD Term TermLines ST

	$Term configure -state normal
	$Term insert end "\n"

	if { $ST(CF) != "" } {
		catch { puts $ST(CF) "" }
	}

	while 1 {
		set ix [$Term index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= $TermLines } {
			break
		}
		# delete the topmost line if above limit
		$Term delete 1.0 2.0
	}

	$Term configure -state disabled
	# make sure the last line is displayed
	$Term yview -pickplace end
	# BOL flag
	set TCMD(BL) 0
}

proc term_dspline { ln } {
#
# Write a line to the terminal
#
	global TCMD

	if $TCMD(BL) {
		term_endline
	}
	term_addtxt $ln
	term_endline
}

proc term_output { } {

	global TCMD

	if [catch { read $TCMD(FD) } chunk] {
		# assume EOF
		stop_term
		return
	}

	if [eof $TCMD(FD)] {
		stop_term
		return
	}

	if { $chunk == "" } {
		return
	}

	# filter out non-printable characters
	while { ![string is ascii -failindex el $chunk] } {
		set chunk [string replace $chunk $el $el]
	}

	append TCMD(BF) $chunk
	# look for CR+LF, LF+CR, CR, LF; if there is only one of those at the
	# end, ignore it for now and keep for posterity
	set sl [string length $TCMD(BF)]

	while { [regexp "\[\r\n\]" $TCMD(BF) m] } {
		set el [string first $m $TCMD(BF)]
		if { $el == 0 } {
			# first character
			if { $sl < 2 } {
				# have to leave it and wait
				return
			}
			# check the second one
			set n [string index $TCMD(BF) 1]
			if { $m == "\r" && $n == "\n" || \
			     $m == "\n" && $n == "\r"    } {
				# two-character EOL
				set TCMD(BF) [string range $TCMD(BF) 2 end]
				incr sl -2
			} else {
				set TCMD(BF) [string range $TCMD(BF) 1 end]
				incr sl -1
			}
			# complete previous line
			term_endline
			set TCMD(BL) 0
			continue
		}
		# send the preceding string to the terminal
		term_addtxt [string range $TCMD(BF) 0 [expr $el - 1]]
		incr sl -$el
		set TCMD(BL) 1
		set TCMD(BF) [string range $TCMD(BF) $el end]
	}

	if { $TCMD(BF) != "" } {
		term_addtxt $TCMD(BF)
		set TCMD(BL) 1
		set TCMD(BF) ""
	}
}

###############################################################################

proc read_piprc { } {
#
# Read the rc file
#
	global env

	if ![info exists env(HOME)] {
		return ""
	}

	if [catch { open [file join $env(HOME) ".piprc"] "r" } fd] {
		# cannot open rc file
		return ""
	}

	if [catch { read $fd } rf] {
		catch { close $fd }
		return ""
	}

	catch { close $fd }

	return $rf
}

proc write_piprc { f } {
#
# Write the rc file
#
	global env

	if ![info exists env(HOME)] {
		return
	}

	if [catch { open [file join $env(HOME) ".piprc"] "w" } fd] {
		# cannot open rc file
		return
	}

	catch { puts -nonewline $fd $f }
	catch { close $fd }
}

proc get_rcoptions { } {
#
# Retrieve the list of options from the .rc file
#
	set rc [read_piprc]

	foreach v { LProjects ESchemes ESchemesA LCSFile } {

		global $v

		set tag [string toupper $v]
		if [catch { dict get $rc $tag } val] {
			# missing == empty
			set val ""
		}
		set $v $val
	}
}

proc set_rcoption { args } {
#
# Update the .rc file after changing an option
#
	set rc [read_piprc]

	foreach v $args {

		global $v

		set tag [string toupper $v]
		eval "set v $[subst $v]"
		catch { dict set rc $tag $v }
	}

	write_piprc $rc
}

###############################################################################

proc file_present { f } {
#
# Checks if the file is present as a regular file
#
	if { [file exists $f] && [file isfile $f] } {
		return 1
	}
	return 0
}

proc reserved_dname { d } {
#
# Checks a root directory name against being reserved
#
	global IGDirs

	foreach m $IGDirs {
		if [regexp -nocase $m $d] {
			return 1
		}
	}
	return 0
}

proc file_class { f } {
#
# Checks if the file name formally qualifies the file as a project member
#
	global LFTypes

	if [catch { file tail $f } f] {
		# this may fail for weird file names, e.g., starting with ~
		# which Cygwin may try to expand
		return ""
	}

	foreach t $LFTypes {
		foreach p [lindex $t 1] {
			if [regexp $p $f] {
				return $t
			}
		}
	}
	return ""
}

proc file_location { f } {
#
# Classifies the location of the specified file; returns:
#
#	"T"	inside project
#	"S"	PicOS (system)
#	"V"	VUEE
#	"B"	Boards
#	"X"	external
#
	global P PicOSPath

	set f [fpnorm $f]

	if { $P(AC) != "" && [string first $P(AC) $f] == 0 } {
		# project
		return "T"
	}

	if { [string first [fpnorm [file join $PicOSPath "PicOS"]] $f] == 0 } {
		# PicOS
		return "S"
	}

	if { [string first [boards_dir] $f] == 0 } {
		# board
		return "B"
	}

	if { [string first [fpnorm [file join $PicOSPath "../VUEE/PICOS"]] $f] \
	    == 0 } {
		return "V"
	}

	return "X"
}

proc relative_path { f } {
#
# Transforms an absolute path into a project-relative path, if possible;
# otherwise, retains the full normalized path
#
	global P

	set f [fpnorm $f]

	if { [file_location $f] != "T" } {
		# not in project
		return $f
	}

	if { [string first $P(AC) $f] != 0 } {
		# not in project
		return $f
	}

	set f [string range $f [string length $P(AC)] end]
	regsub "^//*" $f "" f
	return $f
}

proc board_set { } {
#
# Returns the list (set) of boards used by the project's program
#
	global P

	# bool: multiple boards flag
	set mb [dict get $P(CO) "MB"]
	# list of boards indexed by program number
	set bo [dict get $P(CO) "BO"]

	if { $mb == "" || $bo == "" } {
		# boards not defined for this project yet
		return ""
	}

	return [lsort -unique $bo]
}

proc gfl_erase { } {
#
# Clean up the tree (e.g., after closing the project)
#
	global P

	$P(FL) delete [$P(FL) children {}]
}

proc gfl_tree { } {
#
# Fill/update the treeview file list with files
#
	global LFTypes MKRECV P

	array unset MKRECV
	set fl [gfl_all_rec .]
	# we don't need this any more
	array unset MKRECV

	set tv $P(FL)

	# remove all nodes in treeview; will fill it from scratch
	$tv delete [$tv children {}]

	log "Updating tree view"

	foreach t $LFTypes {
		# header title
		set h [lindex $t 0]
		# the list of items for this header, directories going first
		set l [gfl_spec $fl $h]
		set id [$tv insert {} end -text ${h}: -values [list $h "c"]]
		if [info exists P(FL,c,$h)] {
			set of 1
		} else {
			set of 0
		}
		$tv item $id -open $of
		# tree, parent, list, path so far
		gfl_tree_pop $tv $id $l ""
	}

	gfl_make_ctags

	if { [dict get $P(CO) "OPSYSFILES"] == 0 } {
		# do not include BOARD files, if the user prefers not to care
		return
	}

	set bdir [boards_dir]
	foreach b [board_set] {

		# path to the board directory
		if { $b == "" } {
			# empty
			continue
		}
		set bp [file join $bdir $b]
		if ![file isdirectory $bp] {
			# just in case
			continue
		}

		# all files
		set fl [glob -nocomplain -directory $bp -tails *]
		set l ""
		foreach f $fl {
			# this is flat, so only plain files at this level count
			if ![file isfile [file join $bp $f]] {
				continue
			}
			lappend l $f
		}

		set l [lsort $l]

		set id [$tv insert {} end -text "<${b}>:" \
			-values [list $bp "b"] -tags sboard]
		if [info exists P(FL,b,$b)] {
			set of 1
		} else {
			set of 0
		}
		$tv item $id -open $of
		# the function expects a pair <dirs, files>, but we only have
		# files
		gfl_tree_pop $tv $id [list "" $l] $bp
	}
}

proc gfl_tree_pop { tv node lst path } {
#
# Populate the list of children of the given node with the current contents
# of the list
#
	global P EFST

	foreach t [lindex $lst 0] {
		# the directories
		set n [lindex $t 0]
		# augmented path
		set p [file join $path $n]
		set id [$tv insert $node end -text $n -values [list $p "d"]]
		# check if should be open or closed
		if [info exists P(FL,d,$p)] {
			# open
			set of 1
		} else {
			set of 0
		}
		$tv item $id -open $of
		# add recursively the children
		gfl_tree_pop $tv $id [lindex $t 1] $p
	}
	# now for the files
	foreach t [lindex $lst 1] {
		set p [file join $path $t]
		set f [fpnorm $p]
		set u [file_edit_pipe $f]
		if { $u != "" } {
			# edited
			if { $EFST($u,M) > 0 } {
				# modified
				set tag sred
			} else {
				set tag sgreen
			}
			$tv insert $node end -text $t -values [list $p "f"] \
				-tags $tag
		} else {
			$tv insert $node end -text $t -values [list $p "f"]
		}
	}
}

proc gfl_all_rec { path } {
#
# The recursive part of gfl_tree; returns a two-element list { dirs files } or
# NULL is there is nothing more below this point
#
	global MKRECV

	set sdl [glob -nocomplain -directory $path -tails *]
	if { $sdl == "" } {
		return ""
	}

	set dirs ""
	set fils ""

	foreach f $sdl {
		set p [fpnorm [file join $path $f]]
		if { $p == "" } {
			# just in case
			continue
		}
		if [file isdirectory $p] {
			# should we ignore it
			if [info exists MKRECV($p)] {
				# avoid loops
				continue
			}
			set MKRECV($p) ""
			if [reserved_dname $f] {
				continue
			}
			set rfl [gfl_all_rec [file join $path $f]]
			lappend dirs [list $f $rfl]
			continue
		}
		# a regular file
		set t [file_class $f]
		if { $t != "" } {
			lappend fils [list [lindex $t 0] $f]
		}
	}
	return [list $dirs $fils]
}

proc gfl_spec { fl ft } {
#
# Given a combined global list of all qualified files, extracts from it a
# class-specific sublist; this time the directories and files at each level
# are sorted by name; note that they are sorted independently (the dirs are
# supposed to go first when the thing is displayed)
#
	set dirs ""
	set fils ""

	# flag == at least one file present here or down from here
	set fp 0

	foreach t [lindex $fl 0] {
		# directories first; the name
		set n [lindex $t 0]
		set rfl [gfl_spec [lindex $t 1] $ft]
		if { $rfl != "" } {
			set fp 1
			lappend dirs [list $n $rfl]
		}
	}
	set dirs [lsort -index 0 $dirs]

	foreach t [lindex $fl 1] {
		# files
		if { [lindex $t 0] != $ft } {
			# wrong type
			continue
		}
		# just the name
		lappend fils [lindex $t 1]
		set fp 1
	}

	if !$fp {
		return ""
	}

	set fils [lsort $fils]

	return [list $dirs $fils]
}

proc gfl_open { tree node } {
#
# Keep track of which ones are open and which ones are closed; we will need it
# for updates
#
	global P

	set t [$tree set $node type]

	if { $t == "c" || $t == "d" || $t == "b" } {
		# mark it as open
		set P(FL,$t,[$tree set $node filename]) ""
	}
}

proc gfl_close { tree node } {

	global P

	set t [$tree set $node type]

	if { $t == "c" || $t == "d" || $t == "b" } {
		array unset P "FL,$t,[$tree set $node filename]"
	}
}

proc gfl_files { { pat "" } { neg 0 } } {
#
# Finds all files in the tree view matching the specified pattern; looks only
# at actual project files skipping over the Boards
#
	global P

	set res ""

	foreach d [$P(FL) children {}] {
		set vs [$P(FL) item $d -values]
		if { [lindex $vs 1] != "c" } {
			# ignore Boards; just in case, we assume they need not
			# be all at the end
			continue
		}
		# only headers at this level
		set lres [gfl_files_rec $d $pat $neg]
		if { $lres != "" } {
			set res [concat $res $lres]
		}
	}
	return $res
}

proc gfl_files_rec { nd pat neg } {
#
# The recursive traverser for gfl_files
#
	global P

	set res ""

	foreach d [$P(FL) children $nd] {
		set vs [$P(FL) item $d -values]
		if { [lindex $vs 1] != "f" } {
			# not a file
			set lres [gfl_files_rec $d $pat $neg]
			if { $lres != "" } {
				set res [concat $res $lres]
			}
		} else {
			set fn [lindex $vs 0]
			if { $pat == "" || 
			   ( ( $neg && ![regexp $pat $fn] ) ||
			     (!$neg &&  [regexp $pat $fn] ) ) } {
				lappend res $fn
			}
		}
	}
	return $res
}

proc gfl_find { path } {
#
# Locates the node corresponding to the given file path
#
	global P

	foreach d [$P(FL) children {}] {
		# only headers at this level
		set node [gfl_find_rec $d $path]
		if { $node != "" } {
			return $node
		}
	}
	return ""
}

proc gfl_find_rec { nd path } {
#
# The recursive traverser for gfl_find
#
	global P

	foreach d [$P(FL) children $nd] {
		set vs [$P(FL) item $d -values]
		if { [lindex $vs 1] != "f" } {
			# not a file
			set node [gfl_find_rec $d $path]
			if { $node != "" } {
				return $node
			}
			continue
		}
		if { [fpnorm [lindex $vs 0]] == $path } {
			return $d
		}
	}

	return ""
}

proc gfl_status { path val } {
#
# Change the color of file label in the tree based on the current file status
#
	global P

	set node [gfl_find $path]

	if { $node == "" } {
		return
	}

	if { $val < 0 } {
		$P(FL) item $node -tags {}
	} elseif { $val == 0 } {
		$P(FL) item $node -tags sgreen
	} else {
		$P(FL) item $node -tags sred
	}
}

proc trim_to_existing { fl } {

	set res ""

	foreach f $fl {

		if [file exists $f] {
			lappend res $f
		}
	}

	return $res
}

proc gfl_make_ctags { } {
#
# Create ctags for all files in the current project. We do this somewhat
# nonchalantly (for all files) whenever we suspect that something has changed,
# like after editing a file. Note that this is still a toy implementation of
# our SDK. We shall worry about efficiency later (if ever). Note: this one
# is quite fast compared to ctagging PicOS files.
#
	global TagsCmd PTagsArgs

	# the list of the proper files of the project
	set fl [trim_to_existing [gfl_files "(\\.c|\\.cc|\\.h)$"]]

	if { $fl == "" } {
		# no files (yet?)
		set tl ""
	} elseif [catch { xq $TagsCmd [concat $PTagsArgs $fl] } tl] {
		alert "Cannot generate project tags: $tl"
		set tl ""
	}
	log "Local tag file [string length $tl] characters"
	store_tags $tl "T"
}

proc sys_make_ctags { } {
#
# Create system tags for all system files referenced by the project; this is
# done after every Makefile creation and may take a while, but appears to be
# reasonably fast, so we just do a straight exec
#
	global TagsCmd STagsArgs

	# OK, we are a bit smarter; check if the option is on; if not,
	# remove ctags, which are a memory burden; we shall re-do them
	# when the option is turned on (or something changes and the option
	# is on)

	set tl ""
	if [file_perm "S"] {
		set fl [get_picos_project_files]
		if { $fl != "" } {
			if [catch { xq $TagsCmd [concat $STagsArgs $fl] } tl] {
				alert "Cannot generate PicOS tags: $tl"
				set tl ""
			}
		}
	}
	log "PicOS tag file [string length $tl] characters"
	store_tags $tl "S"
}

proc vue_make_ctags { } {
#
# Create VUEE tags, i.e., for all files in VUEE/PICOS, excluding the links to
# PicOS files. We do this the first time the tags are needed and never update
# them for as long as PIP is up.
#
	global TagsCmd VTagsArgs

	set tl ""
	if [file_perm "V"] {
		set fl [get_vuee_files]
		if { $fl != "" } {
			if [catch { xq $TagsCmd [concat $VTagsArgs $fl] } tl] {
				alert "Cannot generate VUEE tags: $tl"
				set tl ""
			}
		}
	}
	log "VUEE tag file [string length $tl] characters"
	store_tags $tl "V"
}

proc store_tags { tl m } {
#
# Preprocess and store tags from elvtags output
#
	global P

	array unset P "FL,$m,*"

	# preprocess the tags
	set tl [split $tl "\n"]
	foreach t $tl {
		if { [string index $t 0] == "!" } {
			# these are comments, ignore
			continue
		}
		if ![regexp "^(\[^\t\]+)\[\t\]+(\[^\t\]+)\[\t\]+(.+);\"" \
		    $t jnk ta fn cm] {
			# some garbage
			continue
		}
		if ![info exists P(FL,$m,$ta)] {
			set P(FL,$m,$ta) ""
		}
		set ne [list $fn $cm]
		if { [string tolower [file extension $fn]] == ".h" } {
			# headers have lower priority
			lappend P(FL,$m,$ta) [list $fn $cm]
		} else {
			# other files go to front
			set P(FL,$m,$ta) [concat [list $ne] $P(FL,$m,$ta)]
		}
	}
}

###############################################################################

proc tag_find { tag m } {
#
# Locates the specified tag in the tag set described by m
#
	global P

	if ![info exists P(FL,$m,$tag)] {
		# not found
		return ""
	}

	# check for a previous reference
	set nr 0
	if { [info exists P(FL,LT$m)] && [lindex $P(FL,LT$m) 0] == $tag } {
		# same tag referenced multiple times, get reference number
		set nr [lindex $P(FL,LT$m) 1]
		# rotate
		incr nr
		if { $nr >= [llength $P(FL,$m,$tag)] } {
			# wrap around
			set nr 0
		}
	}

	set P(FL,LT$m) [list $tag $nr]

	set ne [lindex $P(FL,$m,$tag) $nr]

	return [list [fpnorm [lindex $ne 0]] [lindex $ne 1]]
}

proc tag_request { fd tag } {
#
# Handles a tag request arriving from one of the editor sessions
#
	global P

	log "Tag request: $tag"

	# first try local tags
	set em "T"
	set ta [tag_find $tag $em]
	if { $ta == "" && [file_perm "S"] } {
		# try system tags
		set em "S"
		set ta [tag_find $tag $em]
	}
	if { $ta == "" && [file_perm "V"] } {
		# try VUEE tags
		set em "V"
		set ta [tag_find $tag $em]
	}
	if { $ta == "" } {
		term_dspline "Tag $tag not found"
		# check if the Search window is present; if so, insert the
		# tag's string into the search text widget
		if { $P(SWN) != "" } {
			$P(SWN,ss) delete 1.0 end
			$P(SWN,ss) insert end $tag
		}
		return
	}

	set fp [lindex $ta 0]
	set cm [lindex $ta 1]

	# get the pipe to the target file
	set u [file_edit_pipe $fp]
	if { $u == "" } {
		# not being edited, try to open it first
		edit_file $fp
		set u [file_edit_pipe $fp]
		if { $u == "" } {
			# failed for some reason
			log "Failed to open file $fm for tag"
			return
		}
	}

	# issue the command and raise the window
	edit_command $u $cm
}

###############################################################################

proc file_perm { em } {
#
# Returns the permission value for a give file type "T", "S", "V", "X"
# file path is normalized
#
	global P

	if { $em == "S" || $em == "B" } {
		return [dict get $P(CO) "OPSYSFILES"]
	} elseif { $em == "V" } {
		return [dict get $P(CO) "OPVUEEFILES"]
	}
	return 3
}

proc find_escheme { fn em } {
#
# Produce the editor scheme for the file in question
#
	global ESchemesA

	set tp [lindex [file_class $fn] 0]

	set sch ""
	foreach s $ESchemesA {
		set t [lindex $s 0]
		if { $t != "All" && $t != $tp } {
			continue
		}
		set k [lindex $s 2]
		if { $k != "All" } {
			if { $em == "T" && $k != "Project" } {
				continue
			}
			if { $em == "S" && $k != "PicOS" } {
				continue
			}
			if { $em == "V" && $k != "VUEE" } {
				continue
			}
			if { $em == "X" && $k != "External" } {
				continue
			}
		}
		set p [lindex $s 1]
		if { $p != "" } {
			if { ![catch { regexp $p $fn } res] && !$res } {
				continue
			}
		}
		# applicable
		set sch [lindex $s 3]
		break
	}

	log "Using elvis scheme $sch"
	return [get_escheme $sch]
}

proc edit_file { fn } {
#
# Open a file for edit
#
	global EFDS EFST EditCommand

	set ar ""

	set em [file_location $fn]
	if { [file_perm $em] < 3 } {
		# read only
		lappend ar "-R"
	}

	# generate configuration arguments for elvis
	set ca ""

	# this must produce something
	set sc [find_escheme $fn $em]

	foreach it $sc {
		set f [lindex $it 0]
		set l [lindex $it 1]
		if { $f == "font" } {
			if { $l != "default" } {
				lappend ar "-font"
				lappend ar $l
			}
			continue
		}
		if { $f == "commands" } {
			set l [split $l "\n"]
			foreach c $l {
				set c [string trim $c]
				if { $c != "" } {
					append ca $c
					append ca "|"
				}
			}
			continue
		}
		append ca "color $f"
		set lk [lindex $l 0]
		set c0 [lindex $l 1]
		set c1 [lindex $l 2]
		set c2 [lindex $l 3]
		set at [lrange $l 4 end]
		if { $lk != "" } {
			append ca " like $lk"
		} else {
			if { $c0 != "" } {
				append ca " $c0"
				if { $c1 != "" } {
					append ca " or $c1"
				}
			}
			if { $c2 != "" } {
				append ca " on $c2"
			}
		}
		foreach a $at {
			append ca " $a"
		}
		append ca "|"
	}
	set ca [string trimright $ca "|"]

	lappend ar "-c"
	lappend ar $ca
	lappend ar $fn

	set ar "$EditCommand $ar"
	if [catch { open "|$ar" "r+" } fd] {
		alert "Cannot start text editor: $fd"
		return
	}

	set EFDS($fd) $fn
	# file status; note: -1 means something like "unsettled"; we want to
	# use this value to mark the moment when the window has been opened
	# and the editor is ready, e.g., to accept commands from STDIN; I don't
	# know what is going on, but it appears that on Linux, if a command is
	# issued too soon, the edit session gets killed; another (minor) issue
	# is that initially the file status starts "modified" then immediately
	# becomes "not modified"; so we shall assume that the initial modified
	# status is ignored, and the true status becomes "settled" upon the
	# first perception of "not modified" status; if you are confused, you
	# are not alone
	set EFST($fd,M) -1
	# this one indicates if the file ever was modified (used to decide if
	# we have to redo ctags)
	set EFST($fd,A) 0
	# open mode: T, S, V, X
	set EFST($fd,E) $em
	# PID (unknown yet)
	set EFST($fd,P) ""
	# command queue
	set EFST($fd,C) ""
	# mark the status in the tree
	gfl_status $fn 0

	log "Editing file: $fn, mode $em, cmd $ar"

	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "edit_status_read $fd"
}

proc edit_status_read { fd } {

	global EFDS EFST

	if [catch { gets $fd line } val] {
		# aborted
		log "Edit session aborted"
		edit_close $fd 1
		return
	}

	if { $val < 0 } {
		# finished normally
		edit_close $fd 0
		return
	}

	set line [string trim $line]

	if [regexp "BST: (\[0-9\])" $line jnk st] {
		if { $EFST($fd,M) < 0 && $st > 0 } {
			# ignore the initial "modified" status, which is
			# bogus
			log "Bogus modified status ignored"
			return
		}
		if { $st != $EFST($fd,M) } {
			set os $EFST($fd,M)
			# an actual change
			log "Edit status change for $EFDS($fd): $EFST($fd,M) ->\
				$st"
			set EFST($fd,M) $st
			gfl_status $EFDS($fd) $st
			if { $os < 0 } {
				global NewCmdDelay
				delay $NewCmdDelay
				# it is not impossible that the
				# pipe has been closed in the
				# meantime
				if [info exists EFST($fd,C)] {
					foreach c $EFST($fd,C) {
						catch { puts $fd $c }
					}
					set EFST($fd,C) ""
				}
			}
			if $st {
				# mark the file as "ever modified"
				set EFST($fd,A) 1
			}
		}
		return
	}

	if [regexp "TAG: +(.+)" $line jnk st] {
		tag_request $fd $st
		return
	}

	if [regexp "PID: (\[0-9\]+)" $line jnk st] {
		log "Edit process ID: $st"
		if { $EFST($fd,P) == "" } {
			set EFST($fd,P) $st
		}
		return
	}

	if [regexp "can't load font +(\[^ \t\]+)" $line jk fn] {
		edit_kill $EFDS($fd)
		alert "Illegal font size for Elvis: $fn, please reconfigure"
		return
	}

	if [regexp "could not contact X server" $line] {
		edit_kill $EFDS($fd)
		alert "No X server available to run the editor! You need an X\
			server for Elvis"
		return
	}

	log "ELVIS: $line"

	# room for more
}

proc edit_command { fd cmd } {

	global EFST

	if { $EFST($fd,M) < 0 } {
		# status "unsettled"
		lappend EFST($fd,C) $cmd
		log "Queueing command $cmd to $fd"
	} else {
		catch { puts $fd $cmd }
		log "Issuing command $cmd to $fd"
	}
}

proc edit_close { fd ab } {

	global EFST EFDS

	catch { close $fd }

	if [info exists EFDS($fd)] {
		# not an external kill, soft close
		if $ab {
			set ab "aborted"
		} else {
			set ab "closed"
		}
		log "Edit session $EFDS($fd) $ab"
		set es $EFST($fd,E)
		set em $EFST($fd,A)
		# update file status in tree view
		gfl_status $EFDS($fd) -1
		array unset EFST "$fd,*"
		unset EFDS($fd)
		# redo the file list; FIXME: don't do this, but redo tags, if
		# the file has (ever) changed
		if { $em > 0 } {
			log "File was modified"
			# modified
			if { $es == "T" } {
				# local modified file -> redo ctags
				gfl_make_ctags
			} elseif { $es == "V" } {
				vue_make_ctags
			} elseif { $es == "S" } {
				# redo system tags, but only if this is the
				# last system file being closed (the operation
				# is slow)
				set st 1
				foreach gd [array names EFDS] {
					if { $EFST($gd,M) > 0 &&
					     $EFST($gd,E) == "S" } {
						set st 0
						break
					}
				}
				if $st {
					sys_make_ctags
				}
			}
		} else {
			log "File was never modified"
		}
	}
}

proc edit_unsaved { } {
#
# Check for unsaved files being edited and prompt the user to do something
# about them
#
	global EFST EFDS

	set nf 0
	set ul ""

	foreach fd [array names EFDS] {
		if { $EFST($fd,M) > 0 } {
			incr nf
			append ul ", $EFDS($fd)"
		}
	}

	if { $nf == 0 } {
		# no unsaved files
		return 0
	}

	set ul [string range $ul 2 end]

	if { $nf == 1 } {
		alert "Unsaved file: $ul. Please save the file or close the\
			editing session and try again."
	} else {
		# more than one unsaved file
		alert "There are $nf unsaved files: $ul. Please save them or\
			close the editing sessions and try again."
	}

	# terminate the editing sessions

	return 1
}

proc close_modified { } {
#
# Closes the modified files, if the user says so
#
	global EFST EFDS

	set nf 0
	set ul ""
	set dl ""

	foreach fd [array names EFDS] {
		if { $EFST($fd,M) > 0 } {
			incr nf
			append ul "$EFDS($fd), "
			lappend dl $fd
		}
	}

	if { $nf == 0 } {
		# no unsaved files
		return 1
	}

	set ul [string range $ul 2 end]

	if { $nf == 1 } {
		set msg "This file: $ul has "
	} else {
		set msg "These files: $ul have "
	}
	append msg "been modified but not saved."

	reset_all_menus 1
	set v [tk_dialog .alert "Attention!" $msg "" 0 \
		"Save" "Do not save" "Cancel"]
	reset_all_menus

	if { $v == 1 } {
		# proceed as is
		return 1
	}

	if { $v == 2 } {
		# cancel
		return 0
	}

	# save the files
	foreach u $dl {
		# the space in front sys "do it quietly" (no need to raise the
		# window)
		edit_command $u " w!"
		delay 10
	}

	# wait for them to get saved
	for { set i 0 } { $i < 10 } { incr i } {
		delay 200
		set ul ""
		set nf 0
		foreach fd [array names EFDS] {
			if { $EFST($fd,M) > 0 } {
				incr nf
				append ul "$EFDS($fd), "
			}
		}
		if { $nf == 0 } {
			# done
			break
		}
		# keep waiting
	}

	if $nf {
		if { $nf > 1 } {
			set msg "Files "
		} else {
			set msg "File "
		}
		append msg "[string range $ul 2 end] couldn't be saved.\
			Do you want to proceed anyway?"

		return [confirm $msg]
	}

	return 1
}

proc edit_kill { { fp "" } } {

	global EFDS EFST

	foreach fd [array names EFDS] {
		if { $fp != "" && $EFDS($fd) != $fp } {
			# not this file
			continue
		}
		gfl_status $EFDS($fd) -1
		unset EFDS($fd)
		set pid $EFST($fd,P)
		array unset EFST($fd,*)
		if { $pid != "" } {
			log "Killing edit process: $pid"
			if [catch { exec kill -INT $pid } err] {
				log "Cannot kill: $err"
			}
		}
	}
}

proc file_is_edited { fn { m 0 } } {

	global EFDS EFST

	foreach fd [array names EFDS] {
		if { $EFDS($fd) == $fn } {
			if { $m == 0 || $EFST($fd,M) > 0 } {
				return 1
			}
		}
	}

	return 0
}

proc file_edit_pipe { fn } {

	global EFDS

	foreach fd [array names EFDS] {
		if { $EFDS($fd) == $fn } {
			return $fd
		}
	}

	return ""
}

proc open_for_edit { x y } {

	global P EFDS

	set tv $P(FL)
	set node [$tv identify item $x $y]

	if { $node == "" } {
		return
	}

	set vs [$tv item $node -values]

	if { [lindex $vs 1] != "f" } {	
		# not a file
		return
	}

	set fp [fpnorm [lindex $vs 0]]
	set u [file_edit_pipe $fp]
	if { $u != "" } {
		# being edited
		edit_command $u ""
		# alert "The file is already being edited"
		return
	}
	edit_file $fp
}

proc do_filename_click { w diag x y } {

	# this is the index of the character that has been clicked on
	set ix @$x,$y

	set if 0
	# go back until hit word boundary
	while 1 {
		set c [$w get -- "${ix} - $if chars"]
		if { $c == "" || [isspace $c] } {
			break
		}
		incr if
	}

	if { $if == 0 } {
		return
	}

	set ib 0
	# go forward
	while 1 {
		set c [$w get -- "${ix} + $ib chars"]
		if { $c == "" || [isspace $c] } {
			break
		}
		incr ib
	}

	incr if -1
	# starting index
	set if "${ix} - $if chars"
	set chunk [$w get -- $if "${ix} + $ib chars"]

	# nc points to the last character of the line number
	if { ![regexp "^(.+):(\[1-9\]\[0-9\]*):\[0-9\]" $chunk ma fn ln] &&
	     ![regexp "^(.+):(\[1-9\]\[0-9\]*)" $chunk ma fn ln] } {
		# doesn't look like a line number in a file
		set chunk [string trimright $chunk ":,;"]
		if { $chunk == "" || [file_class $chunk] == "" } {
			return
		}
		set fn $chunk
		set ma $chunk
		set ln 0
	}

	# ending index for the tag
	set ib [string length $ma]

	log "File line ref: $fn, $ln"

	$w tag add errtag $if "$if + $ib chars"

	if [catch { expr $ln } $ln] {
		log "File line number error"
		return
	}

	# a quick check if this is an actual existing file
	set fm [fpnorm $fn]
	if ![file isfile $fm] {
		# try to match the file to one of the project files;
		# FIXME: this may have to be made smarter, to account for the
		# various manglings performed by picomp
		set ft [file tail $fn]
		set fr [file root $ft]
		set fe [file extension $ft]

		# all project files matching the extension
		set fl [gfl_files "\\${fe}$"]

		# the length of root portion of the file name
		set rl [string length $fr]

		# current quality
		set qu 99999

		# current file name
		set fm ""

		foreach f $fl {
			set r [file root [file tail $f]]
			if { $r == $f } {
				# ultimate match
				set fm $f
				break
			}
			if { [string first $r $fr] >= 0 } {
				# substring
				set q [expr $rl - [string length $r]]
				if { $q < $qu } {
					set qu $q
					set fm $f
				}
			}
		}

		if { $fm == "" } {
			log "No matching file found"
			return
		}
	}
	# open the file at the indicated line
	set fm [fpnorm $fm]
	set u [file_edit_pipe $fm]
	if { $u == "" } {
		set em [file_location $fm]
		if { [file_perm $em] < 2 } {
			# do not open in console
			if { $em == "S" } {
				set wh "PicOS"
			} else {
				set wh "VUEE"
			}
			$diag "Viewing/editing $wh files not allowed!"
			return
		}
		edit_file $fm
		set u [file_edit_pipe $fm]
		if { $u == "" } {
			log "Failed to open file $fm for err ref"
			return
		}
	}

	# issue the positioning command if line number was present
	if $ln {
		edit_command $u $ln
	}
}

proc tree_selection { { x "" } { y "" } } {
#
# Construct the list of selected (or pointed to) items
#
	global P

	set tv $P(FL)

	# first check if there's a selection
	set fl ""
	foreach t [$tv selection] {
		# make sure we only look at file/directory items
		set vs [$tv item $t -values]
		set tp [lindex $vs 1]
		if { $tp == "f" || $tp == "d" || $tp == "b" } {
			lappend fl $vs
		}
	}

	if { $fl != "" || $x == "" } {
		# that's it: selection takes precedence over pointer
		return $fl
	}

	# no selection and pointer present, check if it is pointing at some file
	set t [$tv identify item $x $y]
	if { $t == "" } {
		return ""
	}
	set vs [$tv item $t -values]
	set tp [lindex $vs 1]
	if { $tp == "f" || $tp == "d" || $tp == "b" } {
		lappend fl $vs
	}

	return $fl
}

proc tree_menu { x y X Y } {

	global EXCmd P

	# create the menu
	catch { destroy .popm }
	set m [menu .popm -tearoff 0]

	$m add command -label "Edit" -command "open_multiple $x $y"
	set aed [alt_editor]
	if { $aed != "" } {
		$m add command -label "Edit Alt" -command "open_multi_ate $x $y"
	}
	$m add command -label "Delete" -command "delete_multiple $x $y"
	$m add command -label "Rename ..." -command "rename_file $x $y"
	$m add command -label "New file ..." -command "new_file $x $y"
	$m add command -label "Copy from ..." -command "copy_from $x $y"
	$m add command -label "Copy to ..." -command "copy_to $x $y"
	$m add command -label "New directory ..." -command "new_directory $x $y"
	$m add command -label "Run terminal here" -command \
		"run_xterm_here $x $y"
	if { $EXCmd != "" } {
		$m add command -label "Run File Explorer here" \
			-command "run_explorer_here $x $y"
	}
	$m add separator
	$m add command -label "Refresh" -command "gfl_tree"
	tk_popup .popm $X $Y
}

proc open_multiple { { x "" } { y "" } } {
#
# Open files for editing
#
	global P

	if { $P(AC) == "" } {
		return
	}

	set sel [tree_selection $x $y]

	set fl ""
	foreach f $sel {
		# select files only
		if { [lindex $f 1] == "f" } {
			lappend fl [lindex $f 0]
		}
	}

	if { $fl == "" } {
		alert "No file selected"
		return
	}

	if { [llength $fl] == 1 } {
		set fp [fpnorm [lindex $fl 0]]
		if [file_is_edited $fp] {
			alert "The file is already being edited"
		} else {
			edit_file $fp
		}
		return
	}

	set el ""
	foreach f $fl {
		set fp [fpnorm $f]
		if ![file_is_edited $fp] {
			lappend el $fp
		}
	}

	if { $el == "" } {
		alert "All these files are already being edited"
		return
	}

	foreach fp $el {
		edit_file $fp
	}
}

proc alt_editor { } {
#
# Alternate editor
#
	global P

	if { ![dict exists $P(CO) "OPTATECMND"] } {
		return ""
	}
	return [dict get $P(CO) "OPTATECMND"]
}

proc open_multi_ate { { x "" } { y "" } } {
#
# Open files for editing with the alternate editor
#
	global P

	if { $P(AC) == "" } {
		return
	}

	set sel [tree_selection $x $y]

	set fl ""
	foreach f $sel {
		# select files only
		if { [lindex $f 1] == "f" } {
			lappend fl [lindex $f 0]
		}
	}

	if { $fl == "" } {
		alert "No file selected"
		return
	}

	foreach f $fl {
		set fp [fpnorm $f]
		edit_file_ate $fp
	}
}

proc edit_file_ate { fn } {
#
# Edit a file using the alternate text editor
#
	global P

	set aed [alt_editor]
	if { $aed == "" } {
		return
	}

	log "ATE: $aed"

	regsub {%f} $aed $fn aed
	set pgm [lindex $aed 0]
	set arg ""
	foreach a [lrange $aed 1 end] {
		lappend arg $a
	}
	lappend arg "&"
	catch { xq $pgm $arg }
}

proc delete_multiple { { x "" } { y "" } } {
#
# Delete files or directories (the latter must be empty)
#
	global P

	if { $P(AC) == "" } {
		return
	}

	set sel [tree_selection $x $y]

	set fl ""
	foreach f $sel {
		# files first
		if { [lindex $f 1] == "f" } {
			lappend fl [lindex $f 0]
		}
	}

	set de 0

	if { $fl != "" } {
		delete_files $fl
		incr de
	}

	# now go for directories (include boards)
	set fl ""
	foreach f $sel {
		set tp [lindex $f 1]
		if { $tp == "d" || $tp == "b" } {
			lappend fl [lindex $f 0]
		}
	}

	if { $fl != "" } {
		delete_directories $fl
		incr de
	}

	if !$de {
		alert "Nothing selected for deletion"
		return
	}

	# redo the tree view
	gfl_tree
}

proc delete_directories { fl } {

	set ne ""
	set de ""

	foreach f $fl {
		set fils [glob -nocomplain -directory $f *]
		if { $fils != "" } {
			# nonempty
			lappend ne $f
		} else {
			lappend de $f
		}
	}

	if { $ne != "" } {
		set msg "Director"
		if { [llength $ne] > 1 } {
			append msg "ies: [join $ne ", "] are "
			set wh "their"
		} else {
			append msg "y: [lindex $ne 0] is "
			set wh "its"
		}
		append msg "nonempty. You must delete $wh contents first"
		alert $msg
	}

	# proceed with the empty ones

	foreach f $de {
		log "Deleting file: $f"
		catch { file delete -force -- [fpnorm $f] }
	}
}

proc delete_files { fl } {

	set msg "Are you sure you want to delete "

	if { [llength $fl] < 2 } {
		append msg "this file: [lindex $fl 0]"
	} else {
		append msg "these files: "
		append msg [join $fl ", "]
	}
	append msg "?"

	if ![confirm $msg] {
		return
	}

	# check for modification
	set mf ""

	foreach f $fl {
		if [file_is_edited [fpnorm $f] 1] {
			lappend mf $f
		}
	}

	if { $mf != "" } {
		set msg "File"
		if { [llength $mf] > 1 } {
			append msg "s: "
			append msg [join $mf ", "]
			append msg " have"
		} else {
			append msg ": [lindex $mf 0] has"
		}
		append msg " been edited and modified but not yet\
			saved. If you proceed, the edit sessions will\
			be closed and the changes will be discarded."
		if ![confirm $msg] {
			return
		}
	}

	# delete
	foreach f $fl {
		set fp [fpnorm $f]
		edit_kill $fp
		log "Deleting file: $fp"
		catch { file delete -force -- $fp }
	}
}

###############################################################################

proc md_click { val { lv 0 } } {
#
# Generic done event for modal windows/dialogs
#
	global P

	if { [info exists P(M$lv,EV)] && $P(M$lv,EV) == 0 } {
		set P(M$lv,EV) $val
	}
}

proc md_stop { { lv 0 } } {
#
# Close operation for a modal window
#
	global P

	if [info exists P(M$lv,WI)] {
		catch { destroy $P(M$lv,WI) }
	}
	array unset P "M$lv,*"
	# make sure all upper modal windows are destroyed as well; this is
	# in case grab doesn't work
	for { set l $lv } { $l < 10 } { incr l } {
		if [info exists P(M$l,WI)] {
			md_stop $l
		}
	}
	# if we are at level > 0 and previous level exists, make it grab the
	# pointers
	while { $lv > 0 } {
		incr lv -1
		if [info exists P(M$lv,WI)] {
			catch { grab $P(M$lv,WI) }
			break
		}
	}
}

proc md_wait { { lv 0 } } {
#
# Wait for an event on the modal dialog
#
	global P

	set P(M$lv,EV) 0
	vwait P(M$lv,EV)
	if ![info exists P(M$lv,EV)] {
		return -1
	}
	if { $P(M$lv,EV) < 0 } {
		# cancellation
		md_stop $lv
		return -1
	}

	return $P(M$lv,EV)
}

proc md_window { tt { lv 0 } } {
#
# Creates a modal dialog
#
	global P

	set w [cw].modal$lv
	catch { destroy $w }
	set P(M$lv,WI) $w
	toplevel $w
	wm title $w $tt

	if { $lv > 0 } {
		set l [expr $lv - 1]
		if [info exists P(M$l,WI)] {
			# release the grab of the previous level window
			catch { grab release $P(M$l,WI) }
		}
	}

	# this fails sometimes
	catch { grab $w }
	return $w
}

###############################################################################

proc bad_dirname { } {

	alert "The new directory name is illegal, i.e., is reserved or\
		includes a disallowed exotic character"
}

proc new_directory { { x "" } { y "" } } {
#
# Creates a new directory in the project's directory
#
	global P

	if { $P(AC) == "" } {
		# ignore if no project
		return
	}

	set dir [lindex [tree_sel_params] 0]

	mk_new_dir_window $dir

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}

		if { $ev == 0 } {
			# nothing
			continue
		}

		# validate the directory
		set nd [fpnorm [file join $dir $P(M0,DI)]]

		if { [file_location $nd] != "T" } {
			alert "The new directory is outside the project tree!\
				Use the buttons in the Search window to create\
				directories and files outside the project"
			continue
		}
		if [reserved_dname [file tail $nd]] {
			bad_dirname
			continue
		}
		if [file isdirectory $nd] {
			alert "Directory $nd already exists"
			continue
		}
		log "Creating directory: $nd"
		if [catch { file mkdir $nd } err] {
			alert "Cannot create directory $nd: $err"
			continue
		}

		md_stop
		gfl_tree
		return
	}
}

proc mk_new_dir_window { dir } {
#
# Opens a dialog to specify a new directory
#
	global P FFont

	set w [md_window "New directory"]

	frame $w.tf
	pack $w.tf -side top -expand y -fill x

	label $w.tf.l -text "$dir / "
	pack $w.tf.l -side left -expand n -fill x

	set P(M0,DI) "NEW_DIR"
	entry $w.tf.e -width 8 -font $FFont -textvariable P(M0,DI)
	pack $w.tf.e -side left -expand y -fill x

	frame $w.bf
	pack $w.bf -side top -expand y -fill x

	button $w.bf.b -text "Done" -command "md_click 1"
	pack $w.bf.b -side right -expand n -fill x

	button $w.bf.c -text "Cancel" -command "md_click -1"
	pack $w.bf.c -side left -expand n -fill x

	bind $w <Destroy> "md_click -1"
}

proc tree_sel_params { { x "" } { y "" } } {
#
# Returns the list of selection parameters { dir, type, extension } forcing
# the interpretation as a single selection. Used to determine, e.g., the
# target directory of a new file
#
	global P

	set tv $P(FL)

	set t [$tv selection]

	if { [llength $t] != 1 && $x != "" } {
		# use the pointer
		set t [$tv identify item $x $y]
	} else {
		# use the selection
		set t [lindex $t 0]
	}

	# the defaults
	set dir "."
	set typ Sources
	set ext ""

	while 1 {
		if { $t == "" } {
			# don't know
			break
		}
		set vs [$tv item $t -values]
		set tp [lindex $vs 1]
		if { $tp == "d" } {
			# use this directory and look up the parent class
			if { $dir == "." } {
				# first directory on our path up
				set dir [lindex $vs 0]
			}
			while 1 {
				set t [$tv parent $t]
				if { $t == "" } {
					# will redo for "unknown"
					break
				}
				# node type
				set ht [lindex [$tv item $t -values] 0]
				if { $ht == "c" || $ht == "b" } {
					# the top, i.e., the class node or
					# a board header
					break
				}
			}
			# redo for unknown or class
			continue
		}

		set fn [lindex $vs 0]

		if { $tp == "c" } {
			# class, force the suffix
			set typ $fn
			break
		}

		if { $tp == "b" } {
			# board
			if { $dir == "." } {
				set dir [fpnorm [file join [boards_dir] $fn]]
			}
			# a special type
			set typ "Board"
			break
		}

		# file
		set ext [file extension $fn]
		set dir [file dirname $fn]

		# determine types from the class
		set t [$tv parent $t]
	}

	return [list $dir $typ $ext]
}

proc new_file { { x "" } { y "" } } {

	global P LFTypes

	if { $P(AC) == "" } {
		return
	}

	lassign [tree_sel_params] dir typ ext

	set fo 1
	foreach t $LFTypes {
		if { [lindex $t 0] == $typ } {
			set fo 0
			break
		}
	}

	if $fo {
		# this must be Board, assume Header
		set t [lindex $LFTypes 0]
	}

	set typ [list [lindex $t 2]]
	if { $ext == "" } {
		# the first extension from filetypes
		set ext [lindex [lindex [lindex $typ 0] 1] 0]
	}

	set dir [fpnorm $dir]

	while 1 {

		reset_all_menus 1
		set fn [tk_getSaveFile \
				-defaultextension $ext \
				-filetypes $typ \
				-initialdir $dir \
				-title "New file"]
		reset_all_menus

		if { $fn == "" } {
			# cancelled
			return
		}

		set fn [fpnorm $fn]

		if { [file_class $fn] == "" } {
			alert "Illegal file name or extension"
			continue
		}

		set em [file_location $fn]

		if { $em != "T" } {
			if { [file_perm $em] < 3 } {
				alert "You are not allowed to create files in\
				this directory in this way!\
				Use the buttons in the Search window\
				to create directories and files outside the\
				project"
				continue
			}
		}

		if [file exists $fn] {
			alert "This file already exists"
			continue
		}

		if ![valfname $fn "f"] {
			continue
		}

		break
	}

	catch { exec touch $fn }
	gfl_tree
	edit_file $fn
}

proc copy_from { { x "" } { y "" } } {
#
# Copies an external file (or a bunch of files) to a project's directory
#
	global P

	if { $P(AC) == "" } {
		return
	}

	# the target directory
	set dir [lindex [tree_sel_params] 0]

	if ![info exists P(LCF)] {
		global DefProjDir
		set P(LCF) $DefProjDir
	}

	while 1 {

		reset_all_menus 1
		set fl [tk_getOpenFile \
			-initialdir $P(LCF) \
			-multiple 1 \
			-title "Select file(s) to copy:"]
		reset_all_menus

		if { $fl == "" } {
			# cancelled
			return
		}

		# in the future start from here
		set P(LCF) [file dirname [lindex $fl 0]]

		# verify the extensions
		set ef ""
		foreach f $fl {
			if { [file_class [file tail $f]] == "" } {
				lappend ef $f
			}
		}

		if { $ef == "" } {
			break
		}

		if { [llength $ef] > 1 } {
			set msg "These files: "
			append msg [join $ef ", "]
			append msg " have names/extensions that do not"
		} else {
			set msg "This file: [lindex $ef 0]"
			append msg " has name/extension that does not"
		}
		append msg " fit the project. Nothing copied. Select again"
		alert $msg
	}

	set ef ""
	foreach f $fl {
		set t [file tail $f]
		set u [file join $dir $t]
		if [file exists $u] {
			if ![confirm "File $t already exists in the target\
			    directory. Overwrite?"] {
				continue
			}
		}
		log "Copy file: $f -> $u"
		if [catch { file copy -force -- $f $u } err] {
			lappend ef $f
		}
	}

	if { $ef != "" } {
		if { [llength $ef] > 1 } {
			set msg "These files: "
			append msg [join $ef ", "]
		} else {
			set msg "This file: [lindex $ef 0]"
		}
		append msg " could not be copied. Sorry"
		alert $msg
	}


	gfl_tree
}

proc copy_to { { x "" } { y "" } } {
#
# Copies current file anywhere
#
	global P

	if { $P(AC) == "" } {
		return
	}

	set sel [tree_selection $x $y]

	set nf [llength $sel]

	if { $nf == 0 } {
		alert "Nothing selected to copy"
		return
	}

	if { $nf == 1 && [lindex [lindex $sel 0] 1] == "f" } {
		# single simple file, a special case
		set fp [fpnorm [lindex [lindex $sel 0] 0]]
		set dir [file dirname $fp]
		set ext [file extension $fp]
		while 1 {

			reset_all_menus 1
			set fl [tk_getSaveFile \
				-defaultextension $ext \
				-initialdir $dir \
				-title "Where to copy the file:"]
			reset_all_menus

			if { $fl == "" } {
				# cancelled
				return
			}
			set fl [fpnorm $fl]
			set dir [file dirname $fl]
			set ext [file extension $fl]

			if { $fl == $fp } {
				alert "This is the same file"
				continue
			}

			if { [file_location $fl] != "X" &&
			    ![valfname $fl "f"] } {
				continue
			}

			log "Copying $fp to $fl"
			if ![catch { file copy -force -- $fp $fl } err] {
				gfl_tree
				return
			}
			alert "Cannot copy: $err"
		}
	}

	# multiple things, possibly a mix, copy them all to a directory

	if ![info exists P(LCT)] {
		global DefProjDir
		set P(LCT) $DefProjDir
	}

	while 1 {

		reset_all_menus 1
		set fl [tk_chooseDirectory -initialdir $P(LCT) \
			-mustexist 0 \
			-title "Select target directory:"]
		reset_all_menus

		if { $fl == "" } {
			return
		}

		set fl [fpnorm $fl]

		if { [file_location $fl] != "X" && ![valfname $fl "d"] } {
			continue
		}

		if ![file isdirectory $fl] {
			# try to create
			log "Creating dir $fl"
			if [catch { file mkdir $fl } err] {
				alert "Cannot create directory $fl: $err"
				continue
			}
		}

		break
	}
	set P(LCT) $fl

	# copy them one-by-one

	set ers ""
	foreach f $sel {
		set fn [fpnorm [lindex $f 0]]
		log "Copying $fn to $fl"
		if [catch { file copy -force -- $fn $fl } err] {
			lappend ers "$fn: $err"
		}
	}

	if { $ers != "" } {
		alert "Couldn't copy: [join $ers ,]"
	}

	# in case the target is inside the project
	gfl_tree
}

proc get_items_dir { x y } {
#
# Obtain the path to the selected/pointed to item
#
	global P

	if { $P(AC) == "" } {
		return ""
	}

	set sel [tree_selection $x $y]

	set nf [llength $sel]

	if { $nf == 0 } {
		alert "You have to select a file or directory for this"
		return ""
	}

	if { $nf != 1 } {
		alert "Need a single selection for this"
		return ""
	}

	set fp [lindex $sel 0]
	set ft [lindex $fp 1]
	if { $ft == "f" } {
		set fp [file dirname [fpnorm [lindex $fp 0]]]
	} elseif { $ft == "d" || $ft == "b" } {
		set fp [fpnorm [lindex $fp 0]]
	} else {
		alert "What you have selected is neither a file nor a directory"
		return ""
	}

	return $fp
}

proc run_xterm_here { { x "" } { y "" } } {
#
# Runs xterm for the directory of current item
#
	set fp [get_items_dir $x $y]
	if { $fp == "" } {
		# failed
		return
	}

	set cd [pwd]
	if [catch { cd $fp } err] {
		catch { cd $cd }
		alert "Cannot cd to directory $fp: $err"
		return
	}

	run_xterm

	catch { cd $cd }
}

proc run_explorer_here { { x "" } { y "" } } {
#
# Runs window explorer in the directory of the current item
#
	set fp [get_items_dir $x $y]
	if { $fp == "" } {
		# failed
		return
	}

	run_explorer $fp
}

proc rename_file { { x "" } { y "" } } {
#
# Renames a file or directory
#
	global P

	if { $P(AC) == "" } {
		return
	}

	set sel [tree_selection]

	if { $sel == "" } {
		alert "Nothing selected to rename"
		return
	}

	if { [llength $sel] > 1 } {
		alert "You can only rename one thing at a time"
		return
	}

	set f [lindex $sel 0]
	# type: d or f
	set t [lindex $f 1]

	set fn [lindex $f 0]
	set ta [file tail $fn]

	mk_rename_window $ta

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}

		if { $ev > 0 } {
			# proceed with rename
			set nm $P(M0,NW)
			if { $nm == $ta } {
				alert "This will do nothing"
				continue
			}
			if { $nm == "" } {
				alert "The new name cannot be empty"
				continue
			}

			if ![valfname $nm $t] {
				continue
			}

			if { $t == "d" || $t == "b" } {
				# directory or board
				if { [file dirname [fpnorm $ta]] != \
				    [file dirname [fpnorm $nm]] } {
					alert "A board directory cannot be\
						moved outside its location"
					continue
				}
				if [reserved_dname $nm] {
					bad_dirname
					continue
				}
			} else {
				if { [file_class $nm] == "" } {
					alert "The new file name is illegal or\
						has an illegal extension"
					continue
				}
			}
			# do it
			set tf [file join [file dirname $fn] $nm]

			if [file exists $tf] {
				if ![confirm "File $tf already exists. Do you\
				    want me to try to overwrite?"] {
					continue
				}
			}

			log "Rename file: $fn -> $tf"
			if [catch { file rename -force -- $fn $tf } err] {
				# failed
				alert "Couldn't rename: $err"
				continue
			}
			break
		}
	}

	md_stop
	gfl_tree
}
					
proc mk_rename_window { old } {
#
# Opens a dialog to rename a file or directory
#
	global P FFont

	set w [md_window "Rename"]

	frame $w.tf
	pack $w.tf -side top -expand y -fill x

	label $w.tf.l -text "$old ---> "
	pack $w.tf.l -side left -expand n -fill x

	set P(M0,NW) $old
	entry $w.tf.e -width 16 -font $FFont -textvariable P(M0,NW)
	pack $w.tf.e -side left -expand y -fill x

	frame $w.bf
	pack $w.bf -side top -expand y -fill x

	button $w.bf.b -text "Done" -command "md_click 1"
	pack $w.bf.b -side right -expand n -fill x

	button $w.bf.c -text "Cancel" -command "md_click -1"
	pack $w.bf.c -side left -expand n -fill x

	bind $w <Destroy> "md_click -1"
}

###############################################################################

proc val_prj_dir { dir { na 0 } } {
#
# Validate the formal location of a project directory
#
	global PicOSPath

	set apps [fpnorm [file join $PicOSPath Apps]]

	if ![valfname $dir "d"] {
		return 0
	}

	# removed the restriction on the project directory to be a subdirectory
	# of Apps

	return 1
}

proc prj_name { dir } {

	global PicOSPath

	return [file tail $dir]
}

proc val_prj_incomp { } {
#
# Just an alert
#
	alert "Inconsistent contents of the project's directory: app.cc cannot\
		coexist with app_... files"
}

proc val_prj_exists { dir { try 0 } } {
#
# Check if the directory contains an existing project that appears to be making
# sense; try != 0 -> don't start it - just check, try < 2 -> issue alerts
#
	global P ST

	if ![file isdirectory $dir] {
		if { $try <= 1 } {
			alert "The project directory $dir does not exist"
		}
		return 0
	}

	# if there's a config.prj file, assume the project is OK regardless
	# of the content
	set pcf [file isfile [file join $dir "config.prj"]]

	if $pcf {
		log "config.prj exists"
	}

	set fl [glob -nocomplain -directory $dir -tails *]
	if { $fl == "" } {
		# this will not happen
		if { $try <= 1 } {
			alert "The project directory $dir is empty"
		}
		return 0
	}

	set pl ""
	set es 0

	foreach fn $fl {
		if { $fn == "app.cc" } {
			if { $pl != "" } {
				if { $try <= 1 } {
					val_prj_incomp
				}
				# return code == not a project, but
				# nonempty
				return -1
			}
			set es 1
			continue
		}

		# ignore such inconsistencies if the config file is present
		if { $fn == "app.c" && !$pcf } {
			if { $try <= 1 } {
				alert "This looks like a legacy praxis:\
					file app.c is incompatible with\
					PIP projects, please convert\
					manually and try again"
			}
			return -1
		}

		if [regexp "^app_(\[a-zA-Z0-9\]+)\\.cc$" $fn jnk pn] {
			if { $es && !$pcf } {
				if { $try <= 1 } {
					val_prj_incomp
				}
				return -1
			}
			lappend pl $pn
		}
	}
	
	if { !$pcf && !$es && $pl == "" } {
		if { $try <= 1 } {
			alert "There is nothing resembling a PicOS\
				project in directory $dir"
		}
		return -1
	}

	# project OK

	if $try {
		# do no more
		return 1
	}

	# time to close the previous project and assume the new one

	if [catch { cd $dir } err] {
		alert "Cannot move to the project's directory $dir: $err"
		return 0
	}

	array unset P "FL,d,*"
	array unset P "FL,c,*"
	array unset P "FL,T,*"

	set P(PL) [lsort $pl]

	wm title . "PIP $ST(VER), project [prj_name $dir]"

	setup_project $dir

	gfl_tree

	return 1
}

proc clone_project { } {
#
# Clone a project directory to another directory
#
	global DefProjDir

	if [close_project] {
		# cancelled 
		return
	}

	while 1 {

		# select source directory
		reset_all_menus 1
		set sdir [tk_chooseDirectory -initialdir $DefProjDir \
			-mustexist 1 \
			-title "Select the source directory:"]
		reset_all_menus

		if { $sdir == "" } {
			# cancelled
			return
		}

		# check if this is a proper subdirectory of DefProjDir
		set sdir [fpnorm $sdir]

		if ![val_prj_dir $sdir] {
			continue
		}

		if { [val_prj_exists $sdir 1] > 0 } {
			break
		}
	}

	while 1 {

		# select target directory
		reset_all_menus 1
		set dir [tk_chooseDirectory -initialdir $DefProjDir \
			-mustexist 0 \
			-title "Select the target directory:"]
		reset_all_menus

		if { $dir == "" } {
			# cancelled
			return
		}

		# check if this is a proper subdirectory of DefProjDir
		set dir [fpnorm $dir]

		if ![val_prj_dir $dir] {
			continue
		}

		set v [val_prj_exists $dir 2]

		if { $v != 0 } {
			if { $v > 0 } {
				set tm "Directory $dir contains something that\
					looks like an existing project!"
			} else {
				set tm "Directory $dir is not empty!"
			}
			append tm " Do you want to erase its contents?\
				THIS CANNOT BE UNDONE!!!"
			if ![confirm $tm] {
				continue
			}
		}

		if [file exists $dir] {
			# we remove the directory and create it from scratch
			if [catch { file delete -force -- $dir } err] {
				alert "Cannot erase $dir: $err"
				continue
			}
		}

		# copy source to target
		log "Copy project: $sdir $dir"
		if [catch { file copy -force -- $sdir $dir } err] {
			alert "Cannot copy $sdir to $dir: $err"
			continue
		}

		break
	}

	# now open as a ready project
	open_project -1 $dir
}

proc close_project { } {

	global P

	if [edit_unsaved] {
		# no
		return 1
	}

	edit_kill

	# in case this one is open
	close_search_window

	if { $P(AC) != "" } {
		# a project is open
		set P(AC) ""
		set P(CO) ""
		gfl_erase
		reset_config_menu
		reset_bnx_menus
		reset_file_menu
		# in case something is running
		abort_term
		stop_udaemon
		stop_oss
		# QUESTION: do we want to auto kill bpcs (loader, piter) as
		# well? Perhaps not.
	}

	return 0
}

proc open_project { { which -1 } { dir "" } } {

	global DefProjDir PicOSPath LProjects

	log "open_project: $which, $dir"

	if [close_project] {
		# no
		return
	}

	if { $which < 0 } {

		# open file

		if { $dir != "" } {

			# use the specified directory
			set dir [fpnorm $dir]
			if { [val_prj_exists $dir] <= 0 } {
				return
			}

		} else {
	
			while 1 {

				reset_all_menus 1
				set dir [tk_chooseDirectory \
						-initialdir $DefProjDir \
						-mustexist 1 \
						-parent . \
						-title "Project directory"]
				reset_all_menus

				if { $dir == "" } {
					# cancelled
					return
				}

				set dir [fpnorm $dir]

				if ![val_prj_dir $dir] {
					# formally illegal
					continue
				}

				if { [val_prj_exists $dir] > 0 } {
					break
				}
			}
		}

	} else {

		while 1 {

			set dir [lindex $LProjects $which]
			if { $dir == "" } {
				alert "No such project"
				break
			}

			set dir [fpnorm $dir]

			if ![val_prj_dir $dir] {
				set dir ""
				break
			}

			if { [val_prj_exists $dir] <= 0 } {
				set dir ""
			}
			break
		}

		if { $dir == "" } {
			# cannot open, remove the entry from LProjects
			set LProjects [lreplace $LProjects $which $which]
			set_rcoption LProjects
			catch { reset_file_menu }
			return
		}
	}

	# add to LProjects

	set lp ""
	lappend lp $dir
	set nc 1
	foreach p $LProjects {
		if { $nc >= 6 } {
			# no more than 6
			break
		}
		if { $p == "" } {
			continue
		}
		set p [fpnorm $p]
		if { $p == $dir } {
			continue
		}
		lappend lp $p
		incr nc
	}
	set LProjects $lp
	set_rcoption LProjects
	catch { reset_file_menu }
	reset_bnx_menus
	reset_file_menu
	sys_make_ctags
	vue_make_ctags
}

proc new_project { } {
#
# Initializes a directory for a new project
#
	global P DefProjDir

	if [close_project] {
		# cancelled 
		return
	}

	while 1 {

		# select the directory
		reset_all_menus 1
		set dir [tk_chooseDirectory -initialdir $DefProjDir \
			-mustexist 0 \
			-title "Select directory for the project:"]
		reset_all_menus

		if { $dir == "" } {
			# cancelled
			return
		}

		# check if this is a proper subdirectory of DefProjDir
		set dir [fpnorm $dir]

		if ![val_prj_dir $dir] {
			continue
		}

		set v [val_prj_exists $dir 2]

		if { $v > 0 } {
			# this is an existing project
			if [confirm "Directory $dir contains something that\
				looks like an existing project. Would you like\
				to open that project?"] {

				open_project -1 $dir
				return
			}

			# keep trying
			continue
		}

		if { $v < 0 } {
			if ![confirm "Directory $dir is not empty! Do you want\
				to erase its contents? THIS CANNOT BE\
				UNDONE!!!"] {

				continue
			}

			# we remove the directory and create it from scratch
			log "Erase directory: $dir"
			if {
			   [catch { file delete -force -- $dir } err] ||
			   [catch { file mkdir $dir } err] } {
				alert "Remove failed: $err"
				continue
			}
		}

		if ![file exists $dir] {
			if [catch { file mkdir $dir } err] {
				alert "Cannot create directory $dir: $err"
				continue
			}
		} elseif ![file isdirectory $dir] {
			alert "File $dir exists, but is not a directory"
			continue
		}

		break
	}

	# we have agreed on the directory, now select the praxis type, i.e.,
	# single-program/multiple program; options:
	#
	# single:   single program, create app.cc + options.sys
	# multiple: specify suffixes, create multiple app_xxx.cc and
	#           options_xxx.sys files

	mk_project_selection_window

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}
		if { $ev == 1 } {
			# single program
			set md ""
			break
		}
		if { $ev == 2 } {
			# multiple programs, validate the tags
			set md ""
			set er ""
			set ec 0
			for { set i 0 } { $i < 8 } { incr i } {
				set t $P(M0,E$i)
				if { $t == "" } {
					continue
				}
				if ![regexp -nocase "^\[a-z0-9\]+$" $t] {
					append er ", illegal tag $t"
					incr ec
					continue
				}
				if { [lsearch -exact $md $t] >= 0 } {
					append er ", duplicate tag $t"
					incr ec
					continue
				}
				lappend md $t
			}
			if { $md == "" && $ec == 0 } {
				set ec 1
				set er ", no tags specified"
			}
			if $ec {
				if { $ec > 1 } {
					set ec "Errors:"
				} else {
					set ec "Error:"
				}
				alert "$ec [string range $er 2 end]"
				continue
			}
			# OK
			break
		}
	}

	md_stop

	# done: create placeholder files

	set flist [list "options.sys"]

	if { $md != "" } {
		foreach m $md {
			lappend flist "app_${m}.cc"
			lappend flist "options_${m}.sys"
		}
	} else {
		lappend flist "app.cc"
	}

	foreach m $flist {

		if [regexp "cc$" $m] {
			set fc "#include \"sysio.h\"\n\n"
			append fc "// This is $m\n\n"
			append fc "fsm root {\n\tstate INIT:\n\n}"
		} else {
			set fc "// This is $m (initially empty)"
		}

		set tf [file join $dir $m]
		log "Creating file: $tf"
		if [catch { open $tf "w" } fd] {
			alert "Cannot open $tf for writing"
			return
		}
		if [catch { puts $fd $fc } md] {
			alert "Cannot write to $tf: $md"
			catch { close $fd }
			return
		}
		catch { close $fd }
	}

	# now open as a ready project
	open_project -1 $dir
}

proc mk_project_selection_window { } {
#
# Opens a dialog to select the project type
#
	global P FFont

	set w [md_window "Project type"]

	frame $w.lf
	pack $w.lf -side left -expand y -fill y

	button $w.lf.b -text "Single program" \
		-command "md_click 1"
	pack $w.lf.b -side top -expand n -fill x

	button $w.lf.c -text "Cancel" \
		-command "md_click -1"
	pack $w.lf.c -side bottom -expand n -fill x

	set f $w.rf

	frame $f
	pack $f -side right -expand y -fill x

	button $f.b -text "Multiple programs" \
		-command "md_click 2"
	pack $f.b -side top -expand y -fill x

	for { set i 0 } { $i < 8 } { incr i } {
		# the tags
		set tf $f.f$i
		frame $tf
		pack $tf -side top -expand y -fill x
		label $tf.l -text "Label $i: "
		pack $tf.l -side left -expand n
		set P(M0,E$i) ""
		entry $tf.e -width 8 -font $FFont -textvariable P(M0,E$i)
		pack $tf.e -side left -expand y -fill x
	}

	bind $w <Destroy> "md_click -1"
}

proc get_config { } {
#
# Reads the project configuration from config.prj
#
	global CFItems P TermLines Archs

	# start from the dictionary of defaults
	set P(CO) $CFItems

	if [catch { open "config.prj" "r" } fd] {
		return
	}

	if [catch { read $fd } pf] {
		catch { close $fd }
		alert "Cannot read config.prj: $pf"
		return
	}

	catch { close $fd }

	set D [dict create]

	foreach { k v } $pf {
		if { $k == "" || ![dict exists $P(CO) $k] } {
			# just ignore
			continue
		}
		dict set D $k $v
	}

	set P(CO) [dict merge $P(CO) $D]

	# validate the arch setting
	set a [dict get $P(CO) "ARCH"]
	if { $a == "" || [lsearch -exact $Archs $a] < 0 } {
		log "Arch not set or illegal, reset to default"
		set a [lindex $Archs 0]
		dict set P(CO) "ARCH" $a
		set_config
	}

	# any items to be updated on architecture switch
	update_arch $a

	# this one is optimized a bit for faster access
	set TermLines [dict get $P(CO) "OPTERMLINES"]
}

proc set_config { } {
#
# Saves the project configuration
#
	global P

	if [catch { open "config.prj" "w" } fd] {
		alert "Cannot open config.prj for writing: $fd"
		return
	}

	if [catch { puts -nonewline $fd $P(CO) } er] {
		catch { close $fd }
		alert "Cannot write to config.prj: $er"
		return
	}

	catch { close $fd }
}

proc setup_project { dir } {
#
# Set up the project's parameters and build the dynamic menus; assumes we are
# in the project's directory
#
	global P

	get_config
	set P(AC) $dir
	reset_config_menu
}

###############################################################################

proc mk_menu_button { w } {
#
# Set up a raw menu button
#
	menubutton $w -text "---" -direction right -menu $w.m -relief raised
	menu $w.m -tearoff 0
}

proc set_menu_button { w tx ltx { cmd "" } } {
#
# Set up an existing menu button
#
	if { $tx == "" || [lsearch -exact $ltx $tx] < 0 } {
		set tx "---"
	}

	$w.m delete 0 end

	$w configure -text $tx

	set n 0
	foreach t $ltx {
		$w.m add command -label $t \
			-command "click_menu_button $w $n $cmd"
		incr n
	}
}

proc set_board_menu_button { w sel blist } {
#
# We need a special function for board selection because of the cascades
#
	if { $sel == "" || [lsearch -exact $blist $sel] < 0 } {
		set sel "---"
	}

	$w.m delete 0 end

	$w configure -text $sel

	set mbn "---"
	$w.m add command -label $mbn \
		-command "board_selection_click $w $mbn"

	# cascade submenu counter for naming
	set sc 0

	while { $blist != "" } {

		# the head entry
		set mbn [lindex $blist 0]
		# any prefix?
		if ![regexp "^(\[^_\]+)_" $mbn jnk pfx] {
			set pfx $mbn
		}

		set blist [lrange $blist 1 end]
		set pleng [string length $pfx]

		# check if the same prefix is used in subsequent board names;
		# note that the list is sorted

		set n 0
		while 1 {
			set sbn [lindex $blist $n]
			if { $sbn == "" } {
				# end of list
				break
			}
			set d [string index $sbn $pleng]
			if { $d != "_" && $d != "" || 
			    [string first $pfx $sbn] != 0 } {
				# no match
				break
			}
			# OK
			incr n
		}

		if $n {
			# need a submenu
			set sm $w.m.sm$sc
			incr sc
			# may exist already
			if [catch { menu $sm -tearoff 0 } ] {
				$sm delete 0 end
			}
			$sm add command -label $mbn \
				-command "board_selection_click $w $mbn"
			set m 0
			while { $m < $n } {
				set sbn [lindex $blist $m]
				$sm add command -label $sbn \
				    -command "board_selection_click $w $sbn"
				incr m
			}
			$w.m add cascade -label "${pfx}..." -menu $sm
			set blist [lrange $blist $n end]
		} else {
			# a straightforward entry
			$w.m add command -label $mbn \
				-command "board_selection_click $w $mbn"
		}
	}
}

proc read_menu_button { w } {
#
# Get the current selection from the menu button, i.e., the button's current
# text
#
	set sel [$w cget -text]
	if { $sel == "---" } {
		return ""
	}
	return $sel
}

proc click_menu_button { w n { cmd "" } } {
#
# Menu button selection click
#
	set t [$w.m entrycget $n -label]
	$w configure -text $t
	if { $cmd != "" } {
		if { $t == "---" } {
			set t ""
		}
		$cmd $w $t
	}
}

proc library_present { board } {
#
# Checks if there's a library present for the board
#
	return [file isfile [file join [boards_dir] $board "libpicos.a"]]
}

proc boards_dir { { arch "" } } {
#
# Returns the path to the BOARDS directory for the given arch, or for the
# project's arch, if null
#
	global PicOSPath P

	if { $arch == "" } {
		set arch [dict get $P(CO) "ARCH"]
	}

	return [fpnorm [file join $PicOSPath PicOS $arch BOARDS]]
}

proc board_list { cpu } {

	set dn [boards_dir $cpu]

	set fl [glob -nocomplain -tails -directory $dn *]

	set r ""
	foreach f $fl {
		set bdf [file join $dn $f]
		if ![file isdirectory $bdf] {
			# ignore any non-directory files
			continue
		}
		lappend r $f
	}
	return [lsort $r]
}

proc board_opts { bo } {
#
# Returns the file name of the board options file
#
	set fn [file join [boards_dir] $bo "board_options.sys"]

	if [file isfile $fn] {
		return $fn
	}

	return ""
}

proc do_board_selection { } {
#
# Execute ARCH and board selection from Configuration menu
#
	global P CFBoardItems

	if { $P(AC) == "" } {
		return
	}

	params_to_dialog $CFBoardItems

	set w ""

	while 1 {

		# have to redo this in the loop as the layout of the window
		# may change

		if { $w != "" } {
			catch { destroy $w }
		}

		# get the library modes
		if $P(M0,MB) {
			for { set n 0 } { $n < [llength $P(PL)] } { incr n } {
				set P(M0,LM,$n) [blindex $P(M0,LM) $n]
				set P(M0,LD,$n) [blindex $P(M0,LD) $n]
			}
		} else {
			set lm [lindex $P(M0,LM) 0]
			if { $lm == "" || ($lm != 0 && $lm != 1) } {
				set lm 0
			}
			set P(M0,LM,0) $lm
			set P(M0,LD,0) 1
		}
		
		set w [mk_board_selection_window]

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			reset_build_menu
			gfl_tree
			return
		}
		if { $ev == 1 } {
			# accepted; copy the options
			set P(M0,LM) ""
			set P(M0,LD) ""
			if $P(M0,MB) {
				for { set n 0 } { $n < [llength $P(PL)] } \
				    { incr n } {
					lappend P(M0,LM) $P(M0,LM,$n)
					lappend P(M0,LD) $P(M0,LD,$n)
				}
			} else {
				lappend P(M0,LM) $P(M0,LM,0)
			}
			set mc [dialog_to_params $CFBoardItems]
			md_stop
			if $mc {
				# there was a change
				set_config
				reset_config_menu
				reset_build_menu
				gfl_tree
				force_full_clean
			}
			return
		}
	}
}

proc force_full_clean { } {

	term_dspline "--RECONFIGURATION, FULL CLEAN FORCED--"
	do_cleanup
}

proc sxml_ctxt { it ky } {
	return [string trim [sxml_txt [sxml_child $it $ky]]]
}

proc read_compiler_data { arch } {

	global P PicOSPath

	set cfn [dict get $P(CO) "CCF"]

	if { $cfn == "" } {
		set cfn "compile.xml"
	}

	if { [file extension $cfn] == "" } {
		# supply the default extension
		append cfn ".xml"
	}

	# try to open "as is"
	if [catch { open $cfn "r" } fd] {

		# failed
		if { [file pathtype $cfn] != "relative" } {
			# no way
			error "Cannot open compiler data file $cfn, $fd"
		}

		# try the package version
		set cfn [file join $PicOSPath PicOS $arch "compile.xml"]
		if [catch { open $cfn "r" } fd] {
			error "Cannot open compiler data file $cfn, $fd"
		}
	}

	if [catch { read $fd } cd] {
		error "Cannot read compiler data file $cfn, $cd"
	}

	catch { close $fd }

	return $cd
}

proc update_arch { arch } {
#
# Function to fetch any information specific to the architecture that has just
# changed
#
	global P ARCHINFO

	set ARCHINFO(LOADERS) ""
	set ARCHINFO(DBINITS) ""
	set ARCHINFO(VUEE,DEFS) ""
	set ARCHINFO(LOADERS,LSCAN) 999

	if [catch { read_compiler_data $arch } cdata] {
		log $cdata
		return
	}

	if [catch { sxml_parse cdata } cdata] {
		log "Bad format of compile.xml for $arch, $cdata"
		return
	}

	set cdata [sxml_child $cdata "compile"]
	if { $cdata == "" } {
		log "No <compile> tag in compile.xml for $arch"
		return
	}

	# get to the loaders section
	set lsec [sxml_child $cdata "loaders"]
	foreach ld [sxml_children $lsec "loader"] {
		set cp [sxml_ctxt $ld "path"]
		if { $cp == "" } {
			# ignore if path absent
			continue
		}
		set ca [sxml_ctxt $ld "arguments"]
		lappend ARCHINFO(LOADERS) [list "c" $cp $ca "" "" ""]
	}
	foreach ld [sxml_children $lsec "debugger"] {
		# gdb path (or maybe some other debugger)
		set cp [sxml_ctxt $ld "path"]
		set ca [sxml_ctxt $ld "arguments"]
		set pp [sxml_ctxt $ld "proxypath"]
		if { $cp == "" || $pp == "" } {
			# both must be present for this to work
			continue
		}
		set pa [sxml_ctxt $ld "proxyarguments"]
		set po [sxml_ctxt $ld "port"]
		if [catch { valnum $po 1 65535 } po] {
			set po ""
		}
		lappend ARCHINFO(LOADERS) [list "d" $pp $pa $po $cp $ca]
		# init file
		set po [sxml_child $ld "init"]
		if { $po != "" } {
			# we have an init file, map it to the debugger path
			set pp [sxml_attr $po "file"]
			if { $pp == "" } {
				# the default?
				set pp ".gdbinit"
			}
			set cn ""
			foreach ln [split [sxml_txt $po] "\n"] {
				set ln [string trim $ln]
				if { $ln != "" } {
					append cn "${ln}\n"
				}
			}
			lappend ARCHINFO(DBINITS) [list $cp $pp $cn]
		}
	}
			
	# VUEE defs (needed to properly size the basic types)
	set el [sxml_child $cdata "vuee"]

	foreach gi [sxml_children $el "define"] {
		set cp [string trim [sxml_txt $gi]]
		if { $cp != "" } {
			lappend ARCHINFO(VUEE,DEFS) $cp
		}
	}

	# if no loaders are defined, set defaults
	set all [dict get $P(CO) "${arch}_LDLIST"]
	if { $all == "" || [llength $all] == 1 && [lindex $all 0 1] == "" } {
		# the list is empty
		set all $ARCHINFO(LOADERS)
		dict set P(CO) "${arch}_LDLIST" $all
		set_config
	}
}

proc arch_selection_click { w t } {
#
# A different ARCH has been selected
#
	global P

	if { $t != $P(M0,ARCH) } {
		# an actual change, redefine the board lists
		set P(M0,BO) ""
		set boards [board_list $t]
		foreach m $P(M0,BL) {
			set_board_menu_button $m "" $boards
		}
		set P(M0,ARCH) $t
		update_arch $t
	}
}

proc board_selection_click { w t } {
#
# A board has been selected
#
	global P

	$w configure -text $t
	# the board number
	set nb 0
	regexp "\[0-9\]+$" $w nb
	if { [string index $t 0] == "-" } {
		set t ""
	}
	set P(M0,BO) [mreplace $P(M0,BO) $nb $t]
}

proc ccf_selection_click { } {

	global P

	set P(M0,CCF) [string trim $P(M0,CCF)]

	reset_all_menus 1
	set fi [tk_getOpenFile -initialdir "." -parent $P(M0,WI)]
	reset_all_menus

	if { $fi != "" } {
		set P(M0,CCF) $fi
	}
}

proc mk_board_selection_window { } {
#
# Open the board selection window
#
	global P Archs ST FFont

	set w [md_window "Board selection"]

	set f "$w.main"

	frame $f
	pack $f -side top -expand n -fill x

	# column number for the grid
	set cn 0
	set rn 0
	set rm [expr { $rn + 1 }]
	set ro [expr { $rm + 1 }]

	### Arch selection ####################################################

	label $f.cpl -text "Arch"
	grid $f.cpl -row $cn -column $rn -sticky nw -padx 1 -pady 1

	mk_menu_button $f.cpb
	$f.cpb configure -anchor w
	set_menu_button $f.cpb $P(M0,ARCH) $Archs arch_selection_click
	grid $f.cpb -row $cn -column $rm -sticky we -padx 1 -pady 1

	set rs [expr { $rm + 1 }]
	if $P(M0,MB) {
		# multiple boards
		set ro [expr { $rs + 1 }]
		label $f.lse -text "Disabled:"
		grid $f.lse -row $cn -column $rs -sticky nw -padx 1 -pady 1
	} else {
		set ro $rs
	}

	label $f.lml -text "Lib mode:"
	grid $f.lml -row $cn -column $ro -sticky nw -padx 1 -pady 1

	### Multiple boards/single board ######################################

	if { $P(PL) != "" } {
		# we have a multi-program case, so the "Multiple" checkbox
		# is needed
		incr cn
		label $f.mbl -text "Multiple"
		grid $f.mbl -row $cn -column $rn -sticky nw -padx 1 -pady 1
		checkbutton $f.mbc -variable P(M0,MB) -command "md_click 2"
		grid $f.mbc -row $cn -column $rm -sticky nw -padx 1 -pady 1
	}

	# the list of available boards
	set boards [board_list $P(M0,ARCH)]
	# the list of menus with board lists to update after an ARCH change
	set P(M0,BL) ""

	if $P(M0,MB) {
		# multiple
		set nb 0
		set tb ""
		foreach suf $P(PL) {
			set bn [lindex $P(M0,BO) $nb]
			incr cn
			label $f.bl$nb -text "Board for ${suf}:"
			grid $f.bl$nb -row $cn -column $rn -sticky nw \
				-padx 1 -pady 1
			set mb $f.bm$nb
			lappend P(M0,BL) $mb
			mk_menu_button $mb
			$mb configure -anchor w
			set_board_menu_button $mb $bn $boards
			grid $f.bm$nb -row $cn -column $rm -sticky we \
				-padx 1 -pady 1

			checkbutton $f.la$nb -variable P(M0,LD,$nb) \
				-state normal
			grid $f.la$nb -row $cn -column $rs -sticky nw \
				-padx 1 -pady 1

			if $ST(LO) {
				# make sure the library option is set and it is
				# frozen
				set st "disabled"
				set P(M0,LM,$nb) 1
			} else {
				set st "normal"
			}
			checkbutton $f.lm$nb -variable P(M0,LM,$nb) -state $st
			grid $f.lm$nb -row $cn -column $ro -sticky nw \
				-padx 1 -pady 1

			incr nb
			lappend tb $bn
		}
		set P(M0,BO) $tb
	} else {
		# single board
		incr cn
		set bn [lindex $P(M0,BO) 0]
		label $f.bl0 -text "Board"
		grid $f.bl0 -row $cn -column $rn -sticky nw -padx 1 -pady 1
		set mb $f.bm0
		lappend P(M0,BL) $mb
		mk_menu_button $mb
		set_board_menu_button $mb $bn $boards
		grid $f.bm0 -row $cn -column $rm -sticky nw -padx 1 -pady 1
		if $ST(LO) {
			# make sure the library option is set and it is frozen
			set st "disabled"
			set P(M0,LM,0) 1
		} else {
			set st "normal"
		}
		checkbutton $f.lm0 -variable P(M0,LM,0) -state $st
		grid $f.lm0 -row $cn -column $ro -sticky nw -padx 1 -pady 1
	}

	set f "$w.low"
	frame $f
	pack $f -side top -expand y -fill x

	label $f.cl -text "Toolchain configuration file: "
	pack $f.cl -side left -expand n

	button $f.cb -text "Select" -width 7 -command "ccf_selection_click"
	pack $f.cb -side right -expand n

	entry $f.ce -font $FFont -textvariable P(M0,CCF)
	pack $f.ce -side right -expand y -fill x

	set f "$w.but"
	frame $f
	pack $f -side top -expand y -fill x

	# the done button
	button $f.don -text "Done" -width 7 -command "md_click 1"
	pack $f.don -side right -expand n

	button $f.can -text "Cancel" -width 7 -command "md_click -1"
	pack $f.can -side left -expand n

	bind $w <Destroy> "md_click -1"

	return $w
}

proc terminate { { f "" } } {

	global DEAF

	if $DEAF { return }

	set DEAF 1

	if { $f == "" && [edit_unsaved] } {
		return
	}

	edit_kill
	abort_term
	stop_piter
	stop_sa
	stop_genimage
	stop_udaemon
	stop_oss
	bpcs_kill "FL"
	close_project
	exit 0
}

###############################################################################

proc params_to_dialog { nl } {
#
# Copy relevant configuration parameters to dialog variables ...
#
	global P

	foreach { k j } $nl {
		set P(M0,$k) [dict get $P(CO) $k]
	}
}

proc dialog_to_params { nl } {
#
# ... and the other way around
#
	global P

	set mod 0

	foreach { k j } $nl {
		if { [string index $k 0] != "_" } {
			# modal variables starting with _ are not parameters
			if { [dict get $P(CO) $k] != $P(M0,$k) } {
				dict set P(CO) $k $P(M0,$k)
				incr mod
			}
		}
	}

	# return modification count
	return $mod
}

###############################################################################
# Flash loaders ###############################################################
###############################################################################

proc ldr_null { { tp "c" } } {
#
# Null loader
#
	return [list $tp "" "" "" "" ""]
}

proc do_loaders_config { } {

	global P CFLoadItems

	if { $P(AC) == "" } {
		return
	}

	# make sure a loader is not active while we are doing this
	if [stop_loader 1] {
		# the user says "NO"
		return
	}

	set arch [dict get $P(CO) "ARCH"]

	if ![info exists CFLoadItems($arch)] {
		# something wrong, this will never happen
		alert "Internal error, illegal architecture $arch"
		return
	}

	while 1 {

		params_to_dialog $CFLoadItems($arch)

		mk_loaders_conf_window $arch

		while 1 {

			set ev [md_wait]

			if { $ev < 0 } {
				# cancelled
				return
			}

			if { $ev == 1 } {
				# accepted
				set more [update_loaders $arch]
				dialog_to_params $CFLoadItems($arch)
				md_stop
				set_config
				if $more {
					# redisplay the window
					break
				}
				return
			}
		}
	}
}

proc mk_loaders_conf_window { arch } {

	global P

	set w [md_window "Loader configuration"]

	set als "${arch}_LDSEL"
	set all "${arch}_LDLIST"
	if { $P(M0,$als) == "" } {
		# the default: the first entry
		set P(M0,$als) 0
	}

	if { $P(M0,$all) == "" } {
		set P(M0,$all) [list [ldr_null]]
	}

	set fc 0
	foreach ld $P(M0,$all) {
		# type: "c" command line, "d" debugger + proxy
		set ltp [lindex $ld 0]
		if { $ltp != "c" && $ltp != "d" } {
			# maintain sanity
			set ltp "c"
		}
		ldr_lcw_$ltp $ld $fc $w.f$fc $arch
		incr fc
	}

	## Buttons
	set f $w.fb
	frame $f
	pack $f -side top -expand y -fill x
	button $f.c -text "Cancel" -width 7 -command "md_click -1"
	pack $f.c -side left -expand n
	button $f.d -text "Done" -width 7 -command "md_click 1"
	pack $f.d -side right -expand n

	bind $w <Destroy> "md_click -1"
}

proc ldr_lcw_c { ldr cnt f arch } {
#
# Configuration widget for a command-type loader
#
	global P FFont

	labelframe $f -text "Loader $cnt" -padx 2 -pady 2
	pack $f -side top -expand y -fill x

	# the upper frame
	set fu [frame $f.uf]
	pack $fu -side top -expand y -fill x

	# selector
	radiobutton $fu.sel -text "Use" -variable P(M0,${arch}_LDSEL) \
		-value $cnt
	pack $fu.sel -side left

	# delete button
	button $fu.del -text "Delete" -width 7 -command "ldr_wid_del $cnt $arch"
	pack $fu.del -side right -expand n
	set rlf [$fu.del cget -relief]

	# the new button
	menubutton $fu.new -text "New" -width 7 -direction right \
		-menu $fu.new.m -relief $rlf
	menu $fu.new.m -tearoff 0
	$fu.new.m add command -label "Clone" \
		-command "ldr_wid_new $cnt k"
	$fu.new.m add command -label "Flasher" \
		-command "ldr_wid_new $cnt c"
	$fu.new.m add command -label "Debugger" \
		-command "ldr_wid_new $cnt d"
	pack $fu.new -side right -expand n

	# path to the program
	set fu [frame $f.pp]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Path to the program: "
	pack $fu.l -side left -expand n

	# a special modal variable requiring preprocessing on update
	set vn "_${cnt}pp"
	set P(M0,$vn) [lindex $ldr 1]

	button $fu.b -text "Select" -width 7 \
		-command "ldr_cnf_fsel $cnt $arch $vn"
	pack $fu.b -side right -expand n
	entry $fu.f -font $FFont -textvariable P(M0,$vn)
	pack $fu.f -side right -expand y -fill x

	set fu [frame $f.ag]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Arguments: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}ag"
	# a special modal variable requiring preprocessing on update
	set P(M0,$vn) [lindex $ldr 2]
	entry $fu.a -font $FFont -textvariable P(M0,$vn)
	pack $fu.a -side left -expand y -fill x
}

proc ldr_lcw_d { ldr cnt f arch } {
#
# Configuration widget for a proxy-type loader
#
	global P FFont

	labelframe $f -text "Debugger $cnt" -padx 2 -pady 2
	pack $f -side top -expand y -fill x

	# the upper frame
	set fu [frame $f.uf]
	pack $fu -side top -expand y -fill x

	# selector
	radiobutton $fu.sel -text "Use" -variable P(M0,${arch}_LDSEL) \
		-value $cnt
	pack $fu.sel -side left

	# delete button
	button $fu.del -text "Delete" -width 7 -command "ldr_wid_del $cnt $arch"
	pack $fu.del -side right -expand n
	set rlf [$fu.del cget -relief]

	# new button
	menubutton $fu.new -text "New" -width 7 -direction right \
		-menu $fu.new.m -relief $rlf
	menu $fu.new.m -tearoff 0
	$fu.new.m add command -label "Clone" \
		-command "ldr_wid_new $cnt k"
	$fu.new.m add command -label "Flasher" \
		-command "ldr_wid_new $cnt c"
	$fu.new.m add command -label "Debugger" \
		-command "ldr_wid_new $cnt d"
	pack $fu.new -side right -expand n

	# path to the program
	set fu [frame $f.pp]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Path to the proxy: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}pp"
	set P(M0,$vn) [lindex $ldr 1]

	button $fu.b -text "Select" -width 7 \
		-command "ldr_cnf_fsel $cnt $arch $vn"
	pack $fu.b -side right -expand n
	entry $fu.f -font $FFont -textvariable P(M0,$vn)
	pack $fu.f -side right -expand y -fill x

	set fu [frame $f.ag]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Arguments: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}ag"
	# a special modal variable requiring preprocessing on update
	set P(M0,$vn) [lindex $ldr 2]
	entry $fu.a -font $FFont -textvariable P(M0,$vn)
	pack $fu.a -side left -expand y -fill x

	set fu [frame $f.po]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "GDB connection port: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}po"
	set po [lindex $ldr 3]
	if [catch { valnum $po 1 65535 } po] {
		set po ""
	}
	set P(M0,$vn) $po

	entry $fu.e -font $FFont -textvariable P(M0,$vn) -width 5
	pack $fu.e -side right -expand n

	# path to the debugger
	set fu [frame $f.pd]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Path to GDB: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}pd"
	set P(M0,$vn) [lindex $ldr 4]

	button $fu.b -text "Select" -width 7 \
		-command "ldr_cnf_fsel $cnt $arch $vn"
	pack $fu.b -side right -expand n
	entry $fu.f -font $FFont -textvariable P(M0,$vn)
	pack $fu.f -side right -expand y -fill x

	set fu [frame $f.ad]
	pack $fu -side top -expand y -fill x

	label $fu.l -text "Arguments: "
	pack $fu.l -side left -expand n

	set vn "_${cnt}ad"
	set P(M0,$vn) [lindex $ldr 5]
	entry $fu.a -font $FFont -textvariable P(M0,$vn)
	pack $fu.a -side left -expand y -fill x
}

proc update_loaders { arch } {

	global P ARCHINFO

	set als "${arch}_LDSEL"
	set all "${arch}_LDLIST"
	if { $P(M0,$als) == "" } {
		# the default: the first entry
		set P(M0,$als) 0
	}

	set fc 0
	set nc 0
	set nll ""
	set more 0
	set sel $P(M0,$als)

	foreach ld $P(M0,$all) {
		if [info exists P(M0,_${fc}de)] {
			# delete this widget
			if { $sel == $fc } {
				set sel 0
			}
			incr more
			incr fc
			continue
		}
		if { $sel == $fc } {
			# in case something in front has been deleted
			set sel $nc
		}

		lassign $ld tp pp ag po pd ad

		foreach va { pp ag po pd ad } {
			set vn "_${fc}$va"
			if [info exists P(M0,$vn)] {
				set $va $P(M0,$vn)
			}
		}

		if [catch { valnum $po 1 65535 } po] {
			set po ""
		}

		set ld [list $tp $pp $ag $po $pd $ad]

		lappend nll $ld

		incr nc
		incr fc
	}

	if { $nll == "" } {
		# make sure there is at least one empty entry
		lappend nll [ldr_null]
	}

	if [info exists P(M0,_new)] {
		# new
		lassign $P(M0,_new) cnt tp
		unset P(M0,_new)
		if { $cnt < [llength $nll] } {
			# just in case
			if { $tp == "k" } {
				# clone
				set nw [lindex $nll $cnt]
			} else {
				set nw [ldr_select_default $tp]
				if { $nw == "" } {
					set ns [ldr_null $tp]
				}
			}
			if { $sel < $cnt } {
				incr sel
			}
			incr cnt
			set nll [linsert $nll $cnt $nw]
		}
		incr more
	}

	set P(M0,$als) $sel
	set P(M0,$all) $nll

	return $more
}

proc ldr_select_default { tp } {
#
# Returns the next standard loader to fill in a blank
#
	global ARCHINFO

	set nl [llength $ARCHINFO(LOADERS)]
	for { set i 0 } { $i < $nl } { incr i } {
		if { $ARCHINFO(LOADERS,LSCAN) >= $nl } {
			set ARCHINFO(LOADERS,LSCAN) 0
		}
		set ld [lindex $ARCHINFO(LOADERS) $ARCHINFO(LOADERS,LSCAN)]
		incr ARCHINFO(LOADERS,LSCAN)
		if { [lindex $ld 0] == $tp } {
			# got it
			return $ld
		}
	}

	return ""
}

proc ldr_wid_del { cnt arch } {
#
# Delete the loader widget
#
	global P

	set P(M0,_${cnt}de) ""

	md_click 1
}

proc ldr_wid_new { cnt tp } {

	global P

	set P(M0,_new) [list $cnt $tp]

	md_click 1
}

proc ldr_cnf_fsel { cnt arch vn } {
#
# Select the path to an executable loader, proxy, or debugger
#
	global P

	set P(M0,$vn) [string trim $P(M0,$vn)]

	# local cache for dir name
	set ldd "${arch}LDGPRPATH_D"

	if { [info exists P(M0,$ldd)] && [file isdirectory $P(M0,$ldd)] } {
		set id $P(M0,$ldd)
	} else {
		# use the directory path of last selection
		set id [file dirname $P(M0,$vn)]
		if ![file isdirectory $id] {
			set id ""
		}
		set P(M0,$ldd) $id
	}

	reset_all_menus 1
	set fi [tk_getOpenFile -initialdir $id -parent $P(M0,WI)]
	reset_all_menus

	if { $fi != "" } {
		set P(M0,$vn) $fi
		set P(M0,$ldd) [file dirname $fi]
	}
}

###############################################################################

proc upload_image { } {

	global P CFLoadItems TCMD

	if { $P(AC) == "" } {
		return
	}

	if { $TCMD(FY) != 0 } {
		alert "Loader already open"
		return
	}

	set arch [dict get $P(CO) "ARCH"]

	if ![info exists CFLoadItems($arch)] {
		# something wrong, this will never happen
		alert "Internal error, illegal architecture $arch"
		return
	}

	set als [dict get $P(CO) "${arch}_LDSEL"]
	set all [dict get $P(CO) "${arch}_LDLIST"]
	set use $als

	if [catch { valnum $use 0 [expr { [llength $all] - 1 }] } use] {
		# make sure this is sane
		set use 0
	}

	# the loader in use
	set ldr [lindex $all $use]

	# the type
	if { [lindex $ldr 0] == "d" } {
		# debugger
		ldr_upl_d $arch $ldr
	} else {
		ldr_upl_c $arch $ldr
	}
}

proc ldr_fargt { arg } {
#
# Returns the list: filetype filetype, where the first filetype is the
# required file name suffix of a present image file and the second, if not
# null, is the substitute suffix.
#
	if [regexp -nocase {%f\.([a-z_0-9]+)=([a-z_0-9]+)} $arg jnk ta tb] {
		# the most general case: type substitute
		return [list $ta $tb]
	}

	if [regexp -nocase {%f=([a-z_0-9]+)} $arg jnk tb] {
		# empty first type
		return [list "" $tb]
	}

	if [regexp -nocase {%f\.([a-z_0-9]+)} $arg jnk ta] {
		# type, no substitute
		return [list $ta ""]
	}

	if [regexp -nocase {%f} $arg] {
		return [list "" ""]
	}

	# this pathological case means: no file needed
	return ""
}

proc mk_upload_file_selection_window { flist } {

	global P

	set w [md_window "File selection"]

	frame $w.tf
	pack $w.tf -side top -expand y -fill x

	label $w.tf.l -text "Select the image file to upload: "
	pack $w.tf.l -side left -expand n -fill x

	set P(M0,UFILE) [lindex $flist 0]

	eval "tk_optionMenu $w.tf.r P(M0,UFILE) [split [join $flist]]"
	pack $w.tf.r -side right -expand n

	frame $w.bf
	pack $w.bf -side top -expand y -fill x

	button $w.bf.b -text "Go Ahead" -command "md_click 1"
	pack $w.bf.b -side right -expand n -fill x

	button $w.bf.c -text "Cancel" -command "md_click -1"
	pack $w.bf.c -side left -expand n -fill x

	bind $w <Destroy> "md_click -1"
}

proc ldr_farsub { arg } {
#
# Upload file name substitution
#
	global P

	set ft [ldr_fargt $arg]

	if { $ft == "" } {
		# no file needed
		return $arg
	}

	set ta [lindex $ft 0]
	set tb [lindex $ft 1]
	log "Load file types: $ta -> $tb"
	set fl [glob -nocomplain "Image*"]

	set ffl ""
	foreach f $fl {
		# select the interesting files
		if { $ta == "" && [file extension $f] == "" ||
		     $ta != "" && [file extension $f] == ".$ta" } {
			lappend ffl $f
		}
	}

	if { $ffl == "" } {
		if { $ta == "" } {
			set tp "ELF (assumed)"
		} else {
			set tp $ta
		}
		error "No image file found, type = $tp"
	}

	if { [llength $ffl] == 1 } {
		# a single choice
		set fn [lindex $ffl 0]
	} else {
		set w [mk_upload_file_selection_window $ffl]
		while 1 {
			set ev [md_wait]
			if { $ev < 0 } {
				# cancelled
				return
			}
			if { $ev == 1 } {
				set fn $P(M0,UFILE)
				md_stop
				break
			}
		}
	}

	if { $tb != "" } {
		# need to copy the file
		set fm "[file rootname $fn].$tb"
		if [catch { file copy -force -- $fn $fm } err] {
			error "Cannot copy $fn to $fm, $err"
		}
		set fn $fm
	}

	# do the substitution
	regsub -all -nocase {%f\.[a-z_0-9]+=[a-z_0-9]+} $arg $fn arg
	regsub -all -nocase {%f=[a-z_0-9]+} $arg $fn arg
	regsub -all -nocase {%f\.[a-z_0-9]+} $arg $fn arg
	regsub -all -nocase {%f} $arg $fn arg

	return $arg
}

proc ldr_ver_lst { arg } {
#
# Assemble a substituted argument string into a list of argments; looks like
# a do nothing, but it verifies if the string is a legit list
#
	set res ""

	foreach a $arg {
		lappend res $a
	}

	return $res
}

proc ldr_upl_c { arch ldr } {
#
# A direct flash loader
#
	global TCMD

	set pgm [lindex $ldr 1]

	if { $pgm == "" } {
		alert "Unknown loader program, you have to specify the program\
			path in the loader configuration window"
		return
	}

	if [catch { ldr_farsub [lindex $ldr 2] } arg] {
		alert $arg
		return
	}

	# build the arguments
	set al ""

	if [catch { ldr_ver_lst $arg } al] {
		alert "The effective argument string ($arg) for the loader\
			does not amount to a legit list"
		return
	}

	set TCMD(FY) 1

	term_dspline "UPLOADING: $pgm [join $al]"

	if [catch { run_term_command $pgm $al "upload_action 0" \
	    "upload_action 0" } err] {
		alert "Cannot execute $pgm, $err"
		upload_action 0
	}
}

proc ldr_upl_d { arch ldr } {
#
# Now for the tricky bit: a proxy-based debugger
#
	global TCMD

	set pro [lindex $ldr 1]

	if { $pro == "" } {
		alert "Unknown proxy program, you have to specify the program\
			path in the loader configuration window"
		return
	}

	# proxy argument
	set par [lindex $ldr 2]
	# proxy port
	set ppo [lindex $ldr 3]

	if [catch { valnum $ppo 1 65535 } ppo] {
		set ppo ""
	}

	regsub -all -nocase {%p} $par $ppo par

	# debugger
	set pgm [lindex $ldr 4]

	if { $pgm == "" } {
		alert "Unknown debugger program, you have to specify the\
			program path in the loader configuration window"
		return
	}

	if [catch { ldr_farsub [lindex $ldr 5] } arg] {
		alert $arg
		return
	}

	if [catch { ldr_ver_lst $par } alp] {
		alert "The effective argument string ($par) for the proxy\
			does not amount to a legit list"
		return
	}

	if [catch { ldr_ver_lst $arg } ald] {
		alert "The effective argument string ($arg) for the debugger\
			does not amount to a legit list"
		return
	}

	mk_gdb_init $pgm $ppo

	log "PROXY: $pro $alp"
	term_dspline "STARTING DEBUG PROXY"

	set TCMD(FY) 3
	set TCMD(FY,AR) [concat [list $pgm] $ald]

	if [catch { run_term_command $pro $alp "upload_action 0" \
    	    "upload_action 0" } err] {
		alert "Proxy startup failed, $err"
		upload_action 0
		return
	}

	set TCMD(FY,CB) [after 1000 ldr_start_debugger]
}

proc ldr_start_debugger { } {
#
# A callback to delay debugger startup
#
	global TCMD P

	set gcp [dict get $P(CO) "OPTSHWCMND"]
	set xt [lindex $gcp 0]
	regsub {%f} [lrange $gcp 1 end] $TCMD(FY,AR) al
	bpcs_run $xt $al "FL"
	set TCMD(FY,CB) ""
}

proc upload_action { start } {
#
# To be invoked when a flash loader is started/terminated
#
	global TCMD

	if !$start {
		if { $TCMD(FY) == 0 } {
			# already handled
			return
		}
		if { $TCMD(FY) == 3 } {
			# need to kill two processes; prevent the second copy
			# of upload_action from running
			set TCMD(FY) 0
			if { $TCMD(FL) != "" } {
				bpcs_kill "FL"
			}
			if { $TCMD(FD) != "" } {
				abort_term
			}
			if { $TCMD(FY,CB) != "" } {
				catch { after cancel $TCMD(FY,CB) }
				set TCMD(FY,CB) ""
			}
			# argument for the delayed debugger
			set TCMD(FY,AR) ""
		}
		set TCMD(FY) 0
	}
	reset_exec_menu
}

proc stop_loader { { ask 0 } } {

	global TCMD

	if { $TCMD(FY) == 0 } {
		return 0
	}

	if { $ask && ![confirm "The loader is running. Do you want me to kill\
		it first?"] } {
			return 1
	}

	if { $TCMD(FY) == 1 } {
		bpcs_kill "FL"
	} elseif { $TCMD(FY) == 2 } {
		abort_term
	} else {
		bpcs_kill "FL"
		abort_term
	}

	set TCMD(FY) 0

	return 0
}

proc mk_gdb_init { pgm port } {
#
# Creates the debugger config file
#
	global ARCHINFO

	# find the init file best matching the debugger path
	set pgm [string tolower $pgm]

	set exa ""
	set nex ""
	foreach g $ARCHINFO(DBINITS) {
		set pp [string tolower [lindex $g 0]]
		# try the exact match
		if { $pp == $pgm } {
			set exa $g
			break
		}
		if { [file tail $pp] == [file tail $pgm] } {
			set nex $g
		}
	}

	if { $exa == "" } {
		set exa $nex
	}

	if { $exa == "" } {
		log "No init file available for $pgm"
		return
	}

	set fn [lindex $exa 1]
	set fc [lindex $exa 2]

	log "Init file $fn"

	if { $port != "" } {
		# substitute the port number
		if [regexp -line -indices \
		    {^[[:blank:]]*target.*(:[[:digit:]]+)} $fc ma ma] {
			set fc \
			   "[string range $fc 0 [lindex $ma 0]]$port[string \
				range $fc [expr { [lindex $ma 1] + 1 }] end]"
		} else {
			log "Cannot insert port number into gdbinit"
		}
	}

	if [catch { open $fn "w" } fd] {
		error "unable to open $fn, $fd"
	}
	if [catch { puts $fd $fc } err] {
		catch { close $fd }
		error "unable to write to $fn, $err"
	}
	catch { close $fd }
}

###############################################################################
# End loaders #################################################################
###############################################################################

proc oss_available { } {

	return [file_present "ossi.tcl"]
}

proc do_extra_defines { } {

	global P CFEDefines

	if { $P(AC) == "" } {
		# the usual precaution
		return
	}

	params_to_dialog $CFEDefines

	mk_edefs_window

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancellation
			return
		}

		if { $ev == 0 } {
			# nothing
			continue
		}

		# validate the input
		if [catch { validate_edefines } err] {
			alert "Illegal syntax, $err"
			continue
		}

		set mc [dialog_to_params $CFEDefines]
		md_stop
		if $mc {
			set_config
			force_full_clean
		}
		return
	}
}

proc mk_edefs_window { } {

	global P FFont

	# convert the list to individual entries
	set ne 0
	foreach em $P(M0,XDEFS) {
		if { $em != "" } {
			set P(M0,XD$ne) $em
			incr ne
		}
	}

	# provide 4 extra entries for new defines at the end
	set te [expr { $ne + 4 } ]
	set P(M0,NE) $te

	set w [md_window "Extra defines"]

	set f $w.mf
	frame $f
	pack $f -side top -expand y -fill x

	label $f.dla -text "Symbol\[=value\]"
	grid $f.dla -column 0 -row 0 -padx 4 -pady 3 -sticky n

	label $f.dlc -text "Clear"
	grid $f.dlc -column 1 -row 0 -padx 4 -pady 3 -sticky n

	for { set i 0 } { $i < $te } { incr i } {
		if ![info exists P(M0,XD$i)] {
			set P(M0,XD$i) ""
		}
		entry $f.den$i -font $FFont -textvariable P(M0,XD$i) \
			-width 36
		grid $f.den$i -column 0 -row [expr { $i + 1 }] -padx 4 -pady 2 \
			-sticky we
		button $f.but$i -text " " -command "edefs_clear $i"
		grid $f.but$i -column 1 -row [expr { $i + 1 }] -padx 4 -pady 2 \
			-sticky we
	}

	grid columnconfigure $f 0 -weight 1

	##
	set f $w.bf
	frame $f
	pack $f -side top -expand n -fill x

	button $f.cb -text "Cancel" -command "md_click -1"
	pack $f.cb -side left -expand n

	button $f.db -text "Done" -command "md_click 1"
	pack $f.db -side right -expand n

	bind $w <Destroy> "md_click -1"
}

proc edefs_clear { i } {

	global P

	if [info exists P(M0,XD$i)] {
		set P(M0,XD$i) ""
	}
}

proc validate_edefines { } {

	global P

	set res ""

	for { set i 0 } { $i < $P(M0,NE) } { incr i } {
		set df $P(M0,XD$i)
		if { $df == "" } {
			# ignore erased/empty entries
			continue
		}
		lappend res $df
		set en [expr { $i + 1 }]
		# look up the initial keyword
		if ![regexp {^[_[:alpha:]][_[:alnum:]]*} $df mat] {
			error "entry $en, illegal symbol being defined"
		}
		set rm [string range $df [string length $mat] end]
		if { $rm == "" } {
			continue
		}
		# for now, just check if it starts with =nonblank
		if ![regexp {^=[^[:space:]]} $rm] {
			error "entry $en, symbol can only be followed by =..."
		}
	}

	# return the list
	set P(M0,XDEFS) $res
}
		
proc do_vuee_config { } {

	global P CFVueeItems

	if { $P(AC) == "" } {
		return
	}

	params_to_dialog $CFVueeItems

	mk_vuee_conf_window

	while 1 {

		# enable/disable widgets
		vconf_widgets disable { thrd cmpis dpbc pfac udon udtm ycdn
			oson osnn udpl osnh vudf vuof vusm vuri }

		if !$P(M0,VDISABLE) {
			vconf_widgets normal \
		          { thrd cmpis dpbc pfac udon oson vudf vuof vusm vuri }
			if $P(M0,UDON) {
				vconf_widgets normal { udtm ycdn udpl }
			}
			if $P(M0,OSON) {
				vconf_widgets normal { osnn osnh }
			}
		}

		set ev [md_wait]

		if { $ev < 0 } {
			# cancelled
			return
		}

		if { $ev == 1 } {
			# accepted
			if $P(M0,OSON) {
				# validate node number
				if [catch { valnum $P(M0,OSNN) 0 } val] {
					alert "Illegal host number for oss,\
						$val"
					continue
				}
				set P(M0,OSNN) $val
			}

			# check if should erase VUEE build
			if { $P(M0,THRD) != [dict get $P(CO) "THRD"] } {
				term_dspline \
			  	"--DIMENSIONALITY CHANGED, VUEE BUILD CLEANED--"
				do_clean_vuee
			}
			dialog_to_params $CFVueeItems
			md_stop
			set_config
			reset_bnx_menus
			return
		}
	}
}

proc vconf_widgets { what which } {

	global P

	foreach w $which {

		if [info exists P(M0,$w)] {
			$P(M0,$w) configure -state $what
		}
	}
}

proc mk_vuee_conf_window { } {

	global P FFont

	set w [md_window "VUEE configuration"]
	set row 0

	##
	label $w.tdl -text "Disable VUEE:"
	grid $w.tdl -row $row -column 0 -sticky w -padx 1 -pady 1
	checkbutton $w.tdc -variable P(M0,VDISABLE) -command "set P(M0,EV) 2"
	grid $w.tdc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.tel -text "3d network layout:"
	grid $w.tel -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,thrd) [checkbutton $w.tec -variable P(M0,THRD)]
	grid $w.tec -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.tfl -text "Functions are idiosyncratic:"
	grid $w.tfl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,cmpis) [checkbutton $w.tfc -variable P(M0,CMPIS)]
	grid $w.tfc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.tkl -text "No board config:"
	grid $w.tkl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,dpbc) [checkbutton $w.tkc -variable P(M0,DPBC)]
	grid $w.tkc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.tvl -text "Port for agent connections: "
	grid $w.tvl -row $row -column 0 -sticky w -padx 1 -pady 1
	tk_optionMenu $w.tvm P(M0,PFAC) "Default" 3066 3067 3068 4013 4014 4015\
		4444 4445 4446 4447
	set P(M0,pfac) $w.tvm
	grid $w.tvm -row $row -column 2 -sticky nwe -padx 1 -pady 1

	##
	incr row
	label $w.tgl -text "Run with udaemon:"
	grid $w.tgl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,udon) [checkbutton $w.tgc -variable P(M0,UDON) \
		-command "set P(M0,EV) 2"]
	grid $w.tgc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.ttl -text "    Terminate when udaemon quits:"
	grid $w.ttl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,udtm) [checkbutton $w.ttc -variable P(M0,UDTM)]
	grid $w.ttc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.tql -text "    Y coordinate goes up->down:"
	grid $w.tql -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,ycdn) [checkbutton $w.tqc -variable P(M0,YCDN)]
	grid $w.tqc -row $row -column 2 -sticky ne -padx 1 -pady 1

	##
	incr row
	label $w.upl -text "    Plugin file:"
	grid $w.upl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,udpl_l) [trunc_fname $P(M0,UDPL)]
	label $w.upf -textvariable P(M0,udpl_l)
	grid $w.upf -row $row -column 1 -sticky e -padx 1 -pady 1
	set P(M0,udpl) [button $w.upb -text "Select" -command "vuee_conf_psel"]
	grid $w.upb -row $row -column 2 -sticky nwe -padx 1 -pady 1

	if [oss_available] {

		##
		incr row
		label $w.ogl -text "Run with oss:"
		grid $w.ogl -row $row -column 0 -sticky w -padx 1 -pady 1
		set P(M0,oson) [checkbutton $w.ogc -variable P(M0,OSON) \
			-command "set P(M0,EV) 2"]
		grid $w.ogc -row $row -column 2 -sticky ne -padx 1 -pady 1

		##
		incr row
		label $w.otl -text "    Node number:"
		grid $w.otl -row $row -column 0 -sticky w -padx 1 -pady 1
		set P(M0,osnn) \
		    [entry $w.otc -width 4 -font $FFont -textvariable P(M0,OSNN)]
		grid $w.otc -row $row -column 2 -sticky nwe -padx 1 -pady 1

		##
		incr row
		label $w.oul -text "    Number is HID:"
		grid $w.oul -row $row -column 0 -sticky w -padx 1 -pady 1
		set P(M0,osnh) [checkbutton $w.ouc -variable P(M0,OSNH)]
		grid $w.ouc -row $row -column 2 -sticky ne -padx 1 -pady 1
	}

	##
	incr row
	label $w.thl -text "Praxis data file:"
	grid $w.thl -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,vudf_l) [trunc_fname $P(M0,VUDF)]
	label $w.thf -textvariable P(M0,vudf_l)
	grid $w.thf -row $row -column 1 -sticky e -padx 1 -pady 1
	set P(M0,vudf) [button $w.thb -text "Select" -command "vuee_conf_dsel"]
	grid $w.thb -row $row -column 2 -sticky nwe -padx 1 -pady 1

	##
	incr row
	label $w.tal -text "Praxis output file:"
	grid $w.tal -row $row -column 0 -sticky w -padx 1 -pady 1
	set P(M0,vuof_l) [trunc_fname $P(M0,VUOF)]
	label $w.taf -textvariable P(M0,vuof_l)
	grid $w.taf -row $row -column 1 -sticky e -padx 1 -pady 1
	set P(M0,vuof) [button $w.tab -text "Select" -command "vuee_conf_osel"]
	grid $w.tab -row $row -column 2 -sticky nwe -padx 1 -pady 1

	##
	incr row
	label $w.tsl -text "Slo-mo factor:"
	grid $w.tsl -row $row -column 0 -sticky w -padx 1 -pady 1
	tk_optionMenu $w.tsm P(M0,VUSM) \
		"U" 0.25 0.5 1.0 2.0 3.0 4.0 5.0 10.0 20.0 50.0 100.0
	grid $w.tsm -row $row -column 2 -sticky nwe -padx 1 -pady 1
	set P(M0,vusm) $w.tsm

	##
	incr row
	label $w.tzl -text "Resync interval:"
	grid $w.tzl -row $row -column 0 -sticky w -padx 1 -pady 1
	tk_optionMenu $w.tzm P(M0,VURI) \
		1000 750 500 300 200 100 75 50 40 30 20 10 5
	grid $w.tzm -row $row -column 2 -sticky nwe -padx 1 -pady 1
	set P(M0,vuri) $w.tzm

	##
	incr row
	label $w.tbl -text "Extra comp args:"
	grid $w.tbl -row $row -column 0 -sticky w -padx 1 -pady 1
	entry $w.tbm -width 16 -font $FFont -textvariable P(M0,EBRG)
	grid $w.tbm -row $row -column 1 -columnspan 2 -sticky nwe -padx 1 -pady 1
	set P(M0,ebrg) $w.tbm

	##
	incr row
	label $w.tyl -text "Extra exec args:"
	grid $w.tyl -row $row -column 0 -sticky w -padx 1 -pady 1
	entry $w.tym -width 16 -font $FFont -textvariable P(M0,EARG)
	grid $w.tym -row $row -column 1 -columnspan 2 -sticky nwe -padx 1 -pady 1
	set P(M0,earg) $w.tym

	##
	incr row
	button $w.tjc -text "Cancel" -command "md_click -1"
	grid $w.tjc -row $row -column 0 -sticky nw -padx 1 -pady 1
	button $w.tjd -text "Done" -command "md_click 1"
	grid $w.tjd -row $row -column 2 -sticky ne -padx 1 -pady 1

	bind $w <Destroy> "md_click -1"
}

proc vuee_conf_dsel { } {
#
# Selects a data file for the praxis (VUEE model)
#
	global P

	if { ![info exists P(LFS,VUDF)] ||
	      [file_location $P(LFS,VUDF)] != "T" } {
		# remembers last directory and defaults to the project's
		# directory
		set P(LFS,VUDF) $P(AC)
	}

	set ft [list [list "Praxis data file" [list ".xml"]]]
	set de ".xml"
	set ti "data file for the praxis"

	while 1 {

		reset_all_menus 1
		set fn [tk_getOpenFile 	-defaultextension $de \
					-filetypes $ft \
					-initialdir $P(LFS,VUDF) \
					-title "Select a $ti" \
					-parent $P(M0,WI)]
		reset_all_menus

		if { $fn == "" } {
			# cancelled, cancel
			set P(M0,VUDF) ""
			set P(M0,vudf_l) ""
			return
		}
		set fn [relative_path $fn]
		if ![file isfile $fn] {
			alert "The file doesn't exist"
			continue
		}
		# assume it is OK, but use a relative path
		set P(M0,VUDF) $fn
		# for posterity
		set P(LFS,VUDF) [file dirname $fn]
		set P(M0,vudf_l) [trunc_fname $P(M0,VUDF)]
		return
	}
}

proc vuee_conf_osel { } {
#
# Selects an output file for the praxis (VUEE model)
#
	global P

	if { ![info exists P(LFS,VUOF)] ||
	      [file_location $P(LFS,VUOF)] != "T" } {
		# remembers last directory and defaults to the project's
		# directory
		set P(LFS,VUOF) $P(AC)
	}

	set ft [list [list "Praxis output file" [list ".txt"]]]
	set de ".txt"
	set ti "output file for the praxis"

	while 1 {

		reset_all_menus 1
		set fn [tk_getSaveFile 	-defaultextension $de \
					-filetypes $ft \
					-initialdir $P(LFS,VUOF) \
					-title "Select an $ti" \
					-parent $P(M0,WI)]
		reset_all_menus

		if { $fn == "" } {
			# cancelled, cancel
			set P(M0,VUOF) ""
			set P(M0,vuof_l) ""
			return
		}

		# assume it is OK, but try to use a relative path
		set P(M0,VUOF) [relative_path $fn]
		set P(M0,vuof_l) $P(M0,VUOF)

		# for posterity
		set P(LFS,VUOF) [file dirname $fn]

		return
	}
}

proc vuee_conf_psel { } {
#
# Selects a plugin file for VUEE
#
	global P

	if { ![info exists P(LFS,UDPL)] ||
	      [file_location $P(LFS,UDPL)] != "T" } {
		# remembers last directory and defaults to the project's
		# directory
		set P(LFS,UDPL) $P(AC)
	}

	set ft [list [list "Udaemon plugin file" [list ".tcl"]]]
	set de ".tcl"
	set ti "plugin file for udaemon"

	while 1 {

		reset_all_menus 1
		set fn [tk_getOpenFile 	-defaultextension $de \
					-filetypes $ft \
					-initialdir $P(LFS,UDPL) \
					-title "Select a $ti" \
					-parent $P(M0,WI)]
		reset_all_menus

		if { $fn == "" } {
			# cancelled, cancel
			set P(M0,UDPL) ""
			set P(M0,udpl_l) ""
			return
		}

		# try to use a relative path
		set fn [relative_path $fn]

		if ![file isfile $fn] {
			alert "The file doesn't exist"
			continue
		}

		set P(M0,UDPL) $fn
		set P(M0,udpl_l) $P(M0,UDPL)

		# for posterity
		set P(LFS,UDPL) [file dirname $fn]

		return
	}
}

proc vuee_disabled { } {
#
# Returns 1 if VUEE is disabled for the project
#
	global P ST

	if { $P(AC) == "" || $P(CO) == "" } {
		return 1
	}

	if { !$ST(VP) || [dict get $P(CO) "VDISABLE"] == 0 } {
		return 0
	}

	return 1
}

###############################################################################

proc do_options { } {
#
# Configure "other" options associated with the project
#
	global P CFOptItems CFOptSFModes TermLines DefTerm DefDTerm DefAEdit

	if { $P(AC) == "" || $P(CO) == "" } {
		return
	}

	params_to_dialog $CFOptItems

	mk_options_conf_window

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# cancelled
			return
		}

		set c 0

		if { $ev == 1 } {
			set val [string trim $P(M0,tr)]
			if { $val == "" } {
				# make it the default
				set val $DefTerm
			}
			set P(M0,OPTTRMCMND) $val

			set val [string trim $P(M0,gd)]
			if { $val == "" } {
				# make it the default
				set val $DefDTerm
			}
			set P(M0,OPTSHWCMND) $val

			set val [string trim $P(M0,ae)]
			if { $val == "" } {
				# make it the default
				set val $DefAEdit
			}
			set P(M0,OPTATECMND) $val

			# verify the number
			set n $P(M0,lc)
			if [catch { valnum $n 24 100000 } n] {
				alert "Illegal console line number limit: $n"
				continue
			}

			set P(M0,OPTERMLINES) $n
			set TermLines $n

			set P(M0,OPCOMMENT) [$P(M0,ct) get 1.0 end]

			foreach u { "sf" "vf" } \
			    z { "OPSYSFILES" "OPVUEEFILES" } {
				set n [lsearch $CFOptSFModes $P(M0,$u)]
				if { $n < 0 } {
					# impossible
					set n 0
				}
				set P(M0,$z) $n
			}

			# detect when a file permission value changes from
			# zero to nonzero or the other way around, which will
			# require re-tagging
			set u [dict get $P(CO) "OPSYSFILES"]
			if { ( $u == 0 && $P(M0,OPSYSFILES) != 0 ) ||
			     ( $u != 0 && $P(M0,OPSYSFILES) == 0 ) } {
				set u 1
			} else {
				set u 0
			}
			set z [dict get $P(CO) "OPVUEEFILES"]
			if { ( $z == 0 && $P(M0,OPVUEEFILES) != 0 ) ||
			     ( $z != 0 && $P(M0,OPVUEEFILES) == 0 ) } {
				set z 1
			} else {
				set z 0
			}

			dialog_to_params $CFOptItems
			md_stop
			set_config
			# this can only be done after the changes have settled
			if $u {
				sys_make_ctags
				# to show/hide Boards
				gfl_tree
				reset_config_menu
			}
			if $z {
				vue_make_ctags
			}
			if $c {
				term_dspline \
				  "--TOOL CONFIG CHANGED, FULL CLEAN FORCED --"
				do_cleanup
			}
			return
		}
	}
}

proc mk_options_conf_window { } {

	global P CFOptSFModes FFont ST

	set w [md_window "Options"]

	set ewidth 16

	##
	set f $w.tf
	frame $f
	pack $f -side top -expand n -fill x

	set row 0

	label $f.trl -text "Terminal window command: "
	grid $f.trl -column 0 -row $row -padx 4 -pady 2 -sticky w

	set P(M0,tr) $P(M0,OPTTRMCMND)
	entry $f.tre -width $ewidth -font $FFont -textvariable P(M0,tr)
	grid $f.tre -column 1 -row $row -padx 4 -pady 2 -sticky we

	incr row

	label $f.gdl -text "Shell command in a terminal window: "
	grid $f.gdl -column 0 -row $row -padx 4 -pady 2 -sticky w

	set P(M0,gd) $P(M0,OPTSHWCMND)
	entry $f.gde -width $ewidth -font $FFont -textvariable P(M0,gd)
	grid $f.gde -column 1 -row $row -padx 4 -pady 2 -sticky we

	incr row

	label $f.ael -text "Alternate text editor command: "
	grid $f.ael -column 0 -row $row -padx 4 -pady 2 -sticky w

	set P(M0,ae) $P(M0,OPTATECMND)
	entry $f.aee -width $ewidth -font $FFont -textvariable P(M0,ae)
	grid $f.aee -column 1 -row $row -padx 4 -pady 2 -sticky we

	incr row

	label $f.tll -text "Maximum number of lines saved in console: "
	grid $f.tll -column 0 -row $row -padx 4 -pady 2 -sticky w

	set P(M0,lc) $P(M0,OPTERMLINES)
	entry $f.tle -width $ewidth -font $FFont -textvariable P(M0,lc)
	grid $f.tle -column 1 -row $row -padx 4 -pady 2 -sticky we

	incr row

	label $f.sfl -text "Show and edit PicOS system files: "
	grid $f.sfl -column 0 -row $row -padx 4 -pady 2 -sticky w

	set v $P(M0,OPSYSFILES)
	if [catch { valnum $v 0 3 } v] {
		set v 0
	}
	set P(M0,sf) [lindex $CFOptSFModes $v]
	eval "tk_optionMenu $f.sfe P(M0,sf) $CFOptSFModes"
	grid $f.sfe -column 1 -row $row -padx 4 -pady 2 -sticky we

	incr row

	if $ST(VP) {
		label $f.vfl -text "Show and edit VUEE system files: "
		grid $f.vfl -column 0 -row $row -padx 4 -pady 2 -sticky w

		set v $P(M0,OPVUEEFILES)
		if [catch { valnum $v 0 3 } v] {
			set v 0
		}
		set P(M0,vf) [lindex $CFOptSFModes $v]
		eval "tk_optionMenu $f.vfe P(M0,vf) $CFOptSFModes"
		grid $f.vfe -column 1 -row $row -padx 4 -pady 2 -sticky we
	}

	grid columnconfigure $f 1 -weight 1

	##
	set f $w.cf
	ttk::labelframe $f -text Notes
	pack $f -side top -expand y -fill both
	set t $f.t
	text $t
	$t configure -font $FFont -exportselection 1 -state normal -height 4 \
		-wrap word
	pack $t -side top -expand y -fill both

	$t delete 1.0 end
	$t insert end $P(M0,OPCOMMENT)
	set P(M0,ct) $t

	##
	set f $w.bf
	frame $f
	pack $f -side top -expand n -fill x

	button $f.cb -text "Cancel" -command "md_click -1"
	pack $f.cb -side left -expand n

	button $f.db -text "Done" -command "md_click 1"
	pack $f.db -side right -expand n

	bind $w <Destroy> "md_click -1"
}

###############################################################################

proc do_eschemes { } {
#
# Configure Elvis color/font schemes
#
	global P

	mk_eschemes_window

	while 1 {

		set ev [md_wait]

		if { $ev < 0 } {
			# kill/cancellation
			return
		}

		if { $ev > 0 } {
			# save
			save_eschemes
			md_stop
			return
		}
	}
}

proc get_escheme_names { } {
#
# Returns the list of user-defined scheme names currently available
#
	global ESchemes

	set sl ""

	foreach s $ESchemes {
		# we shall keep them sorted, I guess (or perhaps it makes no
		# difference)
		lappend sl [lindex $s 0]
	}

	return $sl
}

proc get_escheme { nam } {
#
# Get a scheme by name
#
	global ESchemes ESchemesD

	if { $nam == "Default" } {
		return $ESchemesD
	}

	foreach s $ESchemes {
		if { [lindex $s 0] == $nam } {
			return [lindex $s 1]
		}
	}

	return ""
}

proc mk_eschemes_window { } {

	global P NSESchemes FFont

	set w [md_window "Elvis schemes"]

	#######################################################################

	set f $w.lf

	labelframe $f -text "Schemes" -padx 4 -pady 4
	pack $f -side left -expand n -fill y

	##

	# the scheme menu button
	set mb $f.mb

	mk_menu_button $mb
	pack $mb -side top -expand n -fill x

	# to be changed when the list is updated
	set P(M0,EL) $mb

	##
	button $f.bc -text "Close" -command "md_click 1"
	pack $f.bc -side bottom -expand n -fill x

	##
	button $f.bd -text "Delete" -command "delete_escheme"
	pack $f.bd -side bottom -expand n -fill x

	##
	button $f.bn -text "New" -command "new_escheme"
	pack $f.bn -side bottom -expand n -fill x

	##
	button $f.be -text "Edit" -command "edit_escheme"
	pack $f.be -side bottom -expand n -fill x

	#######################################################################

	set f $w.rf

	labelframe $f -text "Assignment" -padx 4 -pady 4
	pack $f -side left -expand y -fill both

	# this is the fixed number of assignment slots; any better ideas?
	set NSESchemes 6

	# frame pointer to the assignment array, items are:
	#	t - file type
	#	p - pattern
	#	k - kind (project, system, both)
	#	s - scheme
	set P(M0,AS) $f
	# Headers
	label $f.t -text "File types" -anchor w
	grid $f.t -column 0 -row 0 -sticky w -padx 2 -pady 2
	label $f.p -text "Regex pattern" -anchor w
	grid $f.p -column 1 -row 0 -sticky w -padx 2 -pady 2
	label $f.k -text "Where" -anchor w
	grid $f.k -column 2 -row 0 -sticky w -padx 2 -pady 2
	label $f.s -text "Scheme" -anchor w
	grid $f.s -column 3 -row 0 -sticky w -padx 2 -pady 2
	for { set i 0 } { $i < $NSESchemes } { incr i } {
		##
		set bu $f.t$i
		mk_menu_button $bu
		set j [expr $i + 1]
		grid $bu -column 0 -row $j -sticky we -padx 2 -pady 2
		##
		set bu $f.p$i
		set P(M0,P$i) ""
		entry $bu -width 12 -font $FFont -textvariable P(M0,P$i)
		grid $bu -column 1 -row $j -sticky we -padx 2 -pady 2
		##
		set bu $f.k$i
		mk_menu_button $bu
		grid $bu -column 2 -row $j -sticky we -padx 2 -pady 2
		##
		set bu $f.s$i
		mk_menu_button $bu
		grid $bu -column 3 -row $j -sticky we -padx 2 -pady 2
	}

	show_eschemes

	bind $w <Destroy> "md_click -1"
}

proc valid_scheme_name { n } {

	if { $n == "Default" } {
		# reserved
		return 0
	}
	return [regexp -nocase "^\[a-z\]\[a-z0-9_\]*$" $n]
}

proc auto_scheme_name { } {
#
# Generate an automatic scheme name
#
	set sl [get_escheme_names]

	set sn 1

	while 1 {
		set n "ElvScheme$sn"
		if { [lsearch -exact $sl $n] < 0 } {
			return $n
		}
		incr sn
	}
}

proc show_eschemes { } {
#
# Display the present configuration of elvis schemes in the window
#
	global P NSESchemes ESchemes ESchemesA LFTypes ESFTypes

	if ![info exists P(M0,AS)] {
		# a precaution in case the window has disappeared while we
		# were waiting for the scheme generator
		return
	}

	# build the list of filetypes
	set ftps "All"
	foreach f $LFTypes {
		lappend ftps [lindex $f 0]
	}

	set esms ""
	set esmn ""
	# build the list of scheme names and validate the schemes along
	# the way
	foreach f $ESchemes {
		# scheme name
		set sn [lindex $f 0]
		if ![valid_scheme_name $sn] {
			# ignore
			continue
		}
		if { [lsearch -exact $esms $sn] >= 0 } {
			# duplicate
			continue
		}
		lappend esmn $f
		lappend esms $sn
	}

	set ESchemes $esmn
	unset esmn

	set f $P(M0,AS)
	for { set i 0 } { $i < $NSESchemes } { incr i } {

		set ent [lindex $ESchemesA $i]
		lassign $ent t p k s

		if { $t == "" || [lsearch -exact $ftps $t] < 0 } {
			set t [lindex $ftps 0]
		}

		if { $k == "" || [lsearch -exact $ESFTypes $k] < 0 } {
			set k [lindex $ESFTypes 0]
		}

		if { $s == "" || [lsearch -exact $esms $s] < 0 } {
			set s "Default"
		}

		set_menu_button $f.t$i $t $ftps
		set P(M0,P$i) $p
		set_menu_button $f.k$i $k $ESFTypes
		set_menu_button $f.s$i $s [concat [list "Default"] $esms]
	}

	set_menu_button $P(M0,EL) [lindex $esms 0] $esms
}

proc save_eschemes { } {
#
# Copy the scheme assignments from the window
#
	global P ESchemesA NSESchemes

	set ESchemesA ""
	set f $P(M0,AS)

	for { set i 0 } { $i < $NSESchemes } { incr i } {
		# the file type
		set t [read_menu_button $f.t$i]
		set p $P(M0,P$i)
		set k [read_menu_button $f.k$i]
		set s [read_menu_button $f.s$i]
		# we do not check the pattern; as we don't trust it anyway,
		# we shall do it at the actual matching attempt
		lappend ESchemesA [list $t $p $k $s]
	}

	set_rcoption "ESchemes" "ESchemesA"
}

proc edit_escheme { } {
#
# Edits an existing scheme
#
	global P ESchemes

	if ![info exists P(M0,EL)] {
		# in case the window no longer exists
		return
	}

	set nam [read_menu_button $P(M0,EL)]

	if { $nam == "" } {
		alert "No scheme to edit"
		return
	}

	set sch ""
	set inx 0
	foreach s $ESchemes {
		if { [lindex $s 0] == $nam } {
			set sch [lindex $s 1]
			break
		}
		incr inx
	}

	if { $sch == "" } {
		# impossible
		alert "Cannot find scheme $nam"
		return
	}

	set res [ece_editor $sch]

	if { $res == "" } {
		# generator cancelled
		return
	}

	set ESchemes [lreplace $ESchemes $inx $inx [list $nam $res]]

	show_eschemes
	save_eschemes
}

proc new_escheme { } {
#
# Create a new scheme
#
	global P ESchemes

	# this should be a second level modal window
	mk_new_escheme_window

	while 1 {

		set ev [md_wait 1]

		if { $ev < 0 } {
			# kill/cancellation, nothing happened, no need to
			# convey anything to the previous window
			return
		}

		if { $ev > 0 } {
			# validate the name
			set nam [string trim $P(M1,SN)]
			if { $nam == "" } {
				alert "The scheme name cannot be empty"
				continue
			}
			if ![valid_scheme_name $nam] {
				alert "The scheme name is invalid, must be\
					alphanumeric (starting with a letter)"
				continue
			}
			# check if duplicate
			if { [lsearch -exact [get_escheme_names] $nam] >= 0 } {
				alert "The scheme name already exists"
				continue
			}
			# OK, get the input scheme
			set sch [read_menu_button $P(M1,BA)]
			break
		}
	}

	# we get here when we are supposed to continue with the scheme
	# definition
	md_stop 1

	# the source scheme
	set sch [get_escheme $sch]

	set res [ece_editor $sch]

	if { $res == "" } {
		# generator cancelled
		return
	}

	# res is the new scheme
	lappend ESchemes [list $nam $res]
	show_eschemes
	save_eschemes
}

proc delete_escheme { } {
#
# Deletes the currently selected user-defined scheme
#
	global ESchemes P

	if ![info exists P(M0,EL)] {
		# a standard precaution
		return
	}

	set sch [read_menu_button $P(M0,EL)]
	if { $sch == "" } {
		alert "No scheme to delete"
		return
	}

	set es ""

	set fnd 0
	foreach s $ESchemes {
		if { [lindex $s 0] == $sch } {
			set fnd 1
		} else {
			lappend es $s
		}
	}

	if { $fnd && [confirm "Scheme $sch to be deleted, please confirm!"] } {
		set ESchemes $es
		show_eschemes
		save_eschemes
	}
}

proc mk_new_escheme_window { } {

	global P FFont

	# level 1 modal window
	set w [md_window "New scheme" 1]

	set P(M1,SN) [auto_scheme_name]

	set f $w.tf
	frame $f
	pack $f -side top -expand n -fill x

	label $f.nl -text "Name:" -anchor w
	grid $f.nl -column 0 -row 0 -padx 2 -pady 2 -sticky nw

	entry $f.ne -width 16 -font $FFont -textvariable P(M1,SN)
	grid $f.ne -column 1 -row 0 -padx 2 -pady 2 -sticky wen

	label $f.sl -text "Derived from:" -anchor w
	grid $f.sl -column 0 -row 1 -padx 2 -pady 2 -sticky nw

	set sl [concat [list "Default"] [get_escheme_names]]
	set P(M1,BA) $f.se
	mk_menu_button $P(M1,BA)
	grid $P(M1,BA) -column 1 -row 1 -padx 2 -pady 2 -sticky wen
	set_menu_button $P(M1,BA) "Default" $sl

	##
	set f $w.bf
	frame $f
	pack $f -side top -expand n -fill x

	button $f.c -text "Cancel" -command "md_click -1 1"
	pack $f.c -side left -expand n

	button $f.n -text "Create" -command "md_click 1 1"
	pack $f.n -side right -expand n

	##
	bind $w <Destroy> "md_click -1 1"
}

###############################################################################
# This is the editor for Elvis color schemes ##################################
###############################################################################

## Font options
set ece_FS { default 5x7 5x8 6x9 6x10 6x12 6x13 7x13 7x14 8x13 9x15 9x18 10x20 }

## Attributes
set ece_ATT { bold italic underlined boxed }

## Implements scrolled frames needed to accommodate the longish window with
## color selections

if {[info exists ::scrolledframe::version]} { return }
  namespace eval ::scrolledframe \
  {
  # beginning of ::scrolledframe namespace definition

    package require Tk 8.5
    namespace export scrolledframe

  # ==============================
  #
  # scrolledframe
  set version 0.9.1
  set (debug,place) 0
  #
  # a scrolled frame
  #
  # (C) 2003, ulis
  #
  # NOL licence (No Obligation Licence)
  #
  # Changes (C) 2004, KJN
  #
  # NOL licence (No Obligation Licence)
  # ==============================
  #
  # Hacked package, no documentation, sorry
  # See example at bottom
  #
  # ------------------------------
  # v 0.9.1
  #  automatic scroll on resize
  # ==============================

    package provide Scrolledframe $version

    # --------------
    #
    # create a scrolled frame
    #
    # --------------
    # parm1: widget name
    # parm2: options key/value list
    # --------------
    proc scrolledframe {w args} \
    {
      variable {}
      # create a scrolled frame
      frame $w
      # trap the reference
      rename $w ::scrolledframe::_$w
      # redirect to dispatch
      interp alias {} $w {} ::scrolledframe::dispatch $w
      # create scrollable internal frame
      frame $w.scrolled -highlightt 0 -padx 0 -pady 0
      # place it
      place $w.scrolled -in $w -x 0 -y 0
      if {$(debug,place)} { puts "place $w.scrolled -in $w -x 0 -y 0" } ;#DEBUG
      # init internal data
      set ($w:vheight) 0
      set ($w:vwidth) 0
      set ($w:vtop) 0
      set ($w:vleft) 0
      set ($w:xscroll) ""
      set ($w:yscroll) ""
      set ($w:width)    0
      set ($w:height)   0
      set ($w:fillx)    0
      set ($w:filly)    0
      # configure
      if {$args != ""} { uplevel 1 ::scrolledframe::config $w $args }
      # bind <Configure>
      bind $w <Configure> [namespace code [list resize $w]]
      bind $w.scrolled <Configure> [namespace code [list resize $w]]
      # return widget ref
      return $w
    }

    # --------------
    #
    # dispatch the trapped command
    #
    # --------------
    # parm1: widget name
    # parm2: operation
    # parm2: operation args
    # --------------
    proc dispatch {w cmd args} \
    {
      variable {}
      switch -glob -- $cmd \
      {
        con*    { uplevel 1 [linsert $args 0 ::scrolledframe::config $w] }
        xvi*    { uplevel 1 [linsert $args 0 ::scrolledframe::xview  $w] }
        yvi*    { uplevel 1 [linsert $args 0 ::scrolledframe::yview  $w] }
        default { uplevel 1 [linsert $args 0 ::scrolledframe::_$w    $cmd] }
      }
    }

    # --------------
    # configure operation
    #
    # configure the widget
    # --------------
    # parm1: widget name
    # parm2: options
    # --------------
    proc config {w args} \
    {
      variable {}
      set options {}
      set flag 0
      foreach {key value} $args \
      {
        switch -glob -- $key \
        {
          -fill   \
          {
            # new fill option: what should the scrolled object do if it is
	    # smaller than the viewing window?
            if {$value == "none"} {
               set ($w:fillx) 0
               set ($w:filly) 0
            } elseif {$value == "x"} {
               set ($w:fillx) 1
               set ($w:filly) 0
            } elseif {$value == "y"} {
               set ($w:fillx) 0
               set ($w:filly) 1
            } elseif {$value == "both"} {
               set ($w:fillx) 1
               set ($w:filly) 1
            } else {
               error "invalid value: should be \"$w configure -fill value\",\
			where \"value\" is \"x\", \"y\", \"none\", or \"both\""
            }
            resize $w force
            set flag 1
          }
          -xsc*   \
          {
            # new xscroll option
            set ($w:xscroll) $value
            set flag 1
          }
          -ysc*   \
          {
            # new yscroll option
            set ($w:yscroll) $value
            set flag 1
          }
          default { lappend options $key $value }
        }
      }
      # check if needed
      if {!$flag || $options != ""} \
      {
        # call frame config
        uplevel 1 [linsert $options 0 ::scrolledframe::_$w config]
      }
    }

    # --------------
    # resize proc
    #
    # Update the scrollbars if necessary, in response to a change in either the
    # viewing window or the scrolled object.
    # Replaces the old resize and the old vresize
    # A <Configure> call may mean any change to the viewing window or the
    # scrolled object.
    # We only need to resize the scrollbars if the size of one of these objects
    # has changed.
    # Usually the window sizes have not changed, and so the proc will not
    # resize the scrollbars.
    # --------------
    # parm1: widget name
    # parm2: pass anything to force resize even if dimensions are unchanged
    # --------------
    proc resize {w args} \
    {
      variable {}
      set force [llength $args]

      set _vheight     $($w:vheight)
      set _vwidth      $($w:vwidth)
      # compute new height & width
      set ($w:vheight) [winfo reqheight $w.scrolled]
      set ($w:vwidth)  [winfo reqwidth  $w.scrolled]

      # The size may have changed, e.g. by manual resizing of the window
      set _height     $($w:height)
      set _width      $($w:width)
      set ($w:height) [winfo height $w] ;# gives the actual height
      set ($w:width)  [winfo width  $w] ;# gives the actual width

      if {$force || $($w:vheight) != $_vheight || $($w:height) != $_height} {
        # resize the vertical scroll bar
        yview $w scroll 0 unit
        # yset $w
      }

      if {$force || $($w:vwidth) != $_vwidth || $($w:width) != $_width} {
        # resize the horizontal scroll bar
        xview $w scroll 0 unit
        # xset $w
      }
    } ;# end proc resize

    # --------------
    # xset proc
    #
    # resize the visible part
    # --------------
    # parm1: widget name
    # --------------
    proc xset {w} \
    {
      variable {}
      # call the xscroll command
      set cmd $($w:xscroll)
      if {$cmd != ""} { catch { eval $cmd [xview $w] } }
    }

    # --------------
    # yset proc
    #
    # resize the visible part
    # --------------
    # parm1: widget name
    # --------------
    proc yset {w} \
    {
      variable {}
      # call the yscroll command
      set cmd $($w:yscroll)
      if {$cmd != ""} { catch { eval $cmd [yview $w] } }
    }

    # -------------
    # xview
    #
    # called on horizontal scrolling
    # -------------
    # parm1: widget path
    # parm2: optional moveto or scroll
    # parm3: fraction if parm2 == moveto, count unit if parm2 == scroll
    # -------------
    # return: scrolling info if parm2 is empty
    # -------------
    proc xview {w {cmd ""} args} \
    {
      variable {}
      # check args
      set len [llength $args]
      switch -glob -- $cmd \
      {
        ""      {set args {}}
        mov*    \
        { if {$len != 1} { error "wrong # args: should be \"$w xview moveto\
			fraction\"" } }
        scr*    \
        { if {$len != 2} { error "wrong # args: should be \"$w xview scroll\
			count unit\"" } }
        default \
        { error "unknown operation \"$cmd\": should be empty,\
			moveto or scroll" }
      }
      # save old values:
      set _vleft $($w:vleft)
      set _vwidth $($w:vwidth)
      set _width  $($w:width)
      # compute new vleft
      set count ""
      switch $len \
      {
        0       \
        {
          # return fractions
          if {$_vwidth == 0} { return {0 1} }
          set first [expr {double($_vleft) / $_vwidth}]
          set last [expr {double($_vleft + $_width) / $_vwidth}]
          if {$last > 1.0} { return {0 1} }
          return [list $first $last]
        }
        1       \
        {
          # absolute movement
          set vleft [expr {int(double($args) * $_vwidth)}]
        }
        2       \
        {
          # relative movement
          foreach {count unit} $args break
          if {[string match p* $unit]} { set count [expr {$count * 9}] }
          set vleft [expr {$_vleft + $count * 0.1 * $_width}]
        }
      }
      if {$vleft + $_width > $_vwidth} { set vleft [expr {$_vwidth - $_width}] }
      if {$vleft < 0} { set vleft 0 }
      if {$vleft != $_vleft || $count == 0} \
      {
        set ($w:vleft) $vleft
        xset $w
        if {$($w:fillx) && ($_vwidth < $_width || $($w:xscroll) == "") } {
          # "scrolled object" is not scrolled, because it is too small or
	  # because no scrollbar was requested
          # fillx means that, in these cases, we must tell the object what
	  # its width should be
          place $w.scrolled -in $w -x [expr {-$vleft}] -width $_width
          if {$(debug,place)} { puts "place $w.scrolled -in $w -x\
		[expr {-$vleft}] -width $_width" } ;#DEBUG
        } else {
          place $w.scrolled -in $w -x [expr {-$vleft}] -width {}
          if {$(debug,place)} { puts "place $w.scrolled -in $w -x\
		[expr {-$vleft}] -width {}" } ;#DEBUG
        }

      }
    }

    # -------------
    # yview
    #
    # called on vertical scrolling
    # -------------
    # parm1: widget path
    # parm2: optional moveto or scroll
    # parm3: fraction if parm2 == moveto, count unit if parm2 == scroll
    # -------------
    # return: scrolling info if parm2 is empty
    # -------------
    proc yview {w {cmd ""} args} \
    {
      variable {}
      # check args
      set len [llength $args]
      switch -glob -- $cmd \
      {
        ""      {set args {}}
        mov*    \
        { if {$len != 1} { error "wrong # args: should be \"$w yview moveto\
		fraction\"" } }
        scr*    \
        { if {$len != 2} { error "wrong # args: should be \"$w yview scroll\
		count unit\"" } }
        default \
        { error "unknown operation \"$cmd\": should be empty,\
		moveto or scroll" }
      }
      # save old values
      set _vtop $($w:vtop)
      set _vheight $($w:vheight)
  #    set _height [winfo height $w]
      set _height $($w:height)
      # compute new vtop
      set count ""
      switch $len \
      {
        0       \
        {
          # return fractions
          if {$_vheight == 0} { return {0 1} }
          set first [expr {double($_vtop) / $_vheight}]
          set last [expr {double($_vtop + $_height) / $_vheight}]
          if {$last > 1.0} { return {0 1} }
          return [list $first $last]
        }
        1       \
        {
          # absolute movement
          set vtop [expr {int(double($args) * $_vheight)}]
        }
        2       \
        {
          # relative movement
          foreach {count unit} $args break
          if {[string match p* $unit]} { set count [expr {$count * 9}] }
          set vtop [expr {$_vtop + $count * 0.1 * $_height}]
        }
      }
      if {$vtop + $_height > $_vheight} {
	  set vtop [expr {$_vheight - $_height}]
      }
      if {$vtop < 0} { set vtop 0 }
      if {$vtop != $_vtop || $count == 0} \
      {
        set ($w:vtop) $vtop
        yset $w
        if {$($w:filly) && ($_vheight < $_height || $($w:yscroll) == "")} {
          # "scrolled object" is not scrolled, because it is too small or
	  # because no scrollbar was requested
          # filly means that, in these cases, we must tell the object what its
	  # height should be
          place $w.scrolled -in $w -y [expr {-$vtop}] -height $_height
          if {$(debug,place)} { puts "place $w.scrolled -in $w -y\
		[expr {-$vtop}] -height $_height" } ;#DEBUG
        } else {
          place $w.scrolled -in $w -y [expr {-$vtop}] -height {}
          if {$(debug,place)} { puts "place $w.scrolled -in $w -y\
		[expr {-$vtop}] -height {}" } ;#DEBUG
        }
      }
    }

  # end of ::scrolledframe namespace definition
  }

  package require Scrolledframe
  namespace import ::scrolledframe::scrolledframe

## End scrolled frame #########################################################

proc cut_copy_paste { w x y { c "" } } {
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
		$m add command -label "Clear" -command "clear_txt $w" \
			-state $st
	}

	tk_popup $m $x $y
}

proc clear_txt { w } {

	if [catch { $w configure -state normal }] {
		return
	}

	$w delete 1.0 end
	$w configure -state disabled
}

proc ece_inp { cs } {
#
# Preprocesses the input configuration transforming it into an array for
# easier access
#
	global ece_V ece_FS ece_ATT ESchemesD

	# preset with defaults
	foreach c $ESchemesD {
		set ece_V([lindex $c 0]) [lindex $c 1]
	}

	set er ""

	set it 0
	foreach c $cs {

		incr it

		set f [lindex $c 0]
		set l [lindex $c 1]

		if { $f == "font" } {
			if { [lsearch -exact $ece_FS $l] < 0 } {
				lappend er "item $it, illegal font size: $l"
			} else {
				set ece_V(font) $l
			}
			continue
		}

		if { $f == "commands" } {
			# this one is not verified
			set ece_V(commands) $l
			continue
		}

		if ![info exists ece_V($f)] {
			# ignore garbage
			lappend er "item $it, unknown face $f"
			continue
		}

		set ll [llength $l]
		if { $ll < 4 } {
			# garbage
			lappend er "item $it, too small number of items: $ll"
			continue
		}

		lassign $l like color alter backgr
		# the list of attributes
		set l [lrange $l 4 end]

		if { $like != "" && ($like == $f ||
		    ![info exists ece_V($like)]) } {
			lappend er "item $it, illegal like redirection\
				$f -> $like"
			continue
		}

		set bad 0

		foreach a { color alter backgr } {
			eval "set cc $[subst $a]"
			if { $cc != "" } {
				if [catch { valcol $cc } cc] {
					lappend er "item $it, $cc"
					set bad 1
					break
				}
				set $a $cc
			}
		}

		if $bad {
			continue
		}

		set al ""

		set bad 0
		foreach a $l {

			if { [lsearch -exact $ece_ATT $a] < 0 } {
				lappend er "item $it, illegal attribute $a"
				set bad 1
				break
			}

			lappend al $a
		}

		if $bad {
			continue
		}

		set ece_V($f) [concat [list $like $color $alter $backgr] $al]
	}

	return $er
}

proc ece_mkwindow { } {
#
# Creates the editor's dialog window
#
	global P ece_V ESchemesD ece_ATT ece_FS ece_CL SFont

	set w [md_window "Elvis configuration" 1]

	set cf "$w.cf"

	# create the ordered list of faces
	set ece_CL ""
	foreach f $ESchemesD {
		set f [lindex $f 0]
		if { $f != "font" && $f != "commands" } {
			lappend ece_CL $f
		}
	}

	labelframe $cf -text "Color scheme" -padx 4 -pady 4
	pack $cf -side top -expand yes -fill both

	scrolledframe $cf.sf -height 360 -width 360 \
        	-xscrollcommand "$cf.hs set" \
		-yscrollcommand "$cf.vs set" \
		-fill none
    	scrollbar $cf.vs -command "$cf.sf yview"
	scrollbar $cf.hs -command "$cf.sf xview" -orient horizontal
	grid $cf.sf -row 0 -column 0 -sticky nsew
	grid $cf.vs -row 0 -column 1 -sticky ns
	grid $cf.hs -row 1 -column 0 -sticky ew
	grid rowconfigure $cf 0 -weight 1
	grid columnconfigure $cf 0 -weight 1

	set cf "$cf.sf.scrolled"

	#######################################################################

	set rc 0
	label $cf.hfa -text "Face"
	grid $cf.hfa -column 0 -row $rc -sticky w -padx 4
	##
	label $cf.hlt -text "Linked to"
	grid $cf.hlt -column 1 -row $rc -sticky w -padx 4
	##
	label $cf.hco -text "Base color"
	grid $cf.hco -column 2 -row $rc -sticky w -padx 4
	##
	label $cf.hal -text "Alt color"
	grid $cf.hal -column 3 -row $rc -sticky w -padx 4
	##
	label $cf.hbg -text "Bgr color"
	grid $cf.hbg -column 4 -row $rc -sticky w -padx 4
	##
	label $cf.hxb -text "B"
	grid $cf.hxb -column 5 -row $rc -sticky w
	##
	label $cf.hxi -text "I"
	grid $cf.hxi -column 6 -row $rc -sticky w
	##
	label $cf.hxu -text "U"
	grid $cf.hxu -column 7 -row $rc -sticky w
	##
	label $cf.hxo -text "O"
	grid $cf.hxo -column 8 -row $rc -sticky w

	#######################################################################

	set ix 0
	foreach f $ece_CL {

		incr rc
		set vlist $ece_V($f)

		# element prefix for this row
		set p $cf.e$rc

		set el [label ${p}fa -text $f]
		grid $el -column 0 -row $rc -sticky w -padx 4

		# like
		set val [lindex $vlist 0]

		# selection: ece_CL - current
		set sel "- [join [lreplace $ece_CL $ix $ix]]"

		if { $val == "" } {
			set val "-"
		}
		set P(M1,$ix,LIK) $val
		set el "${p}lt"
		eval "set em \[tk_optionMenu $el P(M1,$ix,LIK) $sel\]"
		grid $el -column 1 -row $rc -sticky we -padx 4

		set ci 1
		foreach cd { co al bg } {

			set val [lindex $vlist $ci]
			# now this becomes the column number
			incr ci

			if { $val == "" } {
				set txt "none"
				set col "#FFFFFF"
			} else {
				set txt ""
				set col $val
			}
			set el "${p}$cd"
			button $el -text $txt -bg $col \
				-command "ece_colpick $w $ix $el"
			grid $el -column $ci -row $rc -sticky we -padx 4
		}

		# attributes
		set val [lrange $vlist 4 end]
		foreach cd $ece_ATT cu { xb xi xu xo } {
			incr ci
			if { [lsearch -exact $val $cd] >= 0 } {
				# present
				set P(M1,$ix,$cd) 1
			} else {
				set P(M1,$ix,$cd) 0
			}
			set el "${p}$cu"
			checkbutton $el -state normal -variable P(M1,$ix,$cd)
			grid $el -column $ci -row $rc -sticky w
		}
		incr ix
	}

	#######################################################################

	set cf "$w.ff"
	frame $cf
	pack $cf -side top -expand no -fill x

	set lf $cf.lf
	frame $lf
	pack $lf -side left -expand no -fill y

	labelframe $lf.fs -text "Font size" -padx 4 -pady 4
	pack $lf.fs -side top -expand no -fill none

	set el $lf.fs.fo
	set sel [join $ece_FS]
	eval "set em \[tk_optionMenu $el ece_V(font) $sel\]"
	pack $el -side top -expand no -fill x

	button $lf.cb -text "Cancel" -command "md_click -1 1"
	pack $lf.cb -side top -expand no -fill x

	button $lf.db -text "Done" -command "md_click 1 1"
	pack $lf.db -side top -expand no -fill x

	##
	set tf $cf.rf
	labelframe $tf -text "Extra commands" -padx 4 -pady 4
	pack $tf -side left -expand yes -fill both

	set P(M1,EC) $tf.t

	text $tf.t -font $SFont -state normal \
		-yscrollcommand "$tf.scrolly set" \
		-exportselection yes -width 54 -height 2

	scrollbar $tf.scrolly -command "$tf.t yview"

	pack $tf.t -side left -expand yes -fill both
	pack $tf.scrolly -side left -expand no -fill y

	$P(M1,EC) insert end $ece_V(commands)

	bind $P(M1,EC) <ButtonRelease-3> "cut_copy_paste %W %X %Y c"

	bind $w <Destroy> "md_click -1 1"
}

proc ece_gcommands { } {
#
# Extract the extra commands from the text entry
#
	global P ece_V

	set ece_V(commands) [string trim [$P(M1,EC) get 1.0 end]]
}

proc ece_colpick { w ix wi } {
#
# Pick a color, modify the entry, repaint the widget
#
	global ece_V ece_CL

	set cn ""
	# last two characters of the widget name determine the color type
	regexp "..$" $wi cn

	# the face
	set f [lindex $ece_CL $ix]

	# the list of values
	set vals $ece_V($f)

	set ci [lsearch -exact { co al bg } $cn]
	if { $ci < 0 } {
		# impossible
		return
	}

	# index into vals
	incr ci

	set col [lindex $vals $ci]
	if { $col == "" } {
		set col #000000
	}

	reset_all_menus 1
	set col [tk_chooseColor -parent $w -initialcolor $col -title \
	    "Choose [lindex { "" foreground alternate background } $ci] color"]
	reset_all_menus

	# replace the color in the list
	set ece_V($f) [lreplace $vals $ci $ci $col]

	if { $col == "" } {
		$wi configure -bg white -text "none"
	} else {
		$wi configure -bg $col -text ""
	}
}

proc ece_editor { inp } {
#
# This is the editor for Elvis color schemes
#
	global P ece_V ece_CL ESchemesD

	if { $P(AC) == "" } {
		return
	}

	set er [ece_inp $inp]

	if { $er != "" } {
		# errors, basically impossible and fixable, just warn
		alert "Errors in input configuration to scheme editor:\
			[join $er ,]"
	}

	ece_mkwindow

	while 1 {

		set ev [md_wait 1]

		if { $ev < 0 } {
			# cancellation, do some extra killing
			unset ece_CL
			array unset ece_V
			return ""
		}

		if { $ev > 0 } {
			# accepted
			set out ""
			ece_gcommands
			foreach c $ESchemesD {
				set c [lindex $c 0]
				lappend out [list $c $ece_V($c)]
			}
			unset ece_CL
			array unset ece_V
			md_stop 1
			return $out
		}
	}
}

###############################################################################

if { $ST(SYS) == "L" } {

###############################################################################
# Linux versions of bpcs functions ############################################
###############################################################################

proc kill_proc_by_id { id sig } {

	exec kill -$sig $id
}

proc kill_proc_by_name { name } {
#
# A desperate tool to kill something we have spawned, which has escaped, like
# gdbproxy, for instance
#
	if [catch { exec ps x } pl] {
		log "Cannot exec ps x: $pl"
		return
	}

	set pc 0
	foreach ln [split $pl "\n"] {
		if ![regexp "(\[0-9\]+).*:\[0-9\]+(.*)" $ln jnk pid cmd] {
			continue
		}
		if { [string first $name $cmd] >= 0 } {
			if [catch { exec kill -KILL $pid } err] {
				log "Cannot kill pcs $name <$pid>: $err"
			} else {
				log "Killed pcs name <$pid>"
			}
			incr pc
		}
	}
	if !$pc {
		log "Pcs $name not found"
	}
}

proc bpcs_run { path al pi } {
#
# Run a background program on Linux (use a pipe)
#
	global TCMD

	log "Running $path $al <$pi>"

	set cmd [fix_exec $path]

	if { $cmd == "" } {
		alert "Cannot start $path: not found on the path"
		return 1
	}

	foreach a $al {
		append cmd " [list $a]"
	}

	append cmd " 2>@1"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start $path: $fd"
		return 1
	}

	log "Process pipe: $fd"

	set TCMD($pi) $fd
	if { $TCMD($pi,AC) != "" } {
		$TCMD($pi,AC) 1
	}

	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "bpcs_check $pi"

	return 0
}

proc bpcs_check { pi } {
#
# Checks for the presence of a background process
#
	global TCMD

	if { [catch { read $TCMD($pi) } dummy] || [eof $TCMD($pi)] } {
		bpcs_kill $pi
	}
}

proc bpcs_kill { pi } {
#
# Kills a background process
#
	global TCMD

	if { $TCMD($pi) == "" } {
		return
	}

	kill_pipe $TCMD($pi) "KILL"

	set TCMD($pi) ""

	if { $TCMD($pi,AC) != "" } {
		# action after kill
		catch { $TCMD($pi,AC) 0 }
	}
}

} else {

###############################################################################
# Cygwin versions of bpcs functions ###########################################
###############################################################################

proc kill_proc_by_id { id sig } {

	exec kill -f -$sig $id
}

proc kill_proc_by_name { name } {
#
# A desperate tool to kill something we have spawned, which has escaped, like
# gdbproxy, for instance
#
	if [catch { exec ps -W } pl] {
		log "Cannot exec ps -W: $pl"
		return
	}

	set pc 0
	foreach ln [split $pl "\n"] {
		if ![regexp "(\[0-9\]+).*:..:..(.*)" $ln jnk pid cmd] {
			continue
		}
		if { [string first $name $cmd] >= 0 } {
			if [catch { exec kill -f $pid } err] {
				log "Cannot kill pcs $name <$pid>: $err"
			} else {
				log "Killed pcs name <$pid>"
			}
			incr pc
		}
	}
	if !$pc {
		log "Pcs $name not found"
	}
}

proc bpcs_run { path al pi } {
#
# Run a background Windows? program
#
	global TCMD BinPath

	# a simple escape; do we need more?
	regsub -all "\[ \t()\]" $path {\\&} path

	log "Running $path $al <$pi>"

	foreach a $al {
		append path " [list $a]"
	}

	if [catch { exec [file join $BinPath "bash"] -c "exec $path" & } pl] {
		alert "Cannot execute $path: $pl"
		return 1
	}

	log "Process ID: $pl"

	set TCMD($pi) $pl
	if { $TCMD($pi,AC) != "" } {
		$TCMD($pi,AC) 1
	}

	bpcs_check $pi
	return 0
}

proc bpcs_check { pi } {
#
# Checks for the presence of a background process
#
	global TCMD

	if { $TCMD($pi) == "" } {
		set TCMD($pi,CB) ""
		return
	}

	if [catch { exec kill -0 $TCMD($pi) } ] {
		# gone
		log "Background process $TCMD($pi) gone"
		set TCMD($pi) ""
		set TCMD($pi,CB) ""
		if { $TCMD($pi,AC) != "" } {
			$TCMD($pi,AC) 0
		}
		return
	}

	set TCMD($pi,CB) [after 1000 "bpcs_check $pi"]
}

proc bpcs_kill { pi } {
#
# Kills a background process (monitored by a callback)
#
	global TCMD

	if { $TCMD($pi) == "" } {
		set TCMD($pi,CB) ""
		return
	}

	if [catch { exec kill -$TCMD($pi,SI) $TCMD($pi) } err] {
		# something wrong?
		log "Cannot kill process $TCMD($pi): $err"
	}

	set TCMD($pi) ""
	if { $TCMD($pi,CB) != "" } {
		catch { after cancel $TCMD($pi,CB) }
		set TCMD($pi,CB) ""
	}
	if { $TCMD($pi,AC) != "" } {
		# action after kill
		catch { $TCMD($pi,AC) 0 }
	}
}

###############################################################################
}
###############################################################################

proc run_genimage { { fast 0 } } {
#
	global P TCMD

	if { $P(AC) == "" || $P(CO) == "" } {
		# no project
		return
	}

	if { $TCMD(FG) != "" } {
		if !$auto {
			alert "Genimage appears to be running already"
		}
		return
	}

	set cmd [fix_exec "genimage"]
	if { $cmd == "" } {
		alert "Cannot start genimage: not found on the PATH"
		return
	}

	if $fast {
		append cmd " -F"
	}

	append cmd " -C 2>@1"

	log "Running $cmd"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start genimage: $fd"
		return
	}

	set TCMD(FG) $fd
	reset_exec_menu

	# nothing will ever arrive on this pipe; we use it to
	# find out when the script exits
	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "genimage_pipe_event"
}

proc genimage_pipe_event { } {
#
# Detect when the script exits
#
	global TCMD

	if { [catch { read $TCMD(FG) } dummy] || [eof $TCMD(FG)] } {
		stop_genimage
	}
}

proc stop_genimage { } {
#
	global TCMD

	if { $TCMD(FG) != "" } {
		kill_pipe $TCMD(FG)
		set TCMD(FG) ""
		reset_exec_menu
	}
}

###############################################################################

proc run_udaemon { { auto 0 } } {
#
	global P TCMD

	if { $P(AC) == "" || $P(CO) == "" } {
		# no project
		return
	}

	if { $TCMD(FU) != "" } {
		if !$auto {
			alert "Udaemon appears to be running already"
		}
		return
	}

	set cmd [fix_exec "udaemon"]
	if { $cmd == "" } {
		alert "Cannot start udaemon: not found on the PATH"
		return
	}

	if ![catch { valport [dict get $P(CO) "PFAC"] } po] {
		append cmd " $po"
	}

	if [dict get $P(CO) "YCDN"] {
		append cmd " -R"
	}

	if [dict get $P(CO) "UDTM"] {
		append cmd " -T"
	}

	set udpl [dict get $P(CO) "UDPL"]
	if { $udpl != "" } {
		append cmd " -P $udpl"
	}

	append cmd " 2>@1"

	log "Pipe: $cmd"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start udaemon: $fd"
		return
	}

	set TCMD(FU) $fd
	reset_exec_menu

	# nothing will ever arrive on this pipe; we use it to
	# find out when udaemon exits
	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "udaemon_pipe_event"
}

proc udaemon_pipe_event { } {
#
# Detect when udaemon exits
#
	global TCMD

	if { [catch { read $TCMD(FU) } dummy] || [eof $TCMD(FU)] } {
		stop_udaemon
	}
}

proc stop_udaemon { } {
#
	global TCMD

	if { $TCMD(FU) != "" } {
		kill_pipe $TCMD(FU)
		set TCMD(FU) ""
		reset_exec_menu
	}
}

proc run_oss { { auto 0 } { vuee 0 } } {
#
	global P TCMD

	if { $P(AC) == "" || $P(CO) == "" } {
		# no project
		return
	}

	if { $auto == 2 } {
		# just run it as a command
		run_term_command "oss" { -H ossi.h } gfl_tree
		return
	}

	if { $TCMD(FO) != "" } {
		if !$auto {
			alert "Oss appears to be running already"
		}
		return
	}

	set cmd [fix_exec "oss"]
	if { $cmd == "" } {
		alert "Cannot start oss: not found on the PATH"
		return
	}

	if $vuee {
		append cmd " -V"
		if [dict get $P(CO) "OSON"] {
			# use the parameters from VUEE config
			append cmd " "
			if [dict get $P(CO) "OSNH"] {
				append cmd "h"
			}
			append cmd [dict get $P(CO) "OSNN"]
		}
	} else {
		append cmd " -U"
	}

	append cmd " 2>@1"

	log "Pipe: $cmd"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start oss: $fd"
		return
	}

	set TCMD(FO) $fd
	reset_exec_menu

	# nothing will ever arrive on this pipe; we use it to
	# find out when the script exits
	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "oss_pipe_event"
}

proc oss_pipe_event { } {
#
# Detect when the oss script exits
#
	global TCMD

	if { [catch { read $TCMD(FO) } dummy] || [eof $TCMD(FO)] } {
		stop_oss
	}
}

proc stop_oss { } {
#
	global TCMD

	if { $TCMD(FO) != "" } {
		kill_pipe $TCMD(FO)
		set TCMD(FO) ""
		reset_exec_menu
	}
}

proc side_args { deb } {
#
# Build the arguments for side invocation
#
	global P

	# the data file
	set df [dict get $P(CO) "VUDF"]
	if { $df == "" } {
		alert "No data file specified for the model. Configure VUEE\
			first"
		return ""
	}

	# check if the data file exists
	if ![file_present $df] {
		alert "The data file $df does not exist"
		return ""
	}

	set of [dict get $P(CO) "VUOF"]

	set argl [list "-e" $df]
	set argm ""

	if { $of != "" } {
		lappend argl $of
	} elseif !$deb {
		# write to stderr, if output directed to console
		lappend argl "+"
	}

	set ri [dict get $P(CO) "VURI"]

	if { [catch { expr $ri } ri] || $ri < 5 } {
		# force the default in case of any trouble
		set ri 500
	}

	set df [dict get $P(CO) "VUSM"]
	if { $df == "U" } {
		# unsynced
		set df 0
	} else {
		if { [catch { expr $df } df] || $df <= 0.0 } {
			# force the default in case of any trouble
			set df 1.0
		}
	}

	set ea [dict get $P(CO) "EARG"]
	if { $ea != "" } {
		set wh 0
		foreach aa [split $ea] {
			if { $aa == "--" } {
				set wh 1
			} elseif { $wh } {
				lappend argm $aa
			} else {
				lappend argl $aa
			}
		}
	}

	# locate default node data in board directories

	set dp [dict get $P(CO) "DPBC"]
	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set po [dict get $P(CO) "PFAC"]
	set bd [dict get $P(CO) "LD"]

	if { $dp == 0 && $mb != "" && $bo != "" } {
		if $mb {
			# multiple boards
			set bi 0
			foreach b $bo {
				if { $b != "" && ![blindex $bd $bi] } {
					set suf [lindex $P(PL) $bi]
					set fna [file join [boards_dir] $b \
						"node.xml"]
					if [file isfile $fna] {
						lappend argm "-n"
						lappend argm $suf
						lappend argm [unipath $fna]
					}
				}
				incr bi
			}
		} else {
			set fna [file join [boards_dir] $bo "node.xml"]
			if [file isfile $fna] {
				lappend argm "-n"
				lappend argm [unipath $fna]
			}
		}
	}

	# slo-mo/resync
	if { $df > 0 } {
		# scale resync interval to slo-mo
		set ef [expr int($df * $ri)]
		if { $ef <= 0 } {
			set ef 1
		} elseif { $ef > 1000 } {
			set ef 1000
		}
	}
	lappend argm "-s"
	lappend argm $df
	if { $df > 0 } {
		lappend argm "-r"
		lappend argm $ef
	}

	if ![catch { valport $po } po] {
		lappend argm "-p"
		lappend argm $po
	}

	if { $argm != "" } {
		lappend argl "--"
		set argl [concat $argl $argm]
	}

	return $argl
}

proc run_vuee { { deb 0 } } {
#
# Run the model; the arg: 0 - normal, 1 - debug, 2 - debug + go
#
	global P TCMD SIDENAME

	if { $P(AC) == "" || $P(CO) == "" } {
		# no project
		return
	}

	if { $TCMD(FD) != "" } {
		# This shouldn't be possible
		alert "Term window busy running a command. Abort it first"
		return
	}

	if ![file_present $SIDENAME] {
		# Nor should this
		alert "No VUEE model executable. Build it first"
		return
	}

	if $deb {
		# debugger run, remove proxy files
		catch { exec rm -f ".gdbinit" }
		catch { exec rm -f "gdb.ini" }
		# start the debugger
		if [catch { run_term_command "gdb" [list $SIDENAME] \
		    "" "" 1 } err] {
			alert "Cannot execute gdb: $err"
			return
		}
		if { $deb < 2 } {
			# just the debugger, the user will do the rest; ignore
			# udaemon in this case
			return
		}
		# go, i.e., issue a command to start the model, in this case
		# we will also run udaemon, if prescribed by config
		set args [side_args 1]
		if { $args == "" } {
			# error, diagnosed by side_args
			return
		}
		set args "run [join $args]"
		log "debug go: $args"
		catch { puts $TCMD(FD) $args }
		catch { flush $TCMD(FD) }
	} else {
		# regular run, no debugger
		set args [side_args 0]
		if { $args == "" } {
			# error
			return
		}

		if [catch { run_term_command "./$SIDENAME" $args } err] {
			alert "Cannot start the model: $err"
			return
		}
	}

	stop_udaemon
	stop_oss
	delay 500

	if { [dict get $P(CO) "UDON"] && $TCMD(FU) == "" } {
		# start udaemon
		run_udaemon 1
	}

	if { [dict get $P(CO) "OSON"] && $TCMD(FO) == "" } {
		# start oss
		run_oss 1 1
	}
}

proc run_term_command { cmd al { ea "" } { aa "" } { ni 0 } } {
#
# Run a command in term window:
#
#	cmd - the command
#	al  - argument list
#	ea  - end action, i.e., a statement to execute after completion
#	aa  - abort action, i.e., a statement to execute after abort
#	ni  - need input (the pipe should be opened rw with input line enabled)
#
	global TCMD

	if { $TCMD(FD) != "" } {
		error "Already running a command. Abort it first"
	}

	log "Running $cmd $al"

	set ef [fix_exec $cmd]
	if { $ef == "" } {
		error "program $cmd is not executable"
	}

	set cmd $ef

	foreach a $al {
		append cmd " [list $a]"
	}

	# stderr to stdout
	append cmd " 2>@1"

	if $ni {
		set ff "r+"
	} else {
		set ff "r"
	}

	if [catch { open "|$cmd" $ff } fd] {
		error "Cannot execute $cmd, $fd"
	}

	# command started
	set TCMD(FD) $fd
	set TCMD(SH) $ni
	set TCMD(BF) ""
	mark_running 1
	reset_bnx_menus

	set TCMD(EA) $ea
	set TCMD(AA) $aa

	log "Pipe: $cmd <$fd, $ni, $ff, $ea, $aa>"

	fconfigure $fd -blocking 0 -buffering none -eofchar "" -translation lf
	fileevent $fd readable "term_output"
}

proc kill_pipe { fd { sig "KILL" } { stay "" } } {
#
# Kills the process on the other end of our pipe
#
	if { $fd == "" || [catch { pid $fd } pp] || $pp == "" } {
		return
	}
	foreach p $pp {
		log "Killing <$sig> pipe $fd process $p"
		if [catch { kill_proc_by_id $p $sig } err] {
			log "Cannot kill $p: $err"
		}
	}
	if { $stay == "" } {
		catch { close $fd }
	}
}

proc abort_term { } {

	global TCMD

	if { $TCMD(FD) != "" } {
		kill_pipe $TCMD(FD)
		set TCMD(FD) ""
		set TCMD(BF) ""
		# chain action is ignored after abort
		set TCMD(EA) ""
		set aa $TCMD(AA)
		set TCMD(AA) ""
		term_dspline "--ABORTED--"
		mark_running 0
		if { $aa != "" } {
			eval $aa
		}
		# may fail if the master window has been destroyed already
		catch { reset_bnx_menus }
	}
}

proc stop_term { } {

	global TCMD

	set ea ""
	if { $TCMD(FD) != "" } {
		kill_pipe $TCMD(FD)
		set TCMD(FD) ""
		set TCMD(BF) ""
		set ea $TCMD(EA)
		set TCMD(EA) ""
		set TCMD(AA) ""
		reset_bnx_menus
	}
	mark_running 0
	term_dspline "--DONE--"
	if { $ea != "" } {
		eval $ea
	}
}

###############################################################################
###############################################################################

proc run_sa { } {

	global P TCMD SACmd

	if { $P(AC) == "" || $TCMD(SA) != "" } {
		# impossible
		return
	}

	set cmd [fix_exec $SACmd]
	if { $cmd == "" } {
		alert "Cannot run spectrum analyzer, not installed"
		return
	}

	append cmd " -C config.san 2>@1"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start spectrum analyzer: $fd"
		return
	}

	set TCMD(SA) $fd
	reset_exec_menu
	
	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "sa_pipe_event"
}

proc sa_pipe_event { } {

	global TCMD

	if { [catch { read $TCMD(SA) } dummy] || [eof $TCMD(SA)] } {
		stop_sa
	}
}

proc stop_sa { } {

	global TCMD

	if { $TCMD(SA) != "" } {
		kill_pipe $TCMD(SA)
		set TCMD(SA) ""
		reset_exec_menu
	}
}

###############################################################################

proc run_piter { } {

	global P TCMD PiterCmd

	if { $P(AC) == "" } {
		return
	}

	# find the first free slot
	for { set p 0 } { $p < $TCMD(NPITERS) } { incr p } {
		if { $TCMD(PI$p) == "" } {
			break
		}
	}

	if { $p == $TCMD(NPITERS) } {
		# ignore, this should never happen
		alert "No more piters, kill some"
		return
	}

	set cmd [fix_exec $PiterCmd]
	if { $cmd == "" } {
		alert "Cannot start piter: not found on the PATH"
		return
	}

	set th [expr $TCMD(CPITERS) + 1]
	append cmd " -C config.pit -T $th 2>@1"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Cannot start piter: $fd"
		return
	}

	set TCMD(PI$p) $fd
	set TCMD(CPITERS) $th
	set TCMD(PI$p,SN) $th
	reset_exec_menu

	# we may want to show this output?
	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "piter_pipe_event $p"
}

proc piter_pipe_event { p } {

	global TCMD

	if { [catch { read $TCMD(PI$p) } dummy] || [eof $TCMD(PI$p)] } {
		stop_piter $p
	}
}

proc stop_piter { { w "" } } {

	global TCMD

	for { set p 0 } { $p < $TCMD(NPITERS) } { incr p } {
		if { ($w == "" || $p == $w) && $TCMD(PI$p) != "" } {
			kill_pipe $TCMD(PI$p)
			set TCMD(PI$p) ""
			set TCMD(PI$p,SN) 0
			reset_exec_menu
		}
	}
}
	
###############################################################################

proc mk_run_pgm_window { } {

	global P FFont

	set w [md_window "Enter command to run"]

	set f $w.f
	labelframe $f -text "Command to run" -padx 2 -pady 2
	pack $f -side top -expand n -fill x

	set te [entry $f.e -width 38 -font $FFont -textvariable P(M0,cm)]
	pack $f.e -side top -expand n -fill x

	set b $w.b
	frame $b
	pack $b -side top -expand n -fill x

	button $b.cb -text "Cancel" -command "md_click -1"
	pack $b.cb -side left -expand n

	button $b.eb -text "Execute" -command "md_click 1"
	pack $b.eb -side right -expand n

	bind $w <Destroy> "md_click -1"
	bind $te <Return> "md_click 1"
}

proc run_xterm { } {

	global P

	set xtc [dict get $P(CO) "OPTTRMCMND"]

	set pgm [lindex $xtc 0]
	set arg ""
	foreach a [lrange $xtc 1 end] {
		lappend arg $a
	}
	lappend arg "&"

	catch { xq $pgm $arg }
}

proc run_explorer { { wd "" } } {

	global EXCmd ST

	if { $EXCmd == "" } {
		# not available
		alert "No file explorer available on this system"
		return
	}

	if { $wd == "" } {
		set wd [pwd]
	}

	if { $ST(SYS) == "L" } {
		set args [list $wd "&"]
	} else {
		set args [list [dospath $wd] "&"]
	}

	catch { xq $EXCmd $args }
}

proc run_any_program { } {
#
# Executes any program in the console
#
	global P TCMD CFXecItems

	if { $P(AC) == "" } {
		return
	}

	params_to_dialog $CFXecItems
	set P(M0,cm) $P(M0,XELPGM)

	mk_run_pgm_window

	while 1 {

		set ev [md_wait]
		if { $ev < 0 } {
			# cancelled
			return
		}

		if { $ev == 1 } {
			# accepted
			if { $P(M0,cm) == "" } {
				alert "Empty program to run"
				continue
			}

			if [regexp "\[<>&@\]" $P(M0,cm)] {
				alert "Redirections not allowed"
				continue
			}
			set P(M0,XELPGM) $P(M0,cm)
			set cmd $P(M0,cm)
		}

		dialog_to_params $CFXecItems
		md_stop
		set_config
		break
	}

	if { $TCMD(FD) != "" } {
		# cannot happen
		alert "Console busy, wait or kill and try again"
		return
	}

	if [catch { run_term_command $cmd "" "" "" 1 } err] {
		alert "Cannot run $cmd: $err"
	}
}

proc do_console_input { } {
#
# Handles input into the bottom line
#
	global TCMD TEntry

	set tx ""

	regexp "\[^\r\n\]+" [$TEntry get 0.0 end] tx
	$TEntry delete 0.0 end

	if { $TCMD(FD) != "" } {
		# this means that the console is busy
		if { $TCMD(SH) != 1 } {
			# this means that we got here by accident
			return
		}
		# we write the line to the pipe
		log "Console input: $tx"
		catch { puts $TCMD(FD) $tx }
		catch { flush $TCMD(FD) }
	}
}

proc do_console_interrupt { } {
#
# Sends sigint to the console process
#
	global TCMD

	if { $TCMD(FD) != "" && $TCMD(SH) == 1 } {
		log "Console interrupt"
		kill_pipe $TCMD(FD) "INT" STAY
	}
}

proc do_console_save { } {

	global ST P LCSFile

	if { $ST(CF) != "" } {
		# stop saving
		catch { close $ST(CF) }
		set ST(CF) ""
		set_csbut_label
		return
	}

	set lf [fpnorm $LCSFile]

	while 1 {

		set fn [tk_getSaveFile \
			-defaultextension ".txt" \
			-parent "." \
			-title "Save file name" \
			-initialfile [file tail $lf] \
			-initialdir [file dirname $lf]]

		if { $fn == "" } {
			# cancelled
			return
		}

		if [catch { open $fn "w" } std] {
			alert "Cannot open file $fn, $std"
			continue
		}

		fconfigure $std -buffering line
		set ST(CF) $std

		set LCSFile $fn
		set_rcoption "LCSFile"

		set_csbut_label
		return
	}
}

proc set_csbut_label { } {

	global ST CSBut

	if { $ST(CF) == "" } {
		set lab "Save"
	} else {
		set lab "Stop"
	}

	$CSBut configure -text $lab
}

###############################################################################

proc clean_lprojects { } {

	global LProjects

	set LProjects ""
	reset_file_menu
}

proc reset_file_menu { { clear 0 } } {
#
# Create the File menu of the project window; it must be done dynamically,
# because it depends on the list of recently opened projects
#
	global LProjects P EXCmd

	set m .menu.file

	if [catch { $m delete 0 end } ] {
		return
	}

	if $clear {
		return
	}

	if { $P(AC) == "" } {
		set st "disabled"
	} else {
		set st "normal"
	}

	$m add command -label "Open project ..." -command "open_project"
	$m add command -label "New project ..." -command "new_project"
	$m add command -label "Clone project ..." -command "clone_project"
	$m add command -label "Close project ..." -command "close_project" \
		-state $st
	$m add separator

	# validate LProjects
	if { $LProjects != "" } {
		set new ""
		foreach p $LProjects {
			if [val_prj_dir [fpnorm $p] 1] {
				lappend new $p
			}
		}
		if { $LProjects != $new } {
			set LProjects $new
			set_rcoption LProjects
		}
	}

	if { $LProjects != "" } {
		set ix 0
		foreach p $LProjects {
			# this is a full file path, use the last so many
			# characters
			set p [trunc_fname $p]
			$m add command -label $p -command "open_project $ix"
			incr ix
		}
		$m add separator
		$m add command -label "Clear history" -command clean_lprojects
		$m add separator
	}

	$m add command -label "Quit" -command "terminate"
	$m add separator

	$m add command -label "Edit" -command open_multiple -state $st
	set aed [alt_editor]
	if { $aed != "" } {
		$m add command -label "Edit Alt" -command open_multi_ate -state $st
	}
	$m add command -label "Delete" -command delete_multiple -state $st
	$m add command -label "Rename ..." -command "rename_file" -state $st
	$m add command -label "New file ..." -command "new_file" -state $st
	$m add command -label "Copy from ..." -command "copy_from" -state $st
	$m add command -label "Copy to ..." -command "copy_to" -state $st
	$m add command -label "New directory ..." -command "new_directory" \
		-state $st

	$m add separator

	$m add command -label "Run terminal" \
		-command "run_xterm_here" -state $st
	if { $EXCmd != "" } {
		$m add command -label "Run File Explorer" \
			-command "run_explorer_here" -state $st
	}

	$m add separator

	$m add command -label "Search ..." -command "open_search_window" \
		-state $st
}

proc reset_config_menu { { clear 0 } } {
#
# Re-create the configuration menu (after closing/opening a project)
#
	global P ST

	set m .menu.config

	if [catch { $m delete 0 end } ] {
		return
	}

	if { $clear || $P(AC) == "" } {
		# nothing to show
		return
	}

	# Used to be called CPU+Board, let's be a bit more exacting this time
	$m add command -label "Arch+Board ..." -command "do_board_selection"

	if $ST(VP) {
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Extra defines ..." -command "do_extra_defines"
	$m add command -label "VUEE ..." -command "do_vuee_config" -state $st
	$m add command -label "Loaders ..." -command "do_loaders_config"

	set bo [board_set]
	if { [dict get $P(CO) "OPSYSFILES"] != 0 && $bo != "" && !$ST(LO) } {
		# add a build board library menu
		$m add separator
		foreach b $bo {
			$m add command -label "Create lib for $b" \
				-command "do_makelib $b"
		}

		if { [llength $bo] > 1 } {
			$m add separator
			$m add command -label "Create libs for all" \
				-command "do_makelib_all"
		}
	}

	$m add separator
	$m add command -label "Editor schemes ..." -command "do_eschemes"
	$m add command -label "Options ..." -command "do_options"
}

proc scdir_present { { suf "" } } {
#
# Checks if a directory for soft cleaning is present
#
	global SoftCleanDirs

	if { $suf != "" } {
		set suf "_$suf"
	}

	foreach d $SoftCleanDirs {
		set d $d$suf
		if [file isdirectory $d] {
			# check if nonempty
			set sdl [glob -nocomplain -directory $d -tails *]
			if { $sdl != "" } {
				return 1
			}
		}
	}

	return 0
}

proc reset_build_menu { { clear 0 } } {
#
# Re-create the build menu; called whenever something has changed that may
# affect some items on the menu
#
	global P TCMD ST

	set m .menu.build
	if [catch { $m delete 0 end } ] {
		return
	}

	if { $clear || $P(AC) == "" || $P(CO) == "" } {
		# no project, no selection, no build
		return
	}

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set bm [dict get $P(CO) "LM"]
	set bd [dict get $P(CO) "LD"]

	if { $mb != "" && $bo != "" } {
		# mkmk is applicable
		if $mb {
			# multiple boards
			set bi 0
			set nb 0
			foreach b $bo {
				if { $b != "" && ![blindex $bd $bi] } {
					set suf [lindex $P(PL) $bi]
					set lm [blindex $bm $bi]
					if { $ST(LO) || $lm } {
						# library only or lib mode
						set lm "lib"
					} else {
						set lm "src"
					}
					$m add command -label \
					    "Pre-build $suf ($b $lm)" -command \
					    "do_mkmk_node $bi sys_make_ctags"
					incr nb
				}
				incr bi
			}
			$m add separator
			if { $nb > 1 } {
				$m add command -label "Pre-build all" -command \
				"do_mkmk_all"
				$m add separator
			}
			set bi 0
			set nb 0
			foreach b $bo {
				if { $b != "" && ![blindex $bd $bi] } {
					set suf [lindex $P(PL) $bi]
					$m add command -label \
						"Build $suf (make)" \
						-command "do_make_node $bi"
					incr nb
				}
				incr bi
			}
			$m add separator
			if { $nb > 1 } {
				$m add command -label "Build all" \
				-command "do_make_all"
				$m add separator
			}
		} else {
			set lm [blindex $bm 0]
			set bo [lindex $bo 0]
			if { $ST(LO) || $lm } {
				set lm "lib"
			} else {
				set lm "src"
			}
			$m add command -label "Pre-build ($bo $lm)" \
				-command "do_mkmk_node 0 sys_make_ctags"
			$m add command -label "Build (make)" \
				-command "do_make_node 0"
			$m add separator
		}
	}

	if [oss_available] {
		$m add command -label "Generate ossi.h" -command "run_oss 2 0" \
			-state normal
		$m add separator
	}

	if [vuee_disabled] {
		set st "disabled"
	} else {
		set st "normal"
	}

	$m add command -label "VUEE" -state $st \
		-command "do_make_vuee"
	$m add command -label "VUEE (debug)" -state $st \
		-command "do_make_vuee {} { -g }"
	# $m add command -label "VUEE (recompile)" -state $st  -command "do_make_vuee -e"
	$m add command -label "VUEE (status)" -state $st \
		-command "do_make_vuee { -e -n } {}"
	$m add separator

	$m add command -label "Clear (full)" -command "do_cleanup"

	if { $TCMD(FD) != "" || [glob -nocomplain "Makefile*"] == "" } {
		set st "disabled"
	} else {
		set st "normal"
	}

	if { $mb != "" && $bo != "" } {
		if $mb {
			$m add separator
			set bi 0
			foreach n $bo {
				if { $n != "" && ![blindex $bd $bi] } {
					set suf [lindex $P(PL) $bi]
					if [scdir_present $suf] {
						set st "normal"
					} else {
						set st "disabled"
					}
					$m add command -label \
						"Clear (light, $suf)" \
						-state $st \
						-command "do_clean_light $bi"
				}
				incr bi
			}
		} else {
			if [scdir_present] {
				set st "normal"
			} else {
				set st "disabled"
			}
			$m add command -label "Clear (soft)" -state $st \
				-command "do_clean_light"
		}
	}

	$m add separator

	if { $TCMD(FD) != "" } {
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Abort" -command "abort_term" -state $st
}

proc reset_exec_menu { { clear 0 } } {
#
# Re-create the exec menu
#
	global P SIDENAME TCMD SACmd EXCmd

	set m .menu.exec
	if [catch { $m delete 0 end } ] {
		# destroyed already
		return
	}

	if { $clear || $P(AC) == "" } {
		return
	}

	if { [glob -nocomplain "Image*"] == "" } {
		set st "disabled"
	} else {
		set st "normal"
	}

	if !$TCMD(FY) {
		$m add command -label "Upload image ..." \
			-command upload_image -state $st
	} else {
		$m add command -label "Terminate loader" \
			-command "stop_loader" -state $st
	}

	if { $TCMD(FG) == "" } {
		$m add command -label "Customize image ..." \
			-command run_genimage -state $st
		$m add command -label "Customize image (prev)" \
			-command "run_genimage 1" -state $st
	} else {
		$m add command -label "Stop genimage" -command stop_genimage
	}

	$m add separator

	if { ![vuee_disabled] && $TCMD(FD) == "" && [file_present $SIDENAME] } {
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Run VUEE" -command run_vuee -state $st
	$m add command -label "Run VUEE (debug)" -command "run_vuee 1" \
		-state $st
	$m add command -label "Run VUEE (debug+go)" -command "run_vuee 2" \
		-state $st

	if { $TCMD(FD) == "" } {
		set st "disabled"
	} else {
		set st "normal"
	}
	$m add command -label "Abort" -command "abort_term" -state $st

	$m add separator

	if { $P(AC) == "" || ![file_present $SIDENAME] } {
		set st "disabled"
	} else {
		set st "normal"
	}

	if { $TCMD(FU) == "" } {
		$m add command -label "Run udaemon" -command run_udaemon \
			-state $st
	} else {
		$m add command -label "Stop udaemon" -command stop_udaemon
	}

	if { $TCMD(FO) == "" } {
		if [oss_available] {
			$m add command -label "Run oss (real)" \
				-command "run_oss 0 0" -state normal
			if ![dict get $P(CO) "VDISABLE"] {
				$m add command -label "Run oss (VUEE)" \
					-command "run_oss 0 1" -state $st
			}
		}
	} else {
		$m add command -label "Stop oss" -command stop_oss
	}

	$m add separator

	set f 0
	for { set p 0 } { $p < $TCMD(NPITERS) } { incr p } {
		if { $TCMD(PI$p) == "" } {
			incr f
		}
	}

	if { $P(AC) != "" } {
		set st "normal"
	} else {
		set st "disabled"
	}

	if $f {
		# room for more piters
		$m add command -label "Start piter" -command run_piter \
			-state $st
	}

	if { $f < $TCMD(NPITERS) } {
		for { set p 0 } { $p < $TCMD(NPITERS) } { incr p } {
			if { $TCMD(PI$p) != "" } {
				$m add command \
					-label "Stop piter $TCMD(PI$p,SN)" \
					-command "stop_piter $p"
			}
		}
	}

	$m add separator
	if { $TCMD(FD) == "" } {
		set st "normal"
	} else {
		set st "disabled"
	}
	$m add command -label "Run program" -command run_any_program -state $st
	$m add command -label "Terminal" -command run_xterm
	if { $EXCmd != "" } {
		$m add command -label "File Explorer" -command run_explorer
	}

	$m add separator

	if { $TCMD(SA) == "" } {
		if { [auto_execok $SACmd] == "" } {
			set st "disabled"
		} else {
			set st "normal"
		}
		$m add command -label "Run spectrum analyzer" \
			-command run_sa -state $st
	} else {
		$m add command -label "Stop spectrum analyzer" \
			-command stop_sa
	}

	$m add separator
	$m add command -label "Clear console" -command term_clean
}

proc reset_bnx_menus { { clear 0 } } {

	reset_build_menu $clear
	reset_exec_menu $clear
}

proc reset_all_menus { { clear 0 } } {

	reset_file_menu $clear
	reset_config_menu $clear
	reset_bnx_menus $clear
}

proc mark_running_tm { } {

	global P TCMD

	set P(SSV) [format "%3d" $TCMD(CL)]
}

proc mark_running { stat } {

	global P TCMD TEntry

	if $stat {
		# running
		if { $TCMD(CB) != "" } {
			# the callback is active
			return
		}
		if $TCMD(SH) {
			# the command needs input
			$TEntry configure -state normal -relief groove -bg gray
			log "Enabled manual input"
		}
		set TCMD(CL) 0
		set P(SSL) "Running: "
		mark_running_tm
		set TCMD(CB) [after 1000 mark_running_cb]
		return
	}

	if { $TCMD(CB) != "" } {
		after cancel $TCMD(CB)
		set TCMD(CB) ""
	}

	set P(SSL) "Idle:"
	$TEntry configure -state disabled -relief sunken -bg white
	log "Disabled manual input"
}

proc mark_running_cb { } {

	global TCMD

	incr TCMD(CL)
	mark_running_tm
	set TCMD(CB) [after 1000 mark_running_cb]
}

proc mk_project_window { } {

	global P ST Term TEntry FFont CSBut

	# when a project is open, this shows the directory path; also used to
	# tell if a project is currently open
	set P(AC) ""
	# no configuration
	set P(CO) ""

	wm title . "PIP $ST(VER) (ZZ000000A)"

	menu .menu -tearoff 0

	#######################################################################

	set m .menu.file
	menu $m -tearoff 0

	.menu add cascade -label "File" -menu $m -underline 0

	reset_file_menu

	#######################################################################

	set m .menu.config
	menu $m -tearoff 0

	.menu add cascade -label "Configuration" -menu $m -underline 0

	#######################################################################

	set m .menu.build
	menu $m -tearoff 0

	.menu add cascade -label "Build" -menu $m -underline 0

	#######################################################################

	set m .menu.exec
	menu $m -tearoff 0

	.menu add cascade -label "Execute" -menu $m -underline 0

	#######################################################################

	reset_bnx_menus

	#######################################################################

	. configure -menu .menu

	#######################################################################

	panedwindow .pane
	pack .pane -side top -expand y -fill both

	frame .pane.left
	pack .pane.left -side left -expand y -fill both

	frame .pane.left.sf
	pack .pane.left.sf -side top -expand n -fill x

	label .pane.left.sf.ss -textvariable P(SSL) -background white \
		-justify left -anchor w
	pack .pane.left.sf.ss -side left -expand y -fill x

	label .pane.left.sf.sv -textvariable P(SSV) -background white \
		-justify right -anchor e
	pack .pane.left.sf.sv -side right -expand y -fill x

	set cn .pane.left

	#######################################################################

	ttk::treeview $cn.tree 	-columns { filename type } \
				-displaycolumns {} \
				-yscroll "$cn.vsb set" \
				-xscroll "$cn.hsb set" \
				-show tree

	pack $cn.tree -side top -expand y -fill both

	ttk::scrollbar $cn.vsb -orient vertical -command "$cn.tree yview"
	ttk::scrollbar $cn.hsb -orient horizontal -command "$cn.tree xview"

	$cn.tree column \#0 -stretch 1 -width 120

	bind $cn.tree <<TreeviewOpen>> { gfl_open %W [%W focus] }
	bind $cn.tree <<TreeviewClose>> { gfl_close %W [%W focus] }

	lower [ttk::frame $cn.dummy]
	pack $cn.dummy -expand y -fill both
	grid $cn.tree $cn.vsb -sticky nsew -in $cn.dummy
	grid $cn.hsb -sticky nsew -in $cn.dummy
	grid columnconfigure $cn.dummy 0 -weight 1
	grid rowconfigure $cn.dummy 0 -weight 1

	set P(FL) $cn.tree

	bind $P(FL) <Double-1> "open_for_edit %x %y"
	bind $P(FL) <ButtonPress-3> "tree_menu %x %y %X %Y"
	bind $P(FL) <ButtonPress-2> "tree_menu %x %y %X %Y"

	# tags for marking edited files and their status
	$P(FL) tag configure sred -foreground red
	$P(FL) tag configure sgreen -foreground green
	$P(FL) tag configure sboard -background gray

	#######################################################################

	set pw .pane.right
	frame $pw
	pack $pw -side right -expand y -fill both -anchor w

	set w $pw.top
	frame $w
	pack $w -side top -expand y -fill both

	set Term $w.t

	text $Term \
		-yscrollcommand "$w.scroly set" \
		-setgrid true \
        	-width 80 -height 24 -wrap char \
		-font $FFont \
		-exportselection 1 \
		-state normal

	$Term delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"
	# scrollbar $w.scrolx -orient horizontal -command "$w.t xview"
	pack $w.scroly -side right -fill y
	# pack $w.scrolx -side bottom -fill x
	pack $Term -side top -expand yes -fill both

	# tag for file line numbers
	$Term tag configure errtag -background gray

	bind $Term <ButtonRelease-3> "cut_copy_paste %W %X %Y c"

	## the bottom frame
	set bf [frame $pw.bof]
	pack $bf -side top -expand no -fill x

	## the entry line

	set TEntry $bf.e
	text $TEntry -height 1 -width 80 -wrap char -font $FFont \
		-exportselection 1 \
		-state disabled

	pack $TEntry -side left -expand y -fill x

	## the save button
	set  CSBut $bf.s
	button $CSBut -width 8 -command "do_console_save"
	pack $CSBut -side left -expand n -fill y
	set_csbut_label

	bind $TEntry <ButtonRelease-3> "cut_copy_paste %W %X %Y"

	bind $TEntry <Return> "do_console_input"
	bind $TEntry <Control-c> "do_console_interrupt"

	#######################################################################

	bind $Term <Double-1> "do_filename_click $Term term_dspline %x %y"

	#######################################################################

	# make it a paned window, so the tree view area can be easily resized
	.pane add .pane.left .pane.right

	mark_running 0

	bind . <Destroy> "terminate -force"
}

proc get_extra_defines { } {

	global P

	set dl ""
	foreach d [dict get $P(CO) "XDEFS"] {
		if { $d != "" } {
			lappend dl "-D$d"
		}
	}

	return $dl
}

proc do_mkmk_node { { bi 0 } { ea "" } } {

	global P ST

	if ![close_modified] {
		return
	}

	set al [get_extra_defines]

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]

	if { $mb == "" } {
		# a precaution
		return
	}

	set lm [blindex [dict get $P(CO) "LM"] $bi]
	set ar [dict get $P(CO) "ARCH"]
	set bo [lindex $bo $bi]

	# the board argument
	lappend al $bo

	# the arch
	lappend al "-a"
	lappend al $ar

	# compiler configuration file
	set cfn [dict get $P(CO) "CCF"]
	if { $cfn != "" } {
		lappend al "-c"
		lappend al $cfn
	}

	if { $ST(LO) || $lm } {
		# library mode; check if there's a library
		if ![library_present $bo] {
			alert "There's no library at board $bo; you have to\
				create one to be able to use the library\
				mode for the build"
			return
		}
		lappend al "-l"
	}
		
	if $mb {
		# the label
		lappend al [lindex $P(PL) $bi]
	}

	if [catch { run_term_command "mkmk" $al $ea } err] {
		alert $err
	}
}


proc do_make_node { { bi 0 } { m 0 } { ea "" } } {
#
# Does a standard build:
#
#	bi - board index (can be nonzero for multiprogram projects)
#	m  - do not proceed if no Makefile (to detect failed auto prebuilds)
#
	global P

	if ![close_modified] {
		return
	}

	set mb [dict get $P(CO) "MB"]

	set al ""

	if $mb {
		# the index makes sense
		set mf "Makefile_[lindex $P(PL) $bi]"
		lappend al "-f"
		lappend al $mf
	} else {
		set mf "Makefile"
	}

	if $m {
		# doing this after a forced pre-build
		if ![file_present $mf] {
			term_dspline "--PREBUILD FAILED, NO BUILD--"
			return
		}
		# successful pre-build completes, take care of sys ctags
		sys_make_ctags
		term_dspline "--BUILDING--"
	} else {
		# we are doing this the first time around
		if ![file_present $mf] {
			# try pre-build
			term_dspline "--NEED TO PREBUILD FIRST--"
			# afterwards we will get called again with m == 1
			do_mkmk_node $bi "do_make_node $bi 1"
			return
		}
	}

	if [catch { run_term_command "make" $al $ea } err] {
		alert $err
	}
}

proc do_mkmk_all { { m 0 } } {
#
# Pre-build for all programs; m == (running) program index
#
	global P

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set bd [dict get $P(CO) "LD"]

	if { $mb == "" || $mb == 0 } {
		# to prevent stupid crashes on races
		return
	}

	set nb [llength $bo]

	while { $m < $nb } {
		set suf [lindex $P(PL) $m]
		set b [lindex $bo $m]
		if ![blindex $bd $m] {
			break
		}
		# disabled
		set txt $suf
		if { $b != "" } {
			append txt " \[$b\]"
		}
		term_dspline "--DISABLED: $txt--"
		incr m
	}

	if { $m >= $nb } {
		# called for the last time
		sys_make_ctags
		term_dspline "--ALL DONE--"
		return
	}

	# do board number m
	term_dspline "--PREBUILDING $suf for $b--"
	do_mkmk_node $m "do_mkmk_all [expr $m + 1]"
}

proc do_make_all { { m 0 } { s 0 } } {
#
# Build for all programs; m == running program index, s == stage (0,1,2)
#
	global P

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set bd [dict get $P(CO) "LD"]

	if { $mb == "" || $mb == 0 } {
		# to account for races
		return
	}

	if { $m == 0 && $s == 0 } {
		# initialize the pre-build flag (will tell us if we have to
		# recalculate sys ctags)
		set P(WBL) 0
	}

	set nb [llength $bo]

	while { $m < $nb } {

		set suf [lindex $P(PL) $m]
		set b [lindex $bo $m]

		if ![blindex $bd $m] {
			break
		}
		# disabled
		set txt $suf
		if { $b != "" } {
			append txt " \[$b\]"
		}
		term_dspline "--DISABLED: $txt--"
		incr m
	}

	if { $m >= $nb } {
		# called for the last time; note that we postpone sys ctags
		# until the successfull end of the entire chain; thus, a
		# failure somewhere along the line may leave stale sys ctags;
		# this is OK, because we view the entire operation as a single
		# step
		if $P(WBL) {
			sys_make_ctags
		}
		term_dspline "--ALL DONE--"
		return
	}

	set mf "Makefile_$suf"

	if { $s == 0 } {
		# stage 0: check for prebuild present and pre-build if needed
		if ![file_present $mf] {
			# need to prebuild
			term_dspline "--NEED TO PREBUILD $suf--"
			do_mkmk_node $m "do_make_all $m 1"
			incr P(WBL)
			return
		}
		term_dspline "--PREBUILD FOR $suf PRESENT--"
		# fall through proceeding to build
	} elseif { $s == 1 } {
		# stage 1: check if the prebuild triggered by us succeeded
		if ![file_present $mf] {
			term_dspline "--PREBUILD FOR $suf FAILED--"
			return
		}
		# fall through to build
	} else {
		# stage 2: build completed
		if ![file_present "Image_$suf"] {
			term_dspline "--BUILD FOR $suf FAILED--"
			return
		}
		# proceed with next program
		incr m
		do_make_all $m 0
		return
	}

	# stage 1 action, i.e., build
	do_make_node $m 0 "do_make_all $m 2"
}

proc do_make_vuee { { argl "" } { argm ""} } {

	global P ARCHINFO

	if ![close_modified] {
		return
	}

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set bd [dict get $P(CO) "LD"]

	if { $mb == "" || $bo == "" } {
		alert "You must select Arch+Board before building a VUEE\
			model for the project"
		return
	}

	if { [dict get $P(CO) "DPBC"] == 0 && $mb != "" && $bo != "" } {
		# add the defines pertaining to the board
		if $mb {
			set bi 0
			set di ""
			foreach b $bo {
				set suf [lindex $P(PL) $bi]
				if ![blindex $bd $bi] {
					set argl [linsert $argl 0 \
						"-D${suf}+BOARD_$b" \
						"-D${suf}+BOARD_TYPE=$b"]
					set fn [board_opts $b]
					if { $fn != "" } {
						set argl [linsert $argl 0 \
						    "-H${suf}+[unipath $fn]"]
					}
				} else {
					set argl [linsert $argl 0 "-X${suf}"]
					lappend di $suf
				}
				incr bi
			}
			if { $di != "" } {
				term_dspline "--DISABLED: [join $di ", "]"
			}
		} else {
			set argl [linsert $argl 0 "-DBOARD_$bo" \
				"-DBOARD_TYPE=$bo"]
			set fn [board_opts $bo]
			if { $fn != "" } {
				set argl [linsert $argl 0 \
					"-H[unipath $fn]"]
			}
		}
	}

	# arch-specific defines
	if [info exists ARCHINFO(VUEE,DEFS)] {
		foreach b $ARCHINFO(VUEE,DEFS) {
			set argl [linsert $argl 0 "-D$b"]
		}
	}

	if { [dict get $P(CO) "CMPIS"] != 0 } {
		set argl [linsert $argl 0 "-i"]
	}

	# insert the global project defines in the very front of evrything
	set argl [concat [get_extra_defines] $argl]

	if { [dict get $P(CO) "THRD"] != 0 } {
		lappend argl "-3"
	}

	set ea [dict get $P(CO) "EBRG"]
	if { $ea != "" } {
		set wh 0
		foreach aa [split $ea] {
			if { $aa == "--" } {
				set wh 1
			} elseif { $wh } {
				lappend argm $aa
			} else {
				lappend argl $aa
			}
		}
	}

	if { $argm != "" } {
		lappend argl "--"
		set argl [concat $argl $argm]
	}

	if [catch { run_term_command "picomp" $argl } err] {
		alert $err
	}

	reset_bnx_menus
}

proc do_prebuild_lib { board ea } {
#
# Prebuild a library for the specified board
#
	global P

	set al [concat [get_extra_defines] [list $board -L]]

	# compiler configuration file
	set cfn [dict get $P(CO) "CCF"]
	if { $cfn != "" } {
		lappend al "-c"
		lappend al $cfn
	}

	if [catch { run_term_command "mkmk" $al $ea } err] {
		alert "Cannot run library make for $board: $err"
	}
}

proc do_build_lib { board ea } {
#
# Completes the library build, which is a two-stage operation
#
	global BinPath

	set bd [file join [boards_dir] $board]

	set al [list -c "cd $bd; make"]

	if [catch { run_term_command [file join $BinPath "bash"] $al $ea } \
	    err] {
		catch {
			file delete --force [file join $bd "Makefile"]
			file delete --force [file join $bd "libpicos.a"]
		}
		alert "Cannot run library make for $board: $err"
	}
}

proc verify_lib_preconds { board } {
#
# Checks if we can go ahead with a library build for board
#
	# check if params.sys contains a list of files to compile
	set ld [file join [boards_dir] $board "params.sys"]
	set fail 1
	if ![catch { open $ld "r" } fd] {
		if ![catch { read $fd } ps] {
			set fail 0
		}
		catch { close $fd }
	}
	if $fail {
		return 1
	}

	# some heuristics
	if ![regexp -- {-l[[:space:]][^\n\r]*\.c} $ps] {
		return 2
	}

	# check if library exists
	if [library_present $board] {
		return 3
	}

	return 0
}

proc do_makelib { board { st 0 } } {
#
# Creates a library for the specified board
#
	if { $st == 0 } {
		# stage 0: initial call
		set s [verify_lib_preconds $board]
		# failures first
		if { $s == 1 } {
			alert "The board directory of $board appears\
				incomplete: params.sys is missing or unreadable"
			return
		}
		if { $s == 2 } {
			alert "Board $board declares no library files;\
			please edit params.sys in the board directory and add\
			a line starting with -l and including the list of\
			files/headers to compile"
			return
		}
		# now the warning
	     	if { $s == 3 && ![confirm "There already exists a library for\
		    $board. Do you want to re-build it?"] } {
			return
		}
		term_dspline "--PREBUILDING LIBRARY for $board--"
		do_prebuild_lib $board "do_makelib $board 1"
		return
	}

	if { $st == 1 } {
		# stage 1: proper build
		if ![file isfile [file join [boards_dir] $board Makefile]] {
			term_dspline "--PREBUILD FAILED, LIBRARY NOT CREATED--"
			return
		}
		term_dspline "--BUILDING LIBRARY for $board--"
		do_build_lib $board "do_makelib $board 2"
		return
	}

	# stage 2: after build
	if ![file exists [file join [boards_dir] $board libpicos.a]] {
		term_dspline "--BUILD FAILED, LIBRARY NOT CREATED--"
		return
	}

	sys_make_ctags
}
	
proc do_makelib_all { { bx 0 } { bn "" } { st 0 } } {
#
# Creates a library for the specified board
#
	set bo [board_set]
	set bd [boards_dir]

	if { $bx >= [llength $bo] } {
		# all done
		term_dspline "--ALL DONE--"
		sys_make_ctags
		return
	}

	if { $bx == 0 && $st == 0 } {
		# just starting up
		set nf ""
		set nw ""
		foreach b $bo {
			set s [verify_lib_preconds $b]
			if { $s == 1 } {
				lappend nf "no params.sys for $b"
				continue
			}
			if { $s == 2 } {
				lappend nf "no files to compile (-l missing\
					in params.sys) for $b"
				continue
			}
			if { $s == 3 } {
				lappend nw $b
			}
		}

		if { $nf != "" } {
			# fatal condition(s) present
			if { [llength $nf] > 1 } {
				set txt "Problems: "
				append txt [join $nf ", "]
			} else {
				set txt "Problem: [lindex $nf 0]"
			}
			append txt ". Nothing will be built"
			alert $txt
			return
		}

		if { $nw != "" } {
			# existing libs
			if { [llength $nw] > 1} {
				set txt "There already exist libraries for: "
				append txt [join $nw ", "]
				append txt ". Should they be rebuilt?"
			} else {
				set txt "There already exists a library for:\
					[lindex $nw 0]. Should it be rebuilt?"
			}
			append txt " Nothing will be built if you say NO."
			if ![confirm $txt] {
				return
			}
		}
	}

	if { $st == 0 } {
		# stage 0: pre-build
		set bn [lindex $bo $bx]
		term_dspline "--PREBUILDING LIBRARY for $bn--"
		do_prebuild_lib $bn "do_makelib_all $bx $bn 1"
		return
	}

	# sanity check for higher stages
	if { [lindex $bo $bx] != $bn } {
		alert "Board structure changed during library build,\
			operation aborted"
		return
	}

	if { $st == 1 } {
		# stage 1: proper build
		if ![file isfile [file join [boards_dir] $bn Makefile]] {
			term_dspline \
			    "--PREBUILD FAILED, LIBRARY for $bn NOT CREATED--"
			return
		}
		term_dspline "--BUILDING LIBRARY for $bn--"
		do_build_lib $bn "do_makelib_all $bx $bn 2"
		return
	}

	# stage 2: after build
	if ![file exists [file join [boards_dir] $bn libpicos.a]] {
		term_dspline "--BUILD FAILED, LIBRARY for $bn NOT CREATED--"
		return
	}

	term_dspline "--LIBRARY CREATED--"

	# try the next one
	do_makelib_all [expr $bx + 1] "" 0
}
	
###############################################################################

proc do_cleanup { } {
#
# Clean up the project
#
	global P PicOSPath

	if { $P(AC) == "" } {
		return
	}

	set cpm [fpnorm [file join $PicOSPath "Scripts" "cleanapp"]]

	if ![file exists $cpm] {
		alert "Cleaning script (cleanapp) not found in Scripts"
		return
	}

	if [catch { xq $cpm "" } er] {
		alert "Cleanup failed: $er"
	}

	reset_bnx_menus
	sys_make_ctags
}

proc do_clean_light { { ix "" } } {
#
# A light cleanup, no need to pre-build, just remove the binaries
#
	global P SoftCleanDirs

	if { $ix == "" } {
		set suf ""
	} else {
		set suf "_[lindex $P(PL) $ix]"
	}

	foreach d $SoftCleanDirs {
		set d $d$suf
		catch { exec rm -rf $d }
	}
	reset_bnx_menus
}

proc do_clean_vuee { } {
#
# VUEE cleanup
#
	global VueeCleanFiles

	foreach d $VueeCleanFiles {
		catch { exec rm -rf $d }
	}
}

###############################################################################

proc open_search_window { } {
#
	global P FFont CFSearchModes CFSearchItems CFSearchTags CFSearchSFiles
	global EXCmd

	if { $P(AC) == "" } {
		return
	}

	if { $P(SWN) != "" } {
		# already opened, try to raise
		catch { raise $P(SWN) }
		return
	}

	set w "[cw].search0"
	toplevel $w
	wm title $w "Search"
	set P(SWN) $w

	set tf $w.tf
	frame $tf
	pack $tf -side top -expand y -fill both

	set t $tf.t
	set P(SWN,t) $t
	text $t

	$t configure \
		-yscrollcommand "$tf.scroly set" \
		-setgrid true \
		-width 80 -height 24 -wrap char \
		-font $FFont \
		-exportselection 1 \
		-state normal

	# won't hurt
	$t delete 1.0 end
	$t configure -state disabled

	scrollbar $tf.scroly -command "$t yview"
	pack $tf.scroly -side right -fill y
	pack $t -side left -expand yes -fill both

	bind $t <ButtonRelease-3> "cut_copy_paste %W %X %Y"
	bind $t <Double-1> "do_filename_click $t osline %x %y"

	#######################################################################

	# copy options from config
	foreach u $CFSearchTags { k z } $CFSearchItems {
		set P(SWN,$u) [dict get $P(CO) $k]
	}

	# no search in progress
	set P(SST) 0

	validate_search_options 1

	set bf $w.bf
	frame $bf
	pack $bf -side top -expand n -fill x

	##

	set f $bf.sf
	labelframe $f -text "Search" -padx 2 -pady 2
	pack $f -side top -expand y -fill x

	label $f.sl -text "String: "
	pack $f.sl -side left -expand n

	text $f.se -width 24 -height 1 -font $FFont -state normal \
		-exportselection yes
	pack $f.se -side left -expand y -fill x

	$f.se insert end $P(SWN,s)

	# pointer to the text widget
	set P(SWN,ss) $f.se

	bind $f.se <ButtonRelease-3> "cut_copy_paste %W %X %Y"
	bind $f.se <Return> "do_return_key"

	foreach rb $CFSearchModes {
		set r $f.[string tolower $rb]
		radiobutton $r -text $rb -variable P(SWN,m) -value $rb
		pack $r -side left -expand n
	}

	label $f.kl -text "Case:"
	pack $f.kl -side left -expand n
	checkbutton $f.kf -variable P(SWN,k)
	pack $f.kf -side left -expand n

	label $f.xl -text "Sys:"
	pack $f.xl -side left -expand n

	eval "tk_optionMenu $f.xf P(SWN,x) $CFSearchSFiles"
	pack $f.xf -side left -expand n

	label $f.vl -text "VUEE:"
	pack $f.vl -side left -expand n

	checkbutton $f.vf -variable P(SWN,v)
	pack $f.vf -side left -expand n

	button $f.yh -text "H" -command "search_colconf $f.yh h" \
		-background $P(SWN,h)
	pack $f.yh -side left -expand n

	button $f.yt -text "M" -command "search_colconf $f.yt n" \
		-background $P(SWN,n)
	pack $f.yt -side left -expand n

	##

	set zf $bf.lf
	frame $zf
	pack $zf -side top -expand y -fill x

	##

	set f $zf.lf
	labelframe $f -text "Limits" -padx 2 -pady 2
	pack $f -side left -expand y -fill x

	label $f.ll -text "Max lines:"
	grid $f.ll -column 0 -row 0 -sticky w -padx 1 -pady 1

	entry $f.le -width 5 -font $FFont -textvariable P(SWN,l)
	grid $f.le -column 1 -row 0 -sticky ew -padx 1 -pady 1

	label $f.cl -text " Max cases:"
	grid $f.cl -column 2 -row 0 -sticky w -padx 1 -pady 1

	entry $f.ce -width 5 -font $FFont -textvariable P(SWN,c)
	grid $f.ce -column 3 -row 0 -sticky ew -padx 1 -pady 1

	label $f.bl -text " Bracket:"
	grid $f.bl -column 4 -row 0 -sticky w -padx 1 -pady 1

	entry $f.be -width 5 -font $FFont -textvariable P(SWN,b)
	grid $f.be -column 5 -row 0 -sticky ew -padx 1 -pady 1

	label $f.fl -text " FN Pat:"
	grid $f.fl -column 6 -row 0 -sticky w -padx 1 -pady 1

	label $f.fn -text "!"
	grid $f.fn -column 7 -row 0 -sticky w -padx 0 -pady 0

	checkbutton $f.fg -variable P(SWN,g)
	grid $f.fg -column 8 -row 0 -sticky ew -padx 0 -pady 0
	
	entry $f.fe -width 5 -font $FFont -textvariable P(SWN,f)
	grid $f.fe -column 9 -row 0 -sticky ew -padx 1 -pady 1

	grid columnconfigure $f { 1 3 5 9 } -weight 1

	##

	set f $zf.rf
	labelframe $f -text "Actions" -padx 2 -pady 2
	pack $f -side right -expand n

	button $f.cb -text "Close" -command close_search_window
	pack $f.cb -side right -expand n

	set P(SWN,o) $f.gb
	button $f.gb -text "Search" -command do_search
	pack $f.gb -side right -expand n

	button $f.kb -text "Clear" -command do_clean_search
	pack $f.kb -side right -expand n

	button $f.eb -text "Edit" -command "do_edit_any_file 0"
	pack $f.eb -side right -expand n

	set aed [alt_editor]
	if { $aed != "" } {
		button $f.ab -text "Edit Alt" -command "do_edit_any_file 1"
		pack $f.ab -side right -expand n
	}

	button $f.nb -text "New" -command do_edit_new_file
	pack $f.nb -side right -expand n

	button $f.xb -text "Term" -command do_open_xterm
	pack $f.xb -side right -expand n

	if { $EXCmd != "" } {
		button $f.yb -text "Explorer" -command do_open_explorer
		pack $f.yb -side right -expand n
	}

	# tags for marking the match and headers
	$t tag configure mtag -background $P(SWN,n)
	$t tag configure htag -background $P(SWN,h)

	##
	bind $w <Destroy> close_search_window
}

proc validate_search_options { { force 0 } } {
#
# Called to check if the search options make sense; if force, then force them
# to decent
#
	global P CFSearchModes CFSearchSFiles

	set er ""

	if { [lsearch -exact $CFSearchModes $P(SWN,m)] < 0 } {
		# this cannot really happen unless the config file is broken
		set P(SWN,m) [lindex $CFSearchModes 0]
	}

	if { [lsearch -exact $CFSearchSFiles $P(SWN,x)] < 0 } {
		set P(SWN,x) [lindex $CFSearchSFiles 0]
	}

	if { $P(SWN,s) != "" && $P(SWN,m) == "RE" } {
		# check if the regexp is formally ok
		if [catch { regexp $P(SWN,s) "xxx" } ] {
			if $force {
				set P(SWN,s) ""
			} else {
				lappend er "illegal string (not a valid regular\
					expression)"
			}
		}
	}

	if { $P(SWN,f) != "" } {
		# check if the file name regexp is ok
		if [catch { regexp $P(SWN,f) "xxx" } ] {
			if $force {
				set P(SWN,f) ""
			} else {
				lappend er "illegal file name pattern (not a\
					valid regular expression)"
			}
		}
	}

	# checkbuttons cannot really be wrong, unless something behind the
	# scenes is wrong (e.g., the config file has been altered manually)
	if [catch { valnum $P(SWN,g) 0 1 } P(SWN,g)] {
		set P(SWN,g) 0
	}

	if [catch { valnum $P(SWN,k) 0 1 } P(SWN,k)] {
		set P(SWN,k) 0
	}

	if [catch { valnum $P(SWN,v) 0 1 } P(SWN,v)] {
		set P(SWN,v) 0
	}

	if [catch { valnum $P(SWN,l) 24 100000 } v] {
		if $force {
			set P(SWN,l) 1000
		} else {
			lappend er "illegal line number limit: $v"
		}
	} else {
		# normalize
		set P(SWN,l) $v
	}

	if [catch { valnum $P(SWN,c) 1 100000 } v] {
		if $force {
			set P(SWN,c) 256
		} else {
			lappend er "illegal case number limit: $v"
		}
	} else {
		set P(SWN,c) $v
	}

	if [catch { valnum $P(SWN,b) 0 32 } v] {
		if $force {
			set P(SWN,b) 7
		} else {
			lappend er "illegal bracket count: $v"
		}
	} else {
		set P(SWN,b) $v
	}

	if [catch { valcol $P(SWN,h) } v] {
		set P(SWN,h) "#AAAAAA"
	}

	if [catch { valcol $P(SWN,n) } v] {
		set P(SWN,n) "#888888"
	}

	if { $er != "" } {
		return [join $er ", "]
	}

	return ""
}

proc update_search_options { } {
#
# Called to update the project config options related to the search window
# whenever we suspect that some of them may have changed
#
	global P CFSearchItems CFSearchTags

	if { $P(SWN) == "" || $P(CO) == "" } {
		return
	}

	search_usp
	validate_search_options 1

	# change flag
	set c 0

	foreach u $CFSearchTags { k z } $CFSearchItems {
		if { $P(SWN,$u) != [dict get $P(CO) $k] } {
			dict set P(CO) $k $P(SWN,$u)
			set c 1
		}
	}

	if $c {
		set_config
	}
}

proc close_search_window { } {
#
	global P

	if { $P(SWN) != "" } {

		update_search_options
		catch { destroy $P(SWN) }
		set P(SWN) ""
		# search status, to abort a search in progress
		set P(SST) 0
	}
	array unset P "SWN,*"
}

proc osline { ln } {
#
# Writes one line into the search console, tags == optional positions to be
# tagged
#
	global P

	if { $P(SWN) == "" } {
		return
	}

	set t $P(SWN,t)

	$t configure -state normal
	$t insert end $ln
	$t insert end "\n"

	while 1 {
		set ix [$t index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= $P(SWN,l) } {
			break
		}
		$t delete 1.0 2.0
	}

	$t configure -state disabled
	$t yview -pickplace end
}

proc osline_tag { ln tag { tags "" } } {
#
# Tags the last line written to the search term
#
	global P

	if { $P(SWN) == "" } {
		return
	}

	osline $ln

	set t $P(SWN,t)

	set ix [string length $ln]

	if { $tags == "" } {
		set tags [list [list 0 $ix]]
	}

	incr ix 2

	foreach m $tags {
		set a [expr $ix - [lindex $m 0]]
		set b [expr $ix - [lindex $m 1] - 1]
		$t tag add $tag "end - $a chars" "end - $b chars"
	}
}

proc do_clean_search { } {
#
# Clean up the output pane
#
	global P

	if { $P(SWN) != "" } {
		$P(SWN,t) configure -state normal
		$P(SWN,t) delete 1.0 end
		$P(SWN,t) configure -state disabled
		return
	}
}

proc search_usp { } {
#
# Update the pattern from the text widget
#
	global P

	regsub "\[\r\n\].*" [string trim [$P(SWN,ss) get 1.0 end]] "" P(SWN,s)
	$P(SWN,ss) delete 1.0 end
	$P(SWN,ss) insert end $P(SWN,s)
	log "Search pattern: $P(SWN,s)"
}

proc do_return_key { } {
#
# Same as search, but we have to absorb the key first
#
	search_widgets 0
	after 10 do_search
}

proc search_widgets { on } {
#
# Enable disable search widgets
#
	global P

	if { $P(SWN) == "" } {
		return
	}

	if $on {
		$P(SWN,o) configure -text "Search"
		$P(SWN,ss) configure -state normal
	} else {
		$P(SWN,o) configure -text "Stop"
		$P(SWN,ss) configure -state disabled
	}
}

proc do_search { } {
#
# Toggles search/stop
#
	global P PicOSPath

	if { $P(SWN) == "" } {
		return
	}

	# update search string
	search_usp

	if $P(SST) {
		# this means we are searching, stop
		set P(SST) 0
		search_widgets 1
		return
	}

	if $P(SSR) {
		# still running, hold on
		search_widgets 0
		alert "Search engine busy cleaning up, try again in a sec"
		return
	}

	# we are not searching, start search
	set er [validate_search_options]

	if { $er != "" } {
		if { [string first "," $er] < 0 } {
			set tt "s"
		} else {
			set tt ""
		}
		alert "Error$tt in search options: $er"
		search_widgets 1
		return
	}

	# the pattern
	set patt $P(SWN,s)

	# function name suffix
	set sf $P(SWN,m)

	if !$P(SWN,k) {
		# ignore case
		set patt [string tolower $patt]
	}

	if { $P(SWN,m) == "WD" } {
		# word matching, preprocess pattern
		set patt [patt_to_words $patt]
		# this is handled by regexp
		set sf "RE"
	}

	if { $patt == ""} {
		alert "The search string is empty"
		search_widgets 1
		return
	}
	# the matching function
	set sfun "smatch_$sf"

	if !$P(SWN,k) {
		append sfun "_nc"
	}

	# start search, create a complete list of files
	if { $P(SWN,x) == "Only" } {
		# we only want system files
		set fl ""
	} else {
		# start with project files
		set fl [lsort [gfl_files $P(SWN,f) $P(SWN,g)]]
	}

	if { $P(SWN,x) == "Proj" } {
		# system files related to the project
		set tl [get_picos_project_files]
		# this list comes sorted
		if { $P(SWN,f) != "" } {
			# there is a pattern, trim the list
			foreach f $tl {
				if { ( $P(SWN,g) && ![regexp $P(SWN,f) $f] ) ||
				     (!$P(SWN,g) &&  [regexp $P(SWN,f) $f] ) } {
					lappend fl $f
				}
			}
		} else {
			# all files
			set fl [concat $fl $tl]
		}
	} elseif { $P(SWN,x) != "None" } {
		# all system files "All" or "Only"
		set tl [get_picos_files]
		# the files are PicOS-relative
		set t [file join $PicOSPath "PicOS"]
		foreach f $tl {
			set f [fpnorm [file join $t $f]]
			if { $P(SWN,f) != "" } {
				if { ( $P(SWN,g) &&  [regexp $P(SWN,f) $f] ) ||
				     (!$P(SWN,g) && ![regexp $P(SWN,f) $f] ) } {
					continue
				}
			}
			lappend fl $f
		}
	}

	if $P(SWN,v) {
		# VUEE files
		set tl [get_vuee_files]
		if { $P(SWN,f) != "" } {
			foreach f $tl {
				if { ( $P(SWN,g) && ![regexp $P(SWN,f) $f] ) ||
				     (!$P(SWN,g) &&  [regexp $P(SWN,f) $f] ) } {
					lappend fl $f
				}
			}
		} else {
			# all files
			set fl [concat $fl $tl]
		}
	}
		
	if { $fl == "" } {
		osline "No files to search!"
		search_widgets 1
		return
	}

	set P(SST) 1
	set P(SSR) 1

	$P(SWN,o) configure -text "Stop"
	$P(SWN,ss) configure -state disabled

	# forward bracket, i.e., lines following the found one
	set braf [expr $P(SWN,b) / 2]
	# backward bracket, i.e., lines preceding the found one
	set brab $braf
	if [expr $P(SWN,b) & 1] {
		incr brab
	}
	# the terminal
	set t $P(SWN,t)

	# the search loop
	set CNT 0
	set FCN 0
	foreach f $fl {
		update
		if { $P(SST) == 0 || $CNT >= $P(SWN,c) } {
			break
		}
		if [catch { open $f "r" } fd] {
			# this should not happen
			alert "Couldn't open $f: $fd, will skip this file"
			continue
		}
		if [catch { read $fd } fc] {
			# neither should this
			catch { close $fd }
			alert "Couldn't read $f: $fc, will skip this file"
			continue
		}
		catch { close $fd }
		set fc [split $fc "\n"]
		set lc [llength $fc]
		set lm -1
		incr FCN
		for { set i 0 } { $i < $lc } { incr i } {
			# current line
			set ln [lindex $fc $i]
			set ma [$sfun $patt $ln]
			if { $ma == "" } {
				if { [expr $i % 100] == 0 } {
					update
					if { $P(SST) == 0 } {
						break
					}
				}
				continue
			}
			# we have a match
			osline_tag "@@@ $f:[expr $i + 1]" htag
			# backspace
			set bf [expr $i - $brab]
			if { $bf <= $lm } {
				set bf [expr $lm + 1]
			}
			while { $bf < $i } {
				osline [lindex $fc $bf]
				incr bf
			}

			# the matched line
			osline_tag $ln mtag $ma

			for { set bf 0 } { $bf < $braf } { incr bf } {
				incr i
				if { $i >= $lc } {
					break
				}
				osline [lindex $fc $i]
			}
			set lm $i
			update
			if { $P(SST) == 0 } {
				break
			}
			incr CNT
			if { $CNT >= $P(SWN,c) } {
				break
			}
		}
	}

	if { $CNT > 0 } {
		if { $CNT == 1 } {
			set ncm "one match found"
		} else {
			set ncm "$CNT matches found"
		}
	} else {
		set ncm "no matches found"
	}

	set fcm "$FCN files scanned"

	if { $P(SST) == 0 } {
		osline "@@@ Stopped, $fcm, $ncm"
	} elseif { $CNT >= $P(SWN,c) } {
		osline "@@@ Case limit reached, $fcm, $ncm"
	} else {
		osline "@@@ All done, $fcm, $ncm"
	}

	set P(SST) 0
	set P(SSR) 0
	search_widgets 1
}

proc search_colconf { b u } {
#
# Configures a tag color 
#
	global P

	if { $P(SWN) == "" } {
		return
	}

	# initial color
	set col $P(SWN,$u)

	if { $u == "h" } {
		set tp "header"
		set tag "htag"
	} else {
		set tp "match"
		set tag "mtag"
	}

	reset_all_menus 1
	set col [tk_chooseColor -parent $P(SWN) -initialcolor $col -title \
		"Choose $tp color"]
	reset_all_menus

	if { $col == "" } {
		# cancel
		return
	}

	set P(SWN,$u) $col
	$b configure -background $col
	$P(SWN,t) tag configure $tag -background $col
}

proc smatch_RE { pt ln } {
#
# Regular expression match (respecting case)
#
	if ![regexp -indices -- $pt $ln ma] {
		# a quick negative
		return ""
	}

	set res ""

	while 1 {
		lappend res $ma
		if ![regexp -start [expr [lindex $ma 1] + 1] -indices -- $pt \
		    $ln ma] {
			return $res
		}
	}
}
	
proc smatch_RE_nc { pt ln } {
#
# Regular expression match (ignoring case)
#
	return [smatch_RE $pt [string tolower $ln]]
}

proc smatch_ST { pt ln } {
#
# Direct string match (respecting case)
#
	set ix [string first $pt $ln]
	if { $ix < 0 } {
		return ""
	}

	set res ""
	set len [string length $pt]

	while 1 {
		set iy [expr $ix + $len]
		lappend res [list $ix [expr $iy - 1]]
		set ix [string first $pt $ln $iy]
		if { $ix < 0 } {
			return $res
		}
	}
}

proc smatch_ST_nc { pt ln } {
#
# Direct string match (ignoring case)
#
	return [smatch_ST $pt [string tolower $ln]]
}

proc patt_to_words { pt } {
#
# Preprocesses the pattern for "word" matching, i.e., transforms it into a
# list of words
#
	set res ""

	set first 1
	while 1 {
		set pt [string trimleft $pt]
		if { $pt == "" } {
			break
		}
		if [regexp "^(\[a-zA-Z0-9_\]+)(.*)" $pt jnk ma pt] {
			# we have an alpha keyword
			if $first {
				# the first item, force word match at the
				# beginning
				set first 0
				append res "(^|\[^a-zA-Z0-9_\])"
			}
			# the keyword matched verbatim
			append res $ma
			# check if last
			set pt [string trimleft $pt]
			if { $pt == "" } {
				# OK, last, word match at the end
				append res "(\$|\[^a-zA-Z0-9_\])"
				return $res
			} else {
				# not last, insert space match
				append res "\[ \t\n\r\]*"
			}
			continue
		}
		set ma ""
		# this will necessarily succeed ...
		regexp "^(\[^a-zA-Z0-9_ \t\n\r\]+)(.*)" $pt jnk ma pt
		if { $ma == "" } {
			# ... a stupid precaution in case it doesn't
			return ""
		}
		# here we have a bunch of weird characters, some of which may
		# be special for regexp, so let us escape them all
		while { $ma != "" } {
			append res "\\[string index $ma 0]"
			set ma [string range $ma 1 end]
		}
		set pt [string trimleft $pt]
		if { $pt == "" } {
			return $res
		}
		append res "\[ \t\n\r\]*"
		set first 0
	}
}

###############################################################################

proc select_xtex_dir { } {
#
# Selects the directory to open xterm or explorer in
#
	global P

	if { $P(AC) == "" } {
		# A precaution; search won't open if there's no project
		return ""
	}

	if ![info exists P(LOD)] {
		set P(LOD) [pwd]
	}

	reset_all_menus 1
	set fl [tk_chooseDirectory \
		-parent $P(SWN) \
		-initialdir $P(LOD) \
		-mustexist 0 \
		-title "Select directory:"]
	reset_all_menus

	if { $fl == "" } {
		# cancelled
		return ""
	}

	set fl [fpnorm $fl]

	if ![file isdirectory $fl] {
		# try to create
		log "Creating dir $fl"
		if [catch { file mkdir $fl } err] {
			alert "Cannot create directory $fl: $err"
			continue
		}
	}

	set P(LOD) $fl

	return $fl
}

proc do_open_xterm { } {
#
# Opens an xterm in the indicated directory
#
	set fl [select_xtex_dir]

	if { $fl == "" } {
		return
	}

	set cd [pwd]
	if [catch { cd $fl } err] {
		catch { cd $cd }
		alert "Cannot cd to directory $fl: $err"
		return
	}

	run_xterm

	catch { cd $cd }
}

proc do_open_explorer { } {
#
# Opens Windows Explorer in the indicated directory
#
	set fl [select_xtex_dir]

	if { $fl == "" } {
		return
	}

	run_explorer $fl
}

proc do_edit_any_file { { nst 0 } } {
#
# Allows you to open any file from the Search window
#
	global P

	if { $P(AC) == "" } {
		# A precaution; Search won't open if there's no project
		return
	}

	if ![info exists P(LOD)] {
		global DefProjDir
		set P(LOD) $DefProjDir
	}

	reset_all_menus 1
	set fl [tk_getOpenFile \
		-parent $P(SWN) \
		-initialdir $P(LOD) \
		-multiple 0 \
		-title "Select file to edit/view:"]
	reset_all_menus

	if { $fl == "" } {
		# cancelled
		return
	}

	set P(LOD) [file dirname $fl]

	if { $nst } {
		edit_file_ate $fl
	} else {
		edit_file $fl
	}
}

proc do_edit_new_file { } {
#
# Creates a new file anywhere
#
	global P

	if { $P(AC) == "" } {
		return
	}

	if ![info exists P(LOD)] {
		global DefProjDir
		set P(LOD) $DefProjDir
	}
	if ![info exists P(LOE)] {
		set P(LOE) ""
	}

	reset_all_menus 1
	set fl [tk_getSaveFile \
		-parent $P(SWN) \
		-defaultextension $P(LOE) \
		-initialdir $P(LOD) \
		-title "File name:"]
	reset_all_menus

	if { $fl == "" } {
		# cancelled
		return
	}

	if { [file_location $fl] != "X" && ![valfname $fl "f"] } {
		continue
	}

	set P(LOD) [file dirname $fl]
	set P(LOE) [file extension $fl]

	edit_file $fl
}

###############################################################################

proc scan_mkfile { mfn } {
#
# Scans a Makefile for the list of project-related "system" files
#
	global FNARR

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
		# the paths are in "make" format, so basically UNIX
		if ![regexp {^[^[:blank:]]+[[:blank:]]*:[[:blank:]]*(.*)} $ln \
		    flist flist] {
			continue
		}
		foreach fn [split $flist " \t"] {
			set fn [string trim $fn]
			if { $fn == "" } {
				continue
			}
			set fnarr($fn) ""
		}
	}

	foreach fn [array names fnarr] {
		set ex [file extension $fn]
		if { $ex != ".cc" && $ex != ".c" && $ex != ".h" } {
			# ignore
			continue
		}

		set fn [fpnorm $fn]

		if { [file_location $fn] != "S" } {
			# ignore files other than PicOS files
			continue
		}

		if ![file isfile $fn] {
			# make sure the file in fact exists
			continue
		}

		set FNARR($fn) ""
	}
}

proc get_picos_project_files { } {
#
# Produces the list of project-related system (PicOS) files based on the
# current set of Makefiles
#
	global FNARR P ST

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]
	set lm [dict get $P(CO) "LM"]

	set ml ""

	if $mb {
		set n [llength $P(PL)]
	} else {
		set n 1
	}

	for { set i 0 } { $i < $n } { incr i } {
		set l [blindex $lm $i]
		set b [lindex $bo $i]
		if { $ST(LO) || $l } {
			# library mode, use LIBRARY Makefile
			if ![library_present $b] {
				continue
			}
			set lb [file join [boards_dir] $b "Makefile"]
		} else {
			# source mode
			set lb "Makefile"
			if $mb {
				append lb "_[lindex $P(PL) $i]"
			}
		}
		if [file isfile $lb] {
			lappend ml $lb
		}
	}

	log "Makefiles to scan: $ml"

	if { $ml == "" } {
		return ""
	}


	foreach m $ml {
		set msg "--SCANNING FOR CTAGS"
		if { $m != "" } {
			append msg " IN $m"
		}
		append msg " ..."
		term_dspline $msg
		update
		scan_mkfile $m
	}

	set fl [lsort [array names FNARR]]

	array unset FNARR

	term_dspline "--DONE--"

	return $fl
}

proc get_picos_files { } {
#
# Produces the list of all files in the PicOS directory, related to the given
# ARCH, recursing as necessary
#
	global P PicOSPath FNLIST FNDIR FNARCH

	set FNLIST ""
	set FNARCH [dict get $P(CO) "ARCH"]
	set FNDIR [file join $PicOSPath "PicOS"]

	gsf_trv ""

	set fn $FNLIST
	unset FNLIST FNARCH FNDIR
	return $fn
}

proc gsf_trv { p } {
#
# Recursive directory traverser for get_picos_files
#
	global FNLIST FNARCH FNDIR Archs

	# full path to current place
	set wh [file join $FNDIR $p]

	set sdl [glob -nocomplain -directory $wh -tails *]

	foreach f $sdl {
		# full path to the file
		set ff [file join $wh $f]
		if [file isfile $ff] {
			# partial path from PicOS
			lappend FNLIST [file join $p $f]
			continue
		}
		if ![file isdirectory $ff] {
			continue
		} 
		if { $p == "" } {
			# zero level; ignore if the ARCH dir different from
			# ours
			if { [lsearch -exact $Archs $f] >= 0 &&
			    $f != $FNARCH } {
				continue
			}
		}
		gsf_trv [file join $p $f]
	}
}

proc get_vuee_files { } {
#
# Produces the list VUEE (system) files
#
	global P PicOSPath VUEEPath

	set vdir [fpnorm [file join $PicOSPath "../VUEE/PICOS"]]

	if ![file isdirectory $vdir] {
		return ""
	}

	set sdl [glob -nocomplain -directory $vdir -tails *]
	if { $sdl == "" } {
		# this will not happen
		return ""
	}

	# check for rmlinks, which lists those files that we should ignore;
	# note that I cannot just recognize them as links, because such a
	# recognition is impossible ob Cygwin, if the Tcl is from Windows
	# and the links are from Cygwin; this sucks!

	if ![catch { open [file join $vdir "rmlinks"] "r" } fd] {
		if [catch { read $fd } rml] {
			set rml ""
		}
		catch { close $fd }
		set rml [split $rml "\n"]
		log "Exception list (rmlinks) [llength $rml] items"
		foreach rm $rml {
			if [regexp "rm.*\[ \t\]+(.*\\.\[ch\]+)" $rm jnk ef] {
				set EX($ef) ""
			}
		}
	}

	set res ""
	foreach f $sdl {
		set su [file extension $f]
		if { $su != ".h" && $su != ".cc" } {
			continue
		}
		if [info exists EX($f)] {
			continue
		}
		# return full paths this time
		lappend res [fpnorm [file join $vdir $f]]
	}
	log "[llength $res] VUEE files"

	return $res
}

###############################################################################
###############################################################################

proc errab { msg } {

	puts stderr $msg
	exit 99
}

proc arch_dirs { } {
#
# Returns the list of tail subdirectory names that appear to be valid
# target arch identifiers, i.e., being directories named with all capitals and
# digits
#
	global PicOSPath

	set dir [file join $PicOSPath "PicOS"]

	set fl [glob -nocomplain -directory $dir -tails *]
	set res ""

	foreach f $fl {
		if ![regexp {^[A-Z0-9]+$} $f] {
			continue
		}
		if ![file isdirectory [file join $dir $f]] {
			continue
		}
		lappend res $f
	}

	return [lsort $res]
}

proc initialize { } {

	global ST PicOSPath DefProjDir Archs SACmd BinPath CFItems
	global CFLoadItems CFLoadItemsGen

	if $ST(DP) {
		log "preferred path format: DOS"
	} else {
		log "preferred path format: UNIX"
	}

	if { $ST(SYS) != "L" } {
		# path to bin (for explicit preference of some programs, say
		# to distinguish Windows bash from Cygwin bash); use some UNIX
		# characteristic program to make sure it comes from the right
		# place
		set BinPath [file dirname [auto_execok uname]]
	} else {
		set BinPath ""
	}

	log "UNIX bin path: $BinPath"

	# path to PICOS
	log "PicOS path: $PicOSPath"

	# path to the default superdirectory for projects
	foreach DefProjDir [list [file join $PicOSPath Apps VUEE] \
    	[file join $PicOSPath Apps]]  {
		if [file isdirectory $DefProjDir] {
			break
		}
	}

	log "Project superdirectory: $DefProjDir"

	# check if VUEE is present
	if [file isdirectory [file join $PicOSPath "../VUEE/PICOS"]] {
		log "VUEE found"
		set ST(VP) 1
	} else {
		log "VUEE assumed absent"
		set ST(VP) 0
	}

	# architecture directories
	set Archs [arch_dirs]

	# This must be non-empty these days
	if { $Archs == "" } {
		errab "no architectures in this PicOS installation"
	}

	if ![file isfile \
	    [file join $PicOSPath "PicOS" "PLibs" "VNetI" "tcv.c"]] {
		# a simple test for sources present
		log "Library-only installation"
		set ST(LO) 1
	} else {
		log "Architectures: [join $Archs]"
		set ST(LO) 0
	}

	# file descriptor to save console
	set ST(CF) ""

	# Arch loader options
	foreach a $Archs {
		set CFLoadItems($a) ""
		foreach { t v } $CFLoadItemsGen {
			set u "${a}_${t}"
			lappend CFItems $u
			lappend CFItems $v
			lappend CFLoadItems($a) $u
			lappend CFLoadItems($a) $v
		}
	}
}

###############################################################################

initialize

get_rcoptions

mk_project_window

vwait forever
