oss_interface -id 0x00090002 -speed 9600 -length 82 \
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
	byte swtch;
	blob diag;
}

oss_command dht11 0x03 {
#
	byte duration;
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
	byte swtch;
}

oss_message packet 0x02 {

	lword counter;
	byte rssi;
	blob payload;
}

oss_message dht11t 0x03 {

	blob times;
}

###############################################################################
###############################################################################

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

proc parse_range { sel min max } {

	set res [oss_parse -skip -checkpoint -number -skip " \t:,;" -number]
	if { $res == "" } {
		set A [oss_parse -number -return 0]
		if { $A == "" } {
			error "$sel, illegal value"
		}
		set B $A
	} else {
		set A [lindex $res 1]
		set B [lindex $res 3]
	}

	foreach v { A B } {
		if [catch { oss_valint [varvar $v] $min $max } $v] {
			error "$sel, illegal value, [varvar $v]"
		}
	}

	if { $B < $A } {
		error "$sel, $A is less than $B"
	}

	return [list $A $B]
}

proc parse_word { } {

	return [string tolower [oss_parse -skip -match {^[[:alnum:]]+} \
		-return 1]]
}

proc parse_leds { max } {
#
# A sequence of led state values: off, on, blink
#
	set res 0
	set nva 0

	set res 0xFFFF

	set sre $res

	while 1 {

		set cc [oss_parse -skip " \t," -match \
		   {^([[:digit:]])=(on|off?|b[link]*)}]

		set di [lindex $cc 2]
		set st [lindex $cc 3]

		if { $di == "" } {
			break
		}

		if { $di > $max } {
			error "illegal LED indicator $di"
		}

		if { [string index $st 0] == "b" } {
			set st 2
		} elseif { [string index $st 1] == "n" } {
			set st 1
		} else {
			set st 0
		}

		set dj [expr $di * 2]
		set st [expr $st << $dj]
		set ms [expr 0x3 << $dj]

		if { [expr $res & $ms] != $ms } {
			error "duplicate setting for LED $di"
		}

		set res [expr ($res & ~$ms) | $st]
	}

	if { $res == $sre } {
		error "LED value expected"
	}
}


proc parse_choice { choice sel } {

	set cc [parse_word]
	if { $cc == "" } {
		error "choice expected for $sel"
	}

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
	error "illegal value for $sel, must be $em"
}

proc parse_blob { } {
#
	set cc [oss_parse -skip "\t, " -return 0]
	if { $cc == "\"" } {
		return [oss_parse -string -return 0]
	}

	set res ""

	while 1 {
		set cc [oss_parse -skip "\t,;: " -number -return 1]
		if { $cc == "" } {
			return $res
		}
		lappend res $cc
	}
}

proc parse_check_empty { } {

	set cc [oss_parse -skip " \t," -match ".*" -return 1]
	if { $cc != "" } {
		error "superfluous arguments: $cc"
	}
}

proc varvar { v } {
#
# Returns the value of a variable whose name is stored in a variable
#
	upvar $v vv
	return $vv
}

###############################################################################
###############################################################################

set CMDS(radio) 	"parse_cmd_radio"
set CMDS(system)	"parse_cmd_system"
set CMDS(dht11)		"parse_cmd_dht11"

proc parse_cmd { line } {

	variable CMDS

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
		-return 2]

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

	# these are the names of selectors (keys) and also the names of
	# variables storing the selectors' values
	foreach k $klist {
		# initialize to absent
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
				set power [parse_value "power" 0 7]
			}

			"channel" {
				set channel [parse_value "channel" 0 255]
			}

			"interval" {
				set interval [parse_range "interval" 1 65534]
			}

			"length" {
				set length [parse_range "length" 1 60]
			}
		}
	}

	# defaults; these values tell the program to retaing previous settings
	if { $state == "" } 	{ set state 0xFF }
	if { $power == "" } 	{ set power 0xFF }
	if { $channel == "" } 	{ set channel 0xFFFF }
	if { $interval == "" } 	{ set interval [list 0xFFFF 0xFFFF] }
	if { $length == "" } 	{ set length [list 0xFFFF 0xFFFF] }

	set data [parse_blob]

	parse_check_empty

	oss_issuecommand 0x01 [oss_setvalues [list $state $power $channel \
		$interval $length $data] "radio"]
} 

