variable ACKCODE

set ACKCODE(0)		"OK"
set ACKCODE(1)		"Command format error"
set ACKCODE(2)		"Illegal length of command packet"
set ACKCODE(3)		"Illegal command parameter"
set ACKCODE(4)		"Illegal command code"
set ACKCODE(6)		"Module is off"
set ACKCODE(7)		"Module is busy"
set ACKCODE(8)		"Temporarily out of resources"

set ACKCODE(129) 	"Command format error, rejected by AP"
set ACKCODE(130)	"AP command format error"
set ACKCODE(131)	"Command too long for RF, rejected by AP"

#############################################################################
#############################################################################

oss_interface -id 0x00010022 -speed 115200 -length 56 \
	-parser { parse_cmd show_msg }

#############################################################################
#############################################################################
##
## Commands:
##
##	oximeter on -collect -regs n v ...
##	oximeter off
##	registers n n n ...
##
##	status			(battery, radio, oximeter)
##
##	radio [on] [delay]	(sets the delay, cannot off->on)
##	radio off
##
##	ap -node ... -wake ... -retries
##

oss_command oximeter 0x01 {
#
# Oximeter control
#
	word mode;
	blob regs;
}

oss_command registers 0x02 {
#
# See register contents
#
	blob regs;
}

oss_command status 0x03 {

}

oss_command radio 0x04 {
#
# Set radio delay
#
	# 0 == off
	word	delay;
}

oss_command ap 0x80 {
#
# Access point configuration
#
	# WOR wake retry count
	byte	worp;
	# Regular packet retry count
	byte	norp;
	# WOR preamble length
	word	worprl;
	# Node ID
	word	nodeid;
}

#############################################################################
#############################################################################

oss_message report 0x01 {
#
# Oximeter data report; 10 values packaged in a messy sort of way:
#	- second modulo 64K
#	- sequence number modulo 256
#	- two most significan bits of 20 values (20 * 2 / 8) = 5 bytes
#	- 20 word-sized values (less significant 16 bits of them)
#
	word	time;
	byte	seqn;
	byte	sigs [5];
	word	vals [20];
}

oss_message registers 0x02 {
#
# List of register values
#
	blob	regs;
}

oss_message status 0x03 {
#
# Status info
#
	lword	uptime;
	word	oxistat;
	word	delay;
	word 	battery;
	word	freemem;
	word	minmem;
}

oss_message ap 0x80 {
#
# To be extended later
#
	byte	worp;
	byte	norp;
	word	worprl;
	word	nodeid;
}

##############################################################################
##############################################################################

proc parse_selector { } {

	return [oss_parse -skip -match {^-([[:alnum:]]+)} -return 2]
}

proc parse_value { sel min max } {

	set val [oss_parse -skip -number -return 1]

	if { $val == "" } {
		error "$sel, illegal value"
	}

	if [catch { oss_valint $val $min $max } val] {
		error "$sel, illegal value, $val"
	}

	return $val
}

proc parse_check_empty { } {

	set cc [oss_parse -skip " \t," -match ".*" -return 1]
	if { $cc != "" } {
		error "superfluous arguments: $cc"
	}
}

variable CMDS

set CMDS(oximeter)	"parse_cmd_oximeter"
set CMDS(registers)	"parse_cmd_registers"
set CMDS(status)	"parse_cmd_status"
set CMDS(radio)		"parse_cmd_radio"
set CMDS(ap)		"parse_cmd_ap"

set LASTCMD		""

