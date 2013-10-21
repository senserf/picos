oss_interface -id 0x00010001 -speed 9600 -length 82 \
	-parser { parse_cmd show_msg }

###############################################################################
###############################################################################

oss_command radio 0x01 {
	byte status;
	byte power;
	word channel;
	word interval [2];
	word length [2];
	blob data;
}

oss_command system 0x02 {
#
# Request: 0x02 - query
#	   0x01 - reset
#	   0x00 - set
#
	word spin;
	word leds;
	byte request;
	byte blinkrate;
	byte power;
	blob diag;
}

###############################################################################

oss_message status 0x01 {

	byte rstatus;
	byte rpower;
	byte rchannel;
	word rinterval [2];
	word rlength [2];
	word smemstat [3];
	byte spower;
}

oss_message packet 0x02 {

	lword counter;
	byte rssi;
	blob payload;
}

###############################################################################
###############################################################################

proc parse_selector { } {

	return [oss_parse -skip -match {^-([[:alnum:]]+)} -return 2]
}

proc parse_numbers { { cnt 1 } { del ":,;" } } {

	set res ""

	while { $cnt } {

		incr cnt -1

		set cc [oss_parse -skip -number -return 1]
		if { $cc == "" } {
			break
		}
		trc "PNT $cc"

		lappend res $cc

		if { $del != "" } {
			set cc [oss_parse -skip -return 0]
			if { $cc == "" || [string first $cc $del] < 0 } {
				break
			}
			oss_parse -match "."
		}
	}

	return $res
}
			
proc parse_key { } {

	return [string tolower [oss_parse -skip -match {^[[:alnum:]]+} \
		-return 1]]
}

proc parse_bool { { sel "" } } {

	set cc [parse_key $sel $abb]

	if { $cc == "yes" || $cc == "y" || $cc == 1 } {
		return 1
	} elseif { $cc == "no" || $cc == "n" || $cc == 0 } {
		return 0
	}

	if { $sel != "" } {
		error "illegal boolean value of -$sel, must be yes (y), no (n),\
			1, or 0"
	}

	return ""
}

proc parse_leds { max { sel "" } } {
#
# A sequence of led state values: off, on, blink
#
	set res 0
	set nva 0

	# initialize the result
	for { set i 0 } { $i < $max } { incr i } {
		# this means unchanged
		set res [expr $res | (0x3 << ($i * 2))]
	}

	set sre $res

	if { $sel != "" } {
		set sel " for -$sel"
	}

	while 1 {

		set cc [oss_parse -skip " \t," -match \
		   {^([[:digit:]][[:digit:]]*)=(on|off?|b[link]*)}]

		set di [lindex $cc 2]
		set st [lindex $cc 3]

		if { $di == "" } {
			break
		}

		if [catch { oss_valint $di 0 [expr $max - 1] } dio] {
			error "illegal LED indicator $di$sel"
		}

		if { [string index $st 0] == "b" } {
			set st 2
		} elseif { [string index $st 1] == "n" } {
			set st 1
		} else {
			set st 0
		}

		set di [expr $dio * 2]
		set st [expr $st << $di]
		set ms [expr 0x3 << $di]

		if { [expr $res & $ms] != $ms } {
			error "duplicate setting for LED $dio"
		}

		set res [expr ($res & ~$ms) | $st]
	}

	if { $res == $sre } {
		error "missed argument(s)$sel"
	}
}


proc parse_choice { choice { sel "" } } {

	if { $sel != "" } {
		set sel " for -$sel"
	}

	set cc [parse_key]

	set v 0

	foreach ch $choice {
		foreach c $ch {
			if { $cc == $c } {
				return $v
			}
		}
		incr v
	}

	set em ""
	foreach ch $choice {
		set cu "[lindex $ch 0]"
		if { [llength $ch] > 1 } {
			append cu " ("
			foreach cs [lrange $ch 1 end] {
				append cu "$cs "
			}
			set cu "[string trimright $cu])"
		}
		lappend em $cu
	}

	set em [join $em ", "]
	error "illegal value$sel, must be $em"
}

