#!/bin/sh
#####################\
exec wish "$0" "$@"

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

###############################################################################
# Standard packages ###########################################################
###############################################################################

package provide uartpoll 1.0
#################################################################
# Selects polled versus automatic input from asynchronous UART. #
# Copyright (C) 2012 Olsonet Communications Corporation.        #
#################################################################

namespace eval UARTPOLL {

variable DE

proc run_picospath { } {

	set ef [auto_execok "picospath"]
	if ![file executable $ef] {
		return [eval [list exec] [list sh] [list $ef] [list -d]]
	}
	return [eval [list exec] [list $ef] [list -d]]
}

proc get_deploy_param { } {
#
# Extracts the poll argument of deploy
#
	if [catch { run_picospath } a] {
		return ""
	}

	# look up -p
	set l [lsearch -exact $a "-p"]
	if { $l < 0 } {
		return ""
	}

	set l [lindex $a [expr $l + 1]]

	if [catch { expr $l } l] {
		return 40
	}

	if { $l < 0 } {
		return 0
	}

	return [expr int($l)]
}

proc uartpoll_interval { sys fsy } {

	set p [get_deploy_param]

	if { $p != "" } {
		return $p
	}

	# use heuristics
	if { $sys == "L" } {
		# We are on Linux, the only problem is virtual box
		if { [file exists "/dev/vboxuser"] ||
		     [file exists "/dev/vboxguest"] } {
			# VirtualBox; use a longer timeout
			return 400
		}
		return 0
	}

	if { $fsy == "L" } {
		# Cygwin + native Tcl
		return 40
	}

	return 0
}

proc read_cb { dev fun } {

	variable CB

	if [eval $fun] {
		# void call, push the timeout
		if { $CB($dev) < $CB($dev,m) } {
			incr CB($dev)
		}
	} else {
		set $CB($dev) 0
	}

	set CB($dev,c) [after $CB($dev) "::UARTPOLL::read_cb $dev $fun"]
}

proc uartpoll_oninput { dev fun { sys "" } { fsy "" } } {

	variable CB

	if { $sys == "" } {
		set p 0
	} else {
		set p [uartpoll_interval $sys $fsy]
	}

	if { $p == 0 } {
		set CB($dev,c) ""
		fileevent $dev readable $fun
		return
	}

	set CB($dev) 1
	set CB($dev,m) $p
	read_cb $dev $fun
}

proc uartpoll_stop { dev } {

	variable CB

	if [info exists CB($dev)] {
		if { $CB($dev,c) != "" } {
			catch { after cancel $CB($dev,c) }
		}
		array unset CB($dev)
		array unset CB($dev,*)
	}
}

namespace export uartpoll_*

### end of UARTPOLL namespace #################################################

}

namespace import ::UARTPOLL::uartpoll_*


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

###############################################################################
###############################################################################

package provide vuart 1.0
#####################################################################
# This is a package for initiating direct VUEE-UART communication.  #
# Copyright (C) 2012 Olsonet Communications Corporation.            #
#####################################################################

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

	catch {
		upvar #0 $VU(AV) abv
		set abv $abv
	}
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

	if { $VU(SS) == 0 } {
		# expecting the initial code
		if { [catch { read $VU(FD) 4 } res] || $res == "" } {
			# disconnection
			set VU(ER) "Connection refused"
			kick
			return
		}

		set code [dbin_Q res]
		if { $code != 129 } {
			# wrong code
			set VU(ER) "Connection refused by VUEE, code $code"
			kick
			return 
		}

		set VU(SS) 1
		set VU(SI) ""
	}

	# read the signature message

	while 1 {
		if [catch { read $VU(FD) 1 } res] {
			set VU(ER) "Connection broken during handshake"
			kick
			return
		}
		if { $res == "" } {
			return
		}
		if { $res == "\n" } {
			# done
			break
		}
		append VU(SI) $res
	}

	# verify the signature
	kick
	if ![regexp "^P (\[0-9\]+) \[FO\] (\[0-9\]+) (\[0-9\]+) <(\[^ \]*)>:" \
	    $VU(SI) mat nod hos tot tna] {
		set VU(ER) "Illegal node signature: $VU(SI)"
		return
	}

	# connection OK

	set VU(SI) [list $nod $hos $tot $tna]
}

