##
## BMA250 register setting macros
##
variable MACROS

# range (0 = 2g, 1 = 4g, 2 = 8g, 3 = 16g)
# bandwidth (7.81, 15.63, 31.25, 62.5, 125, 250, 500, 1000)
# power (0 = lowest, sleep interval 1s, 11 = full, 10 = 0.5ms, ...
set MACROS(parameters) {
	{ { 0-3 0 } { 0-7 7 } { 0-11 11 } }
	{ 0x0F = { %0 == 0 ? 3 : (%0 == 1 ? 5 : (%0 == 2 ? 8 : 12)) } }
	{ 0x10 = { %1 | 8 } }
	{ 0x11 = { %2 > 10 ? 0 : (%2 == 10 ? 64 : (15 - %2) + 64) } }
}

# nsamples, threshold
set MACROS(motion) {
	{ { 1-4 1 } { 0-255 36 } } 
	{ 0x16 | 0x07 }
	{ 0x27 = { %0 - 1 } }
	{ 0x28 = %1 }
}

# mode (two bits, upper quiet = 20ms (on), 30ms (off), lower shock 75/50ms)
# threshold
# nsamples
# delay (until second tap)
set MACROS(doubletap) {
	{ { 0-3 0 } { 0-31 10 } { 0-3 0 } { 0-7 4 } }
	{ 0x16 | 0x10 }
	{ 0x2A = { (%0 << 6) | %3 } }
	{ 0x2B = { (%2 << 6) | %1 } }
}

# as for doubletap (the first three parameters)
set MACROS(singletap) {
	{ { 0-3 0 } { 0-31 10 } { 0-3 0 } }
	{ 0x16 | 0x20 }
	{ 0x2A = { %0 << 6 } }
	{ 0x2B = { (%2 << 6) | %1 } }
}

# blocking (0 = none, 1 = theta, 2 = theta or slope, 3 = ...)
# mode (0 = symmetrical, 1 = high-assym, 2 = low-assym, 3 = symmetrical)
# theta
# hysteresis
set MACROS(orientation) {
	{ { 0-3 2 } { 0-3 0 } { 0-63 8 } { 0-7 1 } }
	{ 0x16 | 0x40 }
	{ 0x2C = { (%0 << 2) | %1 | (%3 << 4) } }
	{ 0x2D = %2 }
}

# theta, hold
set MACROS(flat) {
	{ { 0-63 8 } { 0-3 1 } }
	{ 0x16 | 0x80 }
	{ 0x2E = %0 }
	{ 0x2F = { %1 << 4 } }
}

# mode (0 = single, 1 = sum)
# threshold
# delay
# hysteresis
set MACROS(fall) {
	{ { 0-1 0 } { 0-255 48 } { 0-255 9 } { 0-3 1 } }
	{ 0x17 | 0x08 }
	{ 0x22 = %2 }
	{ 0x23 = %1 }
	{ 0x24 | { %3 | (%0 << 2) } }
}

# threshold, delay, hysteresis
set MACROS(shock) {
	{ { 0-255 192 } { 0-255 15 } { 0-3 2 } }
	{ 0x17 | 0x07 }
	{ 0x24 | { %2 << 6 } }
	{ 0x25 = %1 }
	{ 0x26 = %0 }
}

##
## Configurable registers of BMA250
##
variable REGLIST { 0x0F 0x10 0x11 0x13 0x16 0x17 0x1E 0x22 0x23 0x24
	           0x25 0x26 0x27 0x28 0x2A 0x2B 0x2C 0x2D 0x2E 0x2F }
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

proc myinit { } {

	variable REGMAP
	variable REGLIST

	set i 0
	foreach r $REGLIST {
		set REGMAP([expr $r]) $i
		incr i
	}

	# oss_dump -incoming -outgoing
}

myinit

#############################################################################
#############################################################################