proc parse_cmd { line } {

	variable CMDS
	variable LASTCMD

	set cc [oss_parse -start $line -skip -return 0]
	if { $cc == "" || $cc == "#" } {
		# empty or comment
		return
	}

	if { $cc == "!" } {
		if { $LASTCMD == "" } {
			error "no previous command"
		}
		parse_cmd $LASTCMD
		return
	}

	if { $cc == ":" } {
		# a script
		set cc [string trim \
			[oss_parse -match "." -match ".*" -return 1]]
		set res [oss_evalscript $cc]
		oss_ttyout $res
		return
	}

	set cmd [oss_parse -subst -skip -match {^[[:alpha:]_][[:alnum:]_]*} \
		-return 2]

	if { $cmd == "" } {
		# no keyword
		error "illegal command syntax, must start with a keyword"
	}

	set cc [oss_keymatch $cmd [array names CMDS]]
	oss_parse -skip

	# check for a (generally) optional alpha keyword following the command
	# word

	$CMDS($cc) [oss_parse -match {^[[:alpha:]]+} -skip -return 0]

	set LASTCMD $line
}

##############################################################################

proc parse_cmd_oximeter { what } {

	if { $what != "" } {
		set kl { "on" "off" }
		if [catch { oss_keymatch $what $kl } what] {
			set kl [join $kl ", "]
			error "expected one of $kl"
		}
	}

	if { $what == "off" } {
		# nothing follows "off"
		parse_check_empty
		oss_issuecommand 0x01 [oss_setvalues [list 0 ""] "oximeter"]
		return
	}

	# on

	if { $what == "" } {
		# default on
		parse_check_empty
		oss_issuecommand 0x01 [oss_setvalues [list 3 ""] "oximeter"]
		return
	}

	# explicit on
	set mode 1
	set tp [parse_selector]
	if { $tp != "" } {
		if [catch { oss_keymatch $tp { "collect" } } k] {
			error "illegal selector -$tp"
		}
		set mode 3
	}

	# registers?

	set regs ""

	while { [oss_parse -skip -return 0] != "" } {

		if [catch { parse_value "regnum" 0 48 } val] {
			error "illegal register number"
		}

		lappend regs $val

		if [catch { parse_value "regval" 0 255 } val] {
			error "register value expected"
		}

		lappend regs $val
	}

	oss_issuecommand 0x01 [oss_setvalues [list $mode $regs] "oximeter"]
}

proc parse_cmd_registers { what } {

	if { $what != "" } {
		error "illegal argument $what"
	}

	set regs ""

	while { [oss_parse -skip -return 0] != "" } {

		if [catch { parse_value "regnum" 0 255 } val] {
			error "illegal register number"
		}

		lappend regs $val
	}

	oss_issuecommand 0x02 [oss_setvalues [list $regs] "registers"]
}
		
proc parse_cmd_status { what } {

	if { $what != "" } {
		error "unexpected $what"
	}

	parse_check_empty

	oss_issuecommand 0x03 [oss_setvalues "" "status"]
}

proc parse_cmd_radio { what } {

	if { $what != "" } {
		set kl { "on" "off" }
		if [catch { oss_keymatch $what $kl } what] {
			error "expected one of on, off"
		}
	}

	if { $what == "" || $what == "on" } {
		# optional delay
		set val [oss_parse -skip -number -return 1]
		if { $val == "" } {
			if { $what == "" } {
				error "expected one of on, off"
			}
			# the default
			set val 2048
		}
	} else {
		# off
		set val 0
	}

	parse_check_empty
	oss_issuecommand 0x04 [oss_setvalues [list $val] "radio"]
}

proc parse_cmd_ap { what } {

	if { $what != "" } {
		error "unexpected $what"
	}

	# unused
	set nodeid 0xFFFF
	set worprl 0xFFFF
	set worp 0xFF
	set norp 0xFF

	while 1 {

		set tp [parse_selector]
		if { $tp == "" } {
			break
		}

		set k [oss_keymatch $tp { "node" "wake" "retries" "preamble" }]

		if { $k == "node" } {
			if { $nodeid != 0xFFFF } {
				error "duplicate node specfification"
			}
			set nodeid [parse_value "node" 1 65534]
			continue
		}

		if { $k == "wake" } {
			if { $worp != 0xFF } {
				error "duplicate wor specification"
			}
			set worp [parse_value "wor" 0 3]
			continue
		}

		if { $k == "preamble" } {
			if { $worprl != 0xFFFF } {
				error "duplicate preamble specification"
			}
			set worprl [parse_value "preamble" 1023 10240]
			continue
		}

		if { $norp != 0xFF } {
			error "duplicate retries specification"
		}

		set norp [parse_value "retries" 0 7]
	}

	parse_check_empty
	oss_issuecommand 0x80 \
		[oss_setvalues [list $worp $norp $worprl $nodeid ] "ap"]
}