proc vuart_conn { ho po no abvar { hi 0 } { sig "" } } {
#
# Connect to UART at the specified host, port, node; hi is the HID flag: when
# nonzero, it means that the node number is to be interpreted as a Host ID;
# abvar is the abort variable (set to 1 <from the outside> to abort the
# connection in progress)
#
	variable VU

	init $abvar

	if [abtd] {
		cleanup
		error "Preaborted"
	}

	# these actions cannot block
	if { [info tclversion] > 8.5 && $ho == "localhost" } {
		# do not use -async, it doesn't seem to work in 8.6
		# for localhost
		set err [catch { socket $ho $po } sfd]
	} else {
		set err [catch { socket -async $ho $po } sfd]
	}

	if $err {
		cleanup
		error "Connection failed: $sfd"
	}

	set VU(FD) $sfd

	if [catch { fconfigure $sfd -blocking 0 -buffering none \
    	    -translation binary -encoding binary } erc] {
		cleanup
		error "Connection failed: $erc"
	}

	if $hi {
		set hi 1
	}

	# prepare a request
	set rqs ""
	abin_S rqs 0xBAB4
	abin_S rqs 1
	abin_I rqs $no
	abin_I rqs $hi

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
	set VU(SS) 0
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

	if { $sig != "" } {
		upvar $sig s
		set s $VU(SI)
	}

	cleanup 1

	fileevent $fd readable {}

	return $fd
}

namespace export vuart_conn

### end of VUART namespace ####################################################
}

namespace import ::VUART::vuart_conn

###############################################################################
###############################################################################

package provide autoconnect 1.0

package require unames

###############################################################################
# Autoconnect #################################################################
###############################################################################

namespace eval AUTOCONNECT {

variable ACB

array set ACB { CLO "" CBK "" }

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::AUTOCONNECT::$fun"
}

proc autocn_heartbeat { } {
#
# Called to provide connection heartbeat, typically after a message reception
#
	variable ACB

	incr ACB(LRM)
}

proc autocn_start { op cl hs hc cc { po "" } { dl "" } } {
#
# op - open function
# cl - close function
# hs - command to issue handshake message
# hc - handshake condition (if true, handshake has been successful)
# cc - connection condition (if false, connection has been broken)
# po - poll function (to kick the node if no heartbeat)
# dl - explicit device list
# 
	variable ACB

	set ACB(OPE) [gize $op]
	set ACB(CLO) [gize $cl]
	set ACB(HSH) [gize $hs]
	set ACB(HSC) [gize $hc]
	set ACB(CNC) [gize $cc]
	set ACB(POL) [gize $po]
	set ACB(DLI) $dl

	# trc "ACN START: $ACB(OPE) $ACB(CLO) $ACB(HSH) $ACB(HSC) $ACB(CNC) $ACB(POL) $ACB(DLI)"

	# last device scan time
	set ACB(LDS) 0

	# automaton state
	set ACB(STA) "L"

	# hearbeat variables
	set ACB(RRM) -1
	set ACB(LRM) -1

	if { $ACB(CBK) != "" } {
		# a precaution
		catch { after cancel $ACB(CBK) }
	}
	set ACB(CBK) [after 10 [lize ac_callback]]
}

proc autocn_stop { } {
#
# Stop the callback and disconnect
#
	variable ACB

	if { $ACB(CBK) != "" } {
		catch { after cancel $ACB(CBK) }
		set ACB(CBK) ""
	}

	if { $ACB(CLO) != "" } {
		$ACB(CLO)
	}
}

proc ac_again { d } {

	variable ACB

	set ACB(CBK) [after $d [lize ac_callback]]
}

