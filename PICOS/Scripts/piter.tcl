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
###############################################################################

proc sy_usage { } {

	global argv0

	puts stderr "Usage: $argv0 args, where args can be:"
	puts stderr ""
	puts stderr "       -p port/dev       UART dev or COM number, required"
	puts stderr "       -s speed          UART speed, default is 9600"
	puts stderr "       -m n|x|s|e|f|p|d  mode, default is d"
	puts stderr "       -l pktlen         max pkt len, default is 82"
	puts stderr "       -b \[file\]         binary mode (optional macro\
		file)"
	puts stderr "       -f file           preserve the output in file"
	puts stderr "       -F file           preserve the input as well"
	puts stderr "       -S                scan for present UART devs"
	puts stderr "       -P file           preprocessor plugin file"
	puts stderr "       -C file           alternative configuration file"
	puts stderr "       -T string         window title string (GUI version)"
	puts stderr "       -V                print the version number and exit"
	puts stderr "       --                terminates args,\
		plugin args follow"
	puts stderr ""
	puts stderr "Note that pktlen should be the same as the length used"
	puts stderr "by the praxis in the respective argument of phys_uart."
	puts stderr ""
	puts stderr "The port argument can be nn:host:port for a direct VUEE"
	puts stderr "connection. Abbreviations are possible, e.g., 0: or"
	puts stderr "even : (meaning 0:localhost:4443)."
	puts stderr ""
	puts stderr "When called without arguments (or with -C as the only"
	puts stderr "argument), the program will operate in a GUI mode."

	exit 99
}

###############################################################################

set ST(WSH) [info tclversion]

if { $ST(WSH) < 8.5 } {
	puts stderr "$argv0 requires Tcl/Tk 8.5 or newer!"
	exit 99
}

# Working directory
set PM(PWD) [pwd]

# home directory
if { [info exists env(HOME)] && $env(HOME) != "" } {
	set PM(HOM) $env(HOME)
} else {
	set PM(HOM) $PM(PWD)
}

# the default rc file
set PM(SAP) [file join $PM(PWD) ".piterrc"]
if ![file exists $PM(SAP)] {
	# if the file exists in current directory, it has precedence
	set PM(SAP) [file join $PM(HOM) ".piterrc"]
}

#
# Prescan arguments:
#
#	1. separate plugin arguments
#	2. extract special arguments (alternative rc file, window title)
#	3. decide between GUI/command line call
#
set f ""

set u [lsearch -exact $argv "--"]
if { $u < 0 } {
	# no plugin args
	set PM(PLA) ""
} else {
	set PM(PLA) [lrange $argv [expr $u + 1] end]
	set argv [lrange $argv 0 [expr $u - 1]]
}

set u [lsearch -exact $argv "-C"]
if { $u >= 0 } {
	set f [lindex $argv [expr $u + 1]]
	if { $f == "" } {
		sy_usage
	}
	set PM(SAP) [file normalize $f]
	set argv [lreplace $argv $u [expr $u + 1]]
}

set PM(TTL) ""
set u [lsearch -exact $argv "-T"]
if { $u >= 0 } {
	set f [lindex $argv [expr $u + 1]]
	if { $f == "" } {
		sy_usage
	}
	set PM(TTL) $f
	set argv [lreplace $argv $u [expr $u + 1]]
}

unset u f

###############################################################################

if [llength $argv] {
	# arguments present, assume command-line call
	set ST(WSH) 0
} else {
	# wish call
	if [catch { package require Tk $ST(WSH) } ] {
		puts stderr "Cannot find Tk version $ST(WSH) matching the\
			Tcl version"
		exit 99
	}
	set ST(WSH) 1
	# this one won't be needed for sure
	catch { close stdin }
}

###############################################################################

package require vuart
package require uartpoll
package require unames

###############################################################################
# Shared initialization #######################################################
###############################################################################

# UART file descriptor
set ST(SFD) ""

# UART connect status string
set ST(UCS) "disconnected"

# for tracing and debugging
set DB(SFD)	""
set DB(LEV)	0

# character zero (aka NULL)
set CH(ZER)	[format %c [expr 0x00]]

# diag preamble (for packet modes) = ASCII DLE
set CH(DPR)	[format %c [expr 0x10]]

# defaut port/timeout for direct VUEE connections
set PM(VPO)	4443
set PM(VCT)	4000
# default port for TCP connection
set PM(TPO)	9022

# maximum message length (for packet modes), set to default, resettable
set PM(MPL)	82

# default plugin and macro file names
set PM(DPF)	[list "piter_plug.tcl" "shared_plug.tcl"]
set PM(DMF)	"piter_mac.txt"

# default bps
set PM(DSP)	9600

# log file
set ST(SFS)	""

# do not log input
set ST(SFB)	0

# output busy flag (current message pending) + output event variable
set ST(OBS)	0

# current outgoing message
set ST(OUT)	""

# queue of outgoing messages
set ST(OQU)	""

# send callback
set ST(SCB)	""

# low-level reception timer
set ST(TIM)  ""

# reception automaton initial state
set ST(STA) 0

# reception automaton remaining byte count
set ST(CNT) 0

# input buffer
set ST(BUF) ""

# output queue bypass flag == not allowed; values: 0 - NA, 1 - tmp off,
# 2 - tmp on, 3 - permanent
set ST(BYP) 0

# app-level input function
set ST(DFN)	"sy_nofun"

# app-level output function
set ST(OFN)	"sy_nofun"

# plug-level direct output function
set ST(LFN)	"sy_nofun"

# echo flag
set ST(ECO)	0

# previous command entered by the user
set ST(PCM)	""

# packet timeout (msec), once reception has started
set IV(PKT)	80

# short retransmit interval
set IV(RTS)	250

# long retransmit interval
set IV(RTL)	1000

###############################################################################
# ISO 3309 CRC ################################################################
###############################################################################