###############################################################################
###############################################################################

proc show_msg { code ref msg } {

	if { $code == 0 } {
		# ACK or NAK
		if { $ref != 0 } {
			variable ACKCODE
			binary scan $msg su msg
			if [info exists ACKCODE($msg)] {
				oss_ttyout "<$ref>: $ACKCODE($msg)"
			} else {
				oss_ttyout "<$ref>: response code $msg"
			}
		}
		return
	}

	set str [oss_getmsgstruct $code name]

	if { $str == "" } {
		# this will trigger default dump
		error "no user support"
	}

	show_msg_$name $msg
}

proc show_msg_registers { msg } {

	lassign [oss_getvalues $msg "registers"] regs

	set res "Values:"

	foreach r regs {
		append res [format " %02x" $r]
	}

	oss_ttyout $res
}

proc get_rss { msg } {

	binary scan [string range $msg end-1 end] cucu lq rs

	return "RSS: $rs, LQI: [expr { $lq & 0x7f }]"
}

proc sectoh { se } {

	set ti ""
	if { $se >= 3600 } {
		set hm [expr { $se / 3600 }]
		set se [expr { $se - ($hm * 3600) }]
		lappend ti "${hm}h"
	}
	if { $se >= 60 } {
		set hm [expr { $se / 60 }]
		set se [expr { $se - ($hm * 60) }]
		lappend ti "${hm}m"
	}
	if { $se > 0 || $ti == "" } {
		lappend ti "${se}s"
	}
	return [join $ti ","]
}

proc show_msg_status { msg } {

	lassign [oss_getvalues $msg "status"] se os de ba fm mm

	set res "Node status ([get_rss $msg]):\n"

	append res "  Uptime:      [sectoh $se]\n"

	append res "  Oximeter:    "

	if { $os == 0 } {
		append res "OFF"
	} else {
		append res "ON"
		if { $os > 1 } {
			append res "+COLL"
		}
	}
	append res "\n"

	append res "  RXOn:        ${de}ms\n"

	append res "  Battery:     [format %4.2f [expr { ($ba*5.0)/4095 }]]V\n"

	append res "  Mem cur/min: [expr $fm * 2]/[expr $mm * 2]B\n"

	oss_ttyout $res
}


proc show_msg_ap { msg } {

	lassign [oss_getvalues $msg "ap"] worp norp worprl nodeid

	set res "AP status:\n"
	append res "  WOR packets: $worp\n"
	append res "  Retries:     $norp\n"
	append res "  Preamble:    $worprl\n"
	append res "  Node:        $nodeid ([format %04X $nodeid])\n"

	oss_ttyout $res
}

proc show_msg_report { msg } {

	lassign [oss_getvalues $msg "report"] tm ser sigs vals

	if { [llength $sigs] != 5 || [llength $vals] != 20 } {
		error "bad report"
	}

	set res "ACC report: [format %5d $tm] [format %3d ser]\n"

	set j 0
	set s 0
	for { set i 0 } { $i < 20 } { incr i 2 } {
		set re [lindex $vals $i]
		set ir [lindex $vals $i+1]
		set rw [lindex $sigs $j]
		set rs [expr { ($rw >> $s) & 3 }]
		incr s 2
		set is [expr { ($rw >> $s) & 3 }]
		if { $s == 6 } {
			incr j
			set s 0
		} else {
			incr s 2
		}
		set re [expr { ($rs << 16) | $re }]
		set ir [expr { ($is << 16) | $is }]
		append res "   [format %6d] [format %6d]\n"
	}

	out_ttyout $res
}