proc ac_callback { } {
#
# This callback checks if we are connected and if not tries to connect us
# automatically to the node
#
	variable ACB

	# trc "AC S=$ACB(STA)"
	if { $ACB(STA) == "R" } {
		# CONNECTED
		if ![$ACB(CNC)] {
			# just got disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		# we are connected, check for heartbeat
		if { $ACB(RRM) == $ACB(LRM) } {
			# stall?
			if { $ACB(POL) != "" } { $ACB(POL) }
			set ACB(STA) "T"
			set ACB(FAI) 0
			ac_again 1000
			return
		}
		set ACB(RRM) $ACB(LRM)
		# we are still connected, no need to panic
		ac_again 2000
		return
	}

	if { $ACB(STA) == "T" } {
		# HEARTBEAT FAILURE
		if ![$ACB(CNC)] {
			# disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		if { $ACB(RRM) == $ACB(LRM) } {
			# count failures
			if { $ACB(FAI) == 5 } {
				set ACB(STA) "L"
				$ACB(CLO)
				ac_again 100
				return
			}
			incr ACB(FAI)
			if { $ACB(POL) != "" } { $ACB(POL) }
			ac_again 600
			return
		}
		set ACB(RRM) $ACB(LRM)
		set ACB(STA) "R"
		ac_again 2000
		return
	}
	
	#######################################################################

	if { $ACB(STA) == "L" } {
		# CONNECTING
		set tm [clock seconds]
		if { $tm > [expr $ACB(LDS) + 5] } {
			# last scan for new devices was done more than 5 sec
			# ago, rescan
			if { $ACB(DLI) != "" } {
				set ACB(DVS) ""
				foreach d $ACB(DLI) {
					if [catch { expr $d } n] {
						lappend ACB(DVS) $d
					} else {
						lappend ACB(DVS) \
							[unames_ntodev $d]
					}
				}
			} else {
				unames_scan
				set ACB(DVS) [lindex [unames_choice] 0]
			}
			set ACB(DVL) [llength $ACB(DVS)]
			set ACB(LDS) $tm
			# trc "AC RESCAN: $ACB(DVS), $ACB(DVL)"
		}
		# index into the device table
		set ACB(CUR) 0
		set ACB(STA) "N"
		ac_again 250
		return
	}

	#######################################################################

	if { $ACB(STA) == "N" } {
		# TRYING NEXT DEVICE
		# trc "AC N CUR=$ACB(CUR), DVL=$ACB(DVL)"
		# try to open a new UART
		if { $ACB(CUR) >= $ACB(DVL) } {
			if { $ACB(DVL) == 0 } {
				# no devices
				set ACB(LDS) 0
				ac_again 1000
				return
			}
			set ACB(STA) "L"
			ac_again 100
			return
		}

		set dev [lindex $ACB(DVS) $ACB(CUR)]
		incr ACB(CUR)
		if { [$ACB(OPE) $dev] == 0 } {
			ac_again 100
			return
		}

		$ACB(HSH)

		set ACB(STA) "C"
		ac_again 2000
		return
	}

	if { $ACB(STA) == "C" } {
		# WAITING FOR HANDSHAKE
		if [$ACB(HSC)] {
			# established, assume connection OK
			set ACB(STA) "R"
			ac_again 2000
			return
		}
		# sorry, try another one
		# trc "AC C -> CLOSING"
		$ACB(CLO)
		set ACB(STA) "N"
		ac_again 100
		return
	}

	set ACB(STA) "L"
	ac_again 1000
}

namespace export autocn_*

}

namespace import ::AUTOCONNECT::*

###############################################################################
###############################################################################

package provide noss 1.0

###############################################################################
# NOSS ########################################################################
###############################################################################

# This is N-mode packet interface with the Network ID field used as part
# of payload (akin to the boss package)