proc parse_range { min max { sel "" } } {

	set vals [parse_numbers 2]
	trc "PRV $vals"

	set A [lindex $vals 0]

	if { $sel != "" } {
		set sel " for -$sel"
	}

	if { $A == "" } {
		error "illegal (empty) range$sel"
	}

	if [catch { oss_valint $A $min $max } A] {
		error "illegal (first) range value$sel, $A"
	}

	set B [lindex $vals 1]
	if { $B == "" } {
		set B $A
	} elseif { [catch { oss_valint $B $min $max } B] } {
		error "illegal second range value$sel, $B"
	}

	if { $B < $A } {
		error "range values$sel are decreasing"
	}

	return [list $A $B]
}

proc parse_value { min max { sel "" } } {

	if { $sel != "" } {
		set sel " for -$sel"
	}

	set val [parse_numbers]

	if { $val == "" } {
		error "illegal (empty) value$sel"
	}

	if [catch { oss_valint $val $min $max } val] {
		error "illegal value$sel, $val"
	}

	return $val
}

proc parse_blob { { bstr 0 } } {
#
# Assumes that this is all that is left in the line
#
	set res ""

	while 1 {

		# skip whatever delimiters you can think of
		set cc [oss_parse -skip " \t,:;" -return 0]
		if { $cc == "" } {
			break
		}

		if { $cc == "\"" } {
			# a string
			set res [concat $res [oss_parse -string -return 0]]
			continue
		}

		set cc [oss_parse -number -return 0]
		if { $cc == "" } {
			break
		}

		if [catch { oss_valint $cc 0 255 } val] {
			error "illegal byte value in the blob: $cc, $val"
		}

		lappend res $val
	}

	if { $bstr && $res != "" && [lindex $res end] != 0 } {
		lappend res 0
	}

	return $res
}

proc parse_check_empty { } {

	if { [oss_parse -skip -return 0] != "" } {
		error "superfluous arguments [oss_parse -match ".*" -return 0]"
	}
}

###############################################################################
###############################################################################

set CMDS(radio) 	"parse_cmd_radio"
set CMDS(system)	"parse_cmd_system"

proc varvar { v } {
#
# Returns the value of a variable whose name is stored in a variable
#
	upvar $v vv
	return $vv
}

proc parse_cmd { line } {

	variable CMDS

	trc "PCMD: $line"
	set cc [oss_parse -start $line -skip -return 0]

	if { $cc == "" || $cc == "#" } {
		# empty line or comment
		return ""
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
		-return 1]
	trc "PCMD: cmd = $cmd"

	if { $cmd == "" } {
		# no keyword
		error "illegal command syntax, must start with a keyword"
	}

	set cc [oss_keymatch $cmd [array names CMDS]]
	oss_parse -skip

	$CMDS($cc)
}

###############################################################################
# radio #######################################################################
###############################################################################

proc parse_cmd_radio_state { } {

	set cc [oss_parse -skip -match {^[onfrxtmw+]+[[:>:]]}]

	set val [string tolower [lindex $cc 1]]

	if [regexp -nocase {^([[:alpha:]]+)\+([[:alpha:]]+)$} $val jnk fi se] {
		# a combined spec
		foreach d { 0 1 } {
			set fi [string tolower $fi]
			set se [string tolower $se]
			if { $fi == "tx" || $fi == "xmt" } {
				set res 0x10
				if { $se == "rx" || $se == "rcv" } {
					incr res 1
				} elseif { $se == "wor" || $se == "wo" } {
					incr res 2
				} else {
					break
				}
				return $res
			}
			# second chance
			set m $fi
			set fi $se
			set se $m
		}
		error "a combined -state spec may only combine rx (rcv) or wor\
			(wo) with tx (xmt)"
	}

	if { $val == "on" } {
		return 0x11
	}
	if { $val == "off" } {
		return 0x00
	}
	if { $val == "tx" || $val == "xmt" } {
		return 0x10
	}
	if { $val == "rx" || $val == "rcv" } {
		return 0x01
	}
	if { $val == "wor" || $val == "wo" } {
		return 0x02
	}

	error "the -state arg can be on, off, rx, rcv, tx, xmt, wor, wo, or a\
		combination of rx (rcv) or wor (wo) with tx (xmt) separated by\
		+"
}

