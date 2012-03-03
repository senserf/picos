#!/bin/sh
###########################\
exec wish "$0" "$@"

##########################################################
#                                                        #
# ConSenT version 1.3.D                                  #
#                                                        #
# Copyright (C) Olsonet Communications Corporation, 2012 #
#                                                        #
##########################################################

##
## This is a modified version of econnect (from eco_demo/OSSI/ECONNECT)
## with some features removed and some added - to be used standalone (no
## installation) on Linux as well as Windows (no Cygwin needed); should work
## with Tk 8.4 as well as 8.5
##

package require Tk

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
	# sanitize arguments; here you a sample of the magnitude of stupidity
	# I have to fight when glueing together Windows and Cygwin stuff;
	# the last argument (sometimes!) has a CR character appended at the
	# end, and you wouldn't believe how much havoc that can cause
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}
	unset u
}

###############################################################################

package provide vuart 1.0
##########################################################
# This is a package for direct VUEE-UART communication.  #
# Copyright (C) 2012 Olsonet Communications Corporation. #
##########################################################

namespace eval VUART {

variable VU

proc abin_S { s h } {
#
# append one short int to string s (in network order)
#
	upvar $s str
	append str [binary format S $h]
}

proc abin_I { s l } {
#
# append one 32-bit int to string s (in network order)
#
	upvar $s str
	append str [binary format I $l]
}

proc dbin_Q { s } {
#
# decode return code from string s
#
	upvar $s str
	if { $str == "" } {
		return -1
	}
	binary scan $str I val
	set str [string range $str 4 end]
	return [expr $val & 0xff]
}

proc init { abv } {

	variable VU

	# callback
	set VU(CB) ""

	# error status
	set VU(ER) ""

	# socket file descriptor
	set VU(FD) ""

	# abort variable
	set VU(AV) $abv
}

proc abtd { } {

	variable VU
	upvar #0 $VU(AV) abv

	return $abv
}

proc kick { } {

	variable VU
	upvar #0 $VU(AV) abv

	set abv $abv
}

proc cleanup { { ok 0 } } {

	variable VU

	stop_cb

	if !$ok {
		catch { close $VU(FD) }
	}
	array unset VU
}

proc stop_cb { } {

	variable VU

	if { $VU(CB) != "" } {
		catch { after cancel $VU(CB) }
		set VU(CB) ""
	}
}

proc sock_flush { } {
#
# Completes the flush for an async socket
#
	variable VU

	stop_cb

	if [abtd] {
		return
	}

	if [catch { flush $VU(FD) } err] {
		set VU(ER) "Write to VUEE failed: $err"
		kick
		return
	}

	if [fblocked $VU(FD)] {
		# keep trying while blocked
		set VU(CB) [after 200 ::VUART::sock_flush]
		return
	}

	# done
	kick
}

proc wkick { } {
#
# Wait for a kick (or abort)
#
	variable VU
	uplevel #0 "vwait $VU(AV)"
}

proc sock_read { } {

	variable VU

	kick

	if { [catch { read $VU(FD) 4 } res] || $res == "" } {
		# disconnection
		set VU(ER) "Connection refused"
		return
	}

	set code [dbin_Q res]

	if { $code != 129 } {
		# wrong code
		set VU(ER) "Connection refused by VUEE, code $code"
		return
	}

	# connection OK
}

proc vuart_conn { ho po no abvar } {
#
# Connect to UART at the specified host, port, node; abvar is the abort 
# variable (if set to 1, it means that we should abort)
#
	variable VU

	init $abvar

	if [abtd] {
		cleanup
		error "Preaborted"
	}

	# these actions cannot block
	if [catch { socket -async $ho $po } sfd] {
		cleanup
		error "Connection failed: $sfd"
	}

	set VU(FD) $sfd

	if [catch { fconfigure $sfd -blocking 0 -buffering none \
    	    -translation binary -encoding binary } erc] {
		cleanup
		error "Connection failed: $erc"
	}

	# prepare a request
	set rqs ""
	abin_S rqs 0xBAB4
	abin_S rqs 1
	abin_I rqs $no

	if [catch { puts -nonewline $sfd $rqs } erc] {
		cleanup
		error "Write to VUEE failed: $erc"
	}

	after 200 ::VUART::sock_flush
	wkick

	if [abtd] {
		cleanup
		error "Aborted"
	}

	set err $VU(ER)
	if { $err != "" } {
		cleanup
		error $err
	}

	# wait for a reply
	fileevent $VU(FD) readable ::VUART::sock_read

	wkick

	if [abtd] {
		cleanup
		error "Aborted"
	}

	set err $VU(ER)
	if { $err != "" } {
		cleanup
		error $err
	}

	set fd $VU(FD)
	cleanup 1

	return $fd
}

namespace export vuart_conn

### end of VUART namespace ####################################################
}

namespace import ::VUART::vuart_conn

###############################################################################

### Parameters ################################################################

# version number
set PM(VER)	1.3.C

# maximum number of ports to try
if { $ST(SYS) == "L" } {
	set PM(MPS)	0
	set PM(MPE)	19
	set PM(MPP)	9
} else {
	set PM(MPS)	1
	set PM(MPE)	32
}

# promiscuous connections
set PM(PMS)	1

# data directory path (relative to HOME/APPDATA/whatever)
set PM(HOM)	"econnect"

# "program" directory path; relative to PROGRAMFILES; will be reset by
# set_home_dir; NOTE: I am disabling this; we shall just try to call
# esdreader
# set PM(PGM)	[file join Olsonet EConnect]
set PM(PGM)	""
set PM(ESR)	"esdreader"

# sensor conversion formulas
set PM(COB)	"snippets.dat"

# Max line count for scrollable terminals
set PM(MLC)	1024

# Aggregator status headers
set PM(ASH)	{
			"Uptime:"
			"Master:"
			"Writing to storage:"
			"Entries stored:"
			"Audit interval:"
		}

# Custodian status headers
set PM(USH)	{
			"Uptime:"
			"Master:"
			"Writing to storage:"
			"Entries stored:"
			"Battery voltage:"
			"Satellite time:"
			"Signal quality:"
			"Lattitude:"
			"Longitude:"
			"Altitude:"
		}

# Collector status headers
set PM(CSH)	{
			"Uptime:"
			"Writing to storage:"
			"Entries stored:"
			"Sampling interval:"
		}

# VUEE host, port, node
set PM(VHO)	"localhost"
set PM(VPO)	4443
set PM(VNO)	0


# Counter to assign distinct names to some global windows
set ST(CNT)	0

# Variable to unhook modal windows
set ST(MOD)	0

# Double Exit Avoidance Flag
set DEAF	0

### Patterns ##################################################################

# time stamp
set PT(TST) 	"(\[0-9\]+-\[0-9\]+-\[0-9\]+ \[0-9\]+:\[0-9\]+:\[0-9\]+)"

# aggregator's status
set PT(AST)	": (\[0-9\]+).: Audit\[^0-9\]+(\[0-9\]+).*a_fl (\[0-9a-f\]+) "
	append PT(AST) "Uptime (\[0-9\]+).*aster (\[0-9\]+)\[^0-9\]+(\[0-9\]+)"

# aggregator's data sample line
set PT(AGS)	"^1007 Col (\[0-9\]+) slot (\[0-9\]+).*A: (\[0-9\]+). "
	append PT(AGS) $PT(TST)
	append PT(AGS) "..A "
	append PT(AGS) $PT(TST)
	append PT(AGS) ". (.+)"

# aggregator's event
set PT(AEV)	"^1011 (\[^ \]+) (\[0-9\]+) "
	append PT(AEV) $PT(TST)
	append PT(AEV) " (.+)"

# aggregator's end of list
set PT(AEL)	"^1008 Did"

# collector's status
set PT(CST)	": (\[0-9\]+).: Maj_freq (\[0-9\]+).*c_fl (\[0-9a-f\]+) "
	append PT(CST) "Uptime (\[0-9\]+) .*reads (\[0-9\]+)"

# collector's data sample line
set PT(COS)	"^2007 (\[^ \]+) slot (\[0-9\]+) "
	append PT(COS) $PT(TST)
	append PT(COS) ". (.+)"

# collector's event
set PT(CEV)	"^2011 (\[^ \]+) (\[0-9\]+) "
	append PT(CEV) $PT(TST)
	append PT(CEV) " (.+)"

# collector's end of list
set PT(CEL)	"^2008 Collector"

# satellite status lines
set PT(SSA)	"^.CMGS: .System Status.,\"(..-..-)(....)(..+)\""
set PT(SSB)	"^(\[0-9\]+),(\[^ ,\]+),(\[^ ,\]+),\[^ ,\]+,\[^ ,\]+,\[^ ,\]+,"
	append PT(SSB) "(\[^ ,\]+),\[^ ,\]+,"

###############################################################################

proc terminate { } {

	global DEAF

	if $DEAF { return }

	set DEAF 1

	exit 0
}

proc abt { msg } {

	alert $msg
	terminate
}

### Snippets ##################################################################

package provide snippets 1.0
#
# Conversion snippets for sensors
#

namespace eval SNIPPETS {

# snippet cache
variable SC

# the snippets themselves
variable SN

# parameters
variable PM

# max snippet name length
set PM(SNL)	32

# number of sensor assignment columns in the snippet map (restricted to fit
# a window, but hardly hard)
set PM(NAS)	4

# maximum number of sensors per node
set PM(SPN)	10

proc snip_getparam { pm } {

	variable PM

	if ![info exists PM($pm)] {
		return ""
	} else {
		return $PM($pm)
	}
}

proc snip_cnvrt { v s c { units "" } { name "" } } {
#
# Convert a raw sensor value
#
	variable SN
	variable SC

	if ![info exists SC($c,$s)] {
		# build the cache entry
		set fn 0
		foreach sn [array names SN] {
			foreach a [lrange $SN($sn) 2 end] {
				if { [lindex $a 0] != $s } {
					# not this sensor
					continue
				}
				# scan the collector range
				set rl [lindex $a 1]
				if { $rl == "" } {
					# all
					set fn 1
					break
				}
				foreach r [lindex $a 1] {
					set x [lindex $r 0]
					if { $c == $x } {
						set fn 1
						break
					}
					set y [lindex $r 1]
					if { $y != "" && $c > $x && $c <= $y } {
						set fn 1
						break
					}
				}
				if $fn {
					break
				}
			}
			if $fn {
				break
			}
		}
		if $fn {
			# the snippet and units
			set SC($c,$s) [lrange $SN($sn) 0 1]
			lappend SC($c,$s) $sn
		} else {
			set SC($c,$s) ""
		}
	}

	set snip [lindex $SC($c,$s) 0]

	if { $units != "" } {
		upvar $units un
		set un [lindex $SC($c,$s) 1]
	}

	if { $name != "" } {
		upvar $name na
		set na [lindex $SC($c,$s) 2]
	}

	if { $snip == "" } {
		return ""
	}

	if [catch { snip_eval $snip $v } v] {
		set v "?"
	}

	if { $v != "?" } {
		if [catch { expr $v } v] {
			set v "?"
		} else {
			set v [format %1.2f $v]
		}
	}
	return $v
}

proc snip_icache { } {
#
# Invalidate snippet cache
#
	variable SC

	array unset SC
}

proc ucs { cl } {
#
# Unpack and validate a collector set
#
	set v ""

	while 1 {

		set cl [string trimleft $cl]

		if { $cl == "" } {
			# empty is OK, it stands for all
			break
		}

		if [regexp -nocase "^all" $cl] {
			# overrides everything
			return "all"
		}

		if { [string index $cl 0] == "," } {
			# ignore commas
			set cl [string range $cl 1 end]
			continue
		}

		# expect a number or range
		set a ""
		set b ""
		if ![regexp "^(\[0-9\]+) *- *(\[0-9\]+)" $cl ma a b] {
			regexp "^(\[0-9\]+)" $cl ma a
		}

		if { $a == "" } {
			# error
			return ""
		}

		set a [vnum $a 0 65536]
		if { $a == "" } {
			return ""
		}

		if { $b == "" } {
			# single value
			lappend v $a
		} else {
			set b [vnum $b 0 65536]
			if { $b == "" || $b < $a } {
				return ""
			}
			# range
			lappend v "$a $b"
		}

		set cl [string range $cl [string length $ma] end]
	}

	if { $v == "" } {
		return "all"
	} else {
		return $v
	}
}

proc vnum { n { min "" } { max "" } } {
#
# Verify integer number
#
	if [catch { expr int($n) } n] {
		return ""
	}

	if { ($min != "" && $n < $min) || ($max != "" && $n >= $max) } {
		return ""
	}

	return $n
}

proc pcs { lv } {
#
# Convert collector set from list to text
#
	if { $lv == "" } {
		return "all"
	}

	set tx ""

	foreach s $lv {
	
		set a [lindex $s 0]
		set b [lindex $s 1]

		if { $tx != "" } {
			append tx " "
		}

		append tx $a

		if { $b != "" } {
			append tx "-$b"
		}
	}

	return $tx
}

proc snip_names { } {

	variable SN

	return [lsort [array names SN]]
}

proc snip_vsn { nm } {
#
# Validate snippet name
#
	if { $nm == "" } {
		return 1
	}

	if { [string length $nm] > [snip_getparam SNL] } {
		return 1
	}

	if ![regexp -nocase "^\[a-z\]\[0-9a-z_\]*$" $nm] {
		return 1
	}

	return 0
}

proc snip_assignments { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	set asl [lrange $SN($nm) 2 end]

	set res ""

	foreach as $asl {
		lappend res [list [lindex $as 0] [pcs [lindex $as 1]]]
	}

	return $res
}

proc snip_units { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	return [lindex $SN($nm) 1]
}

proc snip_script { nm } {

	variable SN

	if ![info exists SN($nm)] {
		return ""
	}

	return [lindex $SN($nm) 0]
}

proc snip_exists { nm } {

	variable SN

	return [info exists SN($nm)]
}

proc snip_delete { nm } {

	variable SN

	if [info exists SN($nm)] {
		unset SN($nm)
	}
}

proc snip_set { nm scr units asl } {

	variable SN

	if [snip_vsn $nm] {
		return "the name is invalid, must be no more than\
			[snip_getparam SNL] alphanumeric characters starting\
			with a letter (no spaces)"
	}

	if [catch { snip_eval $scr 1000 } err] {
		return "execution fails, $err"
	}

	set ssl ""
	set i 0
	foreach as $asl {
		set asg [lindex $as 0]
		set cli [ucs [lindex $as 1]]
		incr i
		if { $cli == "" } {
			return "node set number $i is invalid"
		}
		lappend ssl [list $asg $cli]
	}

	set SN($nm) [concat [list $scr] [list $units] $ssl]

	return ""
}

## version dependent code #####################################################

if { [info tclversion] < 8.5 } {

	proc snip_eval { sn val } {
	#
	# Safely evaluates a snippet for a given value
	#
		set in [interp create -safe]

		if [catch {
			# build the script
			set s "set value $val\n$sn\n"
			append s { return $value }
			set s [interp eval $in $s]
		} err] {
			# make sure to clean up
			interp delete $in
			error $err
		}
		interp delete $in
		return $s
	}

} else {

	proc snip_eval { sn val } {
	#
	# Safely evaluates a snippet for a given value
	#
		set in [interp create -safe]
	
		if [catch {
			# make sure we get out of loops
			interp limit $in commands -value 512

			# build the script
			set s "set value $val\n$sn\n"
			append s { return $value }
			set s [interp eval $in $s]
		} err] {
			# make sure to clean up
			interp delete $in
			error $err
		}

		interp delete $in
		return $s
	}

}

proc snip_parse { cf } {
#
# Parse conversion snippets, e.g., read from a file
#
	variable SN

	# invalidate the cache
	snip_icache

	set ix 0

	set snl [snip_getparam SNL]
	set nas [snip_getparam NAS]

	foreach snip $cf {

		# we start from 1 and end up with the correct count
		incr ix

		# snippet name
		set nm [lindex $snip 0]

		# the code
		set ex [lindex $snip 1]

		# the units
		set un [lindex $snip 2]

		if [snip_vsn $nm] {
			return "illegal name '$nm' of snippet number $ix,\
				must be no more than $snl alphanumeric\
				characters starting with a letter (no spaces)"
		}
		if [info exists SN($nm)] {
			return "duplicate snippet name '$nm', snippet number\
				$ix"
		}

		# this is the list of up to PM(NAS) assignments
		set asgs [lrange $snip 3 end]

		if { [llength $asgs] > $nas } {
			return "too many (> $nas) assignments in snippet\
				'$nm' (number $ix)"
		}

		if [catch { snip_eval $ex 1000 } er] {
			return "cannot evaluate snippet $nm (number $ix), $er"
		}

		# we produce an internal representation, which is a name-indexed
		# bunch of lists
		
		set SN($nm) [list $ex]
		lappend SN($nm) $un
		set spn [snip_getparam SPN]

		set iy 0
		foreach as $asgs {

			incr iy

			# this is a sensor number (small)
			set ss [lindex $as 0]
			set se [vnum $ss 0 $spn]

			if { $se == "" } {
				return "illegal sensor number '$ss' in\
					assignment $iy of snippet '$nm'"
			}

			# this is a set of collectors which consists of
			# individual numbers and/or ranges
			set ss [lindex $as 1]
			set sv [ucs $ss]

			if { $sv == "" } {
				return "illegal node range '$ss' in\
					 assignment $iy of snippet '$nm'"
			}
			if { $sv == "all" } {
				set sv ""
			}
			lappend SN($nm) [list $se $sv]
		}
	}
	return ""
}

proc snip_scmp { a b } {
#
# Compares two strings in a flimsy sort of way
#
	set a [string trim $a]
	regexp -all "\[ \t\n\r\]" $a " " a
	set b [string trim $b]
	regexp -all "\[ \t\n\r\]" $b " " b

	return [string compare $a $b]
}

proc snip_encode { } {
#
# Encode conversion snippets to be written back to the file
#
	variable SN

	set res ""

	foreach nm [array names SN] {

		set curr ""

		set ex [lindex $SN($nm) 0]
		set un [lindex $SN($nm) 1]
		set al [lrange $SN($nm) 2 end]

		lappend curr $nm
		lappend curr $ex
		lappend curr $un

		foreach a $al {

			set se [lindex $a 0]
			set cs [lindex $a 1]

			lappend curr [list $se [pcs $cs]]
		}

		lappend res $curr
	}

	return $res
}

namespace export snip_*

}