oss_interface -id 0x00010021 -speed 115200 -length 56 \
	-parser { parse_cmd show_msg }

#############################################################################
#############################################################################
##
## Commands:
##
##	accel config ... options ... registers, e.g.,
##		accel config -par 1 6 8 -mo -fl -reg 0x0F 7
##	accel on [-from 13:40] [-to 14:55] [-erase]
##	accel erase
##	accel [read]
##	accel stats
##	accel off
##
##	pressure [read]
##
##	status			(time, battery, radio, display, accel status)
##
##	time [set] [date/time]
##
##	radio [on] [delay]	(sets the delay, cannot off->on)
##	radio off
##
##	display [on]
##	display off
##
##	ap -node ... -wake ... -retries
##

oss_command acconfig 0x01 {
#
# Accelerometer configuration
#
	blob	regs;
}

oss_command accturn 0x02 {
#
# Turn accelerometer on/off
#
	# delayed action, how many seconds from now (if nonzero)
	lword	after;
	# for how long (if nonzero)
	lword	duration;
	# 0 - NOP
	# 1 - off
	# 2 - on
	# 4 - erase	(flag)
	byte	what;
}

oss_command time 0x03 {
#
# Set time
#
	byte	time [6];
}

oss_command radio 0x04 {
#
# Set radio delay
#
	# 0 == off
	word	delay;
}

oss_command display 0x05 {
#
# Display on/off
#
	# 0 == off
	byte	what;
}