proc parse_cmd_radio { } {

	set klist { state power channel interval length }

	foreach k $klist {
		set $k ""
	}

	while 1 {
		set sel [parse_selector]
		if { $sel == "" } {
			# no more selectors
			break
		}
		set k [oss_keymatch $sel $klist]
		if { [varvar $k] != "" } {
			error "duplicate selector -$k"
		}

		switch $k {

			"state" {

				set state [parse_cmd_radio_state]
			}

			"power" {

				set power [expr [parse_value 0 7 "power"] \
					& 0xFF]
			}

			"channel" {

				set channel [parse_value 0 255 "channel"]
			}

			"interval" {

				set interval [parse_range 1 65534 "interval"]
			}

			"length" {

				set length [parse_range 1 60 "length"]
			}
		}
	}

	# defaults
	if { $state == "" } 	{ set state 0xFF }
	if { $power == "" } 	{ set power 0xFF }
	if { $channel == "" } 	{ set channel 0xFFFF }
	if { $interval == "" } 	{ set interval [list 0xFFFF 0xFFFF] }
	if { $length == "" } 	{ set length [list 0xFFFF 0xFFFF] }

	set data [parse_blob]

	parse_check_empty

	trc "RADIO: $state $power $channel $interval $length $data"
	oss_issuecommand 0x01 [oss_setvalues [list $state $power $channel \
		$interval $length $data] "radio"]
} 

###############################################################################
# system ######################################################################
###############################################################################

proc parse_cmd_system { } {

	set klist { spin leds blinkrate power reset query }

	foreach k $klist {
		set $k ""
	}

	set otr 0
	set request 0
	set leds [expr 0xFFFF]
	set spin 0
	set blinkrate 255
	set power 255
	set reset 0

	trc "PARSE CMD SYSTEM"
	while 1 {
		set sel [parse_selector]
		trc "PCS: sel = $sel"
		if { $sel == "" } {
			break
		}
		set k [oss_keymatch $sel $klist]
		if { [varvar $k] != "" } {
			error "duplicate selector -$k"
		}

		switch $k {

			"reset" {

				set reset 1
				set request 1
			}

			"spin" {

				set spin [parse_value 1 65534 "spin"]
				incr otr
			}

			"leds" {

				set leds [parse_leds 8 "leds"]
				incr otr
			}

			"blinkrate" {

				set blinkrate [parse_choice \
				    { { fast f } { slow s } } "blinkrate"]
				incr otr
			}

			"power" {

				set power [parse_choice \
				    { { up u } { down d } } "power"]
				incr otr
			}

			"query" {

				set request 2
				incr otr
			}
		}
	}

	if { $reset && $otr } {
		error "-reset cannot be mixed with other parameters"
	}

	# any diag message
	set data [parse_blob 1]

	parse_check_empty

	oss_issuecommand 0x02 [oss_setvalues [list $spin $leds $request \
		$blinkrate $power $data] "system"]
}

###############################################################################
###############################################################################

proc show_msg { code ref msg } {
#
# Message distributor
#
	set str [oss_getmsgstruct $code name]
	trc "SHOW MSG: $code $ref $name"

	if { $str == "" } {
		# throw an error which will trigger default dump
		error "no user support"
	}

	show_msg_$name $msg
}

proc show_msg_status { msg } {

	foreach { rs rp rc ri rl sm sp } [oss_getvalues $msg "status"] { }

	if { $rs == 0 } {
		set qs "OFF"
	} else {
		set qs ""
		if { $rs > 0x0F } {
			lappend qs "TX"
		}
		set rs [expr $rs & 0x0F]
		if { $rs == 1 } {
			lappend qs "RX"
		} elseif { $rs == 2 } {
			lappend qs "WOR"
		}
		set qs [join $qs "+"]
	}

	if $sp {
		set sp "UP"
	} else {
		set sp "DOWN"
	}

	set res "Status:\n"
	append res "  Radio status:   $qs\n"
	append res "  Xmit power:     $rp\n"
	append res "  Channel:        $rc\n"
	append res "  Interval:       [lindex $ri 0]:[lindex $ri 1]\n"
	append res "  Length:         [lindex $rl 0]:[lindex $rl 1]\n"
	append res "  Memstat:        [lindex $sm 0], [lindex $sm 1],\
			[lindex $sm 2]\n"
	append res "  Power:          $sp"

	oss_ttyout $res
}

proc show_msg_packet { msg } {

	foreach { cnt rss con } [oss_getvalues $msg "packet"] { }

	if $rss {
		set res "RCV:"
	} else {
		set res "SND:"
	}

	append res " [format %09d $cnt],"

	if $rss {
		append res " R[format %03d $rss],"
	}

	append res "\[[join $con " "]\]"

	oss_ttyout $res
}