set CRCTAB {
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

###############################################################################

if $ST(WSH) {

###############################################################################
# Wish-only ###################################################################
###############################################################################

# Maximum number of lines to be stored off screen (being scrollable)
set PM(TLC) 	1024

# File label width (chars)
set PM(FLS)	24

# parameters window (not null if displayed)
set WI(PMW) 	""

# last log directory
set WI(LLD) 	$PM(PWD)

# current input line
set WI(CIL)	""

# recursive exit avoidance flag
set WI(REX)	0

###############################################################################

proc sy_alert { msg } {

	tk_dialog .alert "Attention!" "${msg}!" "" 0 "OK"
}

proc sy_exit { } {

	global WI

	if $WI(REX) { return }

	set WI(REX) 1

	if { $WI(PMW) != "" } {
		# parameters dialog present, destroy it first
		catch { destroy $WI(PMW) }
		set WI(PMW) ""
	}

	exit 0
}

proc pt_abort { msg } {

	tk_dialog .abert "Abort!" "Fatal error: ${msg}!" "" 0 "OK"
	sy_exit
}

proc sy_ftrunc { n fn } {
#
# Truncate a file name to be displayed in a label
#
	if { $fn == "" } {
		return "---"
	}

	set ln [string length $fn]
	if { $ln > $n } {
		set fn "...[string range $fn end-[expr $n - 3] end]"
	}
	return $fn
}

proc sy_getsparams { } {
#
# Retrieve saved parameters
#
	global PM

	if [catch { open $PM(SAP) "r" } fd] {
		# no such file
		return ""
	}

	if [catch { read $fd } pars] {
		catch { close $fd }
		return ""
	}

	# this is supposed to look like a list
	return [string trim $pars]
}

proc sy_setsparams { pars } {
#
# Save current parameters
#
	global PM

	if [catch { open $PM(SAP) "w" } fd] {
		# cannot open
		sy_alert "Cannot open $PM(SAP), $fd"
		return
	}

	if [catch { puts $fd $pars } err] {
		sy_alert "Cannot write to $PM(SAP), $err"
	}
	catch { close $fd }
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
	set WI(CIL) $tx
	$ST(TIF)
}

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

proc sy_clearsc { w } {

	if [catch { $w configure -state normal }] {
		return
	}

	$w delete 1.0 end
	$w configure -state disabled
}

proc sy_updtitle { } {

	global ST PM

	if { $PM(TTL) != "" } {
		set hd " \[$PM(TTL)\]"
	} else {
		set hd ""
	}

	if { $ST(UCS) != "" } {
		append hd " <$ST(UCS)>"
	}

	wm title . "Piter (ZZ000000A)$hd"
}

proc sy_cut_copy_paste { w x y } {
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

	tk_popup $m $x $y
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

	text .stat.u -height 1 -font {-family courier -size 10} -state normal \
		-width 10
	pack .stat.u -side left -expand yes -fill x

	bind .stat.u <Return> "sy_terminput"

	frame .stat.fs -borderwidth 0
	pack .stat.fs -side right -expand no

	button .stat.fs.rb -command sy_reconnect -text "Connect"
	pack .stat.fs.rb -side right

	checkbutton .stat.fs.sa -state normal -variable ST(SFB)
	pack .stat.fs.sa -side right
	label .stat.fs.sl -text " All:"
	pack .stat.fs.sl -side right

	set WI(SFS) [button .stat.fs.sf -command "sy_savefile"]
	pack $WI(SFS) -side right

	sy_setsblab

	button .stat.fs.cf -text "Clear" -command "sy_clearsc .t"
	pack .stat.fs.cf -side right

	.t configure -state disabled
	bind . <Destroy> "sy_exit"
	bind .t <ButtonRelease-3> "sy_cut_copy_paste %W %X %Y"
	bind .stat.u <ButtonRelease-3> "sy_cut_copy_paste %W %X %Y"
}

proc sy_setsblab { } {

	global ST WI

	if { $ST(SFS) != "" } {
		set lab "Stop"
	} else {
		set lab "Save"
	}

	$WI(SFS) configure -text $lab
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
			-initialfile "[sy_fndpfx]_piter_log.txt" \
			-initialdir $WI(LLD)]

		if { $fn == "" } {
			# cancelled
			return
		}

		if [catch { open $fn "a" } std] {
			if [tk_dialog .alert "Attention!" "File $fn cannot be\
				opened!" "" 0 "Try another file" "Cancel"] {
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

proc sy_setplugfile { } {

	global WI PM

	set fn [tk_getOpenFile \
		-defaultextension ".tcl" \
		-parent $WI(PMW) \
		-title "Plugin file name"]

	# can be used to revoke the previous/default setting
	set WI(PFN) $fn
	$WI(PFL) configure -text [sy_ftrunc $PM(FLS) $fn]
}

proc sy_setmacfile { } {

	global WI PM

	set fn [tk_getOpenFile \
		-defaultextension ".txt" \
		-parent $WI(PMW) \
		-title "Macro file name"]

	# can be used to revoke the previous/default setting
	set WI(MFN) $fn
	$WI(MFL) configure -text [sy_ftrunc $PM(FLS) $fn]
}

proc sy_clrplugfile { } {

	global WI PM

	set WI(PFN) ""
	$WI(PFL) configure -text [sy_ftrunc $PM(FLS) ""]
}

proc sy_clrmacfile { } {

	global WI PM

	set WI(MFN) ""
	$WI(MFL) configure -text [sy_ftrunc $PM(FLS) ""]
}

proc sy_valpars { } {
#
# Try to set the parameters as specified by the user, verify if they are OK
#
	global WI ST PM MODULE

	set err ""
	set erc 0

	###
	set mod [sy_valmode $WI(MOD)]
	if { $mod == "" } {
		# this is just a wild precaution, not really possible
		append err ", illegal mode"
		incr erc
	}

	###
	set prt [string trim $WI(DEO)]
	if { $prt == "" } {
		set prt [string trim $WI(DEV)]
	}

	if { $prt == "" } {
		append err ", no device specified"
		incr erc
	}

	###
	set spd [sy_valspeed $WI(SPE)]
	if !$spd {
		append err ", illegal bit rate"
		incr erc
	}

	###
	set mpl [sy_valrlen $WI(RLE)]
	if !$mpl {
		append err ", illegal record length"
		incr erc
	}

	set mos "$prt,$mod,$spd,$mpl"

	###
	set bin $WI(BIN)

	###
	set pfn $WI(PFN)
	if { $pfn != "" } {
		# there is a plugin file
		set est [sy_openpl $pfn]
		if { $est != "" } {
			append err ", $est"
			incr erc
			# recovery from plugin "evaluation" errors may be
			# illusory
		}
	}

	###
	if $bin {
		append mos ",B"
		# macro file is only relevant in binary mode
		set pfn $WI(MFN)
		if { $pfn != "" } {
			set est [sy_rdmac $pfn]
			if { $est != "" } {
				append err ", $est"
				incr erc
			}
		}
	}

	###
	if $bin {
		set ifu "sy_ugetb"
		set sfu "sy_sgetb"
		set lfu "sy_outlb"
		set ST(BIN) 1
	} else {
		set ifu "sy_ugett"
		set sfu "sy_sgett"
		set lfu "sy_outlt"
		set ST(BIN) 0
	}

	set PM(MPL) $mpl

	if !$erc {
		# initialize the UART
		set est [sy_ustart $prt $spd]
		if { $est != "" } {
			append err ", $est"
			incr erc
		}
	}

	if !$erc {
		# point of no return, I mean no failure
		set pf $MODULE($mod)
		# module initializer
		[lindex $pf 0]
		# UART input converter: text/binary
		set ST(DFN) $ifu
		# line write function: text/binary
		set ST(LFN) $lfu
		# module write function
		set ST(OFN) [lindex $pf 2]
		# module reset function
		set ST(RFN) [lindex $pf 3]
		# call plug_init before uartpoll_... so all input will be
		# intercepted
		if [catch { plug_init $mod $PM(PLA) } erq] {
			append err ", plugin init failed: $erq"
			incr erc
		} else {
			# module read function
			uartpoll_oninput $ST(SFD) [lindex $pf 1] $ST(SYS) \
				$ST(DEV)
			# Terminal input event
			set ST(TIF) $sfu
			set ST(UCS) $mos
			return 1
		}
	}

	set ST(SFD) ""

	# resume default plugin after failure; recovery may be illusory
	sy_setdefplug

	# revoke macro file after failure
	sy_clearmac

	# alert
	set emsg "Error"
	if { $erc > 1 } {
		append emsg "s"
	}
	sy_alert "${emsg}: [string range $err 2 end]"
	return 0
}

proc sy_setdefdev { } {
#
# Sets the default device selection based on the current list and the
# saved value in the rc file
#
	global WI

	if { $WI(DEV) == "" } {
		set WI(DEV) [lindex $WI(DEL) 0]
	} else {
		# check if present on the list
		if { [lsearch -exact $WI(DEL) $WI(DEV)] < 0 } {
			# absent
			set WI(DEV) [lindex $WI(DEL) 0]
		}
	}

	if { $WI(DEO) != "" } {
		set p $WI(DEO)
		set WI(DEO) ""
	} else {
		# overwritten with previous
		set p [lindex $WI(SPA) 0]
	}
	if { $p != "" } {
		# check if on the list
		if { [lsearch -exact $WI(DEL) $p] >= 0 } {
			set WI(DEV) $p
		} else {
			set WI(DEO) $p
		}
	}

	if { $WI(DEV) == "NO UARTS" } {
		set WI(DEV) ""
	}
}

proc sy_scandev { } {
#
# Produces a scanned device list
#
	global WI

	unames_scan
	set ol [unames_choice]
	set WI(DEL) [concat [lindex $ol 0] [lindex $ol 1]]

	if { $WI(DEL) == "" } {
		set WI(DEL) [list "NO UARTS"]
	}

	$WI(DEM) delete 0 end
	set ix 0

	foreach w $WI(DEL) {
		$WI(DEM) add command -label $w -command "sy_devselect $ix"
		incr ix
	}
	# update the default selection
	sy_setdefdev
}

proc sy_devselect { { ix -1 } } {

	global WI

	if { $ix >= 0 } {
		if [catch { $WI(DEM) entrycget $ix -label } w] {
			set w [lindex $WI(DEL) 0]
		}
	} else {
		set w $WI(DEV)
	}

	if { [lsearch -exact $WI(DEL) $w] < 0 } {
		set w [lindex $WI(DEL) 0]
	}

	if { $w == "NO UARTS" } {
		set WI(DEV) ""
	} else {
		set WI(DEV) $w
	}
	set WI(DEO) ""
}

proc sy_initialize { } {

	sy_clearmac
	sy_mkterm
	update
}

proc sy_reconnect { } {

	global ST MODULE PM WI PM

	# use default device list: real + virtual
	unames_scan
	set ol [unames_choice]
	set WI(DEL) [concat [lindex $ol 0] [lindex $ol 1]]

	if { $WI(DEL) == "" } {
		set WI(DEL) [list "NO UARTS"]
	}

	# read the parameters

	set w .params
	set WI(PMW) $w

	toplevel $w
	wm title $w "Parameters"

	# make it modal
	catch { grab $w }

	# check for saved parameters
	set WI(SPA) [sy_getsparams]

	#######################################################################

	labelframe $w.dev -padx 4 -pady 4 -text "Device"
	pack $w.dev -side top -expand y -fill x -anchor n

	set WI(DEV) ""
	set WI(DEO) ""
	sy_setdefdev

	button $w.dev.scb -text "Scan" -anchor w -command sy_scandev
	eval "set WI(DEM) \[tk_optionMenu $w.dev.dmn WI(DEV) $WI(DEL)\]"
	bind $w.dev.dmn <ButtonRelease-1> sy_devselect

	grid $w.dev.scb -column 0 -row 0 -sticky new
	grid $w.dev.dmn -column 1 -row 0 -sticky new

	label $w.dev.odl -text "Other:"
	grid $w.dev.odl -column 0 -row 1 -sticky new

	entry $w.dev.ode -width 16 -textvariable WI(DEO) -bg gray
	grid $w.dev.ode -column 1 -row 1 -sticky new

	grid columnconfigure $w.dev 0 -weight 0
	grid columnconfigure $w.dev 1 -weight 1

	#######################################################################

	labelframe $w.lab -padx 4 -pady 4 -text "Title"
	pack $w.lab -side top -expand y -fill x -anchor n

	entry $w.lab.lab -width 16 -textvariable PM(TTL) -bg gray
	pack $w.lab.lab -side top -expand y -fill x -anchor n

	#######################################################################

	labelframe $w.mod -padx 4 -pady 4 -text "Mode"
	pack $w.mod -side top -expand y -fill x -anchor n

	label $w.mod.sl -text "Speed: " -anchor w
	grid $w.mod.sl -column 0 -row 0 -sticky new
	label $w.mod.ml -text "Prot: " -anchor w
	grid $w.mod.ml -column 0 -row 1 -sticky new

	set itl \
	    { 1200 2400 4800 9600 14400 19200 28800 38400 76800 115200 256000 }
	set WI(SPE) $PM(DSP)
	set p [lindex $WI(SPA) 1]
	if { [lsearch -exact $itl $p] >= 0 } {
		set WI(SPE) $p
	}
	eval "tk_optionMenu $w.mod.ra WI(SPE) $itl"
	grid $w.mod.ra -column 1 -row 0 -sticky new

	frame $w.mod.db
	grid $w.mod.db -column 1 -row 1 -sticky new

	set itl { Direct Persistent XRS NPacket SDual E-scaped F-scaped }
	set WI(MOD) "Direct"
	set p [lindex $WI(SPA) 2]
	if { [lsearch -exact $itl $p] >= 0 } {
		set WI(MOD) $p
	}
	eval "tk_optionMenu $w.mod.db.mo WI(MOD) $itl"
	pack $w.mod.db.mo -side left -expand y -fill x

	set WI(BIN) 0
	set p [lindex $WI(SPA) 3]
	if { $p == 1 } {
		set WI(BIN) 1
	}
	checkbutton $w.mod.db.mb -state normal -variable WI(BIN)
	pack $w.mod.db.mb -side right -expand n
	label $w.mod.db.bl -text "  Bin: "
	pack $w.mod.db.bl -side right -expand n

	set WI(RLE) $PM(MPL)
	set p [lindex $WI(SPA) 4]
	if { ![catch { expr $p } p] && $p >= 16 && $p <= 255 } {
		set WI(RLE) $p
	}

	spinbox $w.mod.db.rl -width 4 -textvariable WI(RLE) -bg gray \
		-justify right -from 16 -to 255 -wrap 1
	pack $w.mod.db.rl -side right
	label $w.mod.db.re -text " RLen: "
	pack $w.mod.db.re -side right -expand n

	grid columnconfigure $w.mod 0 -weight 0
	grid columnconfigure $w.mod 1 -weight 1

	#######################################################################

	labelframe $w.add -padx 4 -pady 4 -text "Add-ons"
	pack $w.add -side top -expand y -fill x -anchor n

	# check for saved/default plugin file
	set p [lindex $WI(SPA) 5]
	if { $p != "" } {
		if ![catch { open $p "r" } fid] {
			catch { close $fid }
			set PM(DPF) [list $p]
		}
	}

	set WI(PFN) ""
	foreach f $PM(DPF) {
		if ![catch { open $f "r" } fid] {
			# doesn't open
			catch { close $fid }
			set WI(PFN) $f
			break
		}
	}

	set p [lindex $WI(SPA) 6]
	if { $p != "" } {
		if ![catch { open $p "r" } fid] {
			catch { close $fid }
			set PM(DMF) $p
		}
	}

	# check for deafult macro file present
	if [catch { open $PM(DMF) "r" } fid] {
		set WI(MFN) ""
	} else {
		catch { close $fid }
		set WI(MFN) $PM(DMF)
	}

	button $w.add.pl -text "Plugin" -anchor w -command sy_setplugfile
	grid $w.add.pl -column 0 -row 0 -sticky new
	button $w.add.ml -text "Macros" -anchor w -command sy_setmacfile
	grid $w.add.ml -column 0 -row 1 -sticky new

	set WI(PFL) $w.add.plf
	label $w.add.plf -text [sy_ftrunc $PM(FLS) $WI(PFN)] -anchor w \
		-width $PM(FLS)
	grid $w.add.plf -column 1 -row 0 -sticky new

	set WI(MFL) $w.add.maf
	label $w.add.maf -text [sy_ftrunc $PM(FLS) $WI(MFN)] -anchor w \
		-width $PM(FLS)
	grid $w.add.maf -column 1 -row 1 -sticky new

	button $w.add.pc -text "Clr" -command sy_clrplugfile
	grid $w.add.pc -column 2 -row 0 -sticky new

	button $w.add.mc -text "Clr" -command sy_clrmacfile
	grid $w.add.mc -column 2 -row 1 -sticky new

	grid columnconfigure $w.add 0 -weight 0
	grid columnconfigure $w.add 1 -weight 1
	grid columnconfigure $w.add 2 -weight 0

	#######################################################################

	frame $w.but
	pack $w.but -side top -expand y -fill x -anchor n
	button $w.but.qu -text "Cancel" -command "set WI(GOF) 0"
	pack $w.but.qu -side left -expand n
	button $w.but.do -text "Proceed" -command "set WI(GOF) 1"
	pack $w.but.do -side right -expand n
	button $w.but.sp -text "Save & Proceed" -command "set WI(GOF) 2"
	pack $w.but.sp -side right -expand n

	#######################################################################

	bind $w <Destroy> "set WI(GOF) 0"

	set WI(GOF) -1

	raise $w

	catch { plug_close }

	sy_uclose

	while 1 {

		tkwait variable WI(GOF)

		if { $WI(GOF) >= 0 } {
			if { $WI(GOF) == 0 } {
				break
			}
			if [sy_valpars] {
				# done if parameters are OK
				if { $WI(GOF) > 1 } {
					# save parameters
					set dev $WI(DEO)
					if { $dev == "" } {
						set dev $WI(DEV)
					}
					if { $WI(PFN) != "" } {
						set WI(PFN) \
						    [file normalize $WI(PFN)]
					}
					if { $WI(MFN) != "" } {
						set WI(MFN) \
						    [file normalize $WI(MFN)]
					}
					sy_setsparams [list \
						$dev \
						$WI(SPE) \
						$WI(MOD) \
						$WI(BIN) \
						$WI(RLE) \
						$WI(PFN) \
						$WI(MFN) ]
				}
				break
			}
		}
	}

	catch { destroy $w }
	sy_updtitle
	set WI(PMW) ""
}

proc pt_trc { msg } {

	variable DB

	if { $DB(SFD) == "-" } {
		# to the terminal
		sy_dspline $msg
		return
	}

	if { $DB(SFD) == "" } {
		set str stdout
	} else {
		set str $DB(SFD)
	}

	catch { 
		puts $str $msg
		flush $str
	}
}

###############################################################################
###############################################################################

} else {

###############################################################################
# Tcl-only versions of functions ##############################################
###############################################################################

proc sy_onreadable { fun } {

	global ST

	fileevent $ST(SFD) readable $fun
}

proc sy_exit { } {

	exit 0
}

proc pt_abort { ln } {

	puts stderr $ln
	exit 99
}

proc sy_dspline { ln } {

	puts $ln
}

proc sy_initialize { } {
#
	global ST MODULE PM

	set prt ""
	set spd ""
	set mpl ""
	set bin ""
	set sfi ""
	set mod ""
	set sca ""
	set plu ""

	# parse command line arguments

	while 1 {

		set par [sy_argget]

		if { $par == "" } {
			# done
			break
		}

		if ![regexp -nocase "^-(\[a-z\])$" $par jnk par] {
			# must be a flag
			sy_usage
		}

		if { $par == "V" } {
			puts "ZZ000000A"
			exit 0
		}

		sy_argsft

		# the value that follows
		set val [sy_argget]

		if ![regexp -nocase "^-\[a-z\]$" $val] {
			# if it looks like a flag, assume the value is empty
			sy_argsft
		} else {
			set val ""
		}

		# the mode flag
		if { $par == "m" && $mod == "" } {
			set mod [sy_valmode $val]
			if { $mod != "" } {
				continue
			}
		}

		# the UART device
		if { $par == "p" && $prt == "" && $val != "" } {
			set prt $val
			continue
		}

		# speed
		if { $par == "s" && $spd == "" } {
			set spd [sy_valspeed $val]
			if !$spd {
				sy_usage
			}
			continue
		}

		# length
		if { $par == "l" && $mpl == "" } {
			set mpl [sy_valrlen $val]
			if !$mpl {
				sy_usage
			}
			continue
		}

		# binary
		if { $par == "b" && $bin == "" } {
			if { $val == "" } {
				# this really means: expect a macro file
				set bin "+"
			} else {
				# this IS the macro file
				set bin $val
			}
			continue
		}

		if { ($par == "f" || $par == "F") && $sfi == "" } {
			if { $val == "" } {
				# default log file
				set sfi "+"
			} else {
				set sfi $val
			}
			if { $par == "F" } {
				# log all
				set ST(SFB) 1
			}
			continue
		}

		if { $par == "P" && $plu == "" } {
			if { $val == "" } {
				# expect a plugin file
				set plu "+"
			} else {
				set plu $val
			}
			continue
		}

		if { $par == "S" && $val == "" } {
			# just scan for COM ports
			set sca "y"
			continue
		}
			
		sy_usage
	}

	if { $sca != "" } {

		if { $mod != "" || $prt != "" || $spd != "" ||
	     	     $mpl != "" || $bin != "" || $sfi != "" ||
		     $plu != "" 				} {
			sy_usage
		}

		unames_scan
		set ol [unames_choice]
		puts [join [concat [lindex $ol 0] [lindex $ol 1]] "\n"]
		exit 0
	}

	# cleanups and defaults
	if { $mod == "" } {
		set mod "D"
	}

	if { $prt == "" } {
		sy_usage
	}

	if { $spd == "" } {
		set spd $PM(DSP)
	}

	if { $mpl == "" } {
		set mpl $PM(MPL)
	}

	sy_clearmac
	if { $bin != "" } {
		set ifu "sy_ugetb"
		set sfu "sy_sgetb"
		set lfu "sy_outlb"
		set ST(BIN) 1
		if { $bin == "+" } {
			set pf [sy_rdmac $PM(DMF)]
			if { $pf != "" } {
				if { [string first "cannot open" $pf] != 0 } {
					pt_abort $pf
				}
			}
		} else {
			set pf [sy_rdmac $bin]
			if { $pf != "" } {
				pt_abort $pf
			}
		}
	} else {
		set ifu "sy_ugett"
		set sfu "sy_sgett"
		set lfu "sy_outlt"
		set ST(BIN) 0
	}

	if { $sfi != "" } {
		if ![info exists ST(SFB)] {
			set ST(SFB) 0
		}
		set ST(SFS) [sy_openlf $sfi]
		sy_startlog
	} else {
		# for the record
		set ST(SFS) ""
		set ST(SFB) 0
	}

	if { $plu != "" } {
		if { $plu == "+" } {
			foreach f $PM(DPF) {
				set pf [sy_openpl $f]
				if { $pf != "" } {
					if { [string first "cannot open" $pf]
					    != 0 } {
						pt_abort $pf
					}
				} else {
					break
				}
			}
		} else {
			# single file, must open
			set pf [sy_openpl $plu]
			if { $pf != "" } {
				pt_abort $pf
			}
		}
	}

	set PM(MPL) $mpl

	# initialize the UART
	set pf [sy_ustart $prt $spd]
	if { $pf != "" } {
		pt_abort $pf
	}

	set pf $MODULE($mod)
	# module initializer
	[lindex $pf 0]
	# UART input converter: text/binary
	set ST(DFN) $ifu
	# line write function: text/binary
	set ST(LFN) $lfu
	# module write function
	set ST(OFN) [lindex $pf 2]
	# module reset function
	set ST(RFN) [lindex $pf 3]
	# module read function
	uartpoll_oninput $ST(SFD) [lindex $pf 1]

	fconfigure stdin -buffering line -blocking 0 -eofchar ""
	fileevent stdin readable $sfu

	# plugin initializer
	if [catch { plug_init $mod $PM(PLA) } err] {
		pt_abort "Plugin init failed: $err"
	}
}

proc pt_trc { msg } {
#
# Write a debug line
#
	variable DB

	if { $DB(SFD) == "" || $DB(SFD) == "-" } {
		set str stdout
	} else {
		set str $DB(SFD)
	}

	catch { 
		puts $str $msg
		flush $str
	}
}

proc sy_argget { } {

	global argv

	return [string trim [lindex $argv 0]]
}

proc sy_argsft { } {

	global argv

	set argv [lrange $argv 1 end]
}

proc sy_openlf { sfn } {
#
# Try to be intelligent about overwriting an existing file
#
	if { $sfn == "+" } {
		set sfn "[sy_fndpfx]_piter_log.txt"
	}

	set af "w"
	set fo 0
	while 1 {
		if { $af == "w" && $fo == 0 && [file exists $sfn] } {
			catch { close $std }
			set sfn [sy_fnprompt $sfn \
				"seems to already exist" af fo]
			continue
		}
		if [catch { open $sfn $af } std] {
			set sfn [sy_fnprompt $sfn "cannot be opened" af fo]
			continue
		}
		break
	}

	fconfigure $std -buffering line -translation lf

	return $std
}

proc sy_fnprompt { sfn why afg foq } {

	upvar $afg af
	upvar $foq fo

	puts "File $sfn $why!"

	while 1 {
		puts "Type one of these:"
		set opt "QN"
		puts " Q            - to abort"
		if { [string index $why 0] == "s" } {
			puts " O            - to overwrite the file"
			append opt "O"
		}
		puts " N filename   - to specify a different file name"
		if { $af == "w" } {
			puts " A            - to try to append to the file"
			append opt "A"
		}
		if ![regexp "^\[0-9\]\[0-9\]" $sfn] {
			puts \
		   " D            - to tag the current name with date/time"
			append opt "D"
		}
		while 1 {
			if [catch { gets stdin line } stat] {
				sy_exit
			}
			if { $stat < 0 } {
				sy_exit
			}
			set line [string trim $line]
			if { $line != "" } {
				break
			}
		}
		set d [string toupper [string index $line 0]]
		if { [string first $d $opt] < 0 } {
			continue
		}
		if { $d == "Q" } {
			sy_exit
		}
		if { $d == "O" } {
			set fo 1
			return $sfn
		}
		if { $d == "N" } {
			set fn [string trimleft [string range $line 1 end]]
			if { $fn == "" } {
				puts "Filename missing after N"
				continue
			}
			set af "w"
			return $fn
		}
		if { $d == "A" } {
			set af "a"
			return $sfn
		}
		# append date/time
		set af "w"
		return "[sy_fndpfx]_$sfn"
	}
}

###############################################################################
###############################################################################

}

###############################################################################
# Shared functions ############################################################
###############################################################################

proc sy_cygfix { } {
#
# Issue a dummy reference to a file path to trigger a possible DOS-path
# warning, after which things should continue without any further warnings.
# This first reference must be caught as otherwise it would abort the script.
#
	global PM

	catch { exec ls $PM(PWD) }
}

proc sy_fndpfx { } {
#
# Produces the date/time prefix for a file name
#
	return [clock format [clock seconds] -format %y%m%d_%H%M%S]
}

proc pt_ishex { c } {
	return [regexp -nocase "\[0-9a-f\]" $c]
}

proc pt_isoct { c } {
	return [regexp -nocase "\[0-7\]" $c]
}

proc pt_stparse { line } {
#
# Parse a UNIX string into a list of hex codes; update the source to point to
# the first character behind the string
#
	upvar $line ln

	set nc [string index $ln 0]
	if { $nc != "\"" } {
		error "illegal string delimiter: $nc"
	}

	# the original - for errors
	set or "\""

	set ln [string range $ln 1 end]

	set vals ""

	while 1 {
		set nc [string index $ln 0]
		if { $nc == "" } {
			error "unterminated string: $or"
		}
		append or $nc
		set ln [string range $ln 1 end]
		if { $nc == "\"" } {
			# done (this version assumes that the sentinel must be
			# explicit, append 0x00 if this is a problem)
			return $vals
		}
		if { $nc == "\\" } {
			# escapes
			set c [string index $ln 0]
			if { $c == "" } {
				# delimiter error, will be diagnosed at next
				# turn
				continue
			}
			if { $c == "x" } {
				# get hex digits
				set ln [string range $ln 1 end]
				while 1 {
					set d [string index $ln 0]
					if ![pt_ishex $d] {
						break
					}
					append c $d
					set ln [string range $ln 1 end]
				}
				if [catch { expr 0$c % 256 } val] {
					error "illegal hex escape in string: $c"
				}
				lappend vals $val
				continue
			}
			if [pt_isoct $c] {
				if { $c != 0 } {
					set c "0$c"
				}
				# get octal digits
				set ln [string range $ln 1 end]
				while 1 {
					set d [string index $ln 0]
					if ![pt_isoct $d] {
						break
					}
					append c $d
					set ln [string range $ln 1 end]
				}
				if [catch { expr $c % 256 } val] {
					error \
					    "illegal octal escape in string: $c"
				}
				lappend vals $val
				continue
			}
			set ln [string range $ln 1 end]
			set nc $c
			continue
		}
		scan $nc %c val
		lappend vals [expr $val % 256]
	}
}

proc sy_valmode { m } {

	set m [string toupper [string index $m 0]]
	if { [string first $m "NXSPDEF"] < 0 } {
		return ""
	}
	return $m
}

proc sy_valspeed { s } {

	if { [catch { expr $s } s] || $s < 1200 || $s > 256000 } {
		return 0
	}
	return $s
}

proc sy_valrlen { v } {

	# FIXME: the true max depends on the mode
	if { [catch { expr $v } v] || $v < 16 || $v > 255 } {
		return 0
	}
	return $v
}

proc sy_nofun { args } {

	pt_abort "undefined \"implementation\" function called"
}

proc pt_chks { wa } {

	global CRCTAB

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

proc sy_tcp_conn { ho po } {
#
# Connect to a TCP socket
#
	if { [info tclversion] > 8.5 && $ho == "localhost" } {
		# do not use -async, it doesn't seem to work in 8.6
		# for localhost
		set err [catch { socket $ho $po } sfd]
	} else {
		set err [catch { socket -async $ho $po } sfd]
	}

	if $err {
		error "Connection failed: $sfd"
	}

	if [catch { fconfigure $sfd -blocking 0 -buffering none \
    	    -translation binary -encoding binary } erc] {
		cleanup
		error "Connection failed: $erc"
	}

	return $sfd
}

proc sy_ustart { udev speed } {
#
# open the UART
#
	global ST CH PM MODULE


	set nn ""

	set tcp ""
	set vnn ""

	if { [regexp -nocase \
	        {^(vuee[[:blank:]]*)?([[:digit:]]*):} $udev mat jnk vnn] ||
	     [regexp -nocase {^(tcp):} $udev mat tcp] } {
		if { $tcp == "" } {
			# vuee node number or null
			if { $vnn == "" } {
				set no 0
			} else {
				if [catch { expr $vnn } no] {
					return "illegal VUEE node number: $vnn"
				}
			}
			set po $PM(VPO)
		} else {
			set po $PM(TPO)
		}
		# VUEE or TCP connection
		set ho "localhost"
		set udev [string range $udev [string length $mat] end]
		if [regexp "^(\[^:\]+)" $udev ho] {
			set udev [string range $udev [string length $ho] end]
		}
		if { $udev != "" } {
			if { [string index $udev 0] != ":" } {
				return "illegal port syntax: $udev"
			}
			set udev [string range $udev 1 end]
			if { $udev != "" && [catch { expr $udev } po] } {
				return "illegal port syntax: $udev"
			}
		}

		if { $tcp != "" } {
			# striaghtforward TCP connect
			if [catch { sy_tcp_conn $ho $po } ST(SFD)] {
				return "failed to connect to ${ho}:$po,\
					$ST(SFD)"
			}
			return ""
		}

		# VUEE

		set ST(CAN) 0

		set ST(VCB) [after $PM(VCT) "incr ::ST(CAN)"]

		if { [catch { vuart_conn $ho $po $no ::ST(CAN) } ST(SFD)] ||
		    $ST(CAN) } {
			catch { after cancel $ST(VCB) }
			return "failed to connect to VUEE model, $ST(SFD)"
		}

		catch { after cancel $ST(VCB) }

		return ""
	}

	# regular device

	set devlist [unames_fnlist $udev]

	set fail 1

	# Note: apparently, the driver of FTDI USB is broken on Linux, so I
	# have to resort to these tricks (the discovery of which took me two
	# @##$$% days)

	if { $ST(SYS) == "L" } {
		set accs { RDWR NOCTTY NONBLOCK }
	} else {
		set accs "r+"
	}

	foreach udev $devlist {
		if ![catch { open [unames_unesc $udev] $accs } ST(SFD)] {
			set fail 0
			break
		}
	}

	if $fail {
		return "cannot open UART: $devlist"
	}

	# the generic part of UART configuration
	if [catch { fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
	    -blocking 0 -eofchar "" -ttycontrol { RTS 0 } } err] {
		catch { close $ST(SFD) }
		return "cannot configure UART: $err"
	}

	return ""
}

proc sy_uclose { } {
#
# Closes the current connection
#
	global ST

	if { $ST(SFD) == "" } {
		return
	}

	uartpoll_stop $ST(SFD)
	catch { close $ST(SFD) }
	set ST(SFD) ""

	foreach cb { "SCB" "TIM" } {
		if { $ST($cb) != "" } {
			catch { after cancel $ST($cb) }
			set ST($cb) ""
		}
	}

	$ST(RFN)

	sy_setdefplug
	sy_clearmac

	set ST(UCS) "disconnected"
}

proc pt_tout { ln } {
#
# Output a line to the terminal and optionally save it in the log file
#
	global ST

	sy_dspline $ln

	if { $ST(SFS) != "" } {
		catch { puts $ST(SFS) $ln }
	}
}

proc pt_touf { ln } {
#
# Output a line to the terminal and optionally save it in the log file, but the
# latter only if we are saving input; used to save special lines, like error
# messages
#
	global ST

	sy_dspline $ln

	if { $ST(SFS) != "" && $ST(SFB) } {
		catch { puts $ST(SFS) $ln }
	}
}

proc pt_diag { } {

	global ST CH

	if { [string index $ST(BUF) 0] == $CH(ZER) } {
		# binary
		set ln [string range $ST(BUF) 3 5]
		binary scan $ln cuSu lv code
		#set lv [expr $lv & 0xff]
		#set code [expr $code & 0xffff]
		pt_tout "DIAG: \[[format %02x $lv] -> [format %04x $code]\]"
	} else {
		# ASCII
		pt_tout "DIAG: [string trim $ST(BUF)]"
	}
	if !$ST(WSH) {
		flush stdout
	}
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

###############################################################################
# Tracing and debugging #######################################################
###############################################################################

proc sy_dump { hdr buf } {

	set code ""
	set nb [string length $buf]
	binary scan $buf cu$nb code
	set ol ""
	foreach co $code {
		append ol [format " %02x" $co]
	}
	pt_trc "$hdr:$ol"
}
	
proc pt_settrc { lev { file "" } } {
#
# Set tracing parameters
#
	global DB

	if { $DB(SFD) != "" && $DB(SFD) != "-" } {
		# close any previous file
		catch { close $DB(SFD) }
	}

	set DB(SFD) ""

	# new level
	set DB(LEV) $lev

	if { $lev == 0 } {
		return ""
	}

	# open
	if { $file != "" } {
		if { $file != "-" } {
			if [catch { open $file "w" } sfd] {
				set DB(LEV) 0
				return "Failed to open debug file, $sfd!"
			}
			set DB(SFD) $sfd
		} else {
			# to the screen
			set DB(SFD) "-"
		}
	}
	return ""
}

###############################################################################
###############################################################################

proc sy_outloop { } {

	global ST

	if !$ST(OBS) {
		# not busy
		if { $ST(OQU) != "" } {
			# something in queue
			$ST(OFN) [lindex $ST(OQU) 0] 0
			set ST(OQU) [lrange $ST(OQU) 1 end]
		}
	}
}

proc sy_outln { line } {

	global ST

	if { $ST(BYP) > 1 } {
		# queue bypass
		$ST(OFN) $line 1
		return
	}

	lappend ST(OQU) $line
	# to trigger the event
	set ST(OBS) $ST(OBS)
}

proc pt_outln { line } {
#
# This is encapsulated a bit to be more convenient for the plugin; piter
# never calls this function
#
	global ST

	$ST(LFN) $line
}

proc pt_bypass { on } {
#
# Turns the output queue bypass flag on or off, returns the (formal) setting
#
	global ST

	if { $on && $ST(BYP) == 1 } {
		set ST(BYP) 2
	} elseif { !$on && $ST(BYP) == 2 } {
		set ST(BYP) 1
	}
	# otherwise cannot be reset

	return [expr $ST(BYP) > 1]
}

###############################################################################
###############################################################################

proc sy_inline { } {
#
# Preprocess a line input from the keyboard
#
	global ST

	if $ST(WSH) {
		# the wish variant; this doesn't warrant splitting the
		# function into two versions
		global WI
		set line $WI(CIL)
		set WI(CIL) ""
	} else {
		if [catch { gets stdin line } stat] {
			# ignore errors (can they happen at all?)
			return -code return
		}
		if { $stat < 0 } {
			# end of file
			sy_exit
		}
	}

	# we used to trim here, but perhaps we shouldn't; let's try to avoid
	# any interpretation, because the user may actually want to send a
	# bunch of blanks

	if { $ST(SFS) != "" && $ST(SFB) } {
		# writing input to a file
		catch { puts $ST(SFS) "<- $line" }
	}

	# the trimmed version for special command check
	set ltr [string trim $line]

	if { $ltr == "!!" } {
		# previous command
		if { $ST(PCM) == "" } {
			pt_touf "No previous command!"
			return -code return
		}
		set line $ST(PCM)
	} elseif { [string index $ltr 0] == "!" } {
		# not for the board
		sy_icmd [string trimleft [string range $ltr 1 end]]
		return -code return
	} else {
		# last command
		set ST(PCM) $line
	}

	return $line
}

proc sy_icpm { par } {

	if [regexp "^\[0-9\]+" $par num] {
		if [catch { expr $num } num] {
			error "illegal number"
		}
		if $num {
			return 1
		}
		return 0
	}

	set par [string tolower $par]

	if { [string first "on" $par] == 0 } {
		return 1
	}

	if { [string first "off" $par] == 0 } {
		return 0
	}

	error "illegal value: 0, 1, off, on expected"
}

proc sy_icmd { cmd } {
#
# Handle internal commands
#
	global ST

	set par ""
	if ![regexp "^(\[^ \t\]+)(.*)" $cmd jnk cmd par] {
		# impossible
		pt_touf "Empty command!"
		return
	}
	set par [string trim $par]

	if { $cmd == "e" || $cmd == "echo" } {

		if { $par == "" } {
			# toggle
			if $ST(ECO) {
				set par 0
			} else {
				set par 1
			}
		} else {
			if [catch { sy_icpm $par } par] {
				pt_touf "${par}!"
				return
			}
		}

		if $par {
			pt_touf "Echo is now on"
			set ST(ECO) 1
		} else {
			pt_touf "Echo is now off"
			set ST(ECO) 0
		}
		return
	}

	if { $cmd == "t" || $cmd == "trace" } {

		set fnm ""
		if ![regexp "^(\[0-9\])(.*)" $par jnk lev fnm] {
			pt_touf "Illegal parameters, must be level\
				\[filename\]!"
			return
		}
		set fnm [string trim $fnm]

		set err [pt_settrc $lev $fnm]
		if { $err == "" } {
			set err "OK"
		}
		pt_touf $err
		return
	}

	if { $cmd == "o" || $cmd == "oqueue" } {
		set cn [llength $ST(OQU)]
		if $ST(OBS) {
			incr cn
		}
		if { $cn == 0 } {
			pt_touf "Queue is empty"
		} else {
			if { $cn > 1 } {
				set sf "s"
			} else {
				set sf ""
			}
			pt_touf "Queue size: $cn item$sf"
		}
		return
	}

	if { $cmd == "b" || $cmd == "bypass" } {

		if { $par == "" } {
			if { $ST(BYP) == 0 } {
				pt_touf "Bypass is disabled for this module"
				return
			}
			if { $ST(BYP) == 1 } {
				pt_touf "Bypass is off"
				return
			}
			if { $ST(BYP) == 2 } {
				pt_touf "Bypass is on"
				return
			}
			pt_touf "This module writes directly (no queueing)"
			return
		}

		if [catch { sy_icpm $par } par] {
			pt_touf "${par}!"
			return
		}

		if { $ST(BYP) == 0 && $par } {
			pt_touf "Bypass cannot be switched on for this module"
			return
		}

		if { $ST(BYP) == 3 && !$par } {
			pt_touf "Bypass cannot be switched off for this module"
			return
		}

		if { $par && $ST(BYP) == 1 } {
			set ST(BYP) 2
		} elseif { !$par && $ST(BYP) == 2 } {
			set ST(BYP) 1
		}

		if $par {
			pt_touf "Bypass is on"
		} else {
			pt_touf "Bypass is off"
		}

		return
	}

	if { $cmd == "r" || $cmd == "reset" } {
		$ST(RFN)
		pt_touf "Protocol reset"
		if [catch { plug_reset } err] {
			pt_touf "Plugin reset failed: $err"
		}
		return
	}

	if { $cmd == "q" || $cmd == "x" } {
		sy_exit
	}

	pt_touf "Unknown internal command $cmd!"
}
###############################################################################
# ASCII terminal input, device output #########################################
###############################################################################

proc sy_sgett { } {
#
# STDIN becomes readable (or a line is entered in the wish case) [ASCII mode]
#
	global ST PM

	set line [sy_inline]

	if [catch { plug_inppp_t line } ln] {
		pt_touf "INPPP Error: $ln"
		return
	}

	if !$ln {
		# plugin tells us to ignore the line
		return
	}

	sy_outlt $line
}

proc sy_outlt { line } {
#
# Write out a text line
#
	global ST PM

	if $ST(ECO) {
		# echo the line
		sy_dspline $line
	}

	set ln [string length $line]

	if { $ln > $PM(UPL) } {
		pt_touf "Error: line longer than max payload of $PM(UPL) chars"
		return
	}

	sy_outln $line
}

###############################################################################
# Binary terminal input, device output ########################################
###############################################################################

proc sy_sgetb { } {
#
# The binary counterpart of sgett
#
	global ST

	set line [string trim [sy_inline]]

	if [catch { sy_macsub $line } mline] {
		# macro substitutions
		pt_touf "Error: $mline"
		return
	}

	if { $ST(SFS) != "" && $ST(SFB) } {
		# logging all, so log the substituted variant of the function,
		# but only if different from the original
		if { [string trim $line] != [string trim $mline] } {
			# the macro expansion is different
			catch { puts $ST(SFS) "<M $mline" }
		}
	}

	set line $mline

	# plugin preprocessor
	if [catch { plug_inppp_b line } oul] {
		pt_touf "INPPP Error: $oul"
		return
	}

	if !$oul {
		# ignore the line
		return
	}

	sy_outlb $line
}

proc sy_outlb { line } {
#
# Write out a binary line; this function converts a text form of the binary
# line into the actual binary form, also does the echo and logging
#
	global ST PM

	set out ""
	set eco ""
	set oul 0

	while 1 {

		set line [string trimleft $line]

		if { $line == "" } {
			break
		}

		if { [string index $line 0] == "\"" } {
			# looks like a string, parse it into a sequence of
			# bytes
			if [catch { pt_stparse line } val] {
				pt_touf "Error: $val"
				return
			}
		} else {
			set val ""
			regexp "^(\[^ \t\]+)(.*)" $line mat val line
			if [catch { eval "expr $val" } val] {
				pt_touf "Error: illegal expression: $val ...\
					[string range $line 0 5] ..."
				return
			}

			set val [list [expr $val & 0xff]]
		}

		foreach v $val {
			if $ST(ECO) {
				append eco [format "%02x " $v]
			}
			append out [binary format c $v]
			incr oul
		}
	}

	if $ST(ECO) {
		sy_dspline "Sending: < $eco>"
		if { $ST(SFS) != "" && $ST(SFB) } {
			catch { puts $STD(SFS) "<S < $eco>" }
		}
	}

	if { $oul > $PM(UPL) } {
		pt_touf "Error: block longer than max payload of $PM(UPL) bytes"
		return
	}

	sy_outln $out
}

###############################################################################
# Input from device ###########################################################
###############################################################################

proc sy_ugett { msg } {
#
# ASCII mode: just show the line
#
	set msg [string trimright $msg "\r\n"]
	set msh $msg
	if [catch { plug_outpp_t msg } v] {
		pt_touf "OUTPP Error: $v, received line was: <$msh>"
	} elseif { $v != 0 } {
		pt_tout $msg
	}
}

proc sy_ugetb { msg } {
#
# Binary mode: convert to numbers
#
	set len [string length $msg]

	set enc ""
	# encode the bytes into hex numbers
	for { set i 0 } { $i < $len } { incr i } {
		binary scan [string index $msg $i] cu byte
		append enc [format "%02x " $byte]
	}

	set enc [string trimright $enc]

	set ens $enc
	if [catch { plug_outpp_b enc } v] {
		pt_touf "OUTPP Error: $v, received line was: <$ens>"
		return
	}

	if { $v == 0 } {
		# ignore line
		return
	}

	if { $v > 1 } {
		# null preprocessing, use default decorations
		pt_tout "Received: $len < $enc >"
	} else {
		# preprocessed, at face value
		pt_tout $enc
	}
}

###############################################################################
# Macro processing functions ##################################################
###############################################################################

proc sy_msuball { line pars vals } {
#
# Scans the string substituting all occurrences of parameters with their values
#
	foreach par $pars {

		regsub -all $par $line [lindex $vals 0] line
		set vals [lrange $vals 1 end]

	}

	return $line
}

proc sy_macsub { line } {
#
# Macro substitution (no recursion: one scan per macro in definition order)
#
	global BMAC

	set ml $BMAC(+)

	foreach m $ml {

		set ol ""
		set ll [string length $m]
		set ma [lindex $BMAC($m) 0]
		set pl [lindex $BMAC($m) 1]
		set rf [lindex $BMAC($m) 2]

		while 1 {

			set x [string first $m $line]
			if { $x < 0 } {
				break
			}
			if { $rf && $x != 0 } {
				break
			}

			append ol [string range $line 0 [expr $x - 1]]
			set line [string range $line [expr $x + $ll] end]

			if { $pl != "" } {
				# parameters are expected
				if ![regexp "^\[ \t\]*\\((.*)" $line jn rest] {
					# ignore, assume not a macro call
					continue
				}
				set al ""
				set line $rest
				while 1 {
					set c [string index $line 0]
					if { $c == ")" } {
						# all done
						break
					}
					if { $c == "" } {
						error "illegal macro reference"
					}
					if { $c == "," } {
						# empty parameter
						lappend al ""
						set line \
						    [string range $line 1 end]
						continue
					}
					# starts a new parameter
					regexp "^\[^),\]+" $line c
					lappend al $c
					set line [string range $line \
						[string length $c] end]
					if { [string index $line 0] == "," } {
						set line \
						    [string range $line 1 end]
					}
				}
				# skip the closing parenthesis
				set line [string range $line 1 end]
				append ol [sy_msuball $ma $pl $al]
			} else {
				append ol $ma
			}
		}
		append ol $line
		set line $ol
	}

	return $line
}

proc sy_clearmac { } {
#
# Undefine all macros
#
	global BMAC

	array unset BMAC
	set BMAC(+) ""
}

proc sy_rdmac { bin } {
#
# Read the macro file for binary mode
#
	global BMAC

	set bfd ""
	sy_clearmac

	if [catch { open $bin "r" } bfd] {
		return "cannot open the macro file $bin: $bfd"
	}

	if [catch { read $bfd } lines] {
		catch { close $bfd }
		return "cannot read the macro file: $lines"
	}

	catch { close $bfd }

	set lines [split $lines "\n"]

	# parse the macros
	set ML ""

	foreach ln $lines {

		set ln [string trim $ln]
		if { $ln == "" || [string index $ln 0] == "#" } {
			# ignore
			continue
		}

		set rf ""
		if ![regexp -nocase "^(\\^)?(\[_a-z\]\[a-z0-9_\]*)(.*)" \
		    $ln j rf nm t] {
			sy_clearmac
			return "illegal macro name in this line: $ln"
		}

		if [info exists BMAC($nm)] {
			sy_clearmac
			return "duplicate macro name $nm in this line: $ln"
		}

		set t [string trimleft $t]
		set d [string index $t 0]
		set p ""

		# recursive expansion flag
		if { $rf == "" } {
			set rf 0
		} else {
			set rf 1
		}

		if { $d == "(" } {
			# this is a parameterized macro
			set t [string range $t 1 end]
			# parse the parameters
			while 1 {
				set t [string trimleft $t]
				set d [string index $t 0]
				if { $d == "," } {
					set t [string range $t 1 end]
					continue
				}
				if { $d == ")" } {
					set t [string trimleft \
						[string range $t 1 end]]
					break
				}
				if ![regexp -nocase \
				    "^(\[a-z\]\[a-z0-9_\]*)(.*)" $t j pm t] {
					sy_clearmac
					return "illegal macro parameter in\
						this line: $ln"
				}
				set t [string trimleft $t]
				if [info exists parms($pm)] {
					sy_clearmac
					return "duplicate macro parameter in\
						this line: $ln"
				}
				set parms($pm) ""
				lappend p $pm
			}
			array unset parms
		}

		# now the macro body
		if { [string index $t 0] != "=" } {
			sy_clearmac
			return "illegal macro definition in this line: $ln"
		}

		set t [string trimleft [string range $t 1 end]]
		regexp "^\"(.*)\"$" $t j t
		if { $t == "" } {
			sy_clearmac
			return "empty macro in this line: $ln"
		}

		set BMAC($nm) [list $t $p $rf]
		lappend ML $nm
	}
	# list of names
	set BMAC(+) $ML
	return ""
}

###############################################################################
# Plugin handling functions ###################################################
###############################################################################

###############################################################################
# For now, there are just these user plugin functions:
# - input preprocessor, text version; the value 2 means that the line has not
#   been changed
# - input preprocessor, binary version
# - same for output
# - init and reset
###############################################################################

proc sy_setdefplug { } {
#
# Activates the default plugin
#
	set sc    {proc plug_inppp_t \{ inp \} \{ return 2 \}\n}
	append sc {proc plug_inppp_b \{ inp \} \{ return 2 \}\n}
	append sc {proc plug_outpp_t \{ inp \} \{ return 2 \}\n}
	append sc {proc plug_outpp_b \{ inp \} \{ return 2 \}\n}
	append sc {proc plug_init  \{ mod pla \} \{ \}\n}
	append sc {proc plug_reset \{ \} \{ \}\n}
	append sc {proc plug_close \{ \} \{ \}\n}

	uplevel #0 eval $sc
}

sy_setdefplug

proc sy_openpl { fn } {
#
# Opens and runs a file with plugin function definitions
#
	if [catch { open $fn "r" } fd] {
		return "cannot open plugin file $fn: $fd"
	}

	if [catch { read $fd } sc] {
		catch { close $fd }
		return "cannot read plugin file $fn: $sc"
	}

	catch { close $fd }

	if [catch { uplevel #0 eval $sc } err] {
		return "cannot insert the plugin, evaluation error: $err"
	}
}

###############################################################################
# Module functions ############################################################
###############################################################################

proc sy_uwpkt { msg } {
#
# Send a packet to UART (shared by modes N, X, S and P)
#
	global DB ST

	if { $DB(LEV) > 1 } {
		sy_dump "W" $msg
	}

	catch {
		puts -nonewline $ST(SFD) $msg
		flush $ST(SFD)
	}
}

###############################################################################
# Module: mode == X ###########################################################
###############################################################################

proc mo_frame_x { m } {
#
# Frame the message (X mode); when we get it here, the message contains the
# four XRS header bytes in front (they formally count as N-payload), but no
# checksum. This is exactly what is covered by MPL.
#
	upvar $m msg
	global CH PM DB

	set ln [string length $msg]

	if { $ln > $PM(MPL) } {
		# the message has been prechecked for length bounds
		pt_abort "Assert mo_frame_x: length > max"
	}

	if { $ln < 4 } {
		pt_abort "Assert mo_frame_x: length < min"
	} elseif [expr $ln & 1] {
		# odd length, append NULL
		append msg $CH(ZER)
		# do not count the Network ID (used by the first two bytes of
		# XRS header) in the length field
		incr ln -1
	} else {
		incr ln -2
	}

	set msg "$CH(IPR)[binary format c $ln]$msg[binary format s\
		[pt_chks $msg]]"
}

proc mo_fnwrite_x { msg } {
#
# Frame and write in one step
#
	mo_frame_x msg
	sy_uwpkt $msg
}

proc mo_receive_x { } {
#
# Handle received packet (internal for the module)
#
	global ST DB CH IV

	if { $DB(LEV) > 1 } {
		# dump the packet
		sy_dump "R" $ST(BUF)
	}

	# validate CRC
	if [pt_chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			pt_trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# strip off the checksum
	set msg [string range $ST(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 4 } {
		# ignore it, we need at least the XRS header
		return
	}

	# extract the header
	binary scan $msg cucucucu cu ex ma le

	if { $ma != $CH(MAG) } {
		# wrong magic
		if { $DB(LEV) > 2 } {
			pt_trc "receive: wrong magic $ma, packet ignored"
		}
		return
	}

	if $ST(OBS) {
		# we have an outgoing message
		if { $ex != $CH(CUR) } {
			# expected != our current, we are done with this
			# message
			set ST(OUT) ""
			set CH(CUR) $ex
			set ST(OBS) 0
		}
	} else {
		# no outgoing message, set current to expected
		set CH(CUR) $ex
	}

	if { $len == 4 } {
		# empty message, treat as pure ACK and ignore
		return
	}

	if { $cu != $CH(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(RTS) mo_send_x]
		return
	}

	if { $le > [expr $len - 4] } {
		# consistency check
		if { $DB(LEV) > 2 } {
			pt_trc "receive: inconsistent XRS header, $le <-> $len"
		}
		return
	}

	# receive it
	$ST(DFN) [string range $msg 4 [expr 3 + $le]]

	# update expected
	set CH(EXP) [expr ( $CH(EXP) + 1 ) & 0x00ff]

	# force an ACK
	mo_send_x
}

proc mo_send_x { } {
#
# Callback for sending packets out
#
	global ST IV CH
	
	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	if !$ST(OBS) {
		# just in case
		set ST(OUT) ""
	}
	# if len is nonzero, an outgoing message is pending; note: its length
	# has been checked already (and is <= MAXPL)
	set len [string length $ST(OUT)]

	mo_fnwrite_x\
	    "[binary format cccc $CH(CUR) $CH(EXP) $CH(MAG) $len]$ST(OUT)"

	set ST(SCB) [after $IV(RTL) mo_send_x]
}

proc mo_timeout_x { } {

	global ST

	if { $ST(TIM) != "" } {
		mo_rawread_x 1
		set ST(TIM) ""
	}
}

proc mo_rawread_x { { to 0 } } {
#
# Called whenever data is available on the UART (mode X)
#
	global ST PM
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

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $to {
					global DB
					if { $DB(LEV) > 2 } {
					  pt_trc "rawread: packet timeout\
						$ST(STA), $ST(CNT)"
					}
					# reset
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					if { $ST(TIM) == "" } {
						global IV
						set ST(TIM) [after $IV(PKT) \
							mo_timeout_x]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(IPR) } {
					# preamble found
					set ST(STA) 1
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
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
			if { [expr $bl & 1] || $bl > [expr $PM(MPL) - 2] } {
				global DB
				if { $DB(LEV) > 2 } {
					pt_trc "rawread: illegal packet length\
						$bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# how many bytes to expect
			set ST(CNT) [expr $bl + 4]
			set ST(BUF) ""
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				mo_receive_x
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			mo_receive_x
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_write_x { msg byp } {
#
# Send out a message; assumes $ST(OBS) is 0
#
	global ST PM DB

	if $ST(OBS) {
		pt_abort "Assert mo_write_x: output busy"
	}

	# we still need the four XRS header bytes in front
	set lm [expr $PM(MPL) - 4]

	if { [string length $msg] > $lm } {
		# this verification is probably redundant (we do check the
		# user payload size before submitting it), but it shouldn't
		# hurt
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated to $lm bytes"
		}
		set msg [string range $msg 0 [expr $lm - 1]]
	}

	set ST(OUT) $msg
	set ST(OBS) 1
	mo_send_x
}

proc mo_init_x { } {
#
# Initialize
#
	global CH ST PM

	set ST(STA) 0
	set ST(BYP) 0
	set CH(EXP) 0
	set CH(CUR) 0
	set CH(MAG) [expr 0xAB]
	set CH(IPR) [format %c [expr 0x55]]
	set ST(MOD) "X"

	# User packet length: in the X(RS) mode,
	# the specified length (MPL) covers the "Station ID", which is
	# used for two AB control bytes (and it doesn't cover CRC - it never
	# does). So the user length (true payload) is MPL - 4 (two extra bytes
	# are needed for "magic" and true payload length.

	set PM(UPL) [expr $PM(MPL) - 4]

	fconfigure $ST(SFD) -buffering full -translation binary

	# start the write callback
	mo_send_x
}

proc mo_reset_x { } {

	global ST

	set ST(OQU) ""
	set ST(OUT) ""
	set ST(OBS) 0

	set ST(EXP) 0
	set ST(CUR) 0
	set ST(BUF) ""
}

set MODULE(X) [list mo_init_x mo_rawread_x mo_write_x mo_reset_x]

###############################################################################
# Module: mode == P ###########################################################
###############################################################################

proc mo_frame_p { m cu ex } {
#
# Frame the message (P mode)
#
	upvar $m msg
	global CH PM

	set ln [string length $msg]
	if { $ln > $PM(MPL) } {
		# this is an assertion: the message length has been already
		# verified
		pt_abort "Assert mo_frame_p: length > max"
	}

	if [expr $ln & 1] {
		# odd length, append a dummy byte, but leave the				# length as it was
		append msg $CH(ZER)
	}

	# checksum coverage
	set msg "[binary format cc [expr ($ex << 1) | $cu] $ln]$msg"
	set msg "$msg[binary format s [pt_chks $msg]]"
}

proc mo_fnwrite_p { msg cu ex } {
#
# Frame and write in one step
#
	mo_frame_p msg $cu $ex
	sy_uwpkt $msg
}

proc mo_receive_p { } {
#
# Handle received packet
#
	global ST DB CH IV

	if { $DB(LEV) > 1 } {
		# dump the packet
		sy_dump "R" $ST(BUF)
	}

	# validate CRC
	if [pt_chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			pt_trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# extract the preamble
	binary scan [lindex $ST(BUF) 0] cu pre
	# extract the payload and AB flags
	set msg [string range $ST(BUF) 2 end-$ST(RPL)]
	set cu [expr $pre & 1]
	set ex [expr ($pre & 2) >> 1]

	if $ST(OBS) {
		# we have an outgoing message
		if { $ex != $CH(CUR) } {
			# expected != our current, so we are done with the
			# present message
			set ST(OUT) ""
			set ST(OBS) 0
			set CH(CUR) $ex
		}
	} else {
		# no outgoing message, set current to expected
		set CH(CUR) $ex
	}

	if { $msg == "" } {
		# a pure ACK
		return
	}

	if { $cu != $CH(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(RTS) mo_send_p]
		return
	}

	# receive it
	$ST(DFN) $msg

	# new "expected"
	if $CH(EXP) {
		set CH(EXP) 0
	} else {
		set CH(EXP) 1
	}

	# force an ACK
	mo_send_p
}

proc mo_send_p { } {
#
# Callback for sending messages out
#
	global ST IV CH

	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	if $ST(OBS) {
		# there is an outgoing message
		mo_fnwrite_p $ST(OUT) $CH(CUR) $CH(EXP)
	} else {
		# just the ack
		mo_fnwrite_p "" 1 $CH(EXP)
	}

	set ST(SCB) [after $IV(RTL) mo_send_p]
}

proc mo_timeout_p { } {

	global ST

	if { $ST(TIM) != "" } {
		mo_rawread_p 1
		set ST(TIM) ""
	}
}

proc mo_rawread_p { { to 0 } } {
#
# Called whenever data is available on the UART (mode P)
#
	global ST PM
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

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $to {
					global DB
					if { $DB(LEV) > 2 } {
					  pt_trc "rawread: packet timeout\
						$ST(STA), $ST(CNT)"
					}
					# reset
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					if { $ST(TIM) == "" } {
						global IV
						set ST(TIM) [after $IV(PKT) \
							mo_timeout_p]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				# there are four preambles to choose from
				if { [string first $c $CH(IPR)] >= 0 } {
					# preamble found
					set ST(STA) 1
					# needed for checksum
					set ST(BUF) $c
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
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
			# expecting the length byte
			set c [string index $chunk 0]
			binary scan $c cu bl
			set chunk [string range $chunk 1 end]
			if { $bl > $PM(MPL) } {
				global DB
				if { $DB(LEV) > 2 } {
					pt_trc "rawread: illegal packet length\
						$bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# include in message for checksum evaluation
			append ST(BUF) $c
			# adjust on odd length, but do not change formal length
			if [expr $bl & 1] {
				set ST(CNT) [expr $bl + 3]
				# flag == odd length
				set ST(RPL) 3
			} else {
				set ST(CNT) [expr $bl + 2]
				# flag == even length
				set ST(RPL) 2
			}
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				mo_receive_p
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			mo_receive_p
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_write_p { msg byp } {
#
# Send out a message; assumes $ST(OBS) == 0
#
	global ST PM DB

	set ll [string length $msg]

	if { $ll > $PM(MPL) } {
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated from $ll to\					$PM(MPL) bytes"
		}
		set msg [string range $msg 0 [expr $PM(MPL) - 1]]
	}

	set ST(OUT) $msg
	set ST(OBS) 1
	mo_send_p
	return 1
}

proc mo_init_p { } {
#
# Initialize
#
	global CH ST PM

	set ST(BYP) 0
	set ST(STA) 0

	set CH(EXP) 0
	set CH(CUR) 0

	set CH(IPR) ""
	append CH(IPR) [format %c [expr 0x00]]
	append CH(IPR) [format %c [expr 0x01]]
	append CH(IPR) [format %c [expr 0x02]]
	append CH(IPR) [format %c [expr 0x03]]

	set ST(MOD) "P"

	# In this case, the user length is the same as MPL, as the PHY length
	# specification doesn't cover the header
	set PM(UPL) $PM(MPL)

	fconfigure $ST(SFD) -buffering full -translation binary

	# start the write callback
	mo_send_p
}

proc mo_reset_p { } {

	global ST CH

	set ST(OQU) ""
	set ST(OUT) ""
	set ST(OBS) 0

	set CH(EXP) 0
	set CH(CUR) 0
	set ST(BUF) ""
}

set MODULE(P) [list mo_init_p mo_rawread_p mo_write_p mo_reset_p]

###############################################################################
# Module: mode == N ###########################################################
###############################################################################

proc mo_init_n { } {
#
# Initialize
#
	global CH ST PM

	# permanent queue bypass for output
	set ST(BYP) 3
	set ST(STA) 0
	set CH(IPR) [format %c [expr 0x55]]
	set ST(MOD) "N"
	set PM(UPL) $PM(MPL)
	fconfigure $ST(SFD) -buffering full -translation binary
}

proc mo_reset_n { } {
#
# Void
#

}

proc mo_timeout_n { } {

	global ST

	if { $ST(TIM) != "" } {
		mo_rawread_n 1
		set ST(TIM) ""
	}
}

proc mo_rawread_n { { to 0 } } {
#
# Called whenever data is available on the UART (mode N); note: this is
# identical to mo_rawread_x (and mo_rawread_s) except for a different input
# function called upon packet completion. Well, we could optimize a bit,
# but it doesn't really matter, because only one of the multiple code
# instances will be ever actually compiled, so why bother
#
	global ST PM
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

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $to {
					global DB
					if { $DB(LEV) > 2 } {
					  pt_trc "rawread: packet timeout\
						$ST(STA), $ST(CNT)"
					}
					# reset
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					if { $ST(TIM) == "" } {
						global IV
						set ST(TIM) [after $IV(PKT) \
							mo_timeout_n]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(IPR) } {
					# preamble found
					set ST(STA) 1
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
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
			if { [expr $bl & 1] || $bl > [expr $PM(MPL) - 2] } {
				global DB
				if { $DB(LEV) > 2 } {
					pt_trc "rawread: illegal packet length\
						$bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# how many bytes to expect
			set ST(CNT) [expr $bl + 4]
			set ST(BUF) ""
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				mo_receive_n
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			mo_receive_n
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_receive_n { } {
#
# Handle received packet
#
	global ST DB CH

	if { $DB(LEV) > 1 } {
		# dump the packet
		sy_dump "R" $ST(BUF)
	}

	# validate CRC
	if [pt_chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			pt_trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# strip off the checksum
	set msg [string range $ST(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it if too short
		return
	}

	# receive it
	if !$ST(BIN) {
		# strip off any sentinel
		set sl [string first $CH(ZER) $msg]
		if { $sl >= 0 } {
			set msg [string range $msg 0 [expr $sl - 1]]
		}
	}

	$ST(DFN) $msg
}

proc mo_write_n { msg byp } {
#
# Send out a message; assumes $ST(OBS) is 0
#
	global ST PM DB CH

	# we still need the two header bytes in front
	set ln [string length $msg]
	if { $ln > $PM(MPL) } {
		# this verification is probably redundant (we do check the
		# user payload size before submitting it), but it shouldn't
		# hurt
		set ln $PM(MPL)
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated to $ln bytes"
		}
		set msg [string range $msg 0 [expr $ln - 1]]
	} elseif { $ln == 0 } {
		set ln 2
		set msg "$CH(ZER)$CH(ZER)"
	} elseif [expr $ln & 1] {
		incr ln
		append msg $CH(ZER)
	}
 
	set msg "$CH(IPR)[binary format c [expr $ln - 2]]$msg[binary format s\
		[pt_chks $msg]]"

	sy_uwpkt $msg
}

set MODULE(N) [list mo_init_n mo_rawread_n mo_write_n mo_reset_n]

###############################################################################
# Module: mode == S ###########################################################
###############################################################################

proc mo_init_s { } {
#
# Initialize
#
	global CH ST PM

	# optional bypass
	set ST(BYP) 1
	set ST(STA) 0
	set CH(EXP) 0
	set CH(CUR) 0
	# ACK flag
	set CH(ACK) 4
	# Direct
	set CH(MAD) [expr 0xAC]
	# AB
	set CH(MAP) [expr 0xAD]
	set CH(IPR) [format %c [expr 0x55]]
	set ST(MOD) "S"

	# User packet length: in the S mode, the Network ID field is used by
	# the protocol

	set PM(UPL) [expr $PM(MPL) - 2]

	fconfigure $ST(SFD) -buffering full -translation binary

	# start the write callback for the persistent stream
	mo_send_s
}

proc mo_send_s { } {
#
# Callback for sending packets out
#
	global ST IV CH
	
	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	if !$ST(OBS) {
		# just in case
		set ST(OUT) ""
	}
	# if len is nonzero, an outgoing message is pending; note: its length
	# has been checked already (and is <= MAXPL)
	set len [string length $ST(OUT)]

	set flg [expr $CH(CUR) | ( $CH(EXP) << 1 )]

	if { $len == 0 } {
		# ACK only
		set flg [expr $flg | $CH(ACK)]
	}

	mo_fnwrite_s "[binary format cc $CH(MAP) $flg]$ST(OUT)"

	set ST(SCB) [after $IV(RTL) mo_send_s]
}

proc mo_fnwrite_s { msg } {
#
	global PM CH

	set ln [string length $msg]
	if { $ln > $PM(MPL) } {
		pt_abort "Assert mo_frame_s: length > max"
	}

	if { $ln < 2 } {
		pt_abort "Assert mo_frame_s: length < min"
	}

	if [expr $ln & 1] {
		append msg $CH(ZER)
		incr ln -1
	} else {
		incr ln -2
	}

	sy_uwpkt "$CH(IPR)[binary format c $ln]$msg[binary format s\
		[pt_chks $msg]]"
}

proc mo_timeout_s { } {

	global ST

	if { $ST(TIM) != "" } {
		mo_rawread_s 1
		set ST(TIM) ""
	}
}

proc mo_rawread_s { { to 0 } } {
#
# Called whenever data is available on the UART (mode S)
#
	global ST PM
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

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $to {
					global DB
					if { $DB(LEV) > 2 } {
					  pt_trc "rawread: packet timeout\
						$ST(STA), $ST(CNT)"
					}
					# reset
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					if { $ST(TIM) == "" } {
						global IV
						set ST(TIM) [after $IV(PKT) \
							mo_timeout_s]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(IPR) } {
					# preamble found
					set ST(STA) 1
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
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
			if { [expr $bl & 1] || $bl > [expr $PM(MPL) - 2] } {
				global DB
				if { $DB(LEV) > 2 } {
					pt_trc "rawread: illegal packet length\
						$bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# how many bytes to expect
			set ST(CNT) [expr $bl + 4]
			set ST(BUF) ""
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				mo_receive_s
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			mo_receive_s
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_receive_s { } {
#
# Handle received packet
#
	global ST DB CH IV

	if { $DB(LEV) > 1 } {
		# dump the packet
		sy_dump "R" $ST(BUF)
	}

	# validate CRC
	if [pt_chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			pt_trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# strip off the checksum
	set msg [string range $ST(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it
		return
	}

	# extract the header
	binary scan $msg cucu pr fg

	# trim the message
	set msg [string range $msg 2 end]
	if !$ST(BIN) {
		# character mode, strip the sentinel, if any
		set cu [string first $CH(ZER) $msg]
		if { $cu >= 0 } {
			set msg [string range $msg 0 [expr $cu - 1]]
		}
	}

	if { $pr == $CH(MAD) } {
		# direct, receive right away, nothing else to do
		$ST(DFN) $msg
		return
	}

	if { $pr != $CH(MAP) } {
		# wrong magic
		if { $DB(LEV) > 2 } {
			pt_trc "receive: wrong proto $pr, packet ignored"
		}
		return
	}

	set cu [expr $fg & 1]
	set ex [expr ($fg & 2) >> 1]
	set ac [expr $fg & 4]

	if $ST(OBS) {
		# we have an outgoing message
		if { $ex != $CH(CUR) } {
			# expected != our current, we are done with this
			# packet
			set ST(OUT) ""
			set CH(CUR) $ex
			set ST(OBS) 0
		}
	} else {
		# no outgoing message, set current to expected
		set CH(CUR) $ex
	}

	if $ac {
		# treat as pure ACK and ignore
		return
	}

	if { $cu != $CH(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(RTS) mo_send_s]
		return
	}

	# receive it
	$ST(DFN) $msg

	# update expected
	set CH(EXP) [expr 1 - $CH(EXP)]

	# force an ACK
	mo_send_s
}

proc mo_write_s { msg byp } {
#
# Send out a message
#
	global ST PM DB CH

	set lm [expr $PM(MPL) - 2]
	if { [string length $msg] > $lm } {
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated to $lm bytes"
		}
		set msg [string range $msg 0 [expr $lm - 1]]
	}

	if $byp {
		# immediate output, direct protocol
		mo_fnwrite_s "[binary format cc $CH(MAD) 0]$msg"
		return
	}

	if $ST(OBS) {
		pt_abort "Assert mo_write_s: output busy"
	}

	set ST(OUT) $msg
	set ST(OBS) 1
	mo_send_s
}

proc mo_reset_s { } {

	global ST

	set ST(OQU) ""
	set ST(OUT) ""
	set ST(OBS) 0

	set ST(EXP) 0
	set ST(CUR) 0

	set ST(BYP) 1
	set ST(BUF) ""
}

set MODULE(S) [list mo_init_s mo_rawread_s mo_write_s mo_reset_s]

###############################################################################
# Module: mode == E ###########################################################
###############################################################################

proc mo_rawread_e { } {
#
# Called whenever data is available on the UART (mode X)
#
	global ST CH
#
#  STA = 0  -> Waiting for STX
#        1  -> Waiting for payload bytes + ETX
#        2  -> Waiting for end of DIAG preamble
#        3  -> Waiting for EOL until end of DIAG
#        4  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""
	set void 1

	while 1 {

		if { $chunk == "" } {

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				return $void
			}

			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 2
					break
				}
				if { $c == $CH(IPR) } {
					# STX
					set ST(STA) 1
					set ST(ESC) 0
					set ST(BUF) ""
					set ST(BFL) 0
					# initialize the parity byte
					set ST(PAR) $ST(PAI)
					break
				}
			}
			if { $i == $bl } {
				set chunk ""
				continue
			}

			# remove the parsed out portion of the input string
			incr i
			set chunk [string range $chunk $i end]
		}

		1 {
			# parse the chunk for escapes
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if !$ST(ESC) {
					if { $c == $CH(ETX) } {
						# that's it
						set ST(STA) 0
						set chunk [string range $chunk \
							[expr { $i + 1 }] end]
						if { $ST(BFL) > $ST(MPM) ||
						     $ST(PAR) } {
							# too long or parity
							break
						}
						# remove the parity byte
						$ST(DFN) [string range \
							$ST(BUF) 0 end-1]
						break
					}
					if { $c == $CH(IPR) } {
						# reset
						set ST(STA) 0
						set chunk [string range $chunk \
							$i end]
						break
					}
					if { $c == $CH(DLE) } {
						# escape
						set ST(ESC) 1
						continue
					}
				} else {
					set ST(ESC) 0
				}
				if { $ST(BFL) < $ST(MPM) } {
					append ST(BUF) $c
					binary scan $c cu v
					set ST(PAR) [expr { $ST(PAR) ^ $v }]
				}
				incr ST(BFL)
			}
			if $ST(STA) {
				set chunk ""
			}
		}

		2 {
			# waiting for the first non-DLE byte
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 4
				} else {
					# ASCII -> wait for NL
					set ST(STA) 3
				}
			}
		}

		3 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		4 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_write_e { msg byp } {

	global ST DB CH PM


	set out $CH(IPR)
	set par $ST(PAI)
	set lm [string length $msg]

	if { $lm > $PM(MPL) } {
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated to $PM(MPL)\
				bytes"
		}
		set lm $PM(MPL)
	}

	for { set i 0 } { $i < $lm } { incr i } {
		set c [string index $msg $i]
		if { $c == $CH(IPR) || $c == $CH(ETX) || $c == $CH(DLE) } {
			# need to escape
			append out $CH(DLE)
		}
		append out $c
		# parity
		binary scan $c cu v
		set par [expr { $par ^ $v }]
	}

	set par [binary format c $par]
	if { $par == $CH(IPR) || $par == $CH(ETX) || $par == $CH(DLE) } {
		set par "$CH(DLE)$par"
	}

	# complete
	append out "${par}$CH(ETX)"

	if { $DB(LEV) > 1 } {
		sy_dump "W" $out
	}

	catch {
		puts -nonewline $ST(SFD) $out
		flush $ST(SFD)
	}

	set ST(OBS) 0

	return 1
}

proc mo_init_e { } {

	global CH ST PM

	set ST(STA) 0
	set ST(BYP) 3
	set ST(MOD) "E"

	fconfigure $ST(SFD) -buffering none -translation binary

	set PM(UPL) $PM(MPL)

	set CH(IPR) [format %c [expr 0x02]]
	set CH(DLE) [format %c [expr 0x10]]
	set CH(ETX) [format %c [expr 0x03]]

	# initial parity
	set ST(PAI) [expr { 0x02 ^ 0x03 }]

	# max buffer length + 1 to accommodate parity on input
	set ST(MPM) [expr { $PM(MPL) + 1 }]
}

proc mo_reset_e { } {

# void

}

set MODULE(E) [list mo_init_e mo_rawread_e mo_write_e mo_reset_e]

###############################################################################
# Module: mode == F ###########################################################
###############################################################################

proc mo_rawread_f { } {
#
# Called whenever data is available on the UART (mode X)
#
	global ST CH
#
#  STA = 0  -> Waiting for STX
#        1  -> Waiting for payload bytes + ETX
#        2  -> Waiting for end of DIAG preamble
#        3  -> Waiting for EOL until end of DIAG
#        4  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""
	set void 1

	while 1 {

		if { $chunk == "" } {

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				return $void
			}

			set void 0
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 2
					break
				}
				if { $c == $CH(IPR) } {
					# STX
					set ST(STA) 1
					set ST(ESC) 0
					set ST(BUF) ""
					set ST(BFL) 0
					# initialize the parity byte
					set ST(PAR) $ST(PAI)
					break
				}
			}
			if { $i == $bl } {
				set chunk ""
				continue
			}

			# remove the parsed out portion of the input string
			incr i
			set chunk [string range $chunk $i end]
		}

		1 {
			# parse the chunk for escapes
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if !$ST(ESC) {
					if { $c == $CH(ETX) } {
						# that's it
						set ST(STA) 0
						set chunk [string range $chunk \
							[expr { $i + 1 }] end]
						if { $ST(BFL) > $ST(MPM) ||
						     $ST(PAR) } {
							# too long or parity
							break
						}
						# remove the parity byte
						$ST(DFN) [string range \
							$ST(BUF) 0 end-1]
						break
					}
					if { $c == $CH(IPR) } {
						# reset
						set ST(STA) 0
						set chunk [string range $chunk \
							$i end]
						break
					}
					if { $c == $CH(DLE) } {
						# escape
						set ST(ESC) 1
						continue
					}
				} else {
					set ST(ESC) 0
				}
				if { $ST(BFL) < $ST(MPM) } {
					append ST(BUF) $c
					binary scan $c cu v
					set ST(PAR) [expr { ($ST(PAR) + $v)
						& 0xFF }]
				}
				incr ST(BFL)
			}
			if $ST(STA) {
				set chunk ""
			}
		}

		2 {
			# waiting for the first non-DLE byte
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 4
				} else {
					# ASCII -> wait for NL
					set ST(STA) 3
				}
			}
		}

		3 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			pt_diag
		}

		4 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			pt_diag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				pt_trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc mo_write_f { msg byp } {

	global ST DB CH PM


	set out $CH(IPR)
	set par [expr { (-$ST(PAI)) & 0xFF }]
	set lm [string length $msg]

	if { $lm > $PM(MPL) } {
		if { $DB(LEV) > 0 } {
			pt_trc "write: outgoing message truncated to $PM(MPL)\
				bytes"
		}
		set lm $PM(MPL)
	}

	for { set i 0 } { $i < $lm } { incr i } {
		set c [string index $msg $i]
		if { $c == $CH(IPR) || $c == $CH(ETX) || $c == $CH(DLE) } {
			# need to escape
			append out $CH(DLE)
		}
		append out $c
		# parity
		binary scan $c cu v
		set par [expr { ($par - $v) & 0xFF }]
	}

	set par [binary format c $par]
	if { $par == $CH(IPR) || $par == $CH(ETX) || $par == $CH(DLE) } {
		set par "$CH(DLE)$par"
	}

	# complete
	append out "${par}$CH(ETX)"

	if { $DB(LEV) > 1 } {
		sy_dump "W" $out
	}

	catch {
		puts -nonewline $ST(SFD) $out
		flush $ST(SFD)
	}

	set ST(OBS) 0

	return 1
}

proc mo_init_f { } {

	global CH ST PM

	set ST(STA) 0
	set ST(BYP) 3
	set ST(MOD) "F"

	fconfigure $ST(SFD) -buffering none -translation binary

	set PM(UPL) $PM(MPL)

	set CH(IPR) [format %c [expr 0x02]]
	set CH(DLE) [format %c [expr 0x10]]
	set CH(ETX) [format %c [expr 0x03]]

	# initial parity
	set ST(PAI) [expr { 0x02 + 0x03 }]

	# max buffer length + 1 to accommodate parity on input
	set ST(MPM) [expr { $PM(MPL) + 1 }]
}

proc mo_reset_f { } {

# void

}

set MODULE(F) [list mo_init_f mo_rawread_f mo_write_f mo_reset_f]

###############################################################################
# Module: mode == D ###########################################################
###############################################################################

proc mo_rawread_d { } {
#
	global ST PM DB

	if { [catch { read $ST(SFD) } sta] || $sta == "" } {
		return 1
	}

	if $ST(BIN) {
		if { $DB(LEV) > 1 } {
			# dump
			sy_dump "R" $msg
		}
		$ST(DFN) $sta
		return 0
	}

	append ST(BUF) $sta

	while 1 {

		set sta [string first "\n" $ST(BUF)]
		if { $sta < 0 } {
			return 0
		}

		set msg [string range $ST(BUF) 0 [expr $sta - 1]]
		set ST(BUF) [string range $ST(BUF) [expr $sta + 1] end]

		if { $DB(LEV) > 1 } {
			# dump the packet
			sy_dump "R" $msg
		}

		$ST(DFN) $msg
	}
}

proc mo_write_d { msg byp } {
#
# Send out a message
#
	global ST DB

	if { $DB(LEV) > 1 } {
		sy_dump "W" $msg
	}

	if $ST(BIN) {
		catch {
			puts -nonewline $ST(SFD) $msg
			flush $ST(SFD)
		}
	} else {
		catch { puts $ST(SFD) $msg }
	}

	set ST(OBS) 0

	return 1
}

proc mo_init_d { } {
#
# Initialize
#
	global CH ST PM

	# permanent bypass
	set ST(BYP) 3
	set ST(MOD) "D"

	if $ST(BIN) {
		set trn "binary"
	} else {
		set trn { lf crlf }
	}

	fconfigure $ST(SFD) -buffering none -translation $trn

	set PM(UPL) $PM(MPL)
}

proc mo_reset_d { } {
#
# Protocol reset, trivial in this case, this is no protocol at all
#

}

set MODULE(D) [list mo_init_d mo_rawread_d mo_write_d mo_reset_d]

###############################################################################
###############################################################################

sy_cygfix

unames_init $ST(DEV) $ST(SYS)

###############################################################################
# Insert here your default plugin (just insert the plugin file) ###############
###############################################################################

### --->

###############################################################################
###############################################################################

sy_initialize

while 1 {
	# the output event loop is our body
	sy_outloop
	vwait ST(OBS)
} 

###############################################################################