###############################################################################
# system ######################################################################
###############################################################################

proc parse_cmd_system { } {

	set klist { spin leds blinkrate power reset query switch }

	foreach k $klist {
		set $k ""
	}

	# flag = incompatible with reset
	set otr 0

	# any requests? 0 - none, 1 - reset, 2 - query
	set request 0

	while 1 {
		set sel [parse_selector]
		if { $sel == "" } {
			break
		}
		set k [oss_keymatch $sel $klist]
		if { [varvar $k] != "" } {
			error "duplicate selector -$k"
		}

		switch $k {

			"reset" {
				set request 1
				set reset 1
			}

			"spin" {
				set spin [parse_value "spin" 1 65534]
				incr otr
			}

			"leds" {
				set leds [parse_leds 8]
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

			"switch" {
				set switch [parse_choice { off on } "switch"]
				incr otr
			}

			"query" {
				set request 2
				incr otr
			}
		}
	}

	# defaults
	if { $leds == "" } 	{ set leds [expr 0xFFFF] }
	if { $spin == "" } 	{ set spin 0 }
	if { $blinkrate == "" } { set blinkrate 255 }
	if { $power == "" } 	{ set power 255 }
	if { $switch == "" } 	{ set switch 255 }

	if { $reset != "" && $otr } {
		error "-reset cannot be mixed with other parameters"
	}

	# any diag message
	set data [oss_bytestobin [parse_blob]]

	parse_check_empty

	oss_issuecommand 0x02 [oss_setvalues [list $spin $leds $request \
		$blinkrate $power $switch $data] "system" 1]
}

###############################################################################
# dht11 #######################################################################
###############################################################################

proc parse_cmd_dht11 { } {

	set dur [oss_parse -skip -number -return 1]
	if { $dur == "" } {
		set dur 0xFF
	} else if [catch { oss_valint $dur 1 32 } dur] {
		error "illegal duration, $dur"
	}

	parse_check_empty

	oss_issuecommand 0x03 [oss_setvalues [list $dur] "dht11"]
}

###############################################################################
###############################################################################

proc show_msg { code ref msg } {
#
# Message distributor
#
	if { $code == 0 } {
		oss_ttyout "ACK: $ref"
		return
	}
		
	set str [oss_getmsgstruct $code name]

	if { $str == "" } {
		# throw an error which will trigger default dump
		error "no user support"
	}

	show_msg_$name $msg
}

proc show_msg_status { msg } {

	foreach { rs rp rc ri rl sm sp sw } [oss_getvalues $msg "status"] { }

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

	if $sw {
		set sw "ON"
	} else {
		set sw "OFF"
	}

	set res "Status:\n"
	append res "  Radio status:   $qs\n"
	append res "  Xmit power:     $rp\n"
	append res "  Channel:        $rc\n"
	append res "  Interval:       [lindex $ri 0]:[lindex $ri 1]\n"
	append res "  Length:         [lindex $rl 0]:[lindex $rl 1]\n"
	append res "  Memstat:        [lindex $sm 0], [lindex $sm 1],\
			[lindex $sm 2]\n"
	append res "  Power:          $sp\n"
	append res "  Switch:         $sw"

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

proc show_msg_dht11t { msg } {

	set tmg [lindex 0 [oss_getvalues $msg "dht11t"]

	set res "Timing:\n"

	set l "H"
	for { set i 0 } { $i < [expr [llength $tmg] / 2] } { incr i } {
		set dur [expr [lindex $tmg [expr 2 * $i]] | \
			([lindex $tmg [expr 2 * $i + 1]] << 8)]
		append res "  $l [format %5d $dur]\n"
		if { $l == "L" } {
			set l "H"
		} else {
			set l "L"
		}
	}
	append res "end"

	oss_ttyout $res
}