namespace eval NOSS {

###############################################################################
# ISO 3309 CRC + supplementary stuff needed by the protocol module ############
###############################################################################

variable CRCTAB {
    0x0000  0x1021  0x2042  0x3063  0x4084  0x50a5  0x60c6  0x70e7
    0x8108  0x9129  0xa14a  0xb16b  0xc18c  0xd1ad  0xe1ce  0xf1ef
    0x1231  0x0210  0x3273  0x2252  0x52b5  0x4294  0x72f7  0x62d6
    0x9339  0x8318  0xb37b  0xa35a  0xd3bd  0xc39c  0xf3ff  0xe3de
    0x2462  0x3443  0x0420  0x1401  0x64e6  0x74c7  0x44a4  0x5485
    0xa56a  0xb54b  0x8528  0x9509  0xe5ee  0xf5cf  0xc5ac  0xd58d
    0x3653  0x2672  0x1611  0x0630  0x76d7  0x66f6  0x5695  0x46b4
    0xb75b  0xa77a  0x9719  0x8738  0xf7df  0xe7fe  0xd79d  0xc7bc
    0x48c4  0x58e5  0x6886  0x78a7  0x0840  0x1861  0x2802  0x3823
    0xc9cc  0xd9ed  0xe98e  0xf9af  0x8948  0x9969  0xa90a  0xb92b
    0x5af5  0x4ad4  0x7ab7  0x6a96  0x1a71  0x0a50  0x3a33  0x2a12
    0xdbfd  0xcbdc  0xfbbf  0xeb9e  0x9b79  0x8b58  0xbb3b  0xab1a
    0x6ca6  0x7c87  0x4ce4  0x5cc5  0x2c22  0x3c03  0x0c60  0x1c41
    0xedae  0xfd8f  0xcdec  0xddcd  0xad2a  0xbd0b  0x8d68  0x9d49
    0x7e97  0x6eb6  0x5ed5  0x4ef4  0x3e13  0x2e32  0x1e51  0x0e70
    0xff9f  0xefbe  0xdfdd  0xcffc  0xbf1b  0xaf3a  0x9f59  0x8f78
    0x9188  0x81a9  0xb1ca  0xa1eb  0xd10c  0xc12d  0xf14e  0xe16f
    0x1080  0x00a1  0x30c2  0x20e3  0x5004  0x4025  0x7046  0x6067
    0x83b9  0x9398  0xa3fb  0xb3da  0xc33d  0xd31c  0xe37f  0xf35e
    0x02b1  0x1290  0x22f3  0x32d2  0x4235  0x5214  0x6277  0x7256
    0xb5ea  0xa5cb  0x95a8  0x8589  0xf56e  0xe54f  0xd52c  0xc50d
    0x34e2  0x24c3  0x14a0  0x0481  0x7466  0x6447  0x5424  0x4405
    0xa7db  0xb7fa  0x8799  0x97b8  0xe75f  0xf77e  0xc71d  0xd73c
    0x26d3  0x36f2  0x0691  0x16b0  0x6657  0x7676  0x4615  0x5634
    0xd94c  0xc96d  0xf90e  0xe92f  0x99c8  0x89e9  0xb98a  0xa9ab
    0x5844  0x4865  0x7806  0x6827  0x18c0  0x08e1  0x3882  0x28a3
    0xcb7d  0xdb5c  0xeb3f  0xfb1e  0x8bf9  0x9bd8  0xabbb  0xbb9a
    0x4a75  0x5a54  0x6a37  0x7a16  0x0af1  0x1ad0  0x2ab3  0x3a92
    0xfd2e  0xed0f  0xdd6c  0xcd4d  0xbdaa  0xad8b  0x9de8  0x8dc9
    0x7c26  0x6c07  0x5c64  0x4c45  0x3ca2  0x2c83  0x1ce0  0x0cc1
    0xef1f  0xff3e  0xcf5d  0xdf7c  0xaf9b  0xbfba  0x8fd9  0x9ff8
    0x6e17  0x7e36  0x4e55  0x5e74  0x2e93  0x3eb2  0x0ed1  0x1ef0
}

variable B

# abort function (in case of internal fatal error, considered impossible)
set B(ABR) ""

# diag output function
set B(DGO) ""

# character zero (aka NULL)
set B(ZER) [format %c [expr 0x00]]

# preamble byte
set B(IPR) [format %c [expr 0x55]]

# diag preamble (for packet modes) = ASCII DLE
set B(DPR) [format %c [expr 0x10]]

# reception automaton state
set B(STA) 0

# reception automaton remaining byte count
set B(CNT) 0

# function to call on packet reception
set B(DFN) ""

# function to call on UART close (which can happen asynchronously)
set B(UCF) ""

# low-level reception timer
set B(TIM)  ""

# packet timeout (msec), once reception has started
set B(PKT) 8000

###############################################################################

proc no_chks { wa } {

	variable CRCTAB

	set nb [string length $wa]

	set chs 0

	while { $nb > 0 } {

		binary scan $wa su waw
		#set waw [expr $waw & 0x0000ffff]

		set wa [string range $wa 2 end]
		incr nb -2

		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw >>   8)]] )) & \
			0x0000ffff ]
		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw & 0xff)]] )) & \
			0x0000ffff ]
	}

	return $chs
}

proc no_abort { msg } {

	variable B

	if { $B(ABR) != "" } {
		$B(ABR) $msg
		exit 1
	}

	catch { puts stderr $msg }
	exit 1
}