namespace import ::SNIPPETS::*

###############################################################################

### Logging ###################################################################

package provide log 1.0
#
# Log functions
#

namespace eval LOGGING {

variable Log

proc abt { m } {

	if [catch { ::abt $m } ] {
		catch { alert "Aborted: $m" }
		exit 99
	}
}

proc log_open { { fname "" } { maxsize "" } { maxvers "" } } {

	variable Log

	if { $fname == "" } {
		if ![info exists Log(FN)] {
			set Log(FN) "log"
			set Log(SU) ".txt"
		}
	} else {
		set Log(FN) [file rootname $fname]
		set Log(SU) [file extension $fname]
		if { $Log(SU) == "" } {
			set Log(SU) ".txt"
		}
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

	set fn "$Log(FN)$Log(SU)"
	if [catch { file size $fn } fs] {
		# not present
		if [catch { open $fn "w" } fd] {
			abt "Cannot open log file $fn: $fd"
		}
		# empty log
		set Log(SZ) 0
	} else {
		# log file exists
		if [catch { open $fn "a" } fd] {
			abt "Cannot open log file $fn: $fd"
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
		set tfn "$Log(FN)_$i$Log(SU)"
		set ofn $Log(FN)
		if { $i > 1 } {
			append ofn "_[expr $i - 1]"
		}
		append ofn $Log(SU)
		catch { file rename -force $ofn $tfn }
	}

	log_open
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

proc llog { prt msg } {
#
#
	global WN

	if { $prt != "" } {
		log "COM$prt <$WN(NT,$prt)> $msg"
		if [info exists WN(w,$prt,L)] {
			set w $WN(w,$prt,L).t
			catch {
				add_text $w $msg
				end_line $w
			}
		}
		return
	}

	log $msg
}

proc fclose { fd } {

	catch { close $fd }

}

proc u_cdevl { pi } {
#
# Returns the candidate list of devices to open based on the port identifier
#
	if { [regexp "^\[0-9\]+$" $pi] && ![catch { expr $pi } pn] } {
		# looks like a number
		if { $pn < 10 } {
			# use internal Tcl COM id, which is faster
			set wd "COM${pn}:"
		} else {
			set wd "\\\\.\\COM$pn"
		}
		return [list $wd "/dev/ttyUSB$pn" "/dev/tty$pn"]
	}

	# not a number
	return [list $pi "\\\\.\\$pi" "/dev/$pi" "/dev/tty$pi"]
}

proc u_tryopen { pn } {
#
# Tries to open UART port pn
#
	set dl [u_cdevl $pn]

	foreach dev $dl {
		if ![catch { open $dev "r+" } fd] {
			return $fd
		}
	}

	return ""
}

proc u_preopen { } {
#
# Determine which COM ports are openable
#
	global PM ST

	# note: it used to be automatic, but detecting ports above 9 is
	# extremely slow, so let us hardwire this list (leaving auto
	# detection as an (unused) option

	append res " VUEE"

	for { set pn $PM(MPS) } { $pn <= $PM(MPE) } { incr pn } {
		append res " $pn"
	}

	if { $ST(SYS) == "L" } {
		for { set pn $PM(MPS) } { $pn <= $PM(MPP) } { incr pn } {
			append res " /dev/pts/$pn"
		}
	} else {
		append res " CNCA0 CNCA1 CNCA2 CNCA3"
	}

	return $res
}

proc u_conf { prt speed } {
#
# Centralized fconfigure for the UART
#
	global WN

	set fd $WN(FD,$prt)

	# the generic part of UART configuration
	if [catch { fconfigure $fd -mode "$speed,n,8,1" -handshake none \
	    -buffering none \
	    -blocking 0 -eofchar "" -ttycontrol { RTS 0 } } err] {
		llog $prt "couldn't configure $prt: $err"
		return 1
	}

	# accumulated input buffer
	set WN(IB,$prt) ""

	fileevent $fd readable "u_iready $prt"
	return 0
}

proc u_iready { prt } {
#
# Handle "input available" on the UART
#
	global WN

	if { [catch { read $WN(FD,$prt) } sta] || $sta == "" } {
		return
	}

	append WN(IB,$prt) $sta

	while 1 {
		# handle all lines that may have arrived together; note:
		# it used to be different (line buffering events), but that
		# didn't work with virtual UARTs on Windows
		set sta [string first "\n" $WN(IB,$prt)]
		if { $sta < 0 } {
			return
		}
		set line [string range $WN(IB,$prt) 0 [expr $sta - 1]]
		set WN(IB,$prt) [string range $WN(IB,$prt) [expr $sta + 1] end]

		u_rdline $prt [string trim $line]
	}
}

proc u_rdline { prt line } {

	global WN

	if { $line == "" } {
		return
	}

	if { $WN(SM,$prt) != "" } {
		# we have a status monitoring function
		$WN(SM,$prt) $prt $line
	}

	if { $WN(EX,$prt) != "" } {
		# expecting something
		if [regexp -nocase $WN(EX,$prt) $line] {
			set WN(LI,$prt) $line
		}
	}

	if { $WN(SH,$prt) && [regexp "^1002 " $line] } {
		show_sensors $prt $line
	}

	if !$WN(SE,$prt) {
		# log all lines that are not sample extractions
		llog $prt "<- $line"
		return
	}

	#######################################################################
	### sample extraction #################################################
	#######################################################################

	set em $WN(SE,$prt)

	if { $em < 16 } {
		if [extract_aggregator_samples $prt $em $line] {
			# no log
			return
		}
	} else {
		if [extract_collector_samples $prt [expr $em - 16] $line] {
			# no log
			return
		}
	}

	# log it
	llog $prt "<- $line"
}

proc vnum { n { min "" } { max "" } } {
#
# Verify integer number
#
	if [catch { expr int($n) } n] {
		return ""
	}

	if { ($min != "" && $n < $min) || ($max != "" && $n >= $max) } {
		return ""
	}

	return $n
}

proc preconf_port { prt fd } {
#
# Preconfigure the port's record, given the UART descriptor
#
	global WN

	set WN($prt) ""

	set WN(FD,$prt) $fd

	# waiting for some specific line (contains the pattern)
	set WN(EX,$prt) ""

	# showing sensor values
	set WN(SH,$prt) 0

	# status monitor
	set WN(SM,$prt) ""

	# extracting samples
	set WN(SE,$prt) 0

	# expected line (just in case)
	set WN(LI,$prt) ""

	# default identity of the satnode (when driven by a custodian)
	set WN(AI,$prt) 10

	# node type: unknown yet
	set WN(NT,$prt) ""

	# node Id: unknown
	set WN(NI,$prt) ""

	# no command in progress
	set WN(CP,$prt) 1
}

proc stop_port { pn } {
#
# Terminate the indicated connection
#
	global WN ST

	if { $ST(CPR) == $pn } {
		set ST(CPR) ""
	}

	#
	# Here is the naming convention for session attributes (stored in WN):
	#
	#	XX,$pn		- regular variables (to be simply unset)
	#	w,$pn,X		- windows (to be destroyed)
	#	t,$pn,X		- timeouts (to be cancelled)
	#

	# cancel timeouts
	foreach nm [array names WN "t,$pn,*"] {
		if { $WN($nm) != "" } {
			catch { after cancel $WN($nm) }
		}
		catch { unset WN($nm) }
	}

	# destroy windows
	foreach nm [array names WN "w,$pn,*"] {
		catch { destroy $WN($nm) }
		catch { unset WN($nm) }
	}

	# close the UART
	if [info exists WN(FD,$pn)] {
		# UART open
		fclose $WN(FD,$pn)
		unset WN(FD,$pn)
	}

	# unset variables
	array unset WN "*,$pn"
}

proc try_port { prt nt } {
#
# Check if a node is responding at the port and determine its type
#
	global WN PM

	set ret [issue $prt "h" "^\[12\]001.* commands" 3 1 nowarn]

	if { $ret < 0 } {
		# abort
		return -1
	}

	if { $ret > 0 } {
		# failure, check if should try converting satnode to aggregator
		if { $nt != "Any" && $nt != "Aggregator" } {
			return 1
		}
		for { set i 0 } { $i < 3 } { incr i } {
			sendm $prt "s---"
			set ret [issue $prt "h" "^1001.* commands" 3 1 nowarn]
			if { $ret < 0 } {
				return -1
			}
			if { $ret == 0 } {
				# satnode flag
				set WN(SA,$prt) 1
				break
			}
		}
	}

	if $ret {
		return 1
	}

	set r $WN(LI,$prt)

	if { [string first "Aggregator" $r] >= 0 } {
		set tp "aggregator"
	} elseif { [string first "Collector" $r] >= 0 } {
		set tp "collector"
	} elseif { [string first "Custodian" $r] >= 0 } {
		set tp "custodian"
	} else {
		# unknown node type
		alert "Node responding on port $prt has an unknown type, it\
			will be ignored!"
		return 1
	}

	if { $nt != "Any" && [string compare -nocase $nt $tp] != 0 } {
		return 1
	}

	# check the version
	if ![regexp "\[123\]\\.\[0-9\]" $r ver] {
		set ver "unknown"
	}

	regsub "\[.\]\[^.\]*$" $PM(VER) "" vv

	if { $ver != $vv && !$PM(PMS) } {
		if ![confirm "The node ($tp) responding on port [prtname $prt]\
			runs software version $ver, which is incompatible with\
			$vv required by this program. If you say YES,\
			I will try to connect to that node at the risk of\
			problems."] {

			return 1
		}
	}

	set WN(NT,$prt) $tp
	return 0
}

proc itmout { prt } {

	global WN

	set WN(LI,$prt) "@T"
}

proc cancel_timeout { prt id } {

	global WN

	set id "t,$prt,$id"

	if [info exists WN($id)] {
		catch { after cancel $WN($id) }
		set WN($id) ""
	}
}

proc set_timeout { prt id del act } {

	global WN

	set id "t,$prt,$id"

	if { [info exists WN($id)] && $WN($id) != "" } {
		# cancel the previous one
		catch { after cancel $WN($id) }
	}

	set WN($id) [after [expr $del * 1000] $act]
}

proc cancel_issue { prt } {
#
# Cancels an 'issue' in progress
#
	global WN

	cancel_timeout $prt I
	set WN(LI,$prt) "@A"
}

proc sendm { prt cmd } {

	global WN

	llog $prt "-> $cmd"

	catch { puts $WN(FD,$prt) $cmd }
}

proc issue { prt cmd pat ret del { nowarn "" } } {
#
# Issues a command to the node
#
	global WN

	set WN(EX,$prt) $pat
	# make sure it is set
	set WN(LI,$prt) "@@"

	while { $ret > 0 } {
		sendm $prt $cmd

		set_timeout $prt I $del "itmout $prt"
		vwait WN(LI,$prt)
		cancel_timeout $prt I

		set rv $WN(LI,$prt)
		if { $rv == "@A" } {
			# abort
			set WN(EX,$prt) ""
			return -1
		}
		if { $rv != "@T" } {
			# the line: success
			set WN(EX,$prt) ""
			return 0
		}
		# keep waiting
		incr ret -1
	}
	# failure
	set WN(EX,$prt) ""
	if { $nowarn == "" } {
		alert "Timeout: the $WN(NT,$prt) on port $prt has failed to\
			respond on time!"
	}
	return 1
}

proc ciss { prt cmd pat ret del { nowarn "" } } {
#
# Includes command cancellation
#
	global WN

	if $WN(CP,$prt) {
		return -1
	}

	return [issue $prt $cmd $pat $ret $del $nowarn]
}

###############################################################################

proc alert { msg } {

	llog "" "Alert: $msg"
	tk_dialog .alert "Attention!" $msg "" 0 "OK"
}

proc confirm { msg } {

	set res [tk_dialog .alert "Warning!" $msg "" 0 "NO" "YES"]
	llog "" "Confirm: $msg -- returns $res"
	return $res
}

proc cconfirm { msg } {

	set res [tk_dialog .alert "Question!" $msg "" 0 "NO" "YES" "Cancel"]
	llog "" "Confirm: $msg -- returns $res"
	return $res
}

proc disable_main_window { } {

	.conn.connect configure -state disabled

}

proc enable_main_window { } {

	.conn.connect configure -state normal

}

proc mk_mess_window { canc w h txt } {
#
# The first arument is a canceller function - to be called when the user
# clicks the Cancel button (or destroys the window)
#
	global ST

	incr ST(CNT)

	set wn ".msg$ST(CNT)"

	toplevel $wn

	wm title $wn "Message"

	label $wn.t -width $w -height $h -borderwidth 2 -state normal
	pack $wn.t -side top -fill x -fill y
	button $wn.c -text "Cancel" -command $canc
	pack $wn.c -side top
	bind $wn <Destroy> $canc
	$wn.t configure -text $txt
	raise $wn

	return $wn
}

proc outmess { msgw msg } {

	return [catch { $msgw.t configure -text $msg }]
}

proc coutm { prt msgw msg } {
#
# This one includes catching cancellations
#
	global WN

	if $WN(CP,$prt) {
		return 1
	}

	return [outmess $msgw $msg]
}

proc cancel_connect { } {

	global ST

	if { $ST(CPR) != "" } {

		set prt $ST(CPR)
		llog $prt "connect cancelled"

		# avoid loops
		set ST(CPR) ""

		cancel_issue $prt
		set ST(CAN) 1
	}
}

proc cancel_node_command { prt } {

	global WN

	if ![info exists WN(CP,$prt)] {
		# no window
		return
	}

	if !$WN(CP,$prt) {
		# avoid loops
		set WN(CP,$prt) 1
		cancel_issue $prt
	}
}

proc update_status { prt { rcl 0 } } {

	global WN

	# issue a command for immediate status report

	if { $rcl == 0 } {
		# called explicitly rather than from a recall loop
		cancel_timeout $prt S
		# initial repeat interval == 10 seconds
		set WN(SI,$prt) 10
	}

	switch $WN(NT,$prt) {

		"custodian"  {
				if { $WN(NI,$prt) == "" } {
					sendm $prt "a"
				}
				sendm $prt "a $WN(AI,$prt)"
			     }

		"aggregator" {	sendm $prt "a" }

		"collector"  {	sendm $prt "s" }

		default	     {  return }
	}

	set_timeout $prt S $WN(SI,$prt) "update_status $prt 1"
}

proc fltos { fl } {

	switch [expr $fl & 0x3] {

	0 { return "nothing" }
	1 { return "not confirmed" }
	2 { return "confirmed" }

	}

	return "all"
}

proc smw { prt tt } {
#
# Start modal window
#
	global WN

	if [info exists WN(w,$prt,M)] {
		alert "Illegal modal window, internal error 0002!"
		return
	}

	# there should only be one at a time, right?
	set w ".mo"

	set WN(w,$prt,M) $w

	toplevel $w

	wm title $w $tt

	# make it modal
	grab $w

	return $w
}

proc dmw { prt } {
#
# Destroy modal window
#
	global WN MV

	if [info exists WN(w,$prt,M)] {
		catch { destroy $WN(w,$prt,M) }
		catch { unset WN(w,$prt,M) }
	}

	# unset all modal variables
	array unset MV
}

proc uptime { upt } {

	set upt [vnum $upt 0]
	if { $upt == "" } {
		return "unknown"
	}

	set s "[expr $upt % 60]s"
	set m [expr $upt / 60]
	if $m {
		set s "[expr $m % 60]m, $s"
		set m [expr $m / 60]
		if $m {
			set s "[expr $m % 24]h, $s"
			set m [expr $m / 24]
			if $m {
				set s "${m}d, $s"
			}
		}
	}

	return $s
}

proc agg_status_monitor { prt line } {

	global WN PT

	if ![regexp "^1005" $line] {
		return
	}

	# this looks like a status line
	if ![regexp -nocase $PT(AST) $line j agg fre fla upt mas sam] {
		llog $prt "bad response from node: $line"
		return
	}

        if [catch { expr 0x$fla } fla] {
                set fla 0
        }

	set WN(NI,$prt) $agg

	if { $mas == $agg } {
		set mas "this node"
	}
		
	set WN(SV,$prt) [list [uptime $upt] $mas [fltos $fla] $sam "$fre s"]
	out_status $prt

	set WN(SI,$prt) 60

	# slow rate is once per minute for an aggregator
	set_timeout $prt S 60 "update_status $prt 1"
}

proc cus_status_monitor { prt line } {

	global WN PT

	set upd 0

	if [regexp $PT(SSA) $line jk md yr tm] {

		# got time from the satellite
		if { [catch { expr $yr } yr] || $yr < 2009 } {
			# not set
			set tm "??"
		} else {
			set tm "$md$yr$tm"
		}

		set WN(SV,$prt) [lreplace $WN(SV,$prt) 5 5 $tm]
		set upd 1

	} elseif [regexp $PT(SSB) $line jk ss la lo al] {

		set WN(SV,$prt) [lreplace $WN(SV,$prt) 6 6 $ss]

		if { $la == "n/a" } {
			set WN(SV,$prt) [lreplace $WN(SV,$prt) 7 9 \
				"??" "??" "??"]
		} else {
			set WN(SV,$prt) [lreplace $WN(SV,$prt) 7 9 \
				$la $lo $al]
		}
		set upd 1

	} elseif [regexp "^1005" $line] {

		# this looks like a status line
		if ![regexp -nocase $PT(AST) $line j agg fre fla upt mas sam] {
			return
		}

		if { [string first "(5A7E" $line] > 0 } {
			# this is satnode
			if { $agg != $WN(AI,$prt) } {
				# wrong satnode, ignore
				return
			}
			# check for battery status
			if [regexp " inp (\[0-9\]+)" $line j bs] {
				# this is sensor 0
				set cs [snip_cnvrt $bs 1 0]
				if { $cs != "" } {
					set bs $cs
				}
			} else {
				set bs "?"
			} 

       			if [catch { expr 0x$fla } fla] {
        		        set fla 0
        		}

			if { $mas == $agg } {
				set mas "this satnode"
			}

			set WN(SV,$prt) [lreplace $WN(SV,$prt) 0 4\
				[uptime $upt] $mas [fltos $fla] $sam $bs]
			set upd 1
		} else {
			# custodian itself - just the node number
			if { $agg != $WN(NI,$prt) } {
				set upd 1
				set WN(NI,$prt) $agg
			}
		}
	}

	if $upd {
		out_status $prt
		set WN(SI,$prt) 60
		set_timeout $prt S 60 "update_status $prt 1"
	}
}
	
proc col_status_monitor { prt line } {

	global WN PT

	if ![regexp "^2005" $line] {
		return
	}

	# this looks like a status line
	if ![regexp -nocase $PT(CST) $line j col fre fla upt sam] {
		llog $prt "bad response from node: $line"
		return
	}

        if [catch { expr 0x$fla } fla] {
                set fla 0
        }

	set WN(NI,$prt) $col

	set WN(SV,$prt) [list [uptime $upt] [fltos $fla] $sam "$fre s"]
	out_status $prt

	set WN(SI,$prt) 60

	# slow rate is once per minute for an aggregator
	set_timeout $prt S 60 "update_status $prt 1"
}

proc setup_window { prt } {

	global WN

	set mw ".ns$prt"
	set WN(w,$prt,S) $mw
	toplevel $mw

	bind $mw <Destroy> "stop_port $prt"

	switch $WN(NT,$prt) {

		"aggregator" { setup_agg_window $prt }
		"custodian"  { setup_cus_window $prt }
		"collector"  { setup_col_window $prt }
		default {
			alert "Illegal window type, internal error 0001!"
			stop_port $prt
		}
	}
}

proc cnotype { prt } {
#
# Return the node's official title
#
	global WN

	if { $prt == "SD" } {
		return "SD card"
	}

	set tt $WN(NT,$prt)

	if { $tt == "aggregator" && $WN(SA,$prt) } {
		set tt "satnode"
	}

	return "[string toupper [string index $tt 0]][string range $tt 1 end]"
}

proc prtname { prt } {

	set prt [string trim $prt "="]
	regsub -all "=" $prt "," prt
	return $prt
}

proc setup_agg_window { prt } {

	global WN PM

	set w $WN(w,$prt,S)

	if ![info exists WN(SA,$prt)] {
		# clarify the satnode flag
		set WN(SA,$prt) 0
	}

	set sa $WN(SA,$prt)

	set ti [cnotype $prt]

	wm title $w "$ti on port [prtname $prt]"

	# the column of buttons
	labelframe $w.b -text "Actions" -padx 2 -pady 2
	pack $w.b -side left -fill y

	# and the buttons
	button $w.b.bst -text "Start" -state disabled -command "do_start $prt"
	pack $w.b.bst -side top -fill x -pady 2

	button $w.b.bso -text "Stop" -state disabled -command "do_stop $prt"
	pack $w.b.bso -side top -fill x -pady 2

	button $w.b.bsh -text "Show" -state disabled -command "do_show $prt"
	pack $w.b.bsh -side top -fill x -pady 2

	button $w.b.bex -text "Extract" -state disabled \
		-command "do_extract $prt"
	pack $w.b.bex -side top -fill x -pady 2

	button $w.b.blo -text "Log" -state disabled -command "do_echo $prt"
	pack $w.b.blo -side top -fill x -pady 2

	button $w.b.bvi -text "Reset" -state disabled -command "do_virgin $prt"
	pack $w.b.bvi -side top -fill x -pady 2

	button $w.b.ben -text "Close" -state disabled -command "stop_port $prt"
	pack $w.b.ben -side top -fill x -pady 2

	if !$sa {
		# disabled for a satnode
		set bl "bst bso "
	} else {
		set bl ""
	}

	# the list of buttons (for disabling/enabling)
	append bl "bsh bex blo bvi ben"
	set WN(BL,$prt) $bl

	# the right side, i.e., node status label

	labelframe $w.s -text "Node Status" -padx 2 -pady 2
	pack $w.s -side left -fill y

	# the actual pane
	frame $w.s.p
	pack $w.s.p -side top

	# a filler
	frame $w.s.f
	pack $w.s.f -fill y -expand y

	mk_status_pane $prt $w.s.p $ti $PM(ASH)
	out_status $prt

	# plug in the status monitor
	set WN(SM,$prt) agg_status_monitor

	enable_node_window $prt

	update_status $prt
}

proc setup_cus_window { prt } {

	global WN PM

	set w $WN(w,$prt,S)

	set ti [cnotype $prt]

	wm title $w "$ti on port [prtname $prt]"

	# the column of buttons
	labelframe $w.b -text "Actions" -padx 2 -pady 2
	pack $w.b -side left -fill y

	# and the buttons
	button $w.b.bst -text "Start" -state disabled -command "do_start $prt"
	pack $w.b.bst -side top -fill x -pady 2

	button $w.b.bso -text "Stop" -state disabled -command "do_stop $prt"
	pack $w.b.bso -side top -fill x -pady 2

	button $w.b.bsh -text "Show" -state disabled -command "do_show $prt"
	pack $w.b.bsh -side top -fill x -pady 2

	button $w.b.bex -text "Extract" -state disabled \
		-command "do_extract $prt"
	pack $w.b.bex -side top -fill x -pady 2

	button $w.b.blo -text "Log" -state disabled -command "do_echo $prt"
	pack $w.b.blo -side top -fill x -pady 2

	button $w.b.bvi -text "Reset" -state disabled -command "do_virgin $prt"
	pack $w.b.bvi -side top -fill x -pady 2

	button $w.b.ben -text "Close" -state disabled -command "stop_port $prt"
	pack $w.b.ben -side top -fill x -pady 2

	set bl "bst bso bsh bex blo bvi ben"

	set WN(BL,$prt) $bl

	# the right side, i.e., node status label

	labelframe $w.s -text "Node Status" -padx 2 -pady 2
	pack $w.s -side left -fill y

	# the actual pane
	set p $w.s.p
	frame $p
	pack $p -side top

	# a filler
	frame $w.s.f
	pack $w.s.f -fill y -expand y

	mk_status_pane $prt $p $ti $PM(USH)

	# insert satnode change button
	label $p.tf.s -text " -> satnode $WN(AI,$prt)"
	button $p.tf.b -text "Change" -command "change_ai $prt"

	pack $p.tf.s -side left
	pack $p.tf.b -side left

	out_status $prt

	# plug in the status monitor
	set WN(SM,$prt) cus_status_monitor

	enable_node_window $prt

	update_status $prt
}

proc change_ai { prt } {
#
# Change satnode Id
#
	global WN ST MV

	set w [smw $prt "Satnode ID"]

	disable_node_window $prt

	frame $w.o -pady 2 -padx 2
	pack $w.o -side top -fill x

	# the default
	set MV(SN) $WN(AI,$prt)

	label $w.o.sil -text "Enter new satnode ID:"
	grid $w.o.sil -column 0 -row 0 -sticky w -padx 4
	entry $w.o.sie -width 5 -textvariable MV(SN)
	grid $w.o.sie -column 1 -row 0 -sticky w -padx 4

	frame $w.b
	pack $w.b -side top -fill x

	button $w.b.go -text "Enter" -command "set ST(MOD) 1"
	pack $w.b.go -side right

	button $w.b.ca -text "Cancel" -command "set ST(MOD) 0"
	pack $w.b.ca -side left

	bind $w <Destroy> "set ST(MOD) 0"

	wm transient $w $WN(w,$prt,S)
	raise $w

	while 1 {

		tkwait variable ST(MOD)

		if { $ST(MOD) == 0 } {
			# cancelled
			dmw $prt
			enable_node_window $prt
			return
		}

		# verify the argument
		set sn [vnum $MV(SN) 1 65536]

		if { $sn == "" || $sn == $WN(NI,$prt) } {
			alert "Illegal or missing node Id; must be between 1\					and 65535 and different from custodian Id!"
		} else {
			break
		}
	}

	# delete the modal window
	dmw $prt
	enable_node_window $prt
	set WN(AI,$prt) $sn

	set w $WN(SP,$prt)
	$w.tf.s configure -text " -> satnode $sn"
}

proc setup_col_window { prt } {

	global WN PM

	set w $WN(w,$prt,S)

	set ti [cnotype $prt]

	wm title $w "$ti on port [prtname $prt]"

	# the column of buttons
	labelframe $w.b -text "Actions" -padx 2 -pady 2
	pack $w.b -side left -fill y

	# and the buttons
	button $w.b.bst -text "Start" -state disabled
	pack $w.b.bst -side top -fill x -pady 2

	button $w.b.bso -text "Stop" -state disabled -command "do_stop $prt"
	pack $w.b.bso -side top -fill x -pady 2

	button $w.b.bsh -text "Show" -state disabled
	pack $w.b.bsh -side top -fill x -pady 2

	button $w.b.bex -text "Extract" -state disabled \
		-command "do_extract $prt"
	pack $w.b.bex -side top -fill x -pady 2

	button $w.b.blo -text "Log" -state disabled -command "do_echo $prt"
	pack $w.b.blo -side top -fill x -pady 2

	button $w.b.bvi -text "Reset" -state disabled -command "do_virgin $prt"
	pack $w.b.bvi -side top -fill x -pady 2

	button $w.b.ben -text "Close" -state disabled -command "stop_port $prt"
	pack $w.b.ben -side top -fill x -pady 2

	# the list of buttons (for disabling/enabling)
	set bl "bso bex blo bvi ben"
	set WN(BL,$prt) $bl

	# the right side, i.e., node status label

	labelframe $w.s -text "Node Status" -padx 2 -pady 2
	pack $w.s -side left -fill y

	# the actual pane
	frame $w.s.p
	pack $w.s.p -side top

	# a filler
	frame $w.s.f
	pack $w.s.f -fill y -expand y

	mk_status_pane $prt $w.s.p $ti $PM(CSH)
	out_status $prt

	# plug in the status monitor
	set WN(SM,$prt) col_status_monitor

	enable_node_window $prt

	update_status $prt
}

proc enable_node_window { prt } {

	global WN

	if ![info exists WN(w,$prt,S)] {
		return
	}

	set w $WN(w,$prt,S)

	foreach b $WN(BL,$prt) {
		$w.b.$b configure -state normal
	}
}

proc disable_node_window { prt } {

	global WN

	if ![info exists WN(w,$prt,S)] {
		return
	}

	set w $WN(w,$prt,S)

	foreach b $WN(BL,$prt) {
		$w.b.$b configure -state disabled
	}
}

proc mk_status_pane { prt w tt hlist } {

	global WN

	# initialize the value list
	set WN(SV,$prt) ""
	set WN(ST,$prt) $tt

	# the window pane pointer
	set WN(SP,$prt) $w

	# the title (will come up later)
	frame $w.tf
	grid $w.tf -column 0 -row 0 -columnspan 2 -sticky nw -ipady 4
	label $w.tf.t -text "" -justify left
	pack $w.tf.t -side left

	set cnt 0
	foreach h $hlist {
		append WN(SV,$prt) "?? "
		label $w.h$cnt -text "    $h" -justify left
		grid $w.h$cnt -column 0 -row [expr $cnt + 1] -sticky w
		incr cnt
	}

	set WN(SV,$prt) [string trimright $WN(SV,$prt)]

	for { set i 0 } { $i < $cnt } { incr i } {
		# create value labels
		label $w.v$i -text "" -justify left
		grid $w.v$i -column 1 -row [expr $i + 1] -sticky w
	}
}

proc out_status { prt } {

	global WN

	if [catch {
		set w $WN(SP,$prt)
		set v $WN(SV,$prt)
		set t $WN(ST,$prt)
	}] {
		# means that the window has disappeared on us
		return
	}

	if { $WN(NI,$prt) != "" } {
		set ni $WN(NI,$prt)
	} else {
		set ni "??"
	}

	$w.tf.t configure -text "$t $ni"

	set i 0
	foreach x $v {
		$w.v$i configure -text $x
		incr i
	}
}

proc do_echo { prt } {
#
# Start/stop a log viewer for this port
#
	global WN

	if [info exists WN(w,$prt,L)] {
		# logging already
		destroy $WN(w,$prt,L)
		catch { unset WN(w,$prt,L) }
		return
	}

	set w ".lo$prt"
	set WN(w,$prt,L) $w

	toplevel $w

	wm title $w "[cnotype $prt] on port [prtname $prt]: log"

	text $w.t

	$w.t configure \
		-yscrollcommand "$w.scroly set" \
		-setgrid true \
        	-width 80 -height 24 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$w.t delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"
	#scrollbar $w.scrolx -orient horizontal -command "$w.t xview"

	pack $w.scroly -side right -fill y
	#pack $w.scrolx -side bottom -fill x

	pack $w.t -expand yes -fill both

	$w.t configure -state disabled

	# direct input
	frame $w.stat -borderwidth 2
	pack $w.stat -expand no -fill x

	set u $w.stat.l
	label $u -text "Direct input:"
	pack $u -side left

	set u $w.stat.u
	text $u -height 1 -font {-family courier -size 10}
	pack $u -side left -expand yes -fill x
	bind $u <Return> "direct_input $prt $u"

	if { $WN(NT,$prt) == "custodian" } {
		# we have three options regarding where the input goes
		tk_optionMenu $w.stat.m WN(IS,$prt) \
			"To Custodian" "To Satnode" "To Site"
			set WN(IS,$prt) "To Satnode"
		pack $w.stat.m -side left -expand yes
	}
	bind $w <Destroy> "cancel_echo $prt"
}

proc scmd { prt cmd } {
#
# Converts the command for satnode issue
#
	global WN

	regsub -all "\[ \t\]+" $cmd "_" cmd

	return "r $WN(AI,$prt) $cmd"
}

proc direct_input { prt w } {
#
# Handle direct input from the user
#
	global WN

	set tx ""
	regexp "\[^\r\n\]+" [$w get 0.0 end] tx
	$w delete 0.0 end

	set tx [string trim $tx]

	set mark "="
	set cmd $tx
	if { $WN(NT,$prt) == "custodian" } {
		# we have options
		set opt $WN(IS,$prt)
		if { $opt == "To Satnode" } {
			set mark "@"
			set cmd [scmd $prt $tx]
		} elseif { $opt == "To Site" } {
			set mark "$"
			set cmd "s $tx"
		}
	}

	llog $prt "${mark}> $tx"
	catch { puts $WN(FD,$prt) $cmd }
}

proc cancel_echo { prt } {

	global WN

	if [info exists WN(w,$prt,L)] {
		catch { destroy $WN(w,$prt,L) }
		unset WN(w,$prt,L)
	}
}

proc add_text { w txt } {

	$w configure -state normal
	$w insert end "$txt"
	$w configure -state disabled
	$w yview -pickplace end
}

proc end_line { w } {

	global PM
	
	$w configure -state normal
	$w insert end "\n"

	set ix [$w index end]
	set ix [string range $ix 0 [expr [string first "." $ix] - 1]]

	if { $ix > $PM(MLC) } {
		# scroll out the topmost line if above limit
		$w delete 1.0 2.0
	}

	$w configure -state disabled
	# make sure the last line is displayed
	$w yview -pickplace end

}

proc fix_intv { intv } {
#
	if { $intv == "" } {
		alert "Sampling interval not specified!"
		# means to re-try
		return -1
	}

	set iv [vnum $intv 1 32768]

	if { $iv == "" } {
		alert "Sampling interval is invalid;\
			must be a positive integer > 0!"
		return -1
	}

if 0 {
########
	set day [expr 24 * 3600]
	set daz [expr $day / 2]

	if { $iv > $daz } {
		set iv $daz
	} else {
		while { [expr $day % $iv] } {
			incr iv
		}
	}

	if { $iv != $intv } {
		if [confirm "The sampling interval has been adjusted to $iv to\
		    conform to the requirements; is this OK?"] {
			return $iv
		}
		# retry
		return -1
	}
########
}

	return $iv
}

proc fix_plid { plid } {
#
	set plid [vnum $plid 0 32768]

	if { $plid == "" } {
		alert "Illegal or missing plot id;\
			 must be between 0 and 32767!"
		return -1
	}

	return $plid
}

proc do_start { prt } {

	global WN ST MV

	disable_node_window $prt

	set w [smw $prt "Starting [cnotype $prt]"]

	frame $w.o -pady 2 -padx 2
	pack $w.o -side top -fill x

	label $w.o.sil -text "Sampling interval:"
	grid $w.o.sil -column 0 -row 0 -sticky w -padx 4
	entry $w.o.sie -width 5 -textvariable MV(SN)
	grid $w.o.sie -column 1 -row 0 -sticky w -padx 4
	# the default
	set MV(SN) 30

	label $w.o.pnl -text "Plot number:"
	grid $w.o.pnl -column 0 -row 1 -sticky w -padx 4
	entry $w.o.pne -width 5 -textvariable MV(PN)
	grid $w.o.pne -column 1 -row 1 -sticky w -padx 4
	set MV(PN) 0

	label $w.o.erl -text "Pre-erase storage:"
	grid $w.o.erl -column 0 -row 2 -sticky w -padx 4
	checkbutton $w.o.ere -variable MV(ER)
	grid $w.o.ere -column 1 -row 2 -sticky w -padx 4
	set MV(ER) 0

	frame $w.b
	pack $w.b -side top -fill x

	button $w.b.go -text "Start" -command "set ST(MOD) 1"
	pack $w.b.go -side right

	button $w.b.ca -text "Cancel" -command "set ST(MOD) 0"
	pack $w.b.ca -side left

	bind $w <Destroy> "set ST(MOD) 0"

	wm transient $w $WN(w,$prt,S)

	raise $w

	while 1 {

		tkwait variable ST(MOD)

		if { $ST(MOD) == 0 } {
			# cancelled
			dmw $prt
			enable_node_window $prt
			return
		}

		# verify the arguments
		set intv [fix_intv $MV(SN)]

		if { $intv < 0 } {
			# error, retry
			continue
		}

		set plid [fix_plid $MV(PN)]

		if { $plid >= 0 } {
			set erase $MV(ER)
			break
		}
	}

	# delete the modal window
	dmw $prt

	# node window still disabled

	set msgw [mk_mess_window "cancel_node_command $prt" 35 1 ""]

	# mark "command in progress"
	set WN(CP,$prt) 0

	if { $WN(NT,$prt) == "aggregator" } {
		set res [start_aggregator $prt $msgw $intv $plid $erase]
	} else {
		set res [start_custodian $prt $msgw $intv $plid $erase]
	}

	# command done
	set WN(CP,$prt) 1
	catch { destroy $msgw }

	if { $res == 0 } {
		alert "Sampling started!"
		update_status $prt
	}
	enable_node_window $prt
}

proc satrevert { } {

	alert "Looks like that was a satnode that has been just reverted to\
		its standard function. We shall disconnect from this node now!"
}

proc start_aggregator { prt msgw intv plid erase } {

	global WN

	if [coutm $prt $msgw "Resetting the node ..."] {
		# one way of cancelling is to kill the msg window
		return 1
	}

	if [ciss $prt "q" "^(1001|1005|AT.CMGS)" 4 60] {
		# timeout (diagnosed) or cancellation
		return 1
	}

	if { [string first "1005" $WN(LI,$prt)] >= 0 ||
	     [string first "Maint" $WN(LI,$prt)] >= 0 } {

		if [coutm $prt $msgw "Clearing maintenance mode ..."] {
			return 1
		}

		if [ciss $prt "F" "^1001" 4 60] {
			return 1
		}

	} elseif { [string first "1001" $WN(LI,$prt)] < 0 } {

		satrevert
		stop_port $prt
		return 1
	}

	if $erase {
		if [coutm $prt $msgw "Erasing storage ..."] {
			return 1
		}
		if [ciss $prt "E" "^1001" 4 60] {
			return
		}
	}

	# sync
	if [coutm $prt $msgw "Setting parameters ..."] {
		return 1
	}

	if [ciss $prt "Y $intv" "^1013" 6 4] {
		return 1
	}

	# make it a master
	sendm $prt "m"
	# now this doesn't show up ...
	sendm $prt "m"

	if [ciss $prt "P $plid" "^1012" 6 4] {
		return 1
	}

	# set the time stamp to current time
	set ts [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S"]

	if [ciss $prt "T $ts" "^1009" 6 4] {
		return 1
	}

if 0 {
	# stored entries
	if [ciss $prt "a -1 -1 -1 3" "^1005" 6 5] {
		return 1
	}
}

	if [ciss $prt "SA" "^1010" 4 3] {
		return 1
	}
	return 0
}

proc start_custodian { prt msgw intv plid erase } {

	global WN

	set msg "Resetting the satnode "

	if $erase {
		append msg "and erasing its storage "
	}

	append msg "..."

	if [coutm $prt $msgw $msg] {
		return 1
	}

	if $erase {
		if [ciss $prt [scmd $prt "E"] "1001" 4 60] {
			return 1
		}
	} else {
		if [ciss $prt [scmd $prt "q"] "1001" 4 60] {
			return 1
		}
	}

	if [coutm $prt $msgw "Setting parameters ..."] {
		return 1
	}

	if [ciss $prt [scmd $prt "Y $intv"] "^1013" 6 4] {
		return 1
	}

	# make it a master
	set msg [scmd $prt "m"]
	sendm $prt $msg
	sendm $prt $msg
	sendm $prt $msg
	sendm $prt $msg

	if [ciss $prt [scmd $prt "P $plid"] "^1012" 6 4] {
		return 1
	}

	# note, we do not set time for a satnode

if 0 {
	if [ciss $prt [scmd $prt "a -1 -1 -1 3"] "^1005" 6 5] {
		return 1
	}
}

	if [ciss $prt [scmd $prt "SA"] "^1010" 4 10] {
		return 1
	}

	return 0
}

proc do_stop { prt } {

	global WN

	disable_node_window $prt
	set nt $WN(NT,$prt)

	set mai [tk_dialog .alert "Stop options" "Select the action" "" 0 \
		"Reset" "Factory reset" "Maintenance" "Cancel"]

	if { $mai == 3 } {
		# cancelled
		enable_node_window $prt
		return
	}

	set msgw [mk_mess_window "cancel_node_command $prt" 35 1 ""]
	set WN(CP,$prt) 0

	if { $nt == "aggregator" } {
		set res [stop_aggregator $prt $msgw $mai]
	} elseif { $nt == "collector" } {
		set res [stop_collector $prt $msgw $mai]
	} else {
		set res [stop_custodian $prt $msgw $mai]
	}

	set WN(CP,$prt) 1
	catch { destroy $msgw }

	if { $res == 0 } {
		alert "Done!"
	}
	update_status $prt

	enable_node_window $prt
}

proc stop_aggregator { prt msgw mai } {

	global WN

	if [coutm $prt $msgw "Resetting node ..."] {
		return 1
	}

	if { $mai == 2 } {
		if [ciss $prt "M" "^1005" 4 30] {
			return 1
		}
		return 0
	}

	if { $mai == 1 } {
		# factory reset
		if [ciss $prt "Q" "^(1001|AT.CMGS)" 4 60] {
			return 1
		}
		return 0
	}

	# in case already in maintenance mode
	if [ciss $prt "q" "^(1001|1005)" 4 30] {
		return 1
	}
	if { [string first "1005" $WN(LI,$prt)] >= 0 } {
		alert "The node was in maintenance mode;\
			nothing has been stopped!"
		# to prevent another alert
		return 1
	}

	return  0
}

proc stop_custodian { prt msgw mai } {

	global WN

	return [stop_aggregator $prt $msgw $mai]
}

proc stop_collector { prt msgw mai } {

	global WN

	if [coutm $prt $msgw "Resetting node ..."] {
		return 1
	}

	if { $mai == 2 } {
		if [ciss $prt "M" "^2005" 4 30] {
			return 1
		}
		return 0
	}

	if { $mai == 1 } {
		# factory reset
		if [ciss $prt "Q" "^2001" 4 60] {
			return 1
		}
		return 0
	}

	# in case already in maintenance mode
	if [ciss $prt "q" "^(2001|2005)" 4 30] {
		return 1
	}
	if { [string first "2005" $WN(LI,$prt)] >= 0 } {
		alert "The node was in maintenance mode;\
			nothing has been stopped!"
		# to prevent another alert
		return 1
	}

	return  0
}

proc do_virgin { prt } {

	global WN ST MV

	disable_node_window $prt

	set nt $WN(NT,$prt)

	if { $nt == "custodian" } {

		set cus [tk_dialog .alert "Question!" "Would you like to\
			reset the custodian itself or the supervised satnode?"\
				"" 0 "Satnode" "Custodian" "Cancel"]

		if { $cus == 2 } {
			# cancelled
			enable_node_window $prt
			return
		}

	} else {
		set cus 0
	}

	if { $nt != "custodian" || !$cus } {

		# ask (modally) for the interval

		set w [smw $prt "Interval"]

		frame $w.o -pady 2 -padx 2
		pack $w.o -side top -fill x

		label $w.o.sil -text "Sampling interval:"
		grid $w.o.sil -column 0 -row 0 -sticky w -padx 4
		entry $w.o.sie -width 5 -textvariable MV(SN)
		grid $w.o.sie -column 1 -row 0 -sticky w -padx 4
		# the default
		set MV(SN) 30

		if { $nt == "collector" } {
			# store all samples?
			label $w.o.sal -text "Store samples:"
			grid $w.o.sal -column 0 -row 1 -sticky w -padx 4
			tk_optionMenu $w.o.sas MV(SS) "All" "Unconfirmed" \
				"Confirmed" "None"
			set MB(SS) "All"
			grid $w.o.sas -column 1 -row 1 -sticky w -padx 4
		}

		frame $w.b
		pack $w.b -side top -fill x

		button $w.b.go -text "Proceed" -command "set ST(MOD) 1"
		pack $w.b.go -side right

		button $w.b.ca -text "Cancel" -command "set ST(MOD) 0"
		pack $w.b.ca -side left

		bind $w <Destroy> "set ST(MOD) 0"

		wm transient $w .

		raise $w

		while 1 {

			tkwait variable ST(MOD)

			if { $ST(MOD) == 0 } {
				# cancelled
				dmw $prt
				enable_node_window $prt
				return
			}

			# verify the argument
			set intv [fix_intv $MV(SN)]

			if { $intv < 0 } {
				continue
			}
		
			if { $nt == "collector" } {
				set sas $MV(SS)
			}
			break
		}

		# delete the modal window
		dmw $prt
	} else {
		# playing it safe
		set intv 30
	}

	set msgw [mk_mess_window "cancel_node_command $prt" 35 1 ""]
	set WN(CP,$prt) 0

	if { $nt == "aggregator" } {
		set res [virgin_aggregator $prt $msgw $intv]
	} elseif { $nt == "collector" } {
		set res [virgin_collector $prt $msgw $intv $sas]
	} else {
		set res [virgin_custodian $prt $msgw $intv $cus]
	}

	set WN(CP,$prt) 1
	catch { destroy $msgw }

	if { $res == 0 } {
		alert "Done!"
	}
	update_status $prt
	enable_node_window $prt
}

proc virgin_aggregator { prt msgw intv } {

	global WN

	if [coutm $prt $msgw "Returning the aggregator to factory state ..."] {
		return 1
	}

	if [ciss $prt "Q" "^(1001|AT.CMGS)" 4 60] {
		return 1
	}

	if { [string first "1001" $WN(LI,$prt)] < 0 } {
		satrevert
		stop_port $prt
		return 1
	}

	if { $intv == 0 } {
		set intv 30
	}

	if [coutm $prt $msgw "Pre-setting the sampling interval ..."] {
		return 1
	}

	if [ciss $prt "Y $intv" "^1013" 6 4] {
		return 1
	}

	# make it a master
	sendm $prt "m"
	sendm $prt "m"

if 0 {
	if [ciss $prt "a -1 -1 -1 3" "^1005" 4 8] {
		# collect all
		return 1
	}
}

	if [coutm $prt $msgw "Saving sampling parameters ..."] {
		return 1	
	}

	if [ciss $prt "SA" "^1010" 4 6] {
		return 1
	}

	if [coutm $prt $msgw "Bringing the node to stopped state ..."] {
		return 1
	}

if 0 {
	if [ciss $prt "a -1 -1 -1 0" "^1005" 4 5] {
		# do not collect any samples unless told so
		return 1
	}
}

	return 0
}

proc virgin_custodian { prt msgw intv cus } {

	global WN

	if $cus {

		if [coutm $prt $msgw "Returning the custodian to factory\
		    state ..."] {
			return 1
		}

		if [ciss $prt "Q" "^1001" 4 60] {
			return 1
		}

		return 0
	}

	if [coutm $prt $msgw "Returning the satnode to factory state ..."] {
		return 1
	}

	if [ciss $prt [scmd $prt "Q"] "^1001" 4 60] {
		return 1
	}

	if { $intv == 0 } {
		set intv 30
	}

	if [coutm $prt $msgw "Pre-setting the sampling interval ..."] {
		return 1
	}

	if [ciss $prt [scmd $prt "Y $intv"] "^1013" 6 4] {
		return 1
	}

	# make it a master
	set cmd [scmd $prt "m"]
	sendm $prt $cmd
	sendm $prt $cmd
	sendm $prt $cmd
	sendm $prt $cmd

if 0 {
	if [ciss $prt [scmd $prt "a -1 -1 -1 3"] "^1005" 4 8] {
		# collect all
		return 1
	}
}

	if [coutm $prt $msgw "Saving sampling parameters ..."] {
		return 1	
	}

	if [ciss $prt [scmd $prt "SA"] "^1010" 4 6] {
		return 1
	}

	if [coutm $prt $msgw "Bringing the node to stopped state ..."] {
		return 1
	}

if 0 {
	if [ciss $prt [scmd $prt "a -1 -1 -1 0"] "^1005" 4 5] {
		# do not collect any samples unless told so
		return 1
	}
}

	return 0
}

proc virgin_collector { prt msgw intv sas } {

	global WN

	if [coutm $prt $msgw "Returning the collector to factory state ..."] {
		return 1
	}

	if [ciss $prt "Q" "^CC1100" 3 60] {
		return 1
	}

	if { $intv == 0 } {
		# the default
		set intv 30
	}

	if [coutm $prt $msgw "Pre-setting parameters: $intv / $sas ..."] {
		return 1
	}

	switch $sas {
		"All"		{ set sas 3 }
		"Unconfirmed"	{ set sas 1 }
		"Confirmed"	{ set sas 2 }
		default 	{ set sas 0 }
	}

	if [ciss $prt "s $intv -1 0 -1 $sas" "^2005" 6 4] {
		return 1
	}

	if [ciss $prt "SA" "^2010" 6 4] {
		return 1
	}

	return 0
}

proc show_window { prt { redo 0 } } {
#
# Opens or re-opens a show window
#
	global WN

	if [info exists WN(w,$prt,H)] {
		if !$redo {
			return
		}
		# refurbish the window
		set w $WN(w,$prt,H)
		bind $w <Destroy> ""
		set ge [wm geometry $w]
		destroy $w
	} else {
		set ge ""
	}

	set w .show$prt
	toplevel $w

	set WN(w,$prt,H) $w

	wm title $w "Sensor values for [cnotype $prt] on port [prtname $prt]"

	if { $ge != "" } {
		# previous location
		regexp "\\+.*\\+.*" $ge ge
		wm geometry $w $ge
	}

	# the display list
	if [info exists WN(HL,$prt)] {
		set hl $WN(HL,$prt)
	} else {
		set hl ""
	}

	set l $w.l
	frame $l
	pack $l -side top -expand yes -fill x

	if { $hl == "" } {

		label $l.t -text "Waiting for initial sensor data to arrive ..."
		pack $l.t -expand yes -fill x

	} else {

		# fill the findow according to the list
		set mns 1
		foreach t $hl {
			# calculate the maximum number of sensors per collector
			set ns [expr [llength $t] - 2]
			# the first two entries are collector number and
			# time stamp
			if { $ns > $mns } {
				set mns $ns
			}
		}

		# start with the headers
		label $l.ch -text "Collector" -anchor e
		grid $l.ch -column 0 -row 0 -sticky e -padx 4 -pady 4

		label $l.th -text "Time stamp" -anchor e
		grid $l.th -column 1 -row 0 -sticky e -padx 4 -pady 4

		set col 2
		for { set i 0 } { $i < $mns } { incr i } {

			label $l.sh$i -text "S$i" -anchor e
			grid $l.sh$i -column $col -row 0 -sticky e -padx 4 \
				-pady 4

			incr col
		}

		set row 1
		foreach t $hl {

			set cn [lindex $t 0]
			label $l.c$row -text $cn -anchor e -width 5
			grid $l.c$row -column 0 -row $row -sticky e -padx 4 \
				-pady 0

			label $l.t$row -text [lindex $t 1] -anchor e
			grid $l.t$row -column 1 -row $row -sticky e -padx 4 \
				-pady 0

			# now for the values
			set col 2
			set sen 0
			set t [lrange $t 2 end]
			while 1 {
				set vr [lindex $t $sen]
				if { $vr == "" } {
					break
				}
				set s $l.vr${row}_$sen 
				label $s -text $vr -anchor e
				grid $s -column $col -row $row -sticky e \
					-padx 4 -pady 0
				incr col
				incr sen
			}
			incr row
		}
	}

	frame $w.b
	pack $w.b -side top -fill x

	if [info exists WN(SF,$prt)] {
		button $w.b.f -text "Stop File" -command "show_stopf $prt"
	} else {
		button $w.b.f -text "Start File" -command "show_startf $prt"
	}
	pack $w.b.f -side left

	button $w.b.c -text "Close" -command "close_show $prt"
	pack $w.b.c -side right

	bind $w <Destroy> "close_show $prt"
}

proc show_upd { prt col ts rvals } {
#
# Update show values: port, collector, senset_index, timestamp, raw_sensors
#
	global WN

	if ![info exists WN(w,$prt,H)] {
		return
	}

	set w $WN(w,$prt,H)

	if [info exists WN(HL,$prt)] {
		set hl $WN(HL,$prt)
	} else {
		set hl ""
	}

	# prepare the new set of values
	set vs [concat [list $col $ts] $rvals]

	# redo flag for the window
	set redo 0

	# look for the collector on the window's list
	set ix 0
	set nf 1
	foreach t $hl {
		if { [lindex $t 0] == $col } {
			set nf 0
			break
		}
		incr ix
	}

	if $nf {
		# not found
		set redo 1
		lappend hl $vs
		set WN(HL,$prt) [lsort -integer -index 0 $hl]
	} else {
		if { [llength $t] != [llength $vs] } {
			# this is impossible (the number of sensors has changed)
			set redo 1
		}
		set hl [lreplace $hl $ix $ix $vs]
	}

	if $redo {
		# have to redo the entire window
		show_window $prt 1
		return
	}

	# update one existing entry: this is the row number
	incr ix

	set l $w.l
	$l.t$ix configure -text $ts
	set sen 0

	foreach v $rvals {
		if [catch { $l.vr${ix}_$sen configure -text $v } ] {
			# in case we somehow have gotten more values
			break
		}
		incr sen
	}
}

proc do_show { prt } {

	global WN

	if [info exists WN(w,$prt,H)] {
		raise $WN(w,$prt,H)
		return
	}

	show_window $prt

	set WN(SH,$prt) 1
}

proc close_show { prt } {

	global WN

	if [info exists WN(w,$prt,H)] {
		catch { destroy $WN(w,$prt,H) }
	}
	catch { unset WN(w,$prt,H) }
	catch { unset WN(HL,$prt) }

	show_stopf $prt

	set WN(SH,$prt) 0
}

proc show_startf { prt } {

	global WN

	if [info exists WN(SF,$prt)] {
		# impossible
		show_stopf $prt
	}

	while 1 {

		if ![info exists WN(w,$prt,H)] {
			return
		}

		set w $WN(w,$prt,H)

		set fn [tk_getSaveFile \
			-defaultextension ".csv" \
			-initialdir [defhome] \
			-initialfile "values.csv" \
			-parent $w \
			-title "File to store the samples"]

		if { $fn == "" } {
			# cancelled
			return
		}

		if ![catch { open $fn "w" } fd] {
			break
		}
	}

	set WN(SF,$prt) $fd
	$w.b.f configure -text "Stop File" -command "show_stopf $prt"
}

proc show_stopf { prt } {

	global WN

	if [info exists WN(SF,$prt)] {
		catch { close $WN(SF,$prt) }
		unset WN(SF,$prt)
	}

	if [info exists WN(w,$prt,H)] {
		set w $WN(w,$prt,H)
		$w.b.f configure -text "Start File" -command "show_startf $prt"
	}
}

proc t_skip { ln pat } {

	upvar $ln line

	if ![regexp -indices -nocase $pat $line match] {
		set line ""
		return ""
	}

	set res [string range $line [lindex $match 0] [lindex $match 1]]

	set line [string trimleft [string range $line [expr [lindex $match 1] +\
		1] end]]

	return $res
}

proc twod { nn } {

	regsub "^0+" $nn "" nn

	if { $nn == "" || [catch { format "%02d" [expr $nn % 100] } nn] } {
		set nn "00"
	} 

	return $nn
}

proc t_parse { ln } {

	upvar $ln line

	set res ""

	if [regexp "^\[ \t\]*(\[0-9\]+)-(\[0-9\]+)-(\[0-9\]+)" $line jk y u d] {
		set line [string range $line [string length $jk] end]
		append res "20[twod $y]-[twod $u]-[twod $d]"
	}
	if [regexp "^\[ \t\]*(\[0-9\]+):(\[0-9\]+):(\[0-9\]+)" $line jk y u d] {
		set line [string range $line [string length $jk] end]
		if { $res != "" } {
			append res " "
		}
		append res "[twod $y]:[twod $u]:[twod $d]"
	}

	return $res
}
	
proc n_parse { ln { nn 0 } { fl 0 } } {
#
# Parse a number
#
	upvar $ln line

	set line [string trimleft $line]

	if ![regexp "^\[^ ,\t\n\r\]+" $line code] {
		return ""
	}

	if [catch { expr $code } num] {
		return ""
	}

	if !$fl {
		# integer expected
		if { [catch { expr int($num) } iv] || $iv != $num } {
			return ""
		}
		set num $iv
	}

	if { $nn && $num < 0 } {
		return ""
	}

	set line [string range $line [string length $code] end]

	regsub "^\[ ,\t\n\r\]+" $line "" line

	return $num
}

proc show_sensors { prt line } {
#
# Extract sensor values and time stamps from aggregator's line
#
	global WN

	t_skip line " Col "

	set cn [n_parse line 1]
	if { $cn == "" } {
		return
	}

	t_skip line ":"

	# collector's time stamp
	set ts [t_parse line]

	if { $ts == "" } {
		# looks like garbage
		return
	}

	if { [string first "gone" $line] >= 0 } {
		return
	}

	set rv ""
	while 1 {
		set v [n_parse line]
		if { $v == "" } {
			break
		}
		lappend rv $v
	}

	if { [llength $rv] < 2 } {
		# no sensors, at least two entries required
		return
	}

	# port, collector, senset_index, timestamp, raw_sensors
	set si [lindex $rv end]
	set rv [lreplace $rv end end]

	# converted values only
	set tv ""
	set tu ""
	set ix 0
	foreach v $rv {
		set vc [snip_cnvrt $v $ix $si un]
		if { $vc != "" } {
			lappend tv $vc
			if { $vc != "?" } {
				append vc $un
			}
			lappend tu $vc
		}
		incr ix
	}

	if { $tv == "" } {
		# no converted sensors
		return
	}

	show_upd $prt $cn $ts $tu

	if ![info exists WN(SF,$prt)] {
		return
	}

	# write to file
	set line "$cn, $ts"
	foreach v $tv {
		append line ", $v"
	}

	catch { puts $WN(SF,$prt) $line }
}

proc do_extract { prt } {

	global WN MV ST

	if $WN(SE,$prt) {
		# extracting already
		return
	}

	if { $WN(NT,$prt) == "custodian" } {
		if ![confirm "This is a custodian; do you really want to\
		    extract from it?"] {
			return
		}
	}

	set em [extract_dialog $prt]

	if { $em == "" } {
		# cancelled
		return
	}

	foreach { em fr up } $em { }

	# command to start
	set cmd "D $fr $up"

	# calculate the mode parameter
	if { $WN(NT,$prt) == "collector" } {
		# offset to indicate collector
		incr em 16
	} else {
		# aggregator
		if { $em > 2 } {
			append cmd " 65535"
		}
	}

	# this is the way to tell which case is which
	if { $WN(EF,$prt) != "" } {
		# writing to file, need a progress window of sorts
		set WN(w,$prt,E) [mk_mess_window "stop_extraction $prt 1" 25 1\
			"Waiting for data to arrive ..."]
	} else {
		# have to open a terminal-like window to accommodate the stuff
		if { [extract_tty $prt $em] == "" } {
			# failed
			return
		}
	}

	# extraction mode
	set WN(SE,$prt) $em

	# extracted so far
	set WN(EC,$prt) 0
		
	# header
	ewhdr $prt $em

	while 1 {
		# startup
		sendm $prt $cmd
		set_timeout $prt E 30 "extract_timeout $prt"
		# what is going to happen if the variable is unset?
		tkwait variable WN(EC,$prt)
		cancel_timeout $prt E
		if { $WN(SE,$prt) == 0 } {
			# aborted
			return
		}
		if $WN(EC,$prt) {
			# something has arrived
			break
		}
	}
}

proc extract_timeout { prt } {

	global WN

	set ec $WN(EC,$prt)
	set WN(EC,$prt) $ec
}

proc stop_extraction { prt abt } {
#
	global WN

	# FIXME unfortunately, we have no way to tell the node

	if { $WN(SE,$prt) == 0 } {
		return
	}

	cancel_timeout $prt E

	set WN(SE,$prt) 0

	if [info exists WN(EF,$prt)] {

		if $abt {
			set msg \
				"Extraction aborted: note that the node\
				may have been left in a hung state, and you\
				will have to reset it manually by powering it\
				down and up again!"
		}

		# should we close the file
		set fd $WN(EF,$prt)
		if { $fd != "" } {
			catch { close $fd }
			catch { destroy $WN(w,$prt,E) }
			catch { unset WN(w,$prt,E) }

			if !$abt {
				set msg "Extraction complete!"
			}

			if $WN(EC,$prt) {
				alert $msg
			} else {

				append msg "\n\nNothing has been extracted!\
				    Should I remove the empty output file?"

				if [confirm $msg] {
					catch {
						file delete -force $WN(EN,$prt)
					}
				}
			}
	
		} else {
			# there is a window
			if [info exists WN(w,$prt,E)] {
				set w $WN(w,$prt,E)
				if $abt {
					set t "Aborted"
				} else {
					set t "Done"
				}
				$w.stat.st configure -text "$t      "
			}

			if $abt {
				alert $msg
			}
		}
			
		unset WN(EF,$prt)
	}
	enable_node_window $prt
}

proc close_ewindow { prt } {
#
# Close the extraction window
#
	global WN

	if { $prt != "SD" } {
		stop_extraction $prt 1
	}

	if [info exists WN(w,$prt,E)] {
		catch { destroy $WN(w,$prt,E) }
		catch { unset WN(w,$prt,E) }
	}
}

proc ewhdr { prt em { len 0 } } {
#
# Output the extraction header
#
	global WN

	if { $em >= 16 } {
		# collector
		set coll 1
		incr em -16
	} else {
		set coll 0
	}

	if { $WN(EF,$prt) == "" } {
		# this goes to the screen
		if { $em > 2 } {
			# events
			set ln "      Slot Event       Date     Time  Par1"
			append ln "  Par2  Par3"
			if $len {
				return [string length $ln]
			}
		} elseif { $em == 1 } {
			# by collector
			if $coll {
				# collector
				set ln "    Status       Slot       Date"
				append ln "     Time     Value ..."
			} else {
				# aggregator
				set ln " Coll  Coll Slot   Agg Slot   Col Date"
				append ln " Col Time   Agg Date Agg Time "
				append ln "    Value ..."
			}
			if $len {
				return [expr [string length $ln] + 3 * 16 - 4]
			}
		} else {
			# by sensor
			if $coll {
				set ln "Sen              TStamp     Value"
			} else {
				set ln " Coll Sen              TStamp     Value"
			}
			if $len {
				return [string length $ln]
			}
		}
		set w $WN(w,$prt,E)
		add_text $w.t $ln
		end_line $w.t
	} else {
		# this goes to a file
		if { $em > 2 } {
			# events
			set ln "Slot, Event, Time, Par1, Par2, Par3"
		} elseif { $em == 1 } {
			# by collector
			if $coll {
				# collector
				set ln "Status, Slot, Time, Value, ..."
			} else {
				# aggregator
				set ln "Collector, CSlot, ASlot, CTime, ATime, "
				append ln "Value, ..."
			}
		} else {
			# by sensor
			if $coll {
				set ln "Sensor, TStamp, Value"
			} else {
				set ln \
				"Collector, Sensor, TStamp, Value"
			}
			if $len {
				return [string length $ln]
			}
		}
		if $len {
			# irrelevant
			return 0
		}

		catch { puts $WN(EF,$prt) $ln }
	}
	return 0
}
			
proc extract_aggregator_samples { prt mode line } {

	global WN PT

	if { $mode < 3 } {
		if [regexp $PT(AGS) $line jk col csl asl cts ats vls] {
			dump_values $mode $prt $col $csl $asl $cts $ats $vls
			return 1
		}
	} else {
		if [regexp $PT(AEV) $line jk col asl ats vls] {
			dump_event $prt $col $asl $ats $vls
			return 1
		}
	}
	if [regexp $PT(AEL) $line] {
		stop_extraction $prt 0
	}
	return 0
}

proc extract_collector_samples { prt mode line } {

	global WN PT

	set nfn 1

	if { $mode < 3 } {
		if [regexp $PT(COS) $line jk typ csl cts vls] {
			dump_values $mode $prt $typ $csl "" $cts "" $vls
			return 1
		}
	} else {
		if [regexp $PT(CEV) $line jk col asl ats vls] {
			dump_event $prt $col $asl $ats $vls
			return 1
		}
	}
	if [regexp $PT(CEL) $line] {
		stop_extraction $prt 0
	}
	return 0
}

proc eprogress { prt val } {
#
# Show extraction progress
#
	global WN

	outmess $WN(w,$prt,E) "Extracting slot: $val / $WN(EC,$prt)"
}

proc dump_values { mode prt col csl asl cts ats vls } {

	global WN

	if ![info exists WN(EF,$prt)] {
		# this is impossible and means nobody wants us any more
		set WN(SE,$prt) 0
		return
	}

	incr WN(EC,$prt)

	# file descriptor
	set fd $WN(EF,$prt)

	if { $ats == "" } {
		# collector
		set ds $csl
		set cn $WN(NI,$prt)
	} else {
		set ds $asl
		set cn $col
		set ats [t_parse ats]
	}

	set cts [t_parse cts]

	# parse the list of values
	set rvls ""
	while 1 {
		set n [n_parse vls 1]
		if { $n == "" } {
			break
		}
		lappend rvls $n
	}

	if { [llength $rvls] < 2 } {
		# no sensors
		return
	}

	set si [lindex $rvls end]
	set rvls [lreplace $rvls end end]

	# convert
	set cvls ""
	set cvun ""
	set inx 0
	foreach v $rvls {
		set vc [snip_cnvrt $v $inx $si un]
		if { $vc != "" } {
			lappend cvls $vc
			lappend cvun $un
		}
		incr inx
	}

	if { $cvls == "" } {
		# no converted values
		return
	}

	if { $mode > 1 } {
		# by sensor
		set inx 0
		if { $fd == "" } {
			# writing to the screen
			foreach c $cvls u $cvun {
				if { $ats != "" } {
					# this is an aggregator
					set ln "[trims $col 5] "
				} else {
					set ln ""
				}
				# sensor number + time stamp
				append ln "[trims $inx 3] $cts"
				if { $c != "?" } {
					append c $u
				}
				append ln "[trims $c 10]"
				incr inx
				set w $WN(w,$prt,E)
				add_text $w.t $ln
				end_line $w.t
			}
			return
		}

		# writing to file
		foreach c $cvls {
			if { $ats != "" } {
				# this is an aggregator
				set ln "$col, "
			} else {
				set ln ""
			}
			# sensor number + time stamp
			append ln "$inx, $cts, $c"
			catch { puts $fd $ln }
			incr inx
		}
		if { $prt != "SD" } {
			eprogress $prt $ds
		}
		return
	}

	if { $fd == "" } {
		# writing to the screen
		if { $ats == "" } {
			# collector
			set ln [trims $col 10]
			append ln [trims $csl 11]
			append ln " $cts"
		} else {
			# aggregator
			set ln [trims $col 5]
			append ln [trims $csl 11]
			append ln [trims $asl 11]
			append ln " $cts"
			append ln " $ats"
		}
		# the values
		foreach c $cvls u $cvun {
			if { $c != "?" } {
				append c $u
			}
			append ln [trims $c 10]
		}
			
		set w $WN(w,$prt,E)
		add_text $w.t $ln
		end_line $w.t
		return
	}

	if { $ats == "" } {
		# collector
		set ln "$col, $csl, $cts"
	} else {
		# aggregator
		set ln "$col, $csl, $asl, $cts, $ats"
	}
	# the values
	foreach c $cvls {
		append ln ", $c"
	}
		
	catch { puts $fd $ln }

	if { $prt != "SD" } {
		eprogress $prt $ds
	}
}

proc dump_event { prt evt slo tst par } {

	global WN

	if ![info exists WN(EF,$prt)] {
		# this is impossible and means nobody wants us any more
		set WN(SE,$prt) 0
		return
	}

	incr WN(EC,$prt)

	# file descriptor
	set fd $WN(EF,$prt)

	set tst [t_parse tst]

	if { $fd == "" } {

		# writing to the screen

		set ln [format %10d $slo]
		append ln [trims $evt 6]
		append ln " $tst"

		for { set i 0 } { $i < 3 } { incr i } {
			if [catch { expr [lindex $par $i] } v] {
				set v 0
			}
			append ln [format %6d $v]
		}
			
		set w $WN(w,$prt,E)
		add_text $w.t $ln
		end_line $w.t
		return
	}

	set ln "$slo, $evt, $tst"

	for { set i 0 } { $i < 3 } { incr i } {
		if [catch { expr [lindex $par $i] } v] {
			set v 0
		}
		append ln ", $v"
	}
			
	catch { puts $fd $ln }

	if { $prt != "SD" } {
		eprogress $prt $slo
	}
}

proc extract_dialog { prt } {
#
# This is shared by Collector/Aggregator/SD extraction
#
	global WN MV ST

	if [info exists WN(w,$prt,E)] {
		if ![confirm "You have an extraction window open for this node.\
			It will be closed before starting the new extraction.\
			Is that OK?"] {
				return ""
		}
		catch { destroy $WN(w,$prt,E) }
		catch { unset WN(w,$prt,E) }
	}

	set w [smw $prt "Extraction parameters"]

	# not sure if need this, but ... for now ...
	if { $prt != "SD" } {
		disable_node_window $prt
	}

	frame $w.o -pady 2 -padx 2
	pack $w.o -side top -fill x

	set rwn 0

	if { $prt == "SD" } {
		# Include device selector
		label $w.o.dsl -text "Device number:" -anchor w
		grid $w.o.dsl -column 0 -row $rwn -sticky w -padx 4
		tk_optionMenu $w.o.dse MV(DS) "Scan" "Auto" "NoLbl" \
			 "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "10"
		grid $w.o.dse -column 1 -row $rwn -sticky w -padx 4
		# the default
		set MV(DS) "Auto"
		incr rwn
		# Legacy format
		label $w.o.fml -text "Data format:" -anchor w
		grid $w.o.fml -column 0 -row $rwn -sticky w -padx 4
		tk_optionMenu $w.o.fme MV(DF) "V1.3" "V1.2-F" "V1.2-0"
		grid $w.o.fme -column 1 -row $rwn -sticky w -padx 4
		set MV(DF) "V1.3"
		incr rwn
	}

	label $w.o.sbl -text "Starting slot number:" -anchor w
	grid $w.o.sbl -column 0 -row $rwn -sticky w -padx 4

	entry $w.o.sbe -width 9 -textvariable MV(FR)
	grid $w.o.sbe -column 1 -row $rwn -sticky w -padx 4
	# the default
	set MV(FR) 0

	incr rwn

	label $w.o.sel -text "Ending slot number:" -anchor w
	grid $w.o.sel -column 0 -row $rwn -sticky w -padx 4

	entry $w.o.see -width 9 -textvariable MV(UP)
	grid $w.o.see -column 1 -row $rwn -sticky w -padx 4
	# the default
	set MV(UP) 999999999

	incr rwn

	label $w.o.tpl -text "Extract what:" -anchor w
	grid $w.o.tpl -column 0 -row $rwn -sticky w -padx 4

	tk_optionMenu $w.o.tpe MV(EV) "Data-c" "Data-s" "Events"
	grid $w.o.tpe -column 1 -row $rwn -sticky w -padx 4
	# the default
	set MV(EV) "Data-c"

	incr rwn

	label $w.o.twl -text "To where:" -anchor w
	grid $w.o.twl -column 0 -row $rwn -sticky w -padx 4

	tk_optionMenu $w.o.twe MV(FI) "Screen" "File"
	grid $w.o.twe -column 1 -row $rwn -sticky w -padx 4
	# the default
	set MV(FI) "Screen"

	incr rwn

	if { $prt == "SD" || $WN(NT,$prt) != "collector" } {
		# collector number
		label $w.o.cnl -text "Collector number:" -anchor w
		grid $w.o.cnl -column 0 -row $rwn -sticky w -padx 4
		entry $w.o.cne -width 5 -textvariable MV(CN)
		grid $w.o.cne -column 1 -row $rwn -sticky w -padx 4
		set MV(CN) "all"
		incr rwn
	}

	frame $w.b
	pack $w.b -side top -fill x

	button $w.b.go -text "Start" -command "set ST(MOD) 1"
	pack $w.b.go -side right

	button $w.b.ca -text "Cancel" -command "set ST(MOD) 0"
	pack $w.b.ca -side left

	bind $w <Destroy> "set ST(MOD) 0"

	wm transient $w .

	raise $w

	while 1 {

		tkwait variable ST(MOD)

		if { $ST(MOD) == 0 } {
			# cancelled
			dmw $prt
			if { $prt != "SD" } {
				enable_node_window $prt
			}
			return ""
		}

		# flag == scan for a card device
		set scn 0

		if { $prt == "SD" } {
			# device selector
			set ds $MV(DS)
			if { $ds == "Scan" } {
				# scan for a card
				set scn 1
			}
			set df $MV(DF)
		}

		if $scn {

			# scan for a card device

			set fr 0
			set up 0
			set em -1
			set deff "scan.txt"
			set defe ".txt"

		} else {

			# verify the arguments
			set fr [string trim $MV(FR)]
			if { $fr == "" } {
				set fr 0
			} else {
				set fr [vnum $fr 0 1000000000]
				if { $fr == "" } {
					alert "Illegal starting slot number,\
					    must be between 0 and 999999999!"
					continue
				}
			}

			set up [string trim $MV(UP)]
			if { $up == "" } {
				set up 999999999
			} else {
				set up [vnum $up 0 1000000000]
				if { $up == "" } {
					alert "Illegal ending slot number, must\
						be between 0 and 999999999!"
					continue
				}
			}

			if { $up < $fr } {
				alert "Ending slot number is less than starting\
					slot number!"
				continue
			}

			# decide on the mode
			if { $MV(EV) == "Data-c" } {
				# data by collector
				set em 1
			} elseif { $MV(EV) == "Data-s" } {
				# data by sensor
				set em 2
			} else {
				# events
				set em 3
			}

			if { $prt == "SD" || $WN(NT,$prt) != "collector" } {
				set cn [string tolower [string trim $MV(CN)]]
				if { $cn == "" || $cn == "all" || 
				    $cn == "any" } {
					set cn ""
				} else {
					set cn [vnum $cn 100 65536]
					if { $cn == "" } {
						alert "Illegal collector\
						    number, must be between 100\
							and 65535!"
						continue
					}
				}
			}

			if { $em > 2 } {
				# events
				set deff "events.csv"
			} else {
				set deff "values.csv"
			}
	
			set defe ".csv"
		}

		# time to handle the file
		if { $MV(FI) == "File" } {

			while 1 {
				set fn [tk_getSaveFile \
					-defaultextension $defe \
					-initialdir [defhome] \
					-initialfile $deff -parent $w \
					-title "File to store the data"]

				if { $fn == "" } {
					# cancelled
					catch { destroy $w }
					if { $prt != "SD"} {
						enable_node_window $prt
					}
					return ""
				}

				if ![catch { open $fn "w" } fd] {
					break
				}
			}

			# the file name
			set WN(EN,$prt) $fn
			set WN(EF,$prt) $fd
		} else {
			set WN(EF,$prt) ""
		}

		# do it
		break
	}

	# delete the modal window
	dmw $prt

	if { $prt == "SD" } {
		return [list $ds $df $em $fr $up]
	}
	return [list $em $fr $up]
}

proc extract_tty { prt em } {
#
# Create extraction window (for on-screen extraction)
#
	global WN
	
	if [info exists WN(w,$prt,E)] {
		alert "Duplicate extract log, internal error 0003!"
		return ""
	}

	set w ".exl$prt"
	set WN(w,$prt,E) $w
	toplevel $w

	wm title $w "[cnotype $prt] on port [prtname $prt]: data extraction"

	text $w.t

	# header length
	if { $em < 0 } {
		set wi [expr -$em]
	} else {
		set wi [ewhdr $prt $em 1]
	}

	$w.t configure  -yscrollcommand "$w.scroly set" \
			-xscrollcommand "$w.scrolx set" \
			-wrap none \
			-setgrid true \
        		-width $wi -height 24 \
			-font {-family courier -size 10} \
			-exportselection 1 \
			-state disabled

	$w.t delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"
	scrollbar $w.scrolx -orient horizontal -command "$w.t xview"
	pack $w.scroly -side right -fill y
	pack $w.scrolx -side bottom -fill x

	pack $w.t -expand yes -fill both
	$w.t configure -state disabled

	# abort/close

	frame $w.stat -borderwidth 2
	pack $w.stat -expand no -fill x

	button $w.stat.cl -text "Close" -command "close_ewindow $prt"
	pack $w.stat.cl -side right

	if { $prt == "SD" } {

		label $w.stat.st -text "Wait ...       "
		pack $w.stat.st -side right

	} else {

		label $w.stat.st -text "Running ...       "
		pack $w.stat.st -side right

		button $w.stat.ab -text "Abort" \
			-command "stop_extraction $prt 1"
		pack $w.stat.ab -side left
	}

	bind $w <Destroy> "close_ewindow $prt"

	return $w
}

proc esd_clean { } {
#
# Cleanup after SD extraction
#
	global ESD

	if [info exists ESD(FD)] {
		catch { close $ESD(FD) }
	}

	if [info exists ESD(SF)] {
		catch { close $ESD(SF) }
	}

	if [info exists ESD(NO)] {
		catch { destroy $ESD(NO) }
	}

	catch { file delete -force zzxtr.txt }
}

proc esd_mknotf { } {
#
# Notifier message
#
	global ESD ST

	incr ST(CNT)

	set wn ".msg$ST(CNT)"

	toplevel $wn

	wm title $wn "Message"

	set ESD(NO) $wn

	label $wn.t -width 50 -height 1 -borderwidth 2 -state normal
	pack $wn.t -side top -fill x -fill y
	$wn.t configure -text ""
	raise $wn

	return $wn
}

proc esd_note_go { } {

	global ESD

	incr ESD(NG)
}

proc esd_note { msg } {

	global ESD

	set ESD(NG) 0

	if ![catch { $ESD(NO).t configure -text $msg } ] {
		after 100 esd_note_go
		vwait ESD(NG)
	}
}

proc extract_sd { } {
#
# SD card extraction
#
	global ST WN ESD PM

	set em [extract_dialog "SD"]

	if { $em == "" } {
		# cancelled
		return
	}

	foreach { ds df em fr up } $em { }

	if { $WN(EF,SD) != "" } {
		set ESD(SF) $WN(EF,SD)
	}

	esd_mknotf
	esd_note "Running esdreader: be patient, this may take a while ..."

	set esd $PM(ESR)

	if { $esd == "" } {
		alert "No esdreader installed!"
		esd_clean
		return
	}

	if { $ds == "Scan" } {
		# scan for cards
		if [catch { exec $esd -s -n } out] {
			alert "esdreader failed: $out"
			esd_clean
			return
		}
		# out contains the result of scan, write it wherever it should
		# go
		if { $WN(EF,SD) != "" } {
			# write to file
			if [catch { puts -nonewline $WN(EF,SD) $out } err] {
				alert "cannot write to $WN(EN,SD): $err"
				esd_clean
				return
			}
			esd_clean
			alert "Scan complete!"
			return
		}
		# write to screen
		if { [extract_tty "SD" -74] == "" } {
			# failed
			esd_clean
			return
		}
		set w $WN(w,SD,E)
		while 1 {
			set nl [string first "\n" $out]
			if { $nl < 0 } {
				break
			}
			set ln [string trim [string range $out 0 $nl] "\r\n"]
			incr nl
			set out [string range $out $nl end]
			add_text $w.t $ln
			end_line $w.t
		}
		esd_clean
		$w.stat.st configure -text "Done      "
		alert "Scan complete!"
		return
	}


	# arguments
	set eag ""

	if { $ds != "Auto" } {
		lappend eag "-n"
		if { $ds != "NoLbl" } {
			if [catch { expr $ds } ds] {
				alert "illegal scan parameter,\
					internal error 0004!"
				esd_clean
				return
			}
			lappend eag "-d"
			lappend eag $ds
		}
	}

	if { $df != "V1.3" } {
		lappend eag "-o"
		if { $df == "V1.2-0" } {
			lappend eag "0"
		}
	}

	set eag [concat $eag -f $fr -t $up zzxtr.txt]

	if [catch { eval [list exec] [list $esd] $eag } err] {

		alert "esdreader failed: $err"
		esd_clean
		return
	}

	if [catch { open "zzxtr.txt" r } efd] {
		alert "esdreader failed: cannot read data file: $efd"
		esd_clean
		return
	}

	set ESD(FD) $efd

	if { $WN(EF,SD) == "" } {
		# to screen (we need a limit)
		if { [extract_tty "SD" $em] == "" } {
			# failed
			esd_clean
			return
		}
	}

	ewhdr "SD" $em

	set scn 0

	while { [gets $efd line] >= 0 } {
		if { [expr $scn % 1000] == 0 } {
			esd_note "Extracting data ... [expr $scn / 1000]K"
		}
		extract_aggregator_samples "SD" $em $line
		incr scn
	}

	esd_clean

	if [info exists WN(w,SD,E)] {
		set w $WN(w,SD,E)
		$w.stat.st configure -text "Done      "
	}

	alert "Extraction complete!"
}

proc trims { t n } {
#
# Make sure t is at least n chars long
#
	set ln [string length $t]

	set p ""

	while { $ln < $n } {
		append p " "
		incr ln
	}

	return "$p$t"
}

proc set_vuee_connection_params { } {
#
# Open a dialog to set VUEE parameters for connection
#
	global PM ST MV

	disable_main_window

	set w [smw "__" "VUEE connection params"]

	frame $w.f -pady 2 -padx 2
	set f $w.f
	pack $f -side top -fill both

	label $f.hl -text "Host: "
	grid $f.hl -column 0 -row 0 -sticky w -padx 4
	set MV(HO) $PM(VHO)
	entry $f.he -width 12 -textvariable MV(HO)
	grid $f.he -column 1 -row 0 -sticky e -padx 4

	label $f.pl -text "Port: "
	grid $f.pl -column 0 -row 1 -sticky w -padx 4
	set MV(PO) $PM(VPO)
	entry $f.pe -width 6 -textvariable MV(PO)
	grid $f.pe -column 1 -row 1 -sticky e -padx 4

	label $f.nl -text "Node: "
	grid $f.nl -column 0 -row 2 -sticky w -padx 4
	set MV(NO) $PM(VNO)
	entry $f.ne -width 6 -textvariable MV(NO)
	grid $f.ne -column 1 -row 2 -sticky e -padx 4

	frame $w.b
	set f $w.b
	pack $f -side top -fill x

	button $f.go -text "Go ahead" -command "set ST(MOD) 1"
	pack $f.go -side right

	button $f.ca -text "Cancel" -command "set ST(MOD) 0"
	pack $f.ca -side left

	bind $w <Destroy> "set ST(MOD) 0"
	# doesn't work on Linux
	#wm transient $w .

	raise $w

	while 1 {

		tkwait variable ST(MOD)

		if { $ST(MOD) == 0 } {
			# cancelled
			dmw "__"
			enable_main_window
			return 1
		}

		# error list
		set el ""

		# reset the argument
		set MV(HO) [string trim $MV(HO)]
		if { $MV(HO) == "" } {
			lappend el "hostname is empty"
		}
		set n [vnum $MV(PO) 1 65535]
		if { $n == "" } {
			lappend el "illegal port (must be between 1 and 65535)"
		} else {
			set MV(PO) $n
		}
		set n [vnum $MV(NO) 0]
		if { $n == "" } {
			lappend el "illegal node (must be an int >= 0)"
		} else {
			set MV(NO) $n
		}

		if { $el != "" } {
			# errors
			if { [llength $el] > 1 } {
				set em "Errors: "
			} else {
				set em "Error: "
			}
			append em [join $el ", "]
			alert $em
			continue
		}

		# OK
		break
	}

	set PM(VHO) $MV(HO)
	set PM(VPO) $MV(PO)
	set PM(VNO) $MV(NO)

	dmw "__"
	enable_main_window
	return 0
}

proc cancel_vuee_connect { } {
	
	global ST

	set ST(CAN) 1
}

proc do_connect_vuee { nty } {
#
# Handles an attempt to connect to directly to a VUEE model
#
	global WN PM ST

	# get the parameters
	if [set_vuee_connection_params] {
		# cancelled
		return
	}

	# we will use this as an identifier of the connection, like a port
	# name for a UART; note: the first character tells us it is something
	# special
	set prt "=$PM(VHO)=$PM(VPO)=$PM(VNO)"

	if [info exists WN(FD,$prt)] {
		alert "We already have this connection in another session!"
		return
	}

	disable_main_window

	set ST(CAN) 0
	set ST(CPR) $prt

	set msgw [mk_mess_window cancel_connect 50 1 \
	     "Trying to connect to host $PM(VHO), port $PM(VPO), node $PM(VNO)"]

	if { [catch { vuart_conn $PM(VHO) $PM(VPO) $PM(VNO) ::ST(CAN) } fd] ||
	    $ST(CAN) } {
		stop_port $prt
		catch { destroy $msgw }
		if !$ST(CAN) {
			alert $fd
		}
		enable_main_window
		return
	}

	# OK so far, we have connected to the UART socket
	preconf_port $prt $fd
	set ST(CPR) $prt

	# equivalent of u_conf
	set WN(IB,$prt) ""
	fileevent $fd readable "u_iready $prt"

	# this while is just to have something to break out of
	while 1 {

		if { $ST(CAN) || \
		    [outmess $msgw "Waiting for response from node"] } {
			# cancelled
			set ST(CAN) 1
			stop_port $prt
			break
		}

		set fai [try_port $prt $nty]

		if { $fai < 0 } {
			# cancelled
			set ST(CAN) 1
			stop_port $prt
			break
		}

		if { $fai != 0 } {
			stop_port $prt
		}

		break
	}

	enable_main_window
	set ST(CPR) ""
	catch { destroy $msgw }

	if $ST(CAN) {
		# cancelled
		return
	}

	if $fai {
		alert "The node doesn't repond as required!"
	} else {
		setup_window $prt
	}
}

proc do_connect { } {

	global WN PM ST mpVal mrVal mnVal

	if { $mpVal == "Auto" } {
		set prt ""
	} else {
		set prt $mpVal
	}

	if [catch { expr $mrVal } spd] {
		set spd ""
	}

	if { $mnVal == "" || $mnVal == "Any" } {
		set nty "Any"
	} else {
		set nty $mnVal
	}

	if { $prt == "VUEE" } {
		do_connect_vuee $nty
		return
	}

	if { $prt != "" } {
		if [info exists WN(FD,$prt)] {
			alert "Port $prt already used in another session,\
				close that session first!"
			return
		}
	}

	disable_main_window

	# create a message window to show the progress

	set msgw [mk_mess_window cancel_connect 35 1 ""]
	set fai 1
	set ST(CPR) ""

	if { $spd == "" } {
		set splist "19200 9600"
	} else {
		set splist $spd
	}

	if { $prt == "" } {
		set fpr $PM(MPS)
		set tpr $PM(MPE)
	} else {
		# these need not be numeric
		set fpr $prt
		set tpr $prt
	}

	# reset cancel flag
	set ST(CAN) 0

	set prt $fpr

	while 1 {

		if ![info exists WN(FD,$prt)] {

			# not already open
			set fd [u_tryopen $prt]

			if { $fd != "" } {

				# tentatively preconfigure things

				preconf_port $prt $fd
				set ST(CPR) $prt

				foreach sp $splist {

					set fai [u_conf $prt $sp]
					if $fai {
						# can only by > 0, no cancel
						stop_port $prt
						break
					}

					if { $ST(CAN) || [outmess $msgw \
			    		"Connecting to port $prt at $sp"] } {

						# cancelled
						set ST(CAN) 1
						stop_port $prt
						break
					}

					set fai [try_port $prt $nty]

					if { $fai < 0 } {
						# cancelled
						set ST(CAN) 1
						stop_port $prt
						break
					}

					if { $fai == 0 } {
						break
					}

				}

				if { $fai == 0 } {
					break
				}

				stop_port $prt
			}
		}
		if { $prt == $tpr } {
			break
		}
		if [catch { incr prt } ] {
			break
		}
	}

	enable_main_window
	set ST(CPR) ""
	catch { destroy $msgw }

	# must check this first as destroy msgw will set the flag
	if $ST(CAN) {
		# cancelled
		return
	}

	if $fai {
		alert "Couldn't locate a responsive node!"
	} else {
		setup_window $prt
	}
}

proc snp_window { { ge "" } } {
#
# Opens a global window listing all snippets
#
	global ST PM

	if [info exists ST(w,S)] {
		raise $ST(w,S)
		# already present
		return
	}

	set sl [snip_names]

	if { $sl == "" } {
		# no snippets 
		if [confirm "You have no snippets at present. Would you like to\
		    create some?"] {
			# open a snippet editor
			snp_ewin ""
		}
		return
	}

	set w .snip
	toplevel $w

	wm title $w "Conversion snippets"

	set ST(w,S) $w

	if { $ge != "" } {
		# preserve previous location
		regexp "\\+.*\\+.*" $ge ge
		wm geometry $w $ge
	}

	set z $w.s

	labelframe $z -text "Snippet list (Name, Assignments)"

	pack $z -side top -fill x -ipady 2 -pady 6

	set row 0
	set ix 0
	foreach sn $sl {

		button $z.b$ix -text $sn -anchor w -command "snp_ewin $sn"
		grid $z.b$ix -column 0 -row $row -sticky we -padx 10

		set asn [snip_assignments $sn]
		set ast ""

		foreach an $asn {
			# build the assignment string
			if { $ast != "" } {
				append ast " / "
			}
			append ast "[lindex $an 0] @ [lindex $an 1]"
		}
		# maximum length
		if { [string length $ast] > 80 } {
			set ast "[string range $ast 0 76]..."
		}

		label $z.a$ix -text $ast
		grid $z.a$ix -column 1 -row $row -sticky w -ipadx 1

		incr row
		incr ix
	}

	frame $w.b
	pack $w.b -side top -fill x -expand no

	button $w.b.n -text "New" -command "snp_ewin"
	pack $w.b.n -side left

	button $w.b.q -text "Quit" -command "snp_quitm"
	pack $w.b.q -side right

	bind $w <Destroy> snp_quitm
}

proc snp_quitm { } {
#
# Quit the master snippet window
#
	global ST

	if ![info exists ST(w,S)] {
		# already quitted
		return
	}

	if [info exists ST(v,S,H)] {
		# do not destroy editor windows, we are rebuilding the master
		# window
		return
	}

	# quit all editor windows
	snp_qewins

	set w $ST(w,S)
	unset ST(w,S)

	catch { destroy $w }
}

proc snp_redom { } {
#
# Redo the main window
#
	global ST

	if ![info exists ST(w,S)] {
		# just create it
		snp_window
		return
	}

	set w $ST(w,S)

	# flag == do not erase ewins
	set ST(v,S,H)	1

	# preserve the old geometry (position, that is)
	set ge [wm geometry $w]

	destroy $w

	unset ST(w,S)
	unset ST(v,S,H)

	snp_window $ge
}

proc snp_fwbn { nm } {
#
# Find editor window by snippet name
#
	global ST

	set wl [array names ST "v,E,*,N"]

	foreach w $wl {
		if { $nm == $ST($w) } {
			return 1
		}
	}

	return 0
}

proc snp_ewin { { nm "" } } {
#
# Open a snippet editor
#
	global ST PM

	if { $nm != "" } {
		# check is a named window with the same name is not open
		# already
		if [snp_fwbn $nm] {
			alert "An edit window for snippet $nm is already open"
			return
		}
		# the script
		set ex [snip_script $nm]
		set un [snip_units $nm]
		set al [snip_assignments $nm]
	} else {
		set ex ""
		set un ""
		set al ""
	}

	# make them distinct
	incr ST(CNT)

	# the window index
	set wi $ST(CNT)

	# the window name
	set w .sne$wi

	toplevel $w

	# so we know what to close
	set ST(w,E,$wi) $w

	# name (can be empty)
	set ST(v,E,$wi,N) $nm

	# title
	if { $nm != "" } {
		set tt "Snippet $nm"
	} else {
		set tt "New snippet"
	}

	wm title $w $tt

	# the top row is for assignments
	labelframe $w.a -text "Assignments (Sensor, SenSets)" -padx 2 -pady 2
	pack $w.a -side top -fill x -expand no

	set snsel "0"

	set spn [snip_getparam SPN]
	set nas [snip_getparam NAS]

	for { set i 1 } { $i < $spn } { incr i } {
		# build the list of options for sensor number
		append snsel " $i"
	}

	set z $w.a

	set ST(v,E,$wi,U) $un

	set i 0
	while 1 {

		# the i-th assignment

		eval "tk_optionMenu $z.c$i ST(v,E,$wi,C,$i) $snsel"
		pack $z.c$i -side left -expand yes

		entry $z.a$i -width 16 -textvariable ST(v,E,$wi,A,$i)
		pack $z.a$i -side left -expand yes

		set a [lindex $al $i]

		if { $a != "" } {
			set ST(v,E,$wi,C,$i) [lindex $a 0]
			set ST(v,E,$wi,A,$i) [lindex $a 1]
		}

		incr i
		if { $i == $nas } {
			break
		}

		# padding
		label $z.p$i -text "  "
		pack $z.p$i -side left
	}

	text $w.t

	$w.t configure \
		-yscrollcommand "$w.scroly set" \
		-setgrid true \
        	-width 80 -height 12 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$w.t delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"

	pack $w.scroly -side right -fill y

	pack $w.t -side top -expand yes -fill both

	# buttons

	frame $w.b -borderwidth 2
	pack $w.b -side top -expand no -fill x

	if { $nm == "" } {
		label $w.b.l -text "Name:"
		pack $w.b.l -side left
		entry $w.b.n -width 16 -textvariable ST(v,E,$wi,N)
		pack $w.b.n -side left
		# some padding
		label $w.b.p -text "  "
		pack $w.b.p -side left
	} else {
		# display the snippet in the window
		add_text $w.t $ex
	}
	# make sure we can edit it
	$w.t configure -state normal

	button $w.b.t -text "Try it out:" -command "snp_tryout $wi"
	pack $w.b.t -side left

	label $w.b.a -text "value = "
	pack $w.b.a -side left

	entry $w.b.v -width 6 -textvariable ST(v,E,$wi,V)
	pack $w.b.v -side left

	# save/exit
	button $w.b.e -text "Exit" -command "snp_qewin $wi 0"
	pack $w.b.e -side right

	button $w.b.s -text "Save" -command "snp_esave $wi"
	pack $w.b.s -side right

	button $w.b.d -text "Delete" -command "snp_delete $wi"
	pack $w.b.d -side right

	entry $w.b.u -width 6 -textvariable ST(v,E,$wi,U)
	pack $w.b.u -side right

	label $w.b.g -text "Units:"
	pack $w.b.g -side right

	bind $w <Destroy> "snp_qewin $wi 1"
}

proc snp_qewins { } {
#
# Cancel all editor windows
#
	global ST

	set wl [array names ST "w,E,*"]

	foreach w $wl {
		# this cannot fail
		regexp "^w,E,(.+)" $w jk ix
		snp_qewin $ix 1
	}
}

proc snp_scmp { a b } {
#
# Compares two strings in a flimsy sort of way
#
	set a [string trim $a]
	regsub -all "\[ \t\n\r\]" $a " " a
	set b [string trim $b]
	regsub -all "\[ \t\n\r\]" $b " " b

	return [string equal $a $b]
}

proc snp_tryout { wi } {
#
# Try to run the snippet
#
	global ST

	if ![info exists ST(w,E,$wi)] {
		# the window is no more (a race?)
		return
	}

	if [info exists ST(v,E,$wi,V)] {
		set v $ST(v,E,$wi,V)
	} else {
		set v ""
	}

	if { $v == "" } {
		alert "You haven't specified a value for the snippet!"
		return
	}

	set value [vnum $v 0 65536]

	if { $value == "" } {
		alert "Sensor value '$v' is illegal; must be between 0 and\
			65535"
		return
	}

	# get hold of the code
	set w $ST(w,E,$wi)
	set ex [string trim [$w.t get 1.0 end]]

	if { $ex == "" } {
		alert "The snippet is empty!"
		return
	}

	if [catch { snip_eval $ex $value } value] {
		alert "The snippet failed: $value"
		return
	}
	if { $value != "?" } {
		if [catch { expr $value } val] {
			alert "Illegal return value: $value, must be a number\
				or ?"
			return
		}
		set value [format %1.2f $val]
	}

	alert "Converted: F($v) = $value"
}

proc snip_ifmod { wi } {
#
# Check if modified
#
	global ST PM

	set w $ST(w,E,$wi)

	# name
	set nm [string trim $ST(v,E,$wi,N)]

	# code
	set new [string trim [$w.t get 1.0 end]]

	if { $nm == "" || ![snip_exists $nm] } {
		# not one of the existing
		if { $new == "" } {
			# empty, OK
			return 0
		}
		return 1
	}

	if ![snp_scmp $new [snip_script $nm]] {
		# code differs
		return 1
	}

	# how about the assignments?
	set al [snip_assignments $nm]
	set nas [snip_getparam NAS]

	for { set i 0 } { $i < $nas } { incr i } {
		# assignment
		set as [string trim $ST(v,E,$wi,A,$i)]
		# from the original
		set og [lindex $al $i]
		set oa [lindex $og 1]

		if { $as == "" } {
			# nothing there
			if { $oa != "" } {
				# but the original does have something
				return 1
			}
			continue
		}

		# compare the sensors and assignments
		set os [lindex $og 0]
		set se $ST(v,E,$wi,C,$i)
		if { $oa != $as || $se != $os } {
			return 1
		}
	}
	return 0
}

proc snp_qewin { wi abt } {
#
# Quit an editor window
#
	global ST

	if ![info exists ST(w,E,$wi)] {
		# already gone
		return
	}

	# have to check if saved?
	if { !$abt && [snip_ifmod $wi] } {
		if ![confirm "The changes have not been saved. Should the\
		    window be closed?"] {
			return
		}
	}

	set w $ST(w,E,$wi)

	catch { destroy $w }

	# deallocate variables
	array unset ST "v,E,$wi,*"
}

proc snp_delete { wi } {
#
# Delete the snippet
#
	global ST

	if ![info exists ST(w,E,$wi)] {
		# already gone
		return
	}

	set w $ST(w,E,$wi)
	set new [string trim [$w.t get 1.0 end]]

	set nm [string trim $ST(v,E,$wi,N)]

	set warn 1
	set updt 1

	if { $nm == "" || ![snip_exists $nm] } {
		set updt 0
		if { $new == "" } {
			set warn 0
		}
	}

	if $warn {
		if ![confirm "Are you sure that you want to delete this\
		    snippet?"] {
			return
		}
	}

	if $updt {
		snip_delete $nm
		if { [snip_names] == "" } {
			# no more, close the master window
			snp_quitm
		} else {
			snp_redom
		}
		snp_write
	}

	snp_qewin $wi 1
}

proc snp_esave { wi } {
#
# Save the snippet
#
	global ST PM

	if ![info exists ST(w,E,$wi)] {
		# cannot happen
		return
	}

	set w $ST(w,E,$wi)

	# the name
	set nm [string trim $ST(v,E,$wi,N)]

	if { $nm == "" } {
		alert "The snippet has no name!"
		return
	}

	# get the code
	set new [string trim [$w.t get 1.0 end]]
	if { $new == "" } {
		alert "The snippet code is empty!"
		return
	}

	# go through the list of assignments
	set asl ""

	set skip 0
	set cmpr 0

	set nas [snip_getparam NAS]
	for { set i 0 } { $i < $nas } { incr i } {
		# assignment
		set se $ST(v,E,$wi,C,$i)
		set as [string trim $ST(v,E,$wi,A,$i)]

		if { $as == "" } {
			set skip 1
			continue
		}
		if $skip {
			set cmpr 1
		}
		lappend asl [list $se $as]
	}

	if $cmpr {
		# there were holes: compress the assignments in the window
		for { set i 0 } { $i < $nas } { incr i } {
			set el [lindex $asl $i]
			set se [lindex $el 0]
			set as [lindex $el 1]
			set ST(v,E,$wi,A,$i) $as
			if { $as == "" } {
				set se 0
			}
			set ST(v,E,$wi,C,$i) $se
		}
	}

	# OK, ready to save
	set er [snip_set $nm $new $ST(v,E,$wi,U) $asl]

	if { $er != "" } {
		alert "The snippet is invalid: $er"
		return
	}

	# redo the main window (later we may try to be more selective)
	snp_redom

	# write back to file
	snp_write
}

proc snp_read { } {
#
# Read snippets from file
#
	global PM

	if [catch { open $PM(COB) "r" } fd] {
		return "You have no conversion snippets."
	}

	if [catch { read $fd } cf] {
		catch { close $fd }
		return \
	            "Cannot access the (existing) snippets file $PM(COB): $cf."
	}

	catch { close $fd }

	set er [snip_parse $cf]

	if { $er != "" } {
		return "The snippets file $PM(COB) in $PM(HOM) is invalid or\
		    corrupted. The problem is this: $er."
	}

	return ""
}

proc snp_write { } {
#
# Write back conversion snippets
#
	global PM

	set txt [snip_encode]

	# invalidate the cache (we write after every update)
	snip_icache

	if [catch { open $PM(COB) "w" } fd] {
		alert "Cannot open the snippets file $PM(COB) for writing: $fd"
		return
	}

	if [catch { puts $fd $txt } err] {
		catch { close $fd }
		alert "Cannot write to the snippets file $PM(COB): $err"
		return
	}

	catch { close $fd }

	alert "The snippets file has been updated."
}

# Startup #####################################################################

proc set_home_dir { ds } {
#
# Creates the home directory to include logs and configuration files; reads
# in conversion snippets
#
	global PM ST env

	if [info exists env(HOME)] {
		set hfn [file normalize $env(HOME)]
		if { $ST(SYS) == "L" || $ST(SYS) == "C" } {
			# this is user HOME on Unix or Cygwin
			set hd ".$PM(HOM)"
		} else {
			set hd $PM(HOM)
		}
	} else {
		abt "Cannot locate HOME directory! Is this a Unix system?"
	}

	set hfn [file join $hfn $hd]

	if ![file isdirectory $hfn] {
		# first time
		if ![confirm \
		       "You appear to be running EConnect for the first time!\n\
			\nI am going to create this directory: $hfn,\
			where I will be keeping logs and your configuration of\
			conversion snippets for sensor values.\
			The logs will be rotated (up to four files up to 1MB\
			each). The snippet file will be initialized with some\
			default set which you will be able to edit at will.\
			Just to let you know in case you ever want to clean\
			up after me.\n
			\nIs it OK to continue?"] {

			terminate
		}

		if [catch { file mkdir $hfn } err] {
			abt "Cannot create directory: $hfn, sorry!"
		}

		cd $hfn

		if [catch { open $PM(COB) "w" 0666 } fd] {
			abt "Cannot write to $hfn, sorry!"
		}
		puts -nonewline $fd [string trim $ds]
		catch { close $fd }
	}

	# this is where we will be working
	cd $hfn

	# open the log
	log_open

	# read conversion snippets
	set err [snp_read]

	if { $err != "" } {
		if ![confirm "$err I will set up default snippets for you."] {
			terminate
		}
		if [catch { open $PM(COB) "w" 0666 } fd] {
			abt "Cannot write to $hfn, sorry!"
		}
		puts -nonewline $fd [string trim $ds]
		catch { close $fd }

		if { [snp_read] != "" } {
			abt "Default snippets appear to be wrong.\
				This is an internal error. Sorry!"
		}
	}

	set esd $PM(ESR)

	if { $ST(SYS) != "L" } {
		append esd ".exe"
	}

	if { $PM(PGM) != "" } {
		set esd [file join $PM(PGM) $esd]
	}

	set esd [auto_execok $esd]

	if ![file executable $esd] {
		set esd ""
	}
	set PM(ESR) $esd
}

proc defhome { } {
#
# Return the default home directory, like the Desktop, for saving files
#
	global env

	if ![info exists env(HOMEPATH)] {
		return [pwd]
	}

	set dfn [file join $env(HOMEPATH) Desktop]

	if [file isdirectory $dfn] {
		return $dfn
	}

	set dfn [file join $env(HOMEPATH) Documents]

	if [file isdirectory $dfn] {
		return $dfn
	}

	if [file isdirectory $env(HOMEPATH)] {
		return $env(HOMEPATH)
	}

	return [pwd]
}

#################

if 1 {
catch { close stdin }
#catch { close stdout }
catch { close stderr }
}

# current port tried for connection
set ST(CPR)	""

# cancel flag
set ST(CAN)	0

wm title . "EcoNNeCt $PM(VER)"

# use a frame: we may want to add something to the window later
labelframe .conn -text "New Connection" -padx 4 -pady 4
pack .conn -side left -expand 0 -fill x

label .conn.lr -text "Rate:"
tk_optionMenu .conn.mr mrVal "Auto" "9600" "19200"

label .conn.lp -text "Port:"
eval "tk_optionMenu .conn.mp mpVal Auto[u_preopen]"

label .conn.ln -text "Node:"
tk_optionMenu .conn.mn mnVal "Any" "Aggregator" "Custodian" "Collector"

button .conn.connect -text "Connect" -command do_connect

button .conn.quit -text "Quit" -command { destroy .conn }

pack .conn.lr .conn.mr .conn.lp .conn.mp .conn.ln .conn.mn -side left
pack .conn.quit .conn.connect -side right

labelframe .sdc -text "SD Card" -padx 4 -pady 4
pack .sdc -side right -expand 0 -fill none
set sdb [button .sdc.ex -text "Extract" -command extract_sd]
pack .sdc.ex -side left -expand 0 -fill none

labelframe .sne -text "Snippets" -padx 4 -pady 4
pack .sne -side right -expand 0 -fill none
button .sne.ed -text "Editor" -command snp_window
pack .sne.ed -side left -expand 0 -fill none

bind .conn <Destroy> { terminate }
bind . <Destroy> { terminate }

## default snippets; note that the best way to produce the argument to
## set_home_dir is to edit those snippets using EcoNNeCt's snippet editor
## and then insert here the contents of the resultant file

set_home_dir {

{SHT_Temp {if { $value == 0 || $value == -1 } {
        set value "?"
} else {
        set value [expr -39.62 + 0.01 * $value]
}} C {2 {1 2 5}} {4 5} {6 5}} {IR_Motion {set value [expr $value]} N {3 3}} {Light {set value [expr $value * 0.5]} L {2 3}} {Chronos_Temp {if [expr $value & 0x2000] {
        set value [expr (~$value & 0x1fff) + 1]
        set value [expr -$value]
}
set value [expr $value / 20.0]} C {3 4}} {SHT_Humid {if { $value == 0 || $value == -1 } {
        set value "?"
} else {
	set value [expr -4.0 + 0.0405 * $value - 0.0000028 * $value * $value]
	if { $value < 0.0 } {
		set value 0.0
	} elseif { $value > 100.0 } {
		set value 100.0
	}
}} % {3 {1 2 5}} {5 5} {7 5}} {Chip_Temp {set value [expr $value * 0.1032 - 277.75]} C {0 all}} {Chronos_Acc {set value [expr $value]} N {2 4}} {Battery {set value [expr $value * 0.001221]} V {1 all}}

}

if { $PM(ESR) == "" } {
	# disable SD operations
	$sdb configure -state disabled
}

unset sdb
