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

#########################################
# UART front for the various UART modes #
#########################################

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

set ST(DP) 0

if { $ST(SYS) != "L" } {
	# sanitize arguments
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}
	unset u
	set SIDENAME "side.exe"

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
} else {
	set SIDENAME "side"
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

###############################################################################

package provide log 1.0
###############################################################################
# Log functions. Copyright (C) 2008-13 Olsonet Communications Corporation.
###############################################################################

namespace eval LOGGING {

variable Log

proc log_open { { fname "" } { maxsize "" } { maxvers "" } } {

	variable Log

	if { $fname == "" } {
		if ![info exists Log(FN)] {
			set Log(FN) "log"
		}
	} else {
		set Log(FN) $fname
	}

	if { $maxsize == "" } {
		if ![info exists Log(MS)] {
			set Log(MS) 5000000
		}
	} else {
		set Log(MS) $maxsize
	}

	if { $maxvers == "" } {
		if ![info exists Log(MV)] {
			set Log(MV) 4
		}
	} else {
		set Log(MV) $maxvers
	}

	if [info exists Log(FD)] {
		# close previous log
		catch { close $Log(FD) }
		unset Log(FD)
	}

	if [catch { file size $Log(FN) } fs] {
		# not present
		if [catch { open $Log(FN) "w" } fd] {
			error "cannot open log file $Log(FN), $fd"
		}
		# empty log
		set Log(SZ) 0
	} else {
		# log file exists
		if [catch { open $Log(FN) "a" } fd] {
			error "cannot open log file $Log(FN), $fd"
		}
		set Log(SZ) $fs
	}
	set Log(FD) $fd
	set Log(CD) 0
}

proc rotate { } {

	variable Log

	catch { close $Log(FD) }
	unset Log(FD)

	for { set i $Log(MV) } { $i > 0 } { incr i -1 } {
		set tfn "$Log(FN).$i"
		set ofn $Log(FN)
		if { $i > 1 } {
			append ofn ".[expr $i - 1]"
		}
		catch { file rename -force $ofn $tfn }
	}

	catch { log_open }
}

proc outlm { m } {

	variable Log

	catch {
		puts $Log(FD) $m
		flush $Log(FD)
	}

	incr Log(SZ) [string length $m]
	incr Log(SZ)

	if { $Log(SZ) >= $Log(MS) } {
		rotate
	}
}

proc log { m } {

	variable Log 

	if ![info exists Log(FD)] {
		# no log filr
		return
	}

	set sec [clock seconds]
	set day [clock format $sec -format %d]
	set hdr [clock format $sec -format "%H:%M:%S"]

	if { $day != $Log(CD) } {
		# day change
		set today "Today is "
		append today [clock format $sec -format "%h $day, %Y"]
		if { $Log(CD) == 0 } {
			# startup
			outlm "$hdr #### $today ####"
		} else {
			outlm "00:00:00 #### BIM! BOM! $today ####"
		}
		set Log(CD) $day
	}

	outlm "$hdr $m"
}

namespace export log*

### end of LOGGING namespace ##################################################

}

namespace import ::LOGGING::log*

###############################################################################

package provide xml 1.0
###############################################################################
# Mini XML parser. Copyright (C) 2008-12 Olsonet Communications Corporation.
###############################################################################

### Last modified PG111008A ###

namespace eval XML {

proc xstring { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [xspace str] != "" } {
		error "illegal white space"
	}

	set c [string index $str 0]
	if { $c == "" } {
		error "empty string illegal"
	}

	if { $c != "\"" } {
		# no quote; this is formally illegal in XML, but let's be
		# pragmatic
		regexp "^\[^ \t\n\r\>\]+" $str val
		set str [string range $str [string length $val] end]
		return [xunesc $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [xunesc $val]
}

proc xunesc { str } {
#
# Remove escapes from text
#
	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str
	regsub -all "&nbsp;" $str " " str

	return $str
}

proc xspace { s } {
#
# Skip white space
#
	upvar $s str

	if [regexp -indices "^\[ \t\r\n\]+" $str ix] {
		set ix [lindex $ix 1]
		set match [string range $str 0 $ix]
		set str [string range $str [expr $ix + 1] end]
		return $match
	}

	return ""
}

proc xcmnt { s } {
#
# Skip a comment
#
	upvar $s str

	set sav $str

	set str [string range $str 4 end]
	set cnt 1

	while 1 {
		set ix [string first "-->" $str]
		set iy [string first "<!--" $str]
		if { $ix < 0 } {
			error "unterminated comment: [string range $sav 0 15]"
		}
		if { $iy > 0 && $iy < $ix } {
			incr cnt
			set str [string range $str [expr $iy + 4] end]
		} else {
			set str [string range $str [expr $ix + 3] end]
			incr cnt -1
			if { $cnt == 0 } {
				return
			}
		}
	}
}

proc xftag { s } {
#
# Find and extract the first tag in the string
#
	upvar $s str

	set front ""

	while 1 {
		# locate the first tag
		set ix [string first "<" $str]
		if { $ix < 0 } {
			set str "$front$str"
			return ""
		}
		append front [string range $str 0 [expr $ix - 1]]
		set str [string range $str $ix end]
		# check for a comment
		if { [string range $str 0 3] == "<!--" } {
			# skip the comment
			xcmnt str
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z:_\]" $str ix et] {
			# this is a tag
			break
		}
		# skip the thing and keep going
		append front "<"
		set str [string range $str 1 end]
	}

	if { $et != "" } {
		set tm 1
	} else {
		set tm 0
	}

	if { $et != "" } {
		# terminator, skip the '/', so the text is positioned at the
		# beginning of keyword
		set ix 2
	} else {
		set ix 1
	}

	# starting at the keyword
	set str [string range $str $ix end]

	if ![regexp -nocase "^(\[a-z0-9:_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode the attributes
	set attr ""
	array unset atts

	while 1 {
		xspace str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
		if { $c == "/" } {
			# self-terminating
			if { $tm != 0 || [string index $str 1] != ">" } {
				error "broken self-terminating tag:\
					<$et$kwd ... [string range $str 0 15]"
			}
			set str [string range $str 2 end]
			return [list 2 $front $kwd $attr]
		}
		if { $c == ">" } {
			# done
			set str [string range $str 1 end]
			# term preceding_text keyword attributes
			return [list $tm $front $kwd $attr]
		}
		# this must be a keyword
		if ![regexp -nocase "^(\[a-z\]\[a-z0-9_\]*)=" $str match atr] {
			error "illegal attribute: <$et$kwd ... [string range \
				$str 0 15]"
		}
		set atr [string tolower $atr]
		if [info exists atts($attr)] {
			error "duplicate attribute: <$et$kwd ... $atr"
		}
		set atts($atr) ""
		set str [string range $str [string length $match] end]
		if [catch { xstring str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc xadv { s kwd } {
#
# Returns the text + the list of children for the current tag. A child looks
# like this:
#
#	text:		<"" the_text>
#	element:	<tag attributes children_list>
#
	upvar $s str

	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [xftag str]
		if { $tag == "" } {
			# no more
			if { $kwd != "" } {
				error "unterminated tag: <$kwd ...>"
			}

			if { $str != "" } {
				# a tailing text item
				lappend chd [list "" $str]
				return $chd
			}
		}

		set md [lindex $tag 0]
		set fr [lindex $tag 1]
		set kw [lindex $tag 2]
		set at [lindex $tag 3]

		if { $fr != "" } {
			# append a text item
			lappend chd [list "" $fr]
		}

		if { $md == 0 } {
			# opening, not self-closing
			set cl [xadv str $kw]
			# inclusion ?
			set tc [list $kw $at $cl]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} elseif { $md == 2 } {
			# opening, self-closing
			set tc [list $kw $at ""]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} else {
			# closing
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done with the tag
			return $chd
		}
	}
}

proc xincl { s tag } {
#
# Process an include tag
#
	set kw [lindex $tag 0]

	if { $kw != "include" && $kw != "xi:include" } {
		return 0
	}

	set fn [sxml_attr $tag "href"]

	if { $fn == "" } {
		error "href attribute of <$kw ...> is empty"
	}

	if [catch { open $fn "r" } fd] {
		error "cannot open include file $fn: $fd"
	}

	if [catch { read $fd } fi] {
		catch { close $fd }
		error "cannot read include file $fn: $fi"
	}

	# merge it
	upvar $s str

	set str $fi$str

	return 1
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [xadv str ""]

	return [list root "" $v]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	set txt ""

	foreach t [lindex $s 2] {
		if { [lindex $t 0] == "" } {
			append txt [lindex $t 1]
		}
	}

	return $txt
}

proc sxml_snippet { s } {

	if { [lindex $s 0] != "" } {
		return ""
	}

	return [lindex $s 1]
}

proc sxml_attr { s n { e "" } } {

	if { $e != "" } {
		# flag to tell the difference between an empty attribute and
		# its complete lack
		upvar $e ef
		set ef 0
	}

	if { [lindex $s 0] == "" } {
		# this is a text
		return ""
	}

	set al [lindex $s 1]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			if { $e != "" } {
				set ef 1
			}
			return [lindex $a 1]
		}
	}
	return ""
}

proc sxml_children { s { n "" } } {

	# this is automatically null for a text
	set cl [lindex $s 2]

	if { $n == "+" } {
		# all including text
		return $cl
	}

	set res ""

	if { $n == "" } {
		# tagged elements only
		foreach c $cl {
			if { [lindex $c 0] != "" } {
				lappend res $c
			}
		}
		return $res
	} else {
		# all with the given tag name
		foreach c $cl {
			if { [lindex $c 0] == $n } {
				lappend res $c
			}
		}
	}

	return $res
}

proc sxml_child { s n } {

	# null for a text
	set cl [lindex $s 2]

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			return $c
		}
	}

	return ""
}

proc sxml_yes { item attr } {
#
# A useful shortcut
#
	if { [string tolower [string index [sxml_attr $item $attr] 0]] == \
		"y" } {
			return 1
	}
	return 0
}

namespace export sxml_*

### end of XML namespace ######################################################

}

namespace import ::XML::*

###############################################################################
###############################################################################

set AGENT_MAGIC		0xBAB4
set ECONN_OK		129
set AGENT_RQST_ALIVE	6
set AGENT_RQST_STOP	12

###############################################################################

proc usage { } {

	global argv0

	abt "usage: $argv0 \[datafile \[logfile\]\]\n       $argv0 -kill\
		\[datafile \[logfile\]\]"
}

proc abt { msg } {

	log $msg
	puts stderr $msg
	exit 99
}

###############################################################################

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

###############################################################################

proc abinS { s h } {
#
# append one short int to string s (in network order)
#
	upvar $s str
	append str [binary format S $h]
}

proc abinI { s l } {
#
# append one 32-bit int to string s (in network order)
#
	upvar $s str
	append str [binary format I $l]
}

proc dbinI { s } {
#
# decode one binary 32-bit int from string s
#
	upvar $s str
	if { [string length $str] < 4 } {
		return -1
	}
	binary scan $str I val
	set str [string range $str 4 end]
	return $val
}

###############################################################################

proc get_config { } {
#
# Reads the project configuration from config.prj
#
	global D P B PA

	if [catch { open "config.prj" "r" } fd] {
		return 0
	}

	if [catch { read $fd } pf] {
		catch { close $fd }
		log "cannot read config.prj, $pf"
		return 0
	}

	catch { close $fd }

	set D [dict create]
	foreach { k v } $pf {
		if { $k != "" } {
			dict set D $k $v
		}
	}

	# find out about source files, especially suffixes

	set fl [glob -nocomplain *]
	set pl ""
	set es 0

	foreach fn $fl {
		if { $fn == "app.cc" } {
			if { $pl != "" } {
				log "inconsistent project structure"
				return 0
			}
			set es 1
			continue
		}

		if [regexp "^app_(\[a-zA-Z0-9\]+)\\.cc$" $fn jnk pn] {
			if $es {
				log "inconsistent project structure"
				return 0
			}
			lappend pl $pn
		}
	}

	set P [lsort $pl]

	# locate the boards directory

	set ld $PA
	while 1 {
		set kd [file dirname $ld]
		if { $kd == $ld } {
			# not found
			log "cannot locate BOARDS directory"
			return 0
		}
		# check if PicOS/MSP430/BOARDS is reachable from here
		set t [file join $kd "PicOS" "MSP430" "BOARDS"]
		if [file isdirectory $t] {
			set B $t
			break
		}
		set ld $kd
	}

	return 1
}

proc gpp { par } {
#
# Get the value of project parameter
#
	global D

	if [dict exists $D $par] {
		return [dict get $D $par]
	}

	return ""
}

###############################################################################

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

proc fpnorm { fn } {
#
# Normalizes the file-name path, accounts for the Cygwin/Windows duality
#
	global ST

	if { $ST(DP) && [string index $fn 0] == "/" } {
		# this is the only place where we may have a problem: we have
		# a full path from Cygwin, while the script needs DOS
		if ![catch { xq "cygpath" [list -w $fn] } fm] {
			log "Path (L->D): $fn -> $fm"
			set fn $fm
		} else {
			log "cygpath failed: $fn, $fm"
		}
	} elseif { !$ST(DP) && $ST(SYS) != "L" &&
	    [regexp "^\[A-Za-z\]:\[/\\\\\]" $fn] } {
		if ![catch { xq "cygpath" [list $fn] } fm] {
			log "Path (D->L): $fn -> $fm"
			set fn $fm
		} else {
			log "cygpath failed: $fn, $fm"
		}
	}
	return [file normalize $fn]
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

###############################################################################

proc pexpand { pa { un 0 } } {
#
# Expand the path to full
#
	global ST

	if { ![isfullpath $pa] && $ST(DBase) != "" } {
		set pa [file join $ST(DBase) $pa]
	}

	if $un {
		return [unipath $pa]
	} else {
		return [fpnorm $pa]
	}
}

proc start { } {

	global argv XD ST

	set ST(PWD) [pwd]

	if { [lindex $argv 0] == "-kill" } {
		set ST(KILL) 1
		set argv [lrange $argv 1 end]
	} else {
		set ST(KILL) 0
	}

	if { [llength $argv] > 2 } {
		usage
	}

	set dfn [lindex $argv 0]
	set lfn [lindex $argv 1]

	if { $dfn == "" } {
		set dfn "sidemon.xml"
	}

	if [catch { open $dfn "r" } fd] {
		abt "cannot open data file $dfn, $fd"
	}

	if [catch { read $fd } xd] {
		abt "cannot read data file $dfn, $fd"
	}

	catch { close $fd }

	if [catch { sxml_parse xd } XD] {
		abt "error in data, $XD"
	}

	if { $lfn != "" } {
		if [catch { log_open $lfn } fd] {
			abt "error, $fd"
		}
		if $ST(KILL) {
			set k " KILL"
		} else {
			set k ""
		}
		log "starting up$k"
	}
}

proc scandata { } {

	global XD ST

	set XD [sxml_child $XD "sidemon"]
	if { $XD == "" } {
		abt "no <sidemon> tag in input data"
	}

	set ST(DBase) [sxml_attr $XD "dirbase"]
	if { $ST(DBase) != "" } {
		log "dirbase: $ST(DBase)"
	}

	set ST(SBase) [sxml_attr $XD "sockbase"]
	if { $ST(SBase) != "" } {
		if [catch { valport $ST(SBase) } v] {
			abt "illegal sockbase, $v"
		}
		set ST(SBase) $v
		log "sockbase: $v"
	}

	set ST(DETACH) [sxml_yes $XD "detach"]

	set v [sxml_attr $XD "delay"]
	if { $v == "" } {
		set ST(Delay) 4000
	} else {
		if [catch { valnum $v 1 60 } ST(Delay)] {
			abt "illegal delay value $v, $ST(Delay)"
		}
		log "delay: $ST(Delay)"
		set ST(Delay) [expr { $ST(Delay) * 1000 }]
	}

	set v [sxml_attr $XD "interval"]
	if { $v == "" } {
		set ST(Interval) 15
	} else {
		if [catch { valnum $v 1 600 } ST(Interval)] {
			abt "illegal interval value $v, $ST(Interval)"
		}
		log "interval: $ST(Interval)"
		set ST(Interval) [expr { $ST(Interval) * 1000 }]
	}

	set v [sxml_child $XD "params"]
	if { $v != "" } {
		set ST(EPars) [string trim [sxml_txt $v]]
		log "params: $ST(EPars)"
	} else {
		set ST(EPars) ""
	}

	set dsl [sxml_children $XD "side"]
	if { $dsl == "" } {
		abt "no <side> tag, no runs to monitor"
	}

	set ST(Runs) ""

	set ns 0
	set sb $ST(SBase)
	foreach ds $dsl {

		incr ns

		set fp [sxml_yes $ds "followproject"]
		set so [sxml_attr $ds "socket"]
		set pa [sxml_child $ds "path"]
		set da [sxml_child $ds "data"]
		set ou [sxml_child $ds "output"]
		if { $pa != "" } {
			set pa [string trim [sxml_txt $pa]]
		}
		if { $da != "" } {
			set da [string trim [sxml_txt $da]]
		}
		if { $da == "" && !$fp } {
			abt "the data file name in set number $ns is null (and\
				followproject is not set)"
		}
		if { $ou != "" } {
			set ap [sxml_yes $ou "append"]
			set ou [string trim [sxml_txt $ou]]
		} else {
			set ap 0
		}
		if { $so == "" } {
			# use sockbase
			if { $sb == "" } {
				abt "no socket in set number $ns\
					(and no sockbase)"
			}
			set so $sb
			incr sb
		}
		if [catch { valport $so } v] {
			abt "set number $ns, illegal socket number $so, $v"
		}
		set so $v
		log "run $ns, path=$pa, data=$da, output=$ou, socket=$so,\
			fp=$fp, ap=$ap"

		lappend ST(Runs) [list $pa $da $ou $so $fp $ap]
		if [info exists ST(Runs,$so)] {
			abt "set number $ns, duplicate socket number $so"
		}

		set ST(Runs,$so) $ns
		set ST(Runs,$so,CR) 1
	}
}

proc active { sok request } {
#
# Check if the socket responds
#
	global ST

	if [catch { socket "localhost" $sok } Sok] {
		# failed to connect, presume dead
		log "failed to connect to $sok, $Sok"
		return 0
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none \
	    -translation binary -encoding binary } err] {
		log "socket $sok exists, but failed to configure, $err"
		catch { close $Sok }
		return 1
	}

	set ST(Status) 0
	fileevent $Sok writable "sock_write $Sok $request"
	set ST(CBack) [after $ST(Delay) "set ST(Status) -1"]

	while 1 {

		vwait ST(Status)
		if { $ST(Status) == 0 } {
			continue
		}

		catch { after cancel $ST(CBack) }
		catch { close $Sok }

		if { $ST(Status) == 1 } {
			log "illegal response on socket $sok"
		} elseif { $ST(Status) == 2 } {
			log "socket $sok closed unexpectedly"
		}

		return $ST(Status)
	}
}

proc sock_write { sok request } {

	global ST AGENT_MAGIC

	set rqs ""
	abinS rqs $AGENT_MAGIC
	abinS rqs $request
	abinI rqs 0
	abinI rqs 0

	fileevent $sok writable ""

	if [catch { puts -nonewline $sok $rqs } err] {
		set ST(Status) 2
		return
	}

	catch { flush $sok }

	fileevent $sok readable "sock_read $sok"
}

proc sock_read { sok } {

	global ST ECONN_OK

	if [catch { read $sok 4 } res] {
		# disconnection
		set ST(Status) 2
		return
	}

	if { $res == "" } {
		return
	}

	fileevent $sok readable ""

	if { [expr { [dbinI res] & 0xFF } ] != $ECONN_OK } {
		set ST(Status) 1
	} else {
		set ST(Status) 3
	}
}

proc cycle { } {

	global ST AGENT_RQST_ALIVE

	set scn 0

	foreach ru $ST(Runs) {

		set so [lindex $ru 3]

		if { $ST(Runs,$so) == 0 } {
			# bypass
			continue
		}


		if { [active $so $AGENT_RQST_ALIVE] == 3 } {
			# OK, the run is alive
			set ST(Runs,$so,CR) 1
			incr scn
			continue
		}

		# need to restart
		if [restart $ru] {
			incr scn
		}
	}

	if !$scn {
		abt "no runs left after disabling dead ones"
	}

	# to get rid of zombies
	catch { xq "true" "" }
}

proc report_error { ofn } {
#
# Locates the last error info in the output file and reports it in the log
#
	if [catch { open $ofn "r" } fd] {
		log "no previous output"
		return
	}

	if [catch { read $fd } out] {
		log "cannot read previous output file, $out"
		catch { close $fd }
		return
	}

	catch { close $fd }

	set ix [string last "SMURPH Version" $out]
	if { $ix < 0 } {
		log "no information in previous output"
		return
	}

	set out [split [string range $out $ix end] "\n"]

	set ret [lindex $out 0]
	set out [lrange $out 1 end]

	if [regexp {VUEE[[:space:]]+[^[:space:]]+[[:space:]]+(.+)} \
		$ret jnk ret] {
		log "previous run started on $ret"
	}

	set lre ""
	set fre ""
	foreach r $out {
		set r [string trim $r]
		if { [string range $r 0 2] != ">>>" } {
			if { $fre != "" } {
				set lre $fre
				set fre ""
			}
		} else {
			lappend fre [string range $r 4 end]
		}
	}

	if { $fre != "" } {
		set lre $fre
	}

	if { $lre == "" } {
		log "no error info found in output file"
		return
	}

	log "Error:"
	foreach r $lre {
		log "    $r"
	}
}

proc restart { run } {

	global ST P B PA SIDENAME

	foreach { pa da ou so fp ap } $run { }

	log "restarting: $pa, $da, $ou, $so, $fp, $ap"

	# path to the application
	set PA [pexpand $pa]

	if [catch { cd $PA } err] {
		log "no such directory $PA, disabling run $so"
		set ST(Runs,$so) 0
		cd $ST(PWD)
		return 0
	}

	if ![file isfile $SIDENAME] {
		log "no $SIDEFILE in $PA, disabling run $so"
		set ST(Runs,$so) 0
		cd $ST(PWD)
		return 0
	}

	# prepare arguments: side + vuee
	set as [list "-e"]
	set av [concat $ST(EPars) [list "-p" $so]]

	if $fp {
		# follow the project, i.e., include in the data any node data
		# from the respective boards; this requires parsing the project
		# config file and getting hold of the boards
		if ![get_config] {
			log "cannot determine project config file in $PA,\
				disabling run $so"
			set ST(Runs,$so) 0
			cd $ST(PWD)
			return 0
		}
		# unsynced/slomo
		set df [gpp "VUSM"]
		if { $df == "U" } {
			# unsynced
			set df 0
		} else {
			if { [catch { expr $df } df] || $df <= 0.0 } {
				# force the default in case of any trouble
				set df 1.0
			}
		}

		set dp [gpp "DPBC"]
		set mb [gpp "MB"]
		set bo [gpp "BO"]
		set po [gpp "PFAC"]

		if { $dp == 0 && $mb != "" && $bo != "" } {
			if $mb {
				# multiple boards
				set bi 0
				foreach b $bo {
					set suf [lindex $P $bi]
					set fna [file join $B $b "node.xml"]
					if [file isfile $fna] {
						lappend av "-n"
						lappend av $suf
						lappend av [unipath $fna]
					}
					incr bi
				}
			} else {
				set fna [file join $B $bo "node.xml"]
				if [file isfile $fna] {
					lappend av "-n"
					lappend av [unipath $fna]
				}
			}
		}

		if { $df != 1.0 } {
			# default resync interval
			set ef 500
			if { $df > 0 } {
				# calculate the resync interval in milliseconds
				# as 1/2 of the slo-mo factor or 1000,
				# whichever is less
				set ef [expr int($df * 500)]
				if { $ef <= 0 } {
					set ef 1
				} elseif { $ef > 1000 } {
					set ef 1000
				}
			}
			lappend av "-s"
			lappend av $df
			if { $df > 0 } {
				lappend av "-r"
				lappend av $ef
			}
		}

		if { $da == "" } {
			# use the project data file
			set da [gpp "VUDF"]
			if { $da == "" } {
				log "data file unspecified, no project data \
					file, disabling run $so"
				set ST(Runs,$so) 0
				cd $ST(PWD)
				return 0
			}
		}
	}

	if ![file isfile $da] {
		log "data file $da is not present in $PA, disabling run $so"
		set ST(Runs,$so) 0
		cd $ST(PWD)
		return 0
	}

	# detach?
	if $ST(DETACH) {
		lappend as "-b"
		if { $ou == "" || !$ap } {
			# no need for standard output
			lappend as "1"
		}
	}

	lappend as $da

	if { $ou != "" } {
		# there was an output file
		if $ST(Runs,$so,CR) {
			report_error $ou
		}
		if !$ap {
			lappend as $ou
		}
	}

	# mark as reported
	set ST(Runs,$so,CR) 0

	lappend as "--"
	set as [concat $as $av]

	if { $ou != "" && $ap } {
		# the output file is appended
		lappend as ">>"
		lappend as $ou
		lappend as "2>"
	} else {
		lappend as ">&"
	}

	lappend as "/dev/null"
	lappend as "&"

	if [catch { xq "./$SIDENAME" $as } err] {
		log "failed to execute, $err"
	}

	cd $ST(PWD)
	return 1
}

proc zap_them { } {

	global ST AGENT_RQST_STOP

	foreach ru $ST(Runs) {

		set so [lindex $ru 3]

		if { $ST(Runs,$so) == 0 } {
			# bypass, not needed here
			continue
		}


		if { [active $so $AGENT_RQST_STOP] == 3 } {
			log "killed $so"
		}
	}
}

proc main { } {

	global ST

	start
	scandata

	if $ST(KILL) {
		zap_them
		exit 0
	}

	while 1 {
		cycle
		after $ST(Interval)
	}
}

main