proc no_diag { } {

	variable B

	if { [string index $B(BUF) 0] == $B(ZER) } {
		# binary
		set ln [string range $B(BUF) 3 5]
		binary scan $ln cuSu lv code
		set ln "\[[format %02x $lv] -> [format %04x $code]\]"
	} else {
		# ASCII
		set ln "[string trim $B(BUF)]"
	}

	if { $B(DGO) != "" } {
		$B(DGO) $ln
	} else {
		puts "DIAG: $ln"
		flush stdout
	}
}

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::NOSS::$fun"
}

proc no_emu_readable { fun } {
#
# Emulates auto read on readable UART
#
	variable B

	if [$fun] {
		# a void call, increase the timeout
		if { $B(ROT) < $B(RMX) } {
			incr B(ROT)
		}
	} else {
		set B(ROT) 0
	}

	set B(ONR) [after $B(ROT) "[lize no_emu_readable] $fun"]
}

proc no_write { msg } {
#
# Writes a packet to the UART
#
	variable B

	set ln [string length $msg]
	if { $ln > $B(MPL) } {
		# truncate the message to size, probably a bad idea
		set ln $B(MPL)
		set msg [string range $msg 0 [expr $ln - 1]]
	}

	if [expr $ln & 1] {
		# need a filler zero byte
		append msg $B(ZER)
		incr ln -1
	} else {
		incr ln -2
	}

	if [catch {
		puts -nonewline $B(SFD) \
			"$B(IPR)[binary format c $ln]$msg[binary format s\
				[no_chks $msg]]"
		flush $B(SFD)
	}] {
		noss_close "NOSS write error"
	}
}

proc no_timeout { } {

	variable B

	if { $B(TIM) != "" } {
		no_rawread 1
		set B(TIM) ""
	}
}