oss_command getinfo 0x06 {
#
# Get info
#
	# 0 = status, 1 = accel value, 2 = accel stats, 3 = accel conf,
	# 4 = pressure
	byte	what;
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

oss_message status 0x01 {
#
# Status info
#
	lword	uptime;
	lword	after;
	lword	duration;
	byte	accstat;
	byte	display;
	word	delay;
	word 	battery;
	byte	time [6];
}

oss_message presst 0x02 {
#
# Air pressure/temperature
#
	lword	press;
	sint	temp;
}

oss_message accvalue 0x03 {
#
# Accelerator reading
#
	word	stat;
	sint	xx;
	sint	yy;
	sint	zz;
	char	temp;
}

oss_message accstats 0x04 {
#
# Accelerator event stats
#
	lword	after;
	lword	duration;
	lword	nevents;
	lword	total;
	word	max;
	word	last;
	byte	on;
}

oss_message accregs 0x05 {
#
# Accelerator configuration
#
	blob	regs;
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

set CMDS(accelerometer)	"parse_cmd_accel"
set CMDS(pressure)	"parse_cmd_press"
set CMDS(status)	"parse_cmd_status"
set CMDS(time)		"parse_cmd_time"
set CMDS(radio)		"parse_cmd_radio"
set CMDS(display)	"parse_cmd_display"
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

proc rntoi { rn } {
#
# Converts register number to blob index
#
	variable REGMAP

	set rn [expr $rn]

	if { [catch { expr $rn } rg] || ![info exists REGMAP($rg)] } {
		error "illegal register number, $rn
	}

	return $REGMAP($rg)
}

proc itorn { ix } {
#
# Converts blob index to register number
#
	variable REGLIST

	if { $ix < 0 || $ix >= [llength $REGLIST] } {
		error "illegal register index, $ix"
	}

	return [expr { [lindex $REGLIST $ix] } ]
}

proc acc_config_blob { } {
#
# Build the registers blob for accelerometer configuration
#
	variable MACROS
	variable REGLIST

	set klist [array names MACROS]
	lappend klist "register"

	# maximum register number
	set mrn 0
	set rln [llength $REGLIST]

	while 1 {

		set tp [parse_selector]

		if { $tp == "" } {
			break
		}

		set k [oss_keymatch $tp $klist]

		if { $k == "register" } {
			# needs a special treatment
			set rn [parse_value $k]
			if [catch { rntoi $rn } ] {
				error "$k, illegal register number, $rn"
			}
			set cn 0
			while 1 {
				set va [oss_parse -skip -number -return 1]
				if { $va == "" } {
					break
				}
				if [catch { oss_valint $va 0 255 } va] {
					error "$k, illegal value, $va"
				}
				if [catch { rntoi $rn }] {
					error "$k, autoincremented register\
						number $rn is illegal"
				}
				set regset($rn) $va
				if { $rn > $mrn } {
					set mrn $rn
				}
				incr cn
				incr rn
			}
			continue
		}

		set mac $MACROS($k)
		# argument list with default values
		set ags [lindex $mac 0]
		# the assignments
		set mac [lrange $mac 1 end]
		# this is the number of arguments
		set nar [llength $ags]

		set par ""
		set def 1

		foreach a $ags {
			if $def {
				# still looking for actual arguments
				set va [oss_parse -skip -number -return 1]
				if { $va != "" } {
					regexp \
					     {([^[:space:]]+)-([^[:space:]]+)} \
						[lindex $a 0] jnk min max
					if [catch { oss_valint $va $min $max } \
					    va] {
						error "$k, illegal argument,\
							$va"
					}
					lappend par $va
					continue
				}
				# no more actual args, use defaults for rest
				set def 0
			}
			lappend par [lindex $a 1]
		}

		# handle the expressions
		foreach ex $mac {
			lassign $ex reg ope val
			set c 0
			foreach p $par {
				# parameter substitution
				regsub -all "%$c" $val $p val
				incr c
			}
			set val [eval "expr $val"]
			set reg [expr $reg]
			if { $ope == "|" && [info exists regset($reg)] } {
				set val [expr { $regset($reg) | $val }]
			}
			set regset($reg) $val
			if { $reg > $mrn } {
				set mrn $reg
			}
		}
	}

	set vals ""

	if $mrn {
		# initialize the mask
		set ma 0
		for { set i 0 } { $i < $rln } { incr i } {
			set rn [itorn $i]
			if [info exists regset($rn)] {
				set ma [expr { $ma | (1 << $i) }]
				set mx $i
			}
		}
		for { set i 0 } { $i < 4 } { incr i } {
			lappend vals [expr { ($ma >> (8 * $i)) & 0xFF }]
		}
		incr mx
		for { set i 0 } { $i < $mx } { incr i } {
			set rn [itorn $i]
			if [info exists regset($rn)] {
				lappend vals $regset($rn)
			} else {
				lappend vals 0
			}
		}
	}

	return $vals
}

proc stortc { sec } {
#
# Converts time in seconds to rtc setting
#
	set st [clock format $sec -format "%y %m %d %k %M %S"]
	set res ""
	foreach s $st {
		if [regexp "^0." $s] {
			set s [string range $s 1 end]
		}
		lappend res [expr $s]
	}
	return $res
}

proc rtctos { rtc } {
#
# Converts rtc setting to time in seconds
#
	return [clock scan [join $rtc] -format "%y %m %d %k %M %S"]
}

proc parse_cmd_accel { what } {
#
	if { $what != "" } {
		set kl { "configure" "on" "read" "stats" "events" "off"
			 "erase" }
		if [catch { oss_keymatch $what $kl } what] {
			set kl [join $kl ", "]
			error "expected one of $kl"
		}
	}

	if { $what == "" || $what == "read" } {
		parse_check_empty
		oss_issuecommand 0x06 [oss_setvalues [list 1] "getinfo"]
		return
	}

	if { $what == "stats" || $what == "events" } {
		parse_check_empty
		oss_issuecommand 0x06 [oss_setvalues [list 2] "getinfo"]
		return
	}

	if { $what == "configure" } {
		set rb [acc_config_blob]
		parse_check_empty
		if { $rb == "" } {
			oss_issuecommand 0x06 [oss_setvalues [list 3] "getinfo"]
		} else {
			oss_issuecommand 0x01 [oss_setvalues [list $rb] \
				"acconfig"]
		}
		return
	}

	if { $what == "erase" } {
		# erase only
		parse_check_empty
		oss_issuecommand 0x02 [oss_setvalues [list 0 0 4] "accturn"]
		return
	}

	if { $what == "on" } {

		set fr -1
		set to -1
		set er -1

		set klist { "erase" "from" "to" }

		# current time
		set cu [clock seconds]

		while 1 {

			set tp [parse_selector]

			if { $tp == "" } {
				break
			}

			set k [oss_keymatch $tp $klist]

			if { $k == "erase" } {
				if { $er >= 0 } {
					error "duplicate erase"
				}
				set er 1
				continue
			}

			# from or to
			set ti [oss_parse -skip -time -return 1]
			if { $ti == "" } {
				error "date/time expected"
			}

			if { $k == "from" } {
				if { $fr >= 0 } {
					error "duplicate from"
				}
				set fr $ti
				if { $to >= 0 && $fr >= $to } {
					error "from is later than to"
				}
				if { $fr < $cu } {
					error "from is earlier than now"
				}
				continue
			}

			# to
			if { $to >= 0 } {
				error "duplicate to"
			}
			set to $ti
			if { $fr >= 0 && $to <= $fr } {
				error "to is earlier than from"
			}
			if { $to < $cu } {
				error "to is earlier than now"
			}
		}

		if { $er >= 0 } {
			set cm 6
		} else {
			set cm 2
		}

		if { $fr < 0 } {
			set fr $cu
		}

		if { $to > 0 } {
			# duration
			set to [expr $to - $fr]
		} else {
			# infinite
			set to 0
		}

		set fr [expr { $fr - $cu }]

		parse_check_empty
		oss_issuecommand 0x02 [oss_setvalues [list $fr $to $cm] \
			"accturn"]
		return
	}

	# off
	parse_check_empty
	oss_issuecommand 0x02 [oss_setvalues [list 0 0 1] "accturn"]
}

proc parse_cmd_press { what } {
#
	if { $what != "" } {
		# the only legitimate (and optional) word
		oss_keymatch $what { "read" }
	}

	parse_check_empty
	oss_issuecommand 0x06 [oss_setvalues [list 4] "getinfo"]
}

proc parse_cmd_status { what } {
#
	if { $what != "" } {
		# the only legitimate (and optional) word
		oss_keymatch $what { "read" }
	}

	parse_check_empty
	oss_issuecommand 0x06 [oss_setvalues [list 0] "getinfo"]
}

proc parse_cmd_time { what } {

	if { $what != "" } {
		oss_keymatch $what { "set" }
	}

	if { [oss_parse -skip -return 0] == "" } {
		# current time
		set sec [clock seconds]
	} else {
		set sec [oss_parse -time]
		if { $sec == "" } {
			error "time expected"
		}
	}

	# convert to time
	set val [stortc $sec]

	parse_check_empty
	oss_issuecommand 0x03 [oss_setvalues [list $val] "time"]
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

proc parse_cmd_display { what } {

	if { $what != "" } {
		set kl { "on" "off" }
		if [catch { oss_keymatch $what $kl } what] {
			error "expected one of on, off"
		}
	}

	if { $what == "" || $what == "on" } {
		set cmd 1
	} else {
		set cmd 0
	}

	parse_check_empty
	oss_issuecommand 0x05 [oss_setvalues [list $cmd] "display"]
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

##############################################################################

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

	lassign [oss_getvalues $msg "status"] se af du ac di de ba tm

	set res "Node status:\n"

	append res "  Uptime:      [sectoh $se]\n"

	append res "  Time:        [clock format [rtctos $tm]]\n"

	append res "  Accelerator: "
	if { $ac == 0 } {
		append res "OFF"
	} elseif { $ac == 1 } {
		append res "ON"
		if $du {
			append res " [sectoh $du] left"
		}
	} else {
		append res "WAIT [sectoh $ac]"
		if $du {
			append res ", ON [sectoh $du]"
		}
	}
	append res "\n"

	append res "  Display:     "
	if $di {
		append res "ON"
	} else {
		append res "OFF"
	}

	append res "\n"

	append res "  RXOn:        ${de}ms\n"

	append res "  Battery:     [format %4.2f [expr { ($ba*5.0)/4095 }]]V\n"

	oss_ttyout $res
}

proc pacc { x { f 1.0 } } {
#
# Format the acceleration value assuming one unit == 3.91mg
#
	return "[format %6.3f [expr { ($x * 3.91 * $f)/1000.0 }]]g"
}

proc show_msg_accvalue { msg } {

	lassign [oss_getvalues $msg "accvalue"] stat x y z t

	set res "Accelerator reading:\n"
	append res "  Events:      [format %04X $stat]\n"
	append res "  X:           [pacc $x]\n"
	append res "  Y:           [pacc $y]\n"
	append res "  Z:           [pacc $z]\n"
	append res "  Temp:        [format %5.1f [expr { ($t/2.0) + 24.0 }]]C\n"

	oss_ttyout $res
}

proc show_msg_accstats { msg } {

	lassign [oss_getvalues $msg "accstats"] af du ne to ma la on

	set res "Accelerator monitor:\n"
	append res "  Status:      "
	if $on {
		append res "ON"
		if $du {
			append res " for [sectoh $du] more"
		}
	} else {
		append res "OFF"
		if $af {
			append res " for [sectoh $af]"
			if $du {
				append res " then ON for [sectoh $du]"
			}
		}
	}
	append res "\n"

	if !$ne {
		append res "  No events"
	} else {
		append res "  Events:      $ne\n"
		append res "  Last:        [pacc $la 0.333]\n"
		append res "  Average:     [pacc $to [expr { 0.333 / $ne }]]\n"
		append res "  Max:         [pacc $ma 0.333]"
	}

	append res "\n"
	oss_ttyout $res
}

proc show_msg_accregs { msg } {

	variable REGLIST

	lassign [oss_getvalues $msg "accregs"] regs

	set ll [llength $regs]
	if { $ll < 4 } {
		error "accregs message, bitmap too short"
	}

	set bmp 0
	# re-build the bitmask
	for { set i 0 } { $i < 4 } { incr i } {
		set bmp [expr { $bmp | (([lindex $regs $i] & 0xFF) << 8 * $i) }]
	}

	set regs [lrange $regs 4 end]

	set hdr "Accelerometer config:"

	set res ""

	for { set i 0 } { $i < [llength $REGLIST] } { incr i } {
		if { [expr { $bmp & (1 << $i) }] == 0 } {
			# skip
			continue
		}
		set rn [itorn $i]
		set va [lindex $regs $i]
		if { $va == "" } {
			error "accregs message, register list too short"
		}
		append res \
		  "\n  [format %02X $rn]  <---  [format %02X $va] ([expr $va])"
	}

	if { $res == "" } {
		append hdr " NONE"
	} else {
		append hdr $res
	}

	oss_ttyout $hdr
}

proc show_msg_presst { msg } {

	lassign [oss_getvalues $msg "presst"] pr te

	set res "Pressure sensor:\n"
	append res "  Pressure:    $pr (Pa)\n"
	append res "  Temperature: [expr { $te/10 }].[expr { $te%10 }] (C)\n"

	oss_ttyout $res
}

###############################################################################

proc show_msg_ap { msg } {

	lassign [oss_getvalues $msg "ap"] worp norp worprl nodeid

	set res "AP status:\n"
	append res "  WOR packets: $worp\n"
	append res "  Retries:     $norp\n"
	append res "  Preamble:    $worprl\n"
	append res "  Node:        $nodeid ([format %04X $nodeid])\n"

	oss_ttyout $res
}