proc no_rawread { { tm 0 } } {
#
# Called whenever data is available on the UART; returns 1 (if void), 0 (if
# progress, i.e., some data was available)
#
	variable B
#
#  STA = 0  -> Waiting for preamble
#        1  -> Waiting for the length byte
#        2  -> Waiting for (CNT) bytes until the end of packet
#        3  -> Waiting for end of DIAG preamble
#        4  -> Waiting for EOL until end of DIAG
#        5  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""
	set void 1

	while 1 {

		if { $chunk == "" } {

			if [catch { read $B(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $tm {
					# reset
					set B(STA) 0
				} elseif { $B(STA) != 0 } {
					# something has started, set up timer,
					# if not running already
					if { $B(TIM) == "" } {
						set B(TIM) \
				            	    [after $B(PKT) \
							[lize no_timeout]]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $B(TIM) != "" } {
				catch { after cancel $B(TIM) }
				set B(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $B(STA) {

		0 {
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $B(IPR) } {
					# preamble found
					set B(STA) 1
					break
				}
				if { $c == $B(DPR) } {
					# diag preamble
					set B(STA) 3
					break
				}
			}
			if { $i == $bl } {
				# not found, keep waiting
				set chunk ""
				continue
			}
			# found, remove the parsed portion and keep going
			set chunk [string range $chunk [expr $i + 1] end]
		}

		1 {
			# expecting the length byte (note that the byte
			# does not cover the statid field, so its range is
			# up to MPL - 2)
			binary scan [string index $chunk 0] cu bl
			set chunk [string range $chunk 1 end]
			if { [expr $bl & 1] || $bl > [expr $B(MPL) - 2] } {
				# reset
				set B(STA) 0
				continue
			}
			# how many bytes to expect
			set B(CNT) [expr $bl + 4]
			set B(BUF) ""
			# found
			set B(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}

			# end of packet, reset
			set B(STA) 0

			if { $bl == $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				no_receive
				continue
			}

			# merged packets
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			no_receive
		}

		3 {
			# waiting for the end of a diag header
			set chunk [string trimleft $chunk $B(DPR)]
			if { $chunk != "" } {
				set B(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $B(ZER) } {
					# a binary diag, length == 7
					set B(CNT) 7
					set B(STA) 5
				} else {
					# ASCII -> wait for NL
					set B(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append B(BUF) $chunk
				set chunk ""
				continue
			}

			append B(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set B(STA) 0
			no_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}
			# reset
			set B(STA) 0
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			no_diag
		}

		default {
			set B(STA) 0
		}
		}
	}
}

proc no_receive { } {
#
# Handle a received packet
#
	variable B
	
	# dmp "RCV" $B(BUF)

	# validate CRC
	if [no_chks $B(BUF)] {
		return
	}

	# strip off the checksum
	set msg [string range $B(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it
		return
	}

	if { $B(DFN) != "" } {
		$B(DFN) $msg
	}
}

proc noss_init { ufd mpl { inp "" } { dia "" } { clo "" } { emu 0 } } {
#
# Initialize: 
#
#	ufd - UART descriptor
#	mpl - max packet length
#	inp - function to be called on user input
#	dia - function to be called to present a diag message
#	clo - function to call on UART close (can happen asynchronously)
#	emu - emulate 'readable'
#
	variable B

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(SFD) $ufd
	set B(MPL) $mpl

	set B(UCF) $clo

	set B(DFN) $inp
	set B(DGO) $dia

	fconfigure $B(SFD) -buffering full -translation binary

	if $emu {
		# the readable flag doesn't work for UART on some Cygwin
		# setups
		set B(ROT) 1
		set B(RMX) $emu
		no_emu_readable [lize no_rawread]
	} else {
		# do it the easy way
		fileevent $B(SFD) readable [lize no_rawread]
	}
}

proc noss_stop { } {
#
# Stop the protocol
#
	variable B

	if { $B(TIM) != "" } {
		# kill the callback
		catch { after cancel $B(TIM) }
		set B(TIM) ""
	}

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""
}

proc noss_close { { err "" } } {
#
# Close the UART (externally or internally, which can happen asynchronously)
#
	variable B

	# trc "NOSS CLOSE"

	if { [info exist B(ONR)] && $B(ONR) != "" } {
		# we have been emulating 'readable', kill the callback
		catch { after cancel $B(ONR) }
		unset B(ONR)
	}

	if { $B(UCF) != "" } {
		# any extra function to call?
		set bucf $B(UCF)
		# prevents recursion loops
		set B(UCF) ""
		$bucf $err
	}

	catch { close $B(SFD) }

	set B(SFD) ""

	# stop the protocol
	noss_stop

	set B(DFN) ""
	set B(DGO) ""
}

proc noss_send { buf } {
#
# This is the user-level output function
#
	variable B

	if { $B(SFD) == "" } {
		# ignore if disconnected, this shouldn't happen
		return
	}

	# dmp "SND" $buf

	no_write $buf
}

namespace export noss_*

}

namespace import ::NOSS::*

###############################################################################
# End of NOSS #################################################################
###############################################################################

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
			256000 }

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
# Parses a time string than can be parsed by clock scan
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
				error "you cannot have more than one blob"
			}
			set blof 1
		} else {
			if $blof {
				error "a blob can only occur as the last\
					attribute"
			}
		}

		# size
		set tis [lindex $OSST($tp) 0]

		while { $tis > 1 && [expr $len % $tis] } {
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

			if { [catch { oss_valint $arg 1200 115200 } v] ||
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

	set res "#include \"sysio.h\"\n\n"
	append res "#define\tOSS_PRAXIS_ID\t\t$PM(PXI)\n"
	append res "#define\tOSS_UART_RATE\t\t$PM(USP)\n"
	append res "#define\tOSS_PACKET_LENGTH\t$PM(MPL)\n"

	append res {
// =================================================================
// Generated automatically, do not edit (unless you really want to)!
// =================================================================

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

proc sy_mkterm { } {

	global ST WI

	sy_updtitle

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

	button .stat.fs.hb -command sy_genheader -text Hdr
	pack .stat.fs.hb -side right

	set WI(SVA) [checkbutton .stat.fs.sa -state normal -variable ST(SFB)]
	pack .stat.fs.sa -side right
	label .stat.fs.sl -text " All:"
	pack .stat.fs.sl -side right

	set WI(SFS) [button .stat.fs.sf -command "sy_savefile"]
	pack $WI(SFS) -side right

	sy_setsblab

	.t configure -state disabled
	bind . <Destroy> "sy_exit"
	bind .t <ButtonRelease-1> "tk_textCopy .t"
	bind .stat.u <ButtonRelease-1> "tk_textCopy .stat.u"
	bind .stat.u <ButtonRelease-2> "tk_textPaste .stat.u"
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

		sy_abort "(usage) illegal argument $arg"
	}
}
				
proc sy_init { } {

	global PM WI USPEC ST argv

	catch { close stdin}

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

	sy_term_enable 1

	while 1 {

		tkwait variable WI(REX)
	}
}

sy_init
