#!/usr/bin/wish

#package require Tk 8.5

set Unique		0
set MaxLineCount	1024
set HostName		"localhost"
set AGENT_MAGIC		0xBAB4
set ECONN_OK		129
set PortNumber		4443

if { [lindex $argv 0] != "" } {
	set h ""
	set p ""
	set j [lindex $argv 0]
	if ![regexp "(.*):(.*)" $j j h p] {
		if [catch { expr $j } p] {
			set p ""
			set h $j
		} else {
			set h ""
		}
	}
	if { $p != "" } {
		set PortNumber $p
	}
	if { $h != "" } {
		set HostName $h
	}

	if { [catch { set PortNumber [expr $PortNumber ] }] ||
	    $PortNumber < 1 || $PortNumber > 65535 } {
		puts "Error: illegal port number: $PortNumber"
	}
}

array set LED_COLORS	{ 0 red 1 green 2 yellow 3 orange 4 blue }

set NTYPE_COLORS	{ yellow blue orange red green }

#
# Pin status ordinals:
#
#	PINSTAT_INPUT		0
# 	PINSTAT_OUTPUT		1
# 	PINSTAT_ADC		2
# 	PINSTAT_DAC0		3
# 	PINSTAT_DAC1		4
# 	PINSTAT_PULSE		5
# 	PINSTAT_NOTIFIER	6
# 	PINSTAT_ABSENT		7
#

array set PIN_COLORS	{ D,0 "#A4E22E" D,1 "#FFFF7F" D,2 "#FE8592"
			  D,3 "#7EDCED" D,4 "#7EDCED" D,5 "#61A77E"
			  D,6 "#FE3512" D,7 "#C0C0C0" O,0 "#8C82EC"
			  O,1 "#FF0000" O,2 "#C0C0C0" I,0 "#8C82EC"
			  I,1 "#FF0000" I,2 "#C0C0C0" A   "#7EDCED"
			  I   "#C0C0C0" N   "#7EB0ED"
			}

array set PANEL_COLORS	{ ACTIVE  "#F0EC00" DISABLED "#BEBEBE" DEL "#007DFF"
			  ONLABEL "#FF0000" OFFLABEL "#909090"               }

array set PIN_STCODE	{ 0 "I" 1 "O" 2 "A" 3 "D" 4 "D" 5 "P" 6 "N" 7 "-" }

array set HPA		{ 1 "(\[0-9a-f\])"
			  2 "(\[0-9a-f\]\[0-9a-f\])"
			  4 "(\[0-9a-f\]\[0-9a-f\]\[0-9a-f\]\[0-9a-f\])"
			  H "(\[0-9a-f\]+)"
			  D "(\[0-9\]+)"
			  F "(\[0-9.+-E\]+)"
			}

#
# Canvas margins for ROAMER:
#
# NR     = node radius
# ND     = node diameter
# TO     = text label Y offset
# RX, RY = running coordinate offset from left bottom
# MW, MH = minimum width and height
# DW, DH = default (initial) width and height
#
array set CMARGIN	{ L 24 R 24 U 24 D 34 NR 5 ND 10 TO 10 RX 10 RY 10
			  MW 200 MH 200 DW 400 DH 400 }

#
# Stuff for the "LCD" clock
#

array set LCDSHAPE {
    a {3.0 5 5.2 3 7.0 5 6.0 15 3.8 17 2.0 15}
    b {6.3 2 8.5 0 18.5 0 20.3 2 18.1 4 8.1 4}
    c {19.0 5 21.2 3 23.0 5 22.0 15 19.8 17 18.0 15}
    d {17.4 21 19.6 19 21.4 21 20.4 31 18.2 33 16.4 31}
    e {3.1 34 5.3 32 15.3 32 17.1 34 14.9 36 4.9 36}
    f {1.4 21 3.6 19 5.4 21 4.4 31 2.2 33 0.4 31}
    g {4.7 18 6.9 16 16.9 16 18.7 18 16.5 20 6.5 20}
    h {3.5 7.5 6.5 7.5 6.5 11.5 3.5 11.5}
    i {2.0 23.5 5 23.5 5.0 27.5 2.0 27.5}
}

array set LCDONITEMS {
    0 {a b c d e f}
    1 {c d}
    2 {b c e f g}
    3 {b c d e g}
    4 {a c d g}
    5 {a b d e g}
    6 {a b d e f g}
    7 {b c d}
    8 {a b c d e f g}
    9 {a b c d e g}
    - {g}
    { } {}
}

array set LCDOFFITEMS {
    0 {g}
    1 {a b e f g}
    2 {a d}
    3 {a f}
    4 {b e f}
    5 {c f}
    6 {c}
    7 {a e f g}
    8 {}
    9 {f}
    - {a b c d e f}
    { } {a b c d e f g}
}

array set LCDCOLORS 	{ ONE #ff8080 ONI #ff0000 OFE #000000 OFI #0F0F0F }

set MAXSVAL     [expr pow (2.0, 32.0) - 1.0]

proc alert { msg } {

	tk_dialog .alert "Attention!" $msg "" 0 "OK"
}

proc stName { Sok } {

	global Stat

	if { [info exists Stat($Sok,S)] && $Stat($Sok,S) != "" } {
		return " at node $Stat($Sok,S)"
	}

	return ""
}

proc uuName { Sok } {

	global Stat

	if [info exists Stat($Sok,Q)] {
		return " $Stat($Sok,Q)"
	} else {
		return ""
	}
}

proc uuStop { Sok } {

	global Stat

	if [info exists Stat($Sok,FD,U)] {
		if { $Stat($Sok,FD,U) != "" } {
			catch { close $Stat($Sok,FD,U) }
			set Stat($Sok,FD,U) ""
		}
		set Stat($Sok,Q) ""
	}
}

proc stimeout { Sok } {

	global Stat

	set Stat($Sok,T) [after 10000 doAbort $Sok]
}

proc ctimeout { Sok } {

	global Stat

	if [info exists Stat($Sok,T)] {
		after cancel $Stat($Sok,T)
		unset Stat($Sok,T)
	}
}

proc stwin { Sok } {
#
# Safe window acqusition from callbacks
#
	global Wins

	if ![info exists Wins($Sok)] {
		return -code return
	}

	set w $Wins($Sok)

	if { $w == "x" } {
		# stub
		return -code return
	}

	if ![winfo exists $w] {
		return -code return
	}

	return $w
}

proc winlocate { tag stid } {

	global Wins Stat

	set wn ""

	foreach s [array names Wins] {
		# window name
		set w $Wins($s)
		if ![regexp "^.w$tag\[0-9\]" $w] {
			continue
		}
		# check node ID
		if { $stid == "" } {
			if { ![info exists Stat($s,S)] || $Stat($s,S) == "" } {
				set wn $w
				break
			}
		} elseif { [info exists Stat($s,S)] && \
		    $stid == $Stat($s,S) } {
			set wn $w
			break
		}
	}

	if { $wn != "" } {
		if { $wn == "x" } {
			log "connection already in progress"
		} else {
			log "window already present"
			catch { raise $wn }
		}
		return 1
	}

	return 0
}

proc ledColor { i } {

	global LED_COLORS

	if [info exists LED_COLORS($i)] {
		return $LED_COLORS($i)
	} else {
		return "red"
	}
}

proc dealloc { Sok } {

	global Wins Stat Unique

	ctimeout $Sok
	if [info exists Wins($Sok)] {
		set w $Wins($Sok)
		if [winfo exists $w] {
			set Wins(Zombie_$Unique) $w
			bind $w <Destroy> "destroyWindow Zombie_$Unique"
			incr Unique
		}
		unset Wins($Sok)
	}

	foreach it [array names Stat "$Sok,FD,*"] {
		# any file descriptors to close??
		if { $Stat($it) != "" } {
			catch { close $Stat($it) }
		}
		unset Stat($it)
	} 

	array unset Stat "$Sok,*"

	catch { close $Sok }
}

proc uartHandler { stid mode } {

	global PortNumber HostName Wins Stat

	if { $mode == "h" } {
		# HEX
		set umo 1
	} else {
		# straight ASCII
		set umo 0
	}

	# try to locate the window, if it already exists

	if [winlocate "u" $stid] {
		return
	}

	set Wn ""
	
	log "connecting to UART at node $stid ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	# this is a placeholder - to preannounce the window for winlocate
	# until we actually get it up (or otherwise)
	set Wins($Sok) "x"
	set Stat($Sok,S) $stid
	set Stat($Sok,M) $umo

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 1"
	fileevent $Sok readable "uartIni $Sok"
}

proc sendReq { Sok rqnum } {

	global Stat AGENT_MAGIC

	# prepare the request
	set rqs ""
	
	abinS rqs $AGENT_MAGIC
	abinS rqs $rqnum
	if { [info exists Stat($Sok,S)] && $Stat($Sok,S) != "" } {
		abinI rqs $Stat($Sok,S)
	} else {
		abinI rqs -1
	}

	if [catch { puts -nonewline $Sok $rqs } err] {
		log "connection failed: $err"
		dealloc $Sok
		return
	}

	# wait for the timer to detect timeouts
	stimeout $Sok
	fileevent $Sok writable ""
}

proc uartIni { Sok } {

	global Wins Stat Unique ECONN_OK

	# we are reading the initial OK response
	ctimeout $Sok

	# four bytes expected
	if [catch { read $Sok 4 } res] {
		# disconenction
		log "connection failed: $res"
		dealloc $Sok
		return
	}

	if { $res == "" } {
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]

	set cc [expr $code & 0xff]

	if { $cc != $ECONN_OK } {
		log "connection rejected by SIDE: [conerror $cc]"
		dealloc $Sok
		return
	}

	# the rate
	set Stat($Sok,R) [expr (($code >> 8) & 0x0ffff) * 100]

	# create the window

	set Wn ".wu$Unique"
	incr Unique

	set Stat($Sok,B) ""
	set Stat($Sok,F) 0

	set Wins($Sok) $Wn

	mkTerm $Sok "UART at node $Stat($Sok,S)" $Stat($Sok,M)

	# reset the input script
	fileevent $Sok readable "uartRead $Sok"

	log "connected at $Stat($Sok,R)"
}

proc uartRead { Sok } {

	global Wins Stat

	if [catch { read $Sok } chunk] {
		# assume disconnection
		log "connection to UART[stName $Sok] terminated: $chunk"
		dealloc $Sok
		return
	}

	if [eof $Sok] {
		# closed
		log "connection to UART[stName $Sok] closed"
		dealloc $Sok
		return
	}

	if { $chunk == "" } {
		return
	}

	if { $Stat($Sok,FD,U) != "" } {
		# send it to the C-0-C UART
		catch { puts -nonewline $Stat($Sok,FD,U) $chunk }
	}

	set Wn $Wins($Sok)

	if $Stat($Sok,H) {
		# handle the HEX case
		if !$Stat($Sok,M) {
			# just entered
			endLine $Wn.t
			set Stat($Sok,M) 1
			set Stat($Sok,F) 0
		}

		set sl [string length $chunk]
		for { set ix 0 } { $ix < $sl } { incr ix } {
			set b [string index $chunk $ix]
			# this gets converted to three characters
			set enc [tohex $b]
			if { $Stat($Sok,F) >= 80 } {
				endLine $Wn.t
				set Stat($Sok,F) 0
			}
			addText $Wn.t $enc
			incr Stat($Sok,F) 2
			if { $Stat($Sok,F) <= 77 } {
				addText $Wn.t " "
				incr Stat($Sok,F)
			}
		}
	} else {
		if $Stat($Sok,M) {
			# just exited
			set Stat($Sok,M) 0
			set Stat($Sok,F) 0
			endLine $Wn.t
		}
		# we have something
		append Stat($Sok,B) $chunk
		# look for CR+LF, LF+CR, CR, LF; if there is only
		# one of those at the end, ignore it for now and
		# keep for posterity
		set sl [string length $Stat($Sok,B)]

		while { [regexp "\[\r\n\]" $Stat($Sok,B) m] } {
			set el [string first $m $Stat($Sok,B)]
			if { $el == 0 } {
				# first character
				if { $sl < 2 } {
					# have to leave it and wait
					return
				}
				# check the second one
				set n [string index $Stat($Sok,B) 1]
				if { $m == "\r" && $n == "\n" || \
				     $m == "\n" && $n == "\r"    } {
					# two-character EOL
					set Stat($Sok,B) \
					      [string range $Stat($Sok,B) 2 end]
					incr sl -2
				} else {
					set Stat($Sok,B) \
					      [string range $Stat($Sok,B) 1 end]
					incr sl -1
				}
				# complete previous line
				endLine $Wn.t
				set Stat($Sok,F) 0
				continue
			}
			# send the preceding string to the terminal
			addText $Wn.t [string range $Stat($Sok,B) \
						       0 [expr $el - 1]]
			incr sl -$el
			# in the ASCII mode, this is only used to tel
			# wheter we are at the beginning of a line or
			# not
			incr Stat($Sok,F) 1
			set Stat($Sok,B) [string range $Stat($Sok,B) $el end]
		}
		if { $Stat($Sok,B) != "" } {
			addText $Wn.t $Stat($Sok,B)
			incr Stat($Sok,F) 1
			set Stat($Sok,B) ""
		}
	}
}

proc uuRead { Sok } {
#
# U-U input
#
	global Stat

	set sok $Stat($Sok,FD,U)

	if [catch { read $sok } chunk] {
		# assume disconnection
		log "connection to U-U device[uuName $Sok] terminated: $chunk"
		uuStop $Sok
		return
	}

	if [eof $sok] {
		# closed
		log "connection to U-U device[uuName $Sok] closed"
		uuStop $Sok
		return
	}

	if { $chunk == "" } {
		# nothing read
		return
	}

	catch { puts -nonewline $Sok $chunk }
}

proc uartEvnt { Sok } {
#
# We need this nonsense because button release in tk_optionMenu returns the
# old value of the option
#
	after 100 "connUart $Sok"
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

proc connUart { Sok } {
#
# U-U connection/re-connection/drop
#
	global Stat

	if ![info exists Stat($Sok,U)] {
		# we are delayed, so let us check for sure
		return
	}

	set opt $Stat($Sok,U)

	if { $opt == "Off" && $Stat($Sok,Q) == "" } {
		# nothing
		return
	}

	if { $opt == $Stat($Sok,Q) } {
		# nothing again
		return
	}

	# stop any previous connection
	uuStop $Sok

	if { $opt == "Off" } {
		# that's it
		log "U-U connection terminated"
		return
	}

	set devlist [u_cdevl $opt]

	set fail 1

	foreach dev $devlist {
		if ![catch { open $dev "r+" } fd] {
			set fail 0
			break
		}
	}

	if $fail {
		log "cannot open U-U device $opt: $fd"
		set Stat($Sok,U) "Off"
		return
	}

	# we shall ignore bit rate issues (at least for now), but make it large
	# just in case
	set mode "$Stat($Sok,R),n,8,1"
	fconfigure $fd -mode $mode -handshake none \
		-buffering none -translation binary -encoding binary \
			-blocking 0 -eofchar ""

	fileevent $fd readable "uuRead $Sok"
	set Stat($Sok,FD,U) $fd
	set Stat($Sok,Q) $opt

	log "U-U connection successful"
}

proc mkTerm { Sok tt hex } {
#
# Creates a new terminal
#
	global Wins Stat

	set w $Wins($Sok)

	toplevel $w

	wm title $w $tt

	text $w.t


	#	-xscrollcommand "$w.scrolx set" 

	$w.t configure \
		-yscrollcommand "$w.scroly set" \
		-setgrid true \
        	-width 80 -height 24 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$w.t delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"
	# scrollbar $w.scrolx -orient horizontal -command "$w.t xview"

	pack $w.scroly -side right -fill y
	# pack $w.scrolx -side bottom -fill x

	pack $w.t -expand yes -fill both

	frame $w.stat -borderwidth 2
	pack $w.stat -expand no -fill x

	text $w.stat.u -height 1 -font {-family courier -size 10}
	pack $w.stat.u -side left -expand yes -fill x

	bind $w.stat.u <Return> "handleUserInput $Sok"

	frame $w.stat.hsel -borderwidth 2
	pack $w.stat.hsel -side right -expand no

	label $w.stat.hsel.lab -text "HEX"
	pack $w.stat.hsel.lab -side left

	set Stat($Sok,H) $hex
	# U-U file descriptor
	set Stat($Sok,FD,U) ""

	checkbutton $w.stat.hsel.but -state normal -variable Stat($Sok,H)
	pack $w.stat.hsel.but -side left

	frame $w.stat.usel -borderwidth 2
	pack $w.stat.usel -side right -expand no

	label $w.stat.usel.lab -text "U-U"
	pack $w.stat.usel.lab -side left

	set pl "Off CNCB0 CNCB1 CNCB2 CNCB3"
	for { set i 0 } { $i <= 20 } { incr i } {
		append pl " $i"
	}

	eval "tk_optionMenu $w.stat.usel.men Stat($Sok,U) $pl"

	set Stat($Sok,U) "Off"
	# U-U UART device name for diagnostics
	set Stat($Sok,Q) ""

	pack $w.stat.usel.men -side left

	bind $w.stat.usel.men <B1-ButtonRelease> "uartEvnt $Sok"

	$w.t configure -state disabled

	bind $w <Destroy> "destroyWindow $Sok"

	return $w.t
}

proc handleUserInput { Sok } {

	global Stat

	set w [stwin $Sok]

	set tx ""
	# extract the line
	regexp "\[^\r\n\]+" [$w.stat.u get 0.0 end] tx
	# remove it from the input field
	$w.stat.u delete 0.0 end

	if $Stat($Sok,F) {
		set Stat($Sok,F) 0
		endLine $w.t
	}

	if $Stat($Sok,H) {
		set Stat($Sok,M) 1
		addText $w.t $tx
		endLine $w.t
		set os ""
		set tx [string trim $tx]
		set err 0
		while { $tx != "" } {
			set d0 [string index $tx 0]
			set d1 [string index $tx 1]
			set tx [string trimleft [string range $tx 2 end]]
			set d0 [htodec $d0]
			if { $d0 < 0 } {
				set err 1
				break
			}
			set d1 [htodec $d1]
			if { $d1 < 0 } {
				set err 1
				break
			}
			append os [format %c [expr $d0 * 16 + $d1]]
			#abinB os [expr $d0 * 10 + $d1]
		}
		if $err {
			addText $w.t "ILLEGAL HEX INPUT, LINE IGNORED!!!"
			endLine $w.t
			return
		}
		if [catch { puts -nonewline $Sok $os } err] {
			log "connection to UART[stName $Sok] closed: $err"
			dealloc $Sok
			return
		}
	} else {
		set Stat($Sok,M) 0
		# ASCII
		if [catch { puts -nonewline $Sok "${tx}\r\n" } err] {
			log "connection to UART[stName $Sok] closed: $err"
			dealloc $Sok
			return
		}
		# echo
		addText $w.t $tx
		endLine $w.t
	}
}

proc addText { w txt } {

	$w configure -state normal
	$w insert end "$txt"
	$w configure -state disabled
	$w yview -pickplace end

}

proc endLine { w } {

	global	MaxLineCount
	
	$w configure -state normal
	$w insert end "\n"

	set ix [$w index end]
	set ix [string range $ix 0 [expr [string first "." $ix] - 1]]

	if { $ix > $MaxLineCount } {
		# scroll out the topmost line if above limit
		$w delete 1.0 2.0
	}

	$w configure -state disabled
	# make sure the last line is displayed
	$w yview -pickplace end

}

proc clockHandler { } {

	global PortNumber HostName Wins

	if [winlocate "c" ""] {
		return
	}

	set Wn ""
	
	log "connecting to CLOCK ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	# stub
	set Wins($Sok) "x"

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 6"
	fileevent $Sok readable "clockIni $Sok"
}

proc clockIni { Sok } {

	global Wins Stat Unique ECONN_OK CTime

	# we are reading the initial OK response
	ctimeout $Sok

	# connection code
	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]

	if { $code != $ECONN_OK } {
		dealloc $Sok
		log "connection rejected by SIDE: [conerror $code]"
		return
	}

	# create the window

	set Wins($Sok) ".wc$Unique"
	incr Unique

	set Stat($Sok,B) ""
	set Stat($Sok,F) 0

	set CTime(sec) 0
	set CTime(tim) ""
	set CTime(buf) ""

	mkClock $Sok

	# reset the input script
	fileevent $Sok readable "clockRead $Sok"

	log "connected"
}

proc clockRead { Sok } {

	global CTime

	set rd [expr 4 - [string length $CTime(buf)]]

	if [catch { read $Sok $rd } chunk] {
		# assume disconnection
		log "CLOCK connection terminated: $chunk"
		dealloc $Sok
		return
	}

	if [eof $Sok] {
		# closed
		log "CLOCK connection closed"
		dealloc $Sok
		return
	}

	append CTime(buf) $chunk
	if { [string length $chunk] != $rd } {
		return
	}

	set CTime(sec) [dbinI CTime(buf)]
 	set CTime(tim) [sectos $CTime(sec)]

	showClock $Sok
}

proc sectos { sec } {

	set hr [expr ($sec / 3600) % 24]
	set mn [expr ($sec / 60) % 60]
	set se [expr ($sec % 60)]

	if [expr $se & 0x01] {
		return [format "%02d:%02d:%02d" $hr $mn $se]
	} else {
		return [format "%02d %02d %02d" $hr $mn $se]
	}
}

proc showClock { Sok } {

	global CTime LCDSHAPE LCDONITEMS LCDOFFITEMS LCDCOLORS

	set w [stwin $Sok]

	$w.c delete lcd
	set offset 8

	foreach char [split $CTime(tim) ""] {

		if { $char == ":" || $char == " " } {
			if { $char == ":" } {
				$w.c move [eval $w.c create polygon \
					$LCDSHAPE(h) -tags lcd \
						-outline $LCDCOLORS(ONE) \
						-fill $LCDCOLORS(ONI) ] \
							$offset 4
				$w.c move [eval $w.c create polygon \
					$LCDSHAPE(i) -tags lcd \
						-outline $LCDCOLORS(ONE) \
						-fill $LCDCOLORS(ONI) ] \
							$offset 4
			} else {
				$w.c move [eval $w.c create polygon \
					$LCDSHAPE(h) -tags lcd \
						-outline $LCDCOLORS(OFE) \
						-fill $LCDCOLORS(OFI) ] \
							$offset 4
				$w.c move [eval $w.c create polygon \
					$LCDSHAPE(i) -tags lcd \
						-outline $LCDCOLORS(OFE) \
						-fill $LCDCOLORS(OFI) ] \
							$offset 4
			}
        		incr offset 8
			continue
		}

	        foreach sym $LCDONITEMS($char) {
            		$w.c move [eval $w.c create polygon $LCDSHAPE($sym) \
				-tags lcd \
                    		-outline $LCDCOLORS(ONE) \
				-fill $LCDCOLORS(ONI)] $offset 4
        	}
	        foreach sym $LCDOFFITEMS($char) {
            		$w.c move [eval $w.c create polygon $LCDSHAPE($sym) \
				-tags lcd \
                    		-outline $LCDCOLORS(OFE) \
				-fill $LCDCOLORS(OFI)] $offset 4
        	}

		incr offset 24
	}
}

proc mkClock { Sok } {
#
# Creates a clock
#
	global Wins

	set w $Wins($Sok)
	toplevel $w

	wm title $w "CLOCK"

	canvas $w.c -width 170 -height 45 -bg black
	pack $w.c -expand y
	bind $w <Destroy> "destroyWindow $Sok"
}

proc ledsHandler { stid } {

	global PortNumber HostName Wins Stat

	if [winlocate "l" $stid] {
		return
	}

	set Wn ""
	
	log "connecting to LEDS at node $stid ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	set Wins($Sok) "x"
	set Stat($Sok,S) $stid

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 3"
	fileevent $Sok readable "ledsIni $Sok"
}

proc ledsIni { Sok } {

	global Stat ECONN_OK

	# we are reading the initial OK response
	ctimeout $Sok

	# connection code
	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]
	if { $code != $ECONN_OK } {
		dealloc $Sok
		log "connection rejected by SIDE: [conerror $code]"
		return
	}

	# wait for first update
	stimeout $Sok
	set Stat($Sok,B) ""
	fileevent $Sok readable "ledsFirst $Sok"
	log "connection established"
}

proc ledsFirst { Sok } {

	global Wins Stat Unique

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			dealloc $Sok
			log "connection failed: $ch"
			return
		}
		if { $ch == "" } {
			if [eof $Sok] {
				log "connection closed by peer"
				dealloc $Sok
			}
			# wait for more
			return
		}
		if { $ch == "\n" } {
			# end of command line
			break
		}
		append Stat($Sok,B) $ch
	}

	set cmd $Stat($Sok,B)
	set Stat($Sok,B) ""

	if { ![regexp ": (\[0-1\]) (\[0-2\]+)" $cmd junk Fast Leds] } {
		# ignore NOPs and illegal commands
		return
	}

	# terminate the timeout timer
	ctimeout $Sok

	# create the window
	set Wn ".wl$Unique"
	incr Unique

	set Wins($Sok) $Wn
	set Stat($Sok,D) 80

	mkLeds $Sok [string length $Leds] "$Stat($Sok,S) LEDS"

	ledsStatus $Sok $Fast $Leds

	fileevent $Sok readable "ledsUpdate $Sok"
}

proc ledsUpdate { Sok } {

	global Stat

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log \
			  "connection to LEDS[stName $Sok] terminated"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			# wait for more
			if [eof $Sok] {
				log \
				  "connection to LEDS[stName $Sok] terminated"
				dealloc $Sok
				return
			}
			set Stat($Sok,D) 80
			fileevent $Sok readable "ledsUpdate $Sok"
			return
		}
		if { $ch == "\n" } {
			# end of command line
			set nd [expr $Stat($Sok,D) - 10]
			if { $nd < 0 } {
				set nd 0
			}
			set Stat($Sok,D) $nd
			set cmd $Stat($Sok,B)
			set Stat($Sok,B) ""
			if { ![regexp ": (\[0-1\]) (\[0-2\]+)" $cmd junk Fast \
			   Leds] } {
				# ignore NOPs and illegal commands
				return
			}

			ledsStatus $Sok $Fast $Leds
			if { $nd > 0 } {
				fileevent $Sok readable ""
				after $nd ledsUpdate $Sok
				return
			}
			continue
		}
		append Stat($Sok,B) $ch
	}
}

proc mkoneled { canv off } {

	set t [$canv create oval $off 4 [expr 20 + $off] 24 -fill grey]
	return $t
}

proc mkLeds { Sok nleds tt } {
#
# Creates a new leds window
#
	global Wins Stat

	set w $Wins($Sok)

	toplevel $w

	wm title $w $tt

	canvas $w.c -width [expr 32 + $nleds * 24] -height 28

	# blinker callback
	set Stat($Sok,G) ""
	# blinker toggle
	set Stat($Sok,X) 1

	for { set i 0 } { $i < $nleds } { incr i } {
		set Stat($Sok,O,$i) [mkoneled $w.c [expr 16 + $i * 24]]
		set Stat($Sok,V,$i) 0
	}
	grid $w.c -row 0 -column 0
	bind $w <Destroy> "destroyWindow $Sok"
}

proc ledsStatus { Sok fast stat } {

	global Stat

	set w [stwin $Sok]

	set n [string length $stat]
	set blink 0

	for { set i 0 } { $i < $n } { incr i } {
		if ![info exists Stat($Sok,O,$i)] {
			# something wrong
			break
		}
		# new value
		set val [string index $stat $i]
		if { $val == 2 } {
			# some leds blink
			set blink 1
		}
		# old value
		set ova $Stat($Sok,V,$i)
		#
		if { $val != $ova } {
			# new value
			set Stat($Sok,V,$i) $val
			if { $val == 1 || ($val == 2 && $Stat($Sok,X)) } {
				# on
				$w.c itemconfigure $Stat($Sok,O,$i) -fill \
					[ledColor $i]
			} else {
				# off
				$w.c itemconfigure $Stat($Sok,O,$i) -fill grey
			}
		}
	}

	if $blink {
		if $fast {
			set Stat($Sok,J) 128
		} else {
			set Stat($Sok,J) 512
		}
		if { $Stat($Sok,G) == "" } {
			# start the blinker callback
			set Stat($Sok,G) [after $Stat($Sok,J) ledsBlinker $Sok]
		}
	} elseif { $Stat($Sok,G) != "" } {
		after cancel $Stat($Sok,G)
		set Stat($Sok,G) ""
	}
}

proc ledsBlinker { Sok } {

	global Stat

	if { ![info exists Stat($Sok,G)] || $Stat($Sok,G) == "" } {
		# we are gone
		return
	}

	set on $Stat($Sok,X)

	set w [stwin $Sok]

	for { set i 0 } { [info exists Stat($Sok,V,$i)] } { incr i } {
		if { $Stat($Sok,V,$i) == 2 } {
			if $on {
				$w.c itemconfigure $Stat($Sok,O,$i) -fill \
					[ledColor $i]
			} else {
				$w.c itemconfigure $Stat($Sok,O,$i) -fill grey
			}
		}
	}

	if $on {
		set Stat($Sok,X) 0
	} else {
		set Stat($Sok,X) 1
	}

	set Stat($Sok,G) [after $Stat($Sok,J) ledsBlinker $Sok]
}

proc pinsHandler { stid } {

	global PortNumber HostName Wins Stat

	if [winlocate "p" ""] {
		return
	}

	set Wn ""
	
	log "connecting to PINS at node $stid ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	set Wins($Sok) "x"
	set Stat($Sok,S) $stid

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 2"
	fileevent $Sok readable "pinsIni $Sok"
}

proc pinsIni { Sok } {

	global Stat ECONN_OK

	ctimeout $Sok

	# connection code
	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		# closed
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]
	if { $code != $ECONN_OK } {
		log "connection rejected by SIDE: [conerror $code]"
		dealloc $Sok
		return
	}

	# wait for first update
	stimeout $Sok
	set Stat($Sok,B) ""
	fileevent $Sok readable "pinsFirst $Sok"
}

proc pinsFirst { Sok } {

	global Wins Stat Unique

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log "connection failed: $ch"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			if [eof $Sok] {
				log "connection closed by peer"
				dealloc $Sok
			}
			# wait for more
			return
		}
		if { $ch == "\n" } {
			# end of command line
			break
		}
		append Stat($Sok,B) $ch
	}

	# terminate the timeout timer
	ctimeout $Sok

	set cmd $Stat($Sok,B)
	set Stat($Sok,B) ""

	if ![regexp "^N (\[0-9\]+) (\[0-9\]+)" $cmd junk np na] {
		log "incorrect handshake, pin count message expected"
		dealloc $Sok
		return
	}

	if { [catch { set np [expr $np] ; set na [expr $na] } ] || $np < 1 || 
	    $na > $np } {
		log "incorrect handshake, illegal pin numbers $np/$na"
		dealloc $Sok
		return
	}

	# create the window
	set Wn ".wp$Unique"
	incr Unique

	set Wins($Sok) $Wn
	set Stat($Sok,D) 80

	mkPins $Sok $np $na "PINS at node $Stat($Sok,S)"

	fileevent $Sok readable "pinsUpdate $Sok"

	log "connection established"
}

proc pinsUpdate { Sok } {

	global Stat

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log \
			  "connection to PINS[stName $Sok] terminated"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			# wait for more
			if [eof $Sok] {
				log "connection to PINS[stName $Sok] terminated"
				dealloc $Sok
				return
			}
			# revert to default damping delay
			set Stat($Sok,D) 80
			fileevent $Sok readable "pinsUpdate $Sok"
			return
		}
		if { $ch == "\n" } {
			# end of command line
			set nd [expr $Stat($Sok,D) - 10]
			if { $nd < 0 } {
				set nd 0
			}
			set Stat($Sok,D) $nd
			set cmd $Stat($Sok,B)
			set Stat($Sok,B) ""

			newPinStatus $Sok $cmd

			if { $nd > 0 } {
				fileevent $Sok readable ""
				after $nd pinsUpdate $Sok
				return
			}
			continue
		}
		append Stat($Sok,B) $ch
	}
}

proc pinsInput { Sok pin } {

	global Stat PIN_COLORS

	set w [stwin $Sok]

	set b $w.dig.i$pin
	# the previous value
	set ov $Stat($Sok,J$pin)
	if { $ov == "-" } {
		log "pin input ignored, pin $pin is not input"
		return
	}

	if $ov {
		set ov 0
	} else {
		set ov 1
	}

	if [catch { puts -nonewline $Sok "P $pin $ov\n" } ] {
		log "connection to PINS[stName $Sok] terminated"
		dealloc $Sok
		return
	}

	$b configure -text $ov -activebackground $PIN_COLORS(A) \
		-bg $PIN_COLORS(I,$ov)

	set Stat($Sok,J$pin) $ov
}

proc setPinDac { Sok pin v } {

	set w [stwin $Sok]

	# convert the value
	set v [expr int (($v / 3.3) * 32768.0)]
	if { $v > 32767 } {
		set v 32767
	}

	if [catch { puts -nonewline $Sok "D $pin $v\n" } ] {
		log "connection to PINS[stName $Sok] terminated"
		dealloc $Sok
	}
}

proc mkPins { Sok np na tt } {

	global Wins Stat

	set w $Wins($Sok)
	toplevel $w

	wm title $w $tt

	frame $w.dig -borderwidth 4
	pack $w.dig -side top -expand 1 -fill x

	set iw 3
	set ih 2

	set Stat($Sok,NP) $np
	set Stat($Sok,NA) $na

	for { set i 0 } { $i < $np } { incr i } {
		# numbers
		set c [label $w.dig.h$i -width $iw -height $ih -text $i]
		grid $c -column $i -row 0 -sticky nsew
		# directions
		set Stat($Sok,D$i) -1
		set c [label $w.dig.d$i -width $iw -height $ih -text "-" \
			 -relief raised]
		grid $c -column $i -row 1 -sticky nsew
		# output
		set Stat($Sok,O$i) -1
		set c [label $w.dig.o$i -width $iw -height $ih -text "-" \
			-relief sunken]
		grid $c -column $i -row 2 -sticky nsew
		# buttons
		set Stat($Sok,I$i) -1
		set c [button $w.dig.i$i -width $iw -height $ih -text "-" \
			-command "pinsInput $Sok $i" -state normal]
		set Stat($Sok,J$i) "-"
		grid $c -column $i -row 3 -sticky nsew
	}

	for { set i 0 } { $i < $np } { incr i } {
		grid rowconfigure $w.dig.d$i 0 -weight 1
		grid rowconfigure $w.dig.o$i 0 -weight 1
		grid rowconfigure $w.dig.i$i 0 -weight 1
	}


	# Analog input
	if { $na > 0 } {

		frame $w.adc -borderwidth 4
		pack $w.adc -side top -expand 0 -fill x

		for { set i 0 } { $i < $na } { incr i } {
			set Stat($Sok,A$i) 0
			set f [frame $w.adc.f$i -borderwidth 0]
			pack $f -side top -expand 0 -fill x
			scale $f.s -orient horizontal -length 200 \
				-from 0.0 -to 3.3 -resolution 0.01 \
				-command "setPinDac $Sok $i" -state normal
			pack $f.s -side right
			$f.s set 0.0
			label $f.l -text "ADC $i"
			pack $f.l -side right -expand 1 -fill x
		}
	}

	frame $w.dac -borderwidth 4 -relief raised
	pack $w.dac -side top -expand 0 -fill x

	# DAC output
	for { set i 0 } { $i < 2 } { incr i } {
		set f [frame $w.dac.f$i -borderwidth 0]
		pack $f -side left -expand 1 -fill x
		label $f.v -text "00.000 V" -relief sunken
		pack $f.v -side right
		label $f.l -justify left -text "DAC $i"
		pack $f.l -side left -expand 1 -fill x
	}

	bind $w <Destroy> "destroyWindow $Sok"
}

proc toVoltage { v } {

	if { $v > 32767 } {
		# this is in fact negative
		return 00.000
	}
	return [format %06.3f [expr 3.3 * ($v / 32768.0)]]
}

proc newPinStatus { Sok upd } {

	global Stat PIN_COLORS PIN_STCODE

	set w [stwin $Sok]

	if ![regexp ": (\[0-9\]+) (\[0-9\]+) (\[0-9\]+)" $upd jnk pin sta val] {
		# ignore
		return
	}

	# a few sanity checks
	set np $Stat($Sok,NP)
	set na $Stat($Sok,NA)

	if { [catch {
		set pin [expr $pin]
		set val [expr $val]
	     }] || $pin >= $np } {
	
		return
	}

	if ![regexp "^\[0-7\]$" $sta] {
		# incorrect status
		return
	}

	$w.dig.d$pin configure -bg $PIN_COLORS(D,$sta) -text $PIN_STCODE($sta)

	if { $sta == 0 || $sta == 5 || $sta == 6 } {
		# input
		if $val {
			# make sure this is zero or 1
			set $val 1
		}
		$w.dig.i$pin configure -activebackground $PIN_COLORS(A) \
			-bg $PIN_COLORS(I,$val) -text $val -state normal
		set Stat($Sok,J$pin) $val
	} else {
		$w.dig.i$pin configure -bg $PIN_COLORS(I,2) -text "-" \
			-state disabled
		set Stat($Sok,J$pin) "-"
	}

	if { $sta == 1 } {
		#output
		if $val {
			# make sure this is zero or 1
			set $val 1
		}
		$w.dig.o$pin configure -bg $PIN_COLORS(O,$val) -text $val
	} else {
		$w.dig.o$pin configure -bg $PIN_COLORS(O,2) -text "-"
	}

	if { $sta == 2 } {
		# ADC
		if { $pin >= $na } {
			return
		}
		set f $w.adc.f$pin
		$f.s configure -state normal -activebackground $PIN_COLORS(A) \
			-bg $PIN_COLORS(N)
		$f.s set [toVoltage $val]
	} elseif { $pin < $na } {
		$w.adc.f${pin}.s configure -state disabled -bg $PIN_COLORS(I)
	}

	if { $sta == 3 } {
		$w.dac.f0.v configure -text [toVoltage $val]
	} elseif { $sta == 4 } {
		$w.dac.f1.v configure -text [toVoltage $val]
	}
}

proc sensorsHandler { stid } {

	global PortNumber HostName Wins Stat

	if [winlocate "s" ""] {
		return
	}

	set Wn ""

	log "connecting to SENSORS at node $stid ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	set Wins($Sok) "x"
	set Stat($Sok,S) $stid

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 7"
	fileevent $Sok readable "sensorsIni $Sok"
}

proc sensorsIni { Sok } {

	global Stat ECONN_OK

	ctimeout $Sok

	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		# closed
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]
	if { $code != $ECONN_OK } {
		log "connection rejected by SIDE: [conerror $code]"
		dealloc $Sok
		return
	}

	# wait for first update
	stimeout $Sok
	set Stat($Sok,B) ""
	fileevent $Sok readable "sensorsFirst $Sok"
}

proc sensorsFirst { Sok } {

	global Wins Stat Unique

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log "connection failed: $ch"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			if [eof $Sok] {
				log "connection closed by peer"
				dealloc $Sok
			}
			# wait for more
			return
		}
		if { $ch == "\n" } {
			# end of command line
			break
		}
		append Stat($Sok,B) $ch
	}

	ctimeout $Sok

	set cmd $Stat($Sok,B)
	set Stat($Sok,B) ""

	# This must be the first update with parameters
	if ![regexp "^N (\[0-9\]+) (\[0-9\]+)" $cmd junk na ns] {
		log "incorrect handshake, sensor count message expected"
		dealloc $Sok
		return
	}

	if { [catch { set na [expr $na] ; set ns [expr $ns] } ] } {
		log "incorrect handshake, illegal sensor numbers $na/$ns"
		dealloc $Sok
		return
	}

	# create the window
	set Wn ".wp$Unique"
	incr Unique

	set Wins($Sok) $Wn
	# damping delay
	set Stat($Sok,D) 80

	mkSensors $Sok $ns $na "SENSORS at node $Stat($Sok,S)"

	fileevent $Sok readable "sensorsUpdate $Sok"

	log "connection established"
}

proc mkSensors { Sok ns na tt } {

	global Wins Stat MAXSVAL PIN_COLORS

	set w $Wins($Sok)
	toplevel $w

	wm title $w $tt

	set Stat($Sok,NS) $ns
	set Stat($Sok,NA) $na

	if { $na > 0 } {

		if { $ns > 0 } {
			frame $w.act -borderwidth 4 -pady 4 -relief sunken
		} else {
			frame $w.act -borderwidth 4
		}
		pack $w.act -side top -expand 0 -fill x

		for { set i 0 } { $i < $na } { incr i } {
			set f [frame $w.act.f$i -borderwidth 0]
			pack $f -side top -expand 0 -fill x
			# min/max/value: initialized to unusable
			set Stat($Sok,AC$i) ""
			scale $f.s -orient horizontal -from 0.0 -length 300 \
				-command {} -state disabled \
				-sliderlength 7
			# rescale them later once we know everything about
			# the window
			pack $f.s -side right -expand 1 -fill x
			$f.s set 0.0
			label $f.l -text "Act $i"
			pack $f.l -side right -expand 0 -fill x
		}
	}

	if { $ns > 0 } {

		if { $na > 0 } {
			frame $w.sen -borderwidth 4 -pady 4 -relief sunken
		} else {
			frame $w.sen -borderwidth 4
		}
		pack $w.sen -side top -expand 0 -fill x

		for { set i 0 } { $i < $ns } { incr i } {
			set f [frame $w.sen.f$i -borderwidth 0]
			pack $f -side top -expand 0 -fill x
			# min/max/value
			set Stat($Sok,SE$i) ""
			scale $f.s -orient horizontal -from 0.0 -length 300 \
				-command "setSensor $Sok $i" -state disabled
			pack $f.s -side right -expand 1 -fill x
			$f.s set 0.0
			label $f.l -text "Sen $i"
			pack $f.l -side right -expand 0 -fill x
		}
	}

	bind $w <Destroy> "destroyWindow $Sok"
	bind $w <B1-ButtonRelease> "sensorSend $Sok"
}

proc sensorsUpdate { Sok } {

	global Stat

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log \
			  "connection to SENSORS[stName $Sok] terminated"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			# wait for more
			if [eof $Sok] {
			     	log \
				 "connection to SENSORS[stName $Sok] terminated"
				dealloc $Sok
				return
			}
			# revert to default damping delay
			set Stat($Sok,D) 80
			fileevent $Sok readable "sensorsUpdate $Sok"
			return
		}
		if { $ch == "\n" } {
			# end of command line
			set nd [expr $Stat($Sok,D) - 10]
			if { $nd < 0 } {
				set nd 0
			}
			set Stat($Sok,D) $nd
			set cmd $Stat($Sok,B)
			set Stat($Sok,B) ""

			newSensorStatus $Sok $cmd

			if { $nd > 0 } {
				fileevent $Sok readable ""
				after $nd sensorsUpdate $Sok
				return
			}
			continue
		}
		append Stat($Sok,B) $ch
	}
}

proc newSensorStatus { Sok upd } {

	global Stat

	set w [stwin $Sok]

	set par ""
	if ![regexp "^(\[AS\])\[^:\]+: (\[0-9\]+) (\[^ \]+)(.*)" $upd jnk \
	    typ sen val max] {
		# ignore
		return
	}

	if [catch {
		set sen [expr $sen]
		# this (at least in version 8.4) treats val as unsigned, i.e.,
		# the result is alwayn nonnegative; good!
		set val [expr double (0x$val)]
	     }] {
		# garbage
		return
	}

	if { $typ == "A" } {
		if { $sen >= $Stat($Sok,NA) } {
			# illegal
			return
		}
		set xx "AC$sen"
		set f "$w.act.f$sen"
	} else {
		if { $sen >= $Stat($Sok,NS) } {
			return
		}
		set xx "SE$sen"
		set f "$w.sen.f$sen"
	}

	if { $Stat($Sok,$xx) == "" } {
		# undefined yet, check for max specification
		set max [string trim $max]
		if { $max == "" } {
			return
		}
		if [catch {
			set max [expr double (0x$max)]
		   }] {
			return
		}
		if { $max <= 0.0 } {
			# a precaution
			return
		}
		set Stat($Sok,$xx) $max
		$f.s configure -to $max -tickinterval [expr $max / 5.0]
		if { $typ != "A" } {
			# enable input
			$f.s configure -state normal
		}
	}

	if { $val > $Stat($Sok,$xx) } {
		# another precaution
		set val $Stat($Sok,$xx)
	}
	if { $typ == "A" } {
		$f.s configure -state normal
		$f.s set $val
		$f.s configure -state disabled
	} else {
		$f.s set $val
	}
}

proc setSensor { Sok sen v } {

	global Stat

	set w [stwin $Sok]

	set Stat($Sok,SEV,$sen) [list $sen $v]
}

proc sensorSend { Sok } {

	global Stat

	foreach va [array names Stat "$Sok,SEV,*"] {
		set sn [lindex $Stat($va) 0]
		set sv [lindex $Stat($va) 1]
		set ln "S $sn [format 0x%1x [expr int ($sv)]]\n"
		if [catch { puts -nonewline $Sok $ln } ] {
			log "connection to SENSORS[stName $Sok] terminated"
			dealloc $Sok
			return
		}
	}
}

proc panelHandler { } {

	global PortNumber HostName Wins

	if [winlocate "a" ""] {
		return
	}

	set Wn ""

	log "connecting to PANEL ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	# stub
	set Wins($Sok) "x"

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 5"
	fileevent $Sok readable "panelIni $Sok"
}

proc panelIni { Sok } {

	global Stat ECONN_OK

	ctimeout $Sok

	# connection code
	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		# closed
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]
	if { $code != $ECONN_OK } {
		log "connection rejected by SIDE: [conerror $code]"
		dealloc $Sok
		return
	}

	# issue a request for node 0, which is the default display
	set Stat($Sok,NN) -1

	log "receiving status info ..."

	fileevent $Sok readable "panelZero $Sok"

	# query for node 0
	if [catch { puts -nonewline $Sok "Q 0\n" } res] {
		log "connection aborted: $res"
		dealloc $Sok
		return
	}

	stimeout $Sok
}

proc panelZero { Sok } {

	global Wins Stat HPA Unique NTYPE_COLORS

	if ![info exists Stat($Sok,NN)] {
		# sanity check
		return
	}

	# read the response line
	while 1 {

		if [catch { read $Sok 1 } ch] {
			# disconnection
			log "connection failed: $ch"
			dealloc $Sok
			return
		}

		if { $ch == "" } {
			if [eof $Sok] {
				log "connection closed by peer"
				dealloc $Sok
			}
			# wait for more
			return
		}

		if { $ch == "\n" } {

			# a new line
			set cmd [string trim $Stat($Sok,B)]
			set Stat($Sok,B) ""
			set nnm ""
			if ![regexp "^(\[0-9\]+) (\[FO\])(.*)" $cmd junk nod \
			    sta nnm] {
				continue
			}

			if [catch { set nod [expr $nod] } ] {
				# ignore
				continue
			}

			if { $Stat($Sok,NN) < 0 } {
				# waiting for the number of nodes to display
				# the window for the first time
				if { $nod != 0 } {
					continue
				}
				if ![regexp "(\[0-9\]+) (.*)" $nnm junk tot \
				    nnm] {
					continue
				}
				if [catch { set tot [expr $tot] } ] {
					continue
				}
				if { $tot < 0 } {
					continue
				}
				# start the window
				ctimeout $Sok
				set Stat($Sok,NN) $tot
				set nnm [string trim $nnm]
				# current node list
				set Stat($Sok,SL) [list [list 0 $sta $nnm]]
				# current displayed list
				set Stat($Sok,DL) ""
				# asked for (waiting to be included)
				set Stat($Sok,EL) ""
				set Wins($Sok) ".wa$Unique"
				incr Unique
				log "connection established"
				updPanel $Sok
				continue
			}

			# the window is up and running, so this must be an
			# update; look up the node number

			set nl ""
			set up 0
			foreach no $Stat($Sok,SL) {
				if { $nod == [lindex $no 0] } {
					set up 1
					if { $sta != [lindex $no 1] } {
						# actual status change
						set up 2
						lappend nl [list $nod $sta \
							[lindex $no 2]]
					}
				} else {
					lappend nl $no
				}
			}

			if $up {
				# node found on the list
				if { $up > 1 } {
					# actual change
					set Stat($Sok,SL) $nl
					updPanel $Sok
				}
				continue
			}

			# node not found; check if this is a long status message
			if ![regexp "(\[0-9\]+) (.*)" $nnm junk tot nnm] {
				# nope, ignore it
				continue
			}

			# are we waiting for it?
			set ix [lsearch -exact $Stat($Sok,EL) $nod]
			if { $ix < 0 } {
				# we are not
				continue
			}

			# remove the node from the waiting list
			set Stat($Sok,EL) [lreplace $Stat($Sok,EL) $ix $ix]

			# and add it to the panel
			lappend Stat($Sok,SL) \
				[list $nod $sta [string trim $nnm]]

			updPanel $Sok
		}
		append Stat($Sok,B) $ch
	}
}

proc addPanel { Sok } {

	global Stat

	set w [stwin $Sok]

	if { ![info exists Stat($Sok,NA)] || $Stat($Sok,NA) == "" } {
		# no node
		log "node number required"
		return
	}

	if { [catch { expr $Stat($Sok,NA) } nod] || $nod < 0 } {
		# this is just a precaution
		log "illegal node number"
		return
	}

	if { $nod >= $Stat($Sok,NN) } {
		log "node number out of range"
		return
	}

	# check if the node is not already present
	foreach no $Stat($Sok,SL) {
		if { [lindex $no 0] == $nod } {
			log "node $nod already present in panel"
			return
		}
	}

	# OK, can request this node

	if [catch {
		puts -nonewline $Sok "Q $nod\n"
	} res] {
		log "connection aborted: $res"
		dealloc $Sok
	}

	# append the node to the awaited list
	if { [lsearch -exact $Stat($Sok,EL) $nod] < 0 } {
		lappend Stat($Sok,EL) $nod
	}
}

proc reqPanel { Sok ix rq } {

	global Stat

	set w [stwin $Sok]

	set no [lindex $Stat($Sok,SL) $ix]

	if { $no == "" } {
		log "request ignored, bad node index"
		return
	}

	if [catch {
		puts -nonewline $Sok "$rq [lindex $no 0]\n"
	} res] {
		log "connection aborted: $res"
	}
}

proc delPanel { Sok ix } {

	global Stat

	set w [stwin $Sok]

	set no [lindex $Stat($Sok,SL) $ix]

	if { $no == "" } {
		log "request ignored, bad node index"
		return
	}

	set Stat($Sok,SL) [lreplace $Stat($Sok,SL) $ix $ix]

	updPanel $Sok
}

proc moveHandler { } {

	global PortNumber HostName Wins Stat

	# FIXME: I don't want to implement too much at this moment as we are
	# going to revise the whole concept of network geometry by admitting
	# explicit loss matrices among node pairs. The proper implementation
	# of this part will wait until we figure out how to do that.

	if [winlocate "m" ""] {
		return
	}

	log "connecting to ROAMER ..."

	if [catch { socket -async $HostName $PortNumber } Sok] {
		log "connection failed: $Sok"
		return
	}

	if [catch { fconfigure $Sok -blocking 0 -buffering none -translation \
	    binary -encoding binary } err] {
		dealloc $Sok
		log "connection failed: $err"
		return
	}

	set Wins($Sok) "x"

	# send the request when the socket becomes writeable
	fileevent $Sok writable "sendReq $Sok 4"
	fileevent $Sok readable "moveIni $Sok"
}

proc moveIni { Sok } {

	global Stat ECONN_OK

	ctimeout $Sok

	# connection code
	if [catch { read $Sok 4 } res] {
		# disconenction
		dealloc $Sok
		log "connection failed: $res"
		return
	}

	if { $res == "" } {
		# closed
		log "connection closed by peer"
		dealloc $Sok
		return
	}

	set code [dbinI res]
	if { $code != $ECONN_OK } {
		log "connection rejected by SIDE: [conerror $code]"
		dealloc $Sok
		return
	}

	# must collect node info: total number of nodes unknown
	set Stat($Sok,NN) -1
	# number of node colors used so far, one color per node type
	set Stat($Sok,NT) 0
	# current node number requested
	set Stat($Sok,CI) 0
	# message acquisition buffer
	set Stat($Sok,B) ""

	log "receiving node positions ..."

	fileevent $Sok readable "moveGNP $Sok"
	# query for node 0
	if [catch { puts -nonewline $Sok "Q 0\n" } res] {
		log "connection aborted: $res"
		dealloc $Sok
		return
	}
	stimeout $Sok
}

proc moveGNP { Sok } {

	global Wins Stat HPA Unique NTYPE_COLORS

	if ![info exists Stat($Sok,NN)] {
		# play it safe
		return
	}

	# read the response line
	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log "connection failed: $ch"
			dealloc $Sok
			return
		}
		if { $ch == "" } {
			if [eof $Sok] {
				log "connection closed by peer"
				dealloc $Sok
			}
			# wait for more
			return
		}
		if { $ch == "\n" } {
			# a new line
			set cmd $Stat($Sok,B)
			set Stat($Sok,B) ""
			if ![regexp \
			   "^P $HPA(D) $HPA(D) $HPA(F) $HPA(F) (\[^ \]+)" \
			      $cmd junk nod tot x y tn] {
				continue
			}

			if [catch {
				set nod [expr $nod]
				set tot [expr $tot]
			} ] {
				continue
			}

			if { $nod != $Stat($Sok,CI) } {
				# the expected node number
				log \
				   "illegal or out of sequence node number $nod"
				dealloc $Sok
				return
			}

			if { $Stat($Sok,NN) < 0 } {
				set Stat($Sok,NN) $tot
			} elseif { $Stat($Sok,NN) != $tot } {
				log "illegal response '$cmd'"
				dealloc $Sok
				return
			}

			if [catch {
				set x [expr $x]
				set y [expr $y]
			} ] {
				log "illegal node coordinates <$x, $y>"
				dealloc $Sok
				return
			}

			if { $x < 0.0 || $y < 0.0 } {
				log "negative node coordinate <$x, $y>"
				dealloc $Sok
				return
			}

			# we have received a new sample
			ctimeout $Sok

			# node type color
			if ![info exists Stat($Sok,NT,$tn)] {
				# this is a new type
				if { $Stat($Sok,NT) >= \
				    [llength $NTYPE_COLORS] } {
					# no more colors
					set co "grey"
					set Stat($Sok,NT,$tn) $co
				} else {
					set co \
					   [lindex $NTYPE_COLORS $Stat($Sok,NT)]
					set Stat($Sok,NT,$tn) $co
					incr Stat($Sok,NT)
				}
			} else {
				set co $Stat($Sok,NT,$tn)
			}
	
			set Stat($Sok,NL,$nod) [list $x $y $tn $co ""]

			incr Stat($Sok,CI)
			if { $Stat($Sok,CI) == $tot } {
				# done
				unset Stat($Sok,CI)
				fileevent $Sok readable "moveUpd $Sok"
				set Wins($Sok) ".wm$Unique"
				incr Unique
				log "connection established"
				mkMove $Sok
				return
			}
			if [catch {
				puts -nonewline $Sok "Q $Stat($Sok,CI)\n"
			} res] {
				log "connection aborted: $res"
				dealloc $Sok
				return
			}
			continue

		}
		append Stat($Sok,B) $ch
	}
}

proc xtocv { Sok x } {

	global Stat CMARGIN

	return [expr round (($Stat($Sok,M,w) * \
		($x - $Stat($Sok,M,XL)))/$Stat($Sok,M,W)) + $CMARGIN(L)]
}

proc ytocv { Sok y } {

	global Stat CMARGIN

	set ch $Stat($Sok,M,h)
	return [expr $ch - round (($ch * \
		($y - $Stat($Sok,M,YL)))/$Stat($Sok,M,H)) + $CMARGIN(U)]
}

proc xfromcv { Sok nx } {

	global Stat CMARGIN

	return [expr ($Stat($Sok,M,W) * \
		double ($nx - $CMARGIN(L))) / double ($Stat($Sok,M,w)) + \
			$Stat($Sok,M,XL)]
}

proc yfromcv { Sok ny } {

	global Stat CMARGIN

	set ch $Stat($Sok,M,h)
	return [expr ($Stat($Sok,M,H) * \
		double ($ch - $ny + $CMARGIN(U))) / double ($ch) + \
			$Stat($Sok,M,YL)]
}

proc cvcalc { Sok { cre 0 }} {
#
# Recalculate canvas parameters based on current network bounds
#
	global Stat CMARGIN

	# calculate the minimum and maximum network coordinates
	set XL ""
	set XH ""
	set YL ""
	set YH ""

	for { set n 0 } { $n < $Stat($Sok,NN) } { incr n } {
		set p $Stat($Sok,NL,$n)
		set x [lindex $p 0]
		set y [lindex $p 1]
		if { $XL == "" || $XL > $x } {
			set XL $x
		}
		if { $XH == "" || $XH < $x } {
			set XH $x
		}
		if { $YL == "" || $YL > $y } {
			set YL $y
		}
		if { $YH == "" || $YH < $y } {
			set YH $y
		}
	}

	set W [expr $XH - $XL]
	set H [expr $YH - $YL]

	# make it at least one meter on each dimension
	if { $W < 1.0 } {
		set D [expr (1.0 - $W) / 2.0]
		set XL [expr $XL - $D]
		set XH [expr $XH + $D]
		set W [expr $XH - $XL]
	}

	if { $H < 1.0 } {
		set D [expr (1.0 - $H) / 2.0]
		set YL [expr $YL - $D]
		set YH [expr $YH + $D]
		set H [expr $YH - $YL]
	}

	# absolute parameters of the canvas
	foreach { aw ah } $Stat($Sok,NC) { }

	# subtract the margins to obtain the active width and height
	set cw [expr $aw - $CMARGIN(L) - $CMARGIN(R)]
	set ch [expr $ah - $CMARGIN(U) - $CMARGIN(D)]

	if { $cw < 10 || $ch < 10 } {
		# and what to do now? formally, this is impossible because the
		# windows has constrained minimum dimensions of 200x200
		return
	}

	# make the coordinates fit

	if { [expr double ($ch) / double ($cw)] > \
	    [expr double ( $H) / double ( $W)] } {
		# constrained by width, calculate height increment
		set D [expr (((double ($W) * double ($ch)) / double ($cw)) \
			- $H) / 2.0]
		set YL [expr $YL - $D]
		set YH [expr $YH + $D]
		set H [expr $YH - $YL]
	} else {
		# vice versa
		set D [expr (((double ($H) * double ($cw)) / double ($ch)) \
			- $W) / 2.0]
		set XL [expr $XL - $D]
		set XH [expr $XH + $D]
		set W [expr $XH - $XL]
	}

	set Stat($Sok,M,XL) $XL
	set Stat($Sok,M,XH) $XH
	set Stat($Sok,M,YL) $YL
	set Stat($Sok,M,YH) $YH
	
	# network width and height
	set Stat($Sok,M,W) $W
	set Stat($Sok,M,H) $H

	# active canvas width and height
	set Stat($Sok,M,w) $cw
	set Stat($Sok,M,h) $ch
}

proc updPanel { Sok } {
#
# Make or re-make, e.g., after adding/deleting a node
#
	global Wins Stat PANEL_COLORS

	set w $Wins($Sok)

	if { $Stat($Sok,SL) == $Stat($Sok,DL) } {
		# no need to update
		return
	}

	set ne [llength $Stat($Sok,SL)]

	if { $ne != [llength $Stat($Sok,DL)] } {

		# redo the whole window
		if [catch { wm geometry $w } ge] {
			# window absent
			set ge ""
		} else {
			set Wins($Sok,NODESTROY) 1
			destroy $w
			unset Wins($Sok,NODESTROY)
		}

		toplevel $w
		wm title $w "PANEL"
		bind $w <Destroy> "destroyWindow $Sok"
		if { $ge != "" } {
			# preserve previous location
			regexp "\\+.*\\+.*" $ge ge
			wm geometry $w $ge
		}

		frame $w.stat -borderwidth 1
		pack $w.stat -side top -expand 1 -fill x

		for { set ix 0 } { $ix < $ne } { incr ix } {
			# create placeholders for the node descriptors
			set fr [frame $w.stat.pa$ix]
			pack $fr -side top -expand 1 -fill x
			label $fr.ide -text ""
			pack $fr.ide -side left
			button $fr.del -text "Delete" -state normal \
				-command "delPanel $Sok $ix" \
				-bg $PANEL_COLORS(DEL)
			button $fr.res -text "Reset" -state normal \
				-command "reqPanel $Sok $ix R" \
				-bg $PANEL_COLORS(ACTIVE)
			button $fr.off -text "Off" \
				-command "reqPanel $Sok $ix F"
			button $fr.on -text "On" \
				-command "reqPanel $Sok $ix O"
			pack $fr.del $fr.res $fr.off $fr.on -side right
		}

		frame $w.add -borderwidth 1
		pack $w.add -side top -expand 1 -fill x
		entry $w.add.non -width 6 -relief sunken \
			-textvariable Stat($Sok,NA) -validate key \
				-vcmd {validSid %P} -invcmd bell
		button $w.add.but -text "Add" -command "addPanel $Sok"
		pack $w.add.non $w.add.but -side left
	}

	# update labels and buttons
	set ix 0
	foreach no $Stat($Sok,SL) {
		set non [lindex $no 0]
		set nos [lindex $no 1]
		set not [lindex $no 2]
		set fr $w.stat.pa$ix
		$fr.ide configure -text [format "%4d: %s" $non $not]
		if { $nos == "F" } {
			# node is down: ON active, OFF disabled
			$fr.ide configure -bg $PANEL_COLORS(OFFLABEL)
			$fr.on configure -state normal -bg \
				$PANEL_COLORS(ACTIVE)
			$fr.off configure -state disabled -bg \
				$PANEL_COLORS(DISABLED)
		} else {
			# node is up: OFF active, ON disabled
			$fr.ide configure -bg $PANEL_COLORS(ONLABEL)
			$fr.on configure -state disabled -bg \
				$PANEL_COLORS(DISABLED)
			$fr.off configure -state normal -bg \
				$PANEL_COLORS(ACTIVE)
		}
		incr ix
	}
		
	set Stat($Sok,DL) $Stat($Sok,SL)
}

proc mkMove { Sok } {

	global Wins Stat CMARGIN

	set w $Wins($Sok)
	toplevel $w

	wm title $w "ROAMER"
	wm minsize $w $CMARGIN(MW) $CMARGIN(MH)

	# initial canvas size
	set Stat($Sok,NC) [list $CMARGIN(DW) $CMARGIN(DH)]

	# compute the canvas
	cvcalc $Sok 1

	foreach { aw ah } $Stat($Sok,NC) { }

	canvas $w.c -width $aw -height $ah
	pack $w.c -expand 1 -fill both

	moveRedraw $Sok

	bind $w <Destroy> "destroyWindow $Sok"
	bind $w.c <Configure> "moveResize $Sok %w %h"
}

proc moveRedraw { Sok } {

	global Stat CMARGIN

	set w [stwin $Sok]
	set N $Stat($Sok,NN)

	set W $Stat($Sok,M,W)
	set H $Stat($Sok,M,H)
	set dmsg [format "%4.3fm x %4.3fm" $W $H]
	foreach { aw ah } $Stat($Sok,NC) { }
	set ddx [expr $aw - $CMARGIN(RX)]
	set ddy [expr $ah - $CMARGIN(RY)]

	if ![info exists Stat($Sok,AX)] {
		# we are creating the canvas
		for { set n 0 } { $n < $N } { incr n } {
			foreach { x y z o t } $Stat($Sok,NL,$n) { }
			set cx [xtocv $Sok $x]
			set cy [ytocv $Sok $y]
			set o [$w.c create oval \
			    [expr $cx - $CMARGIN(NR)] \
			    [expr $cy - $CMARGIN(NR)] \
			    [expr $cx + $CMARGIN(NR)] \
			    [expr $cy + $CMARGIN(NR)] \
				-fill $o]
			$w.c bind $o <B1-Motion> "moveMove $Sok $n %x %y"
			$w.c bind $o <B1-ButtonRelease> "moveUpButton $Sok $n"
			$w.c bind $o <Enter> "moveEnter $Sok $n"
			$w.c bind $o <Leave> "moveLeave $Sok"
			# the node number
			set t [$w.c create text $cx [expr $cy - $CMARGIN(TO)] \
				-anchor s -text $n -state disabled]
			set Stat($Sok,NL,$n) [list $x $y $z $o $t]
		}
		set Stat($Sok,AX) [$w.c create text $ddx $ddy -anchor e \
			-text $dmsg -state disabled]
	} else {
		for { set n 0 } { $n < $N } { incr n } {
			foreach { x y z o t } $Stat($Sok,NL,$n) { }
			set cx [xtocv $Sok $x]
			set cy [ytocv $Sok $y]
  			$w.c coords $o \
			    [expr $cx - $CMARGIN(NR)] \
			    [expr $cy - $CMARGIN(NR)] \
			    [expr $cx + $CMARGIN(NR)] \
			    [expr $cy + $CMARGIN(NR)]
			$w.c coords $t $cx [expr $cy - $CMARGIN(TO)]
		}
		$w.c coords $Stat($Sok,AX) $ddx $ddy
		$w.c itemconfigure $Stat($Sok,AX) -text $dmsg
	}
}

proc moveEnter { Sok n } {

	global Stat CMARGIN

	set w [stwin $Sok]
	set p $Stat($Sok,NL,$n)

	set msg [format "<%4.3f,%4.3f>: %s" [lindex $p 0] [lindex $p 1] \
		[lindex $p 2]]

	if [info exists Stat($Sok,RC)] {
		$w.c delete $Stat($Sok,RC)
	}
	set ch $Stat($Sok,M,h)
	set Stat($Sok,RC) [$w.c create text $CMARGIN(RX) \
		[expr [lindex $Stat($Sok,NC) 1] - $CMARGIN(RY)] -anchor sw \
			-text $msg -state disabled]
}

proc moveLeave { Sok } {

	global Stat

	set w [stwin $Sok]

	if [info exists Stat($Sok,RC)] {
		$w.c delete $Stat($Sok,RC)
		unset Stat($Sok,RC)
	}
}

proc moveMove { Sok n nx ny } {

	global Stat CMARGIN

	set w [stwin $Sok]

	foreach { x y z o t } $Stat($Sok,NL,$n) { }

	# update the actual coordinates
	set x [xfromcv $Sok $nx]
	set y [yfromcv $Sok $ny]

	# make sure the network coordinates never get below zero
	if { $x < 0.0 } {
		set x 0.0
		set nx [xtocv $Sok 0.0]
	}

	if { $y < 0.0 } {
		set y 0.0
		set ny [ytocv $Sok 0.0]
	}

	# update the canvas coordinates
	$w.c coords $o \
	    [expr $nx - $CMARGIN(NR)] \
	    [expr $ny - $CMARGIN(NR)] \
	    [expr $nx + $CMARGIN(NR)] \
	    [expr $ny + $CMARGIN(NR)]

	set Stat($Sok,NL,$n) [list $x $y $z $o $t]

	if [catch { puts -nonewline $Sok "M $n $x $y\n" } res] {
		log "ROAMER connection aborted: $res"
		dealloc $Sok
		return
	}

	# update node label
	$w.c coords $t $nx [expr $ny - $CMARGIN(TO)]

	# update running coordinate display
	if [info exists Stat($Sok,RC)] {
		$w.c itemconfigure $Stat($Sok,RC) -text \
			[format "<%4.3f,%4.3f>: %s" $x $y $z]
	}

	# move flag for moveUpButton
	set Stat($Sok,MF) $n
}

proc moveShowDist { Sok n0 n1 } {

	global Stat

	set w [stwin $Sok]

	set p $Stat($Sok,NL,$n0)
	set x0 [lindex $p 0]
	set y0 [lindex $p 1]

	set p $Stat($Sok,NL,$n1)
	set x1 [lindex $p 0]
	set y1 [lindex $p 1]

	set X0 [xtocv $Sok $x0]
	set Y0 [ytocv $Sok $y0]
	set X1 [xtocv $Sok $x1]
	set Y1 [ytocv $Sok $y1]

	set d [expr sqrt (double ($x1 - $x0) * double ($x1 - $x0) + \
		double ($y1 - $y0) * double ($y1 - $y0))]

	set XM [expr round (($X0 + $X1) / 2.0)]
	set YM [expr round (($Y0 + $Y1) / 2.0)]
	set Stat($Sok,PU) [$w.c create text $XM $YM -text [format "%4.3fm" $d] \
		-state disabled]

	set Stat($Sok,PT) [$w.c create line $X0 $Y0 $X1 $Y1 -state disabled \
		-dash { 2 4 }]

	after 2000 moveClearDist $Sok
}

proc moveClearDist { Sok } {

	global Stat

	set w [stwin $Sok]

	if [info exists Stat($Sok,PT)] {
		$w.c delete $Stat($Sok,PT)
		$w.c delete $Stat($Sok,PU)
		unset Stat($Sok,PT)
		unset Stat($Sok,PU)
	}
}

proc moveUpButton { Sok n } {

	global Stat

	# just checking if we are still around
	stwin $Sok

	if ![info exists Stat($Sok,MF)] {
		# no move to complete
		if [info exists Stat($Sok,PS)] {
			if { $Stat($Sok,PS) == $n } {
				# cancel
				unset Stat($Sok,PS)
				return
			}
			# draw a line between the two nodes and display
			# the distance
			if [info exists Stat($Sok,PT)] {
				# previous distance still displayed
				return
			}
			moveShowDist $Sok $Stat($Sok,PS) $n
			unset Stat($Sok,PS)
			return
		}
		# the first point
		set Stat($Sok,PS) $n
		return
	}

	if [info exists Stat($Sok,PS)] {
		unset Stat($Sok,PS)
	}

	unset Stat($Sok,MF)

	foreach { x y z o t } $Stat($Sok,NL,$n) { }

	moveRedim $Sok $x $y
}

proc moveResize { Sok nw nh } {

	global Stat

	stwin $Sok

	if ![info exists Stat($Sok,RM)] {
		# this is a dummy first time after startup; we use it to
		# determine the window margin size, i.e., the difference
		# between the resized width and height and the actual
		# canvas parameters
		foreach { aw ah } $Stat($Sok,NC) { }
		set Stat($Sok,RM) [list [expr $nw - $aw] [expr $nh - $ah]]
		return
	}

	if [info exists Stat($Sok,T)] {
		# update timer running, kill it
		after cancel $Stat($Sok,T)
	}

	set Stat($Sok,NC) [list $nw $nh]

	# delay the actual action until we have stabilized
	set Stat($Sok,T) [after 1000 moveDoResize $Sok]

	# FIXME: how to avoid shrinking the canvas below a minimum decent size
}

proc moveDoResize { Sok } {

	global Stat

	stwin $Sok

	if [info exists Stat($Sok,T)] {
		unset Stat($Sok,T)
	}

	# correct for the window boundary
	foreach { aw ah } $Stat($Sok,NC) { }
	foreach { dw dh } $Stat($Sok,RM) { }

	set aw [expr $aw - $dw]
	set ah [expr $ah - $dh]

	set Stat($Sok,NC) [list $aw $ah]

	cvcalc $Sok

	moveRedraw $Sok
}

proc moveUpd { Sok } {
#
# read location updates
#
	global Stat HPA

	while 1 {
		if [catch { read $Sok 1 } ch] {
			# disconnection
			log \
			  "connection to ROAMER terminated"
			return
		}
		if { $ch == "" } {
			# wait for more
			if [eof $Sok] {
				log "connection to ROAMER terminated"
				dealloc $Sok
			}
			return
		}
		if { $ch == "\n" } {
			# end of line
			set cmd $Stat($Sok,B)
			set Stat($Sok,B) ""

			if ![regexp "^U $HPA(D) $HPA(F) $HPA(F)" \
			      $cmd junk nod x y] {
				continue
			}

			if { [catch {
				set nod [expr $nod]
				set x [expr $x]
				set y [expr $y]
			}] || $x < 0.0 || $y < 0.0 || $nod >= $Stat($Sok,NN) } {
				# just ignore
				continue
			}

			moveNewLocation $Sok $nod $x $y
			continue
		}
		append Stat($Sok,B) $ch
	}
}

proc moveRedim { Sok nx ny } {
#
# Checks if the location is out of present canvas bounds and optionally
# redimensions the canvas
#
	global Stat

	set XL $Stat($Sok,M,XL)
	set XH $Stat($Sok,M,XH)
	set YL $Stat($Sok,M,YL)
	set YH $Stat($Sok,M,YH)

	if { $nx < $XL || $nx > $XH || $ny < $YL || $ny > $YH } {
		cvcalc $Sok
		moveRedraw $Sok
		return 1
	}

	return 0
}

proc moveNewLocation { Sok nod nx ny } {

	global Stat CMARGIN

	set w [stwin $Sok]

	foreach { x y z o t } $Stat($Sok,NL,$nod) { }

	if { $x == $nx && $y == $ny } {
		# easy way out
		return
	}

	set Stat($Sok,NL,$nod) [list $nx $ny $z $o $t]

	if [moveRedim $Sok $nx $ny] {
		# done - the canvas has been redrawn
		return
	}

	# move just the one node

	set cx [xtocv $Sok $nx]
	set cy [ytocv $Sok $ny]

	$w.c coords $o \
	    [expr $cx - $CMARGIN(NR)] \
	    [expr $cy - $CMARGIN(NR)] \
	    [expr $cx + $CMARGIN(NR)] \
	    [expr $cy + $CMARGIN(NR)]

	$w.c coords $t $cx [expr $cy - $CMARGIN(TO)]
}

proc doAbort { Sok } {

	log "connection timed out"
	dealloc $Sok
}

proc destroyWindow { Sok } {

	global Wins Stat

	if [info exists Wins($Sok,NODESTROY)] {
		return
	}

	if [info exists Wins($Sok)] {
		unset Wins($Sok)
	}

	if ![regexp "^Zombie" $Sok] {
		dealloc $Sok
	}

}

proc log { txt } {
#
# Writes a line to log
#
	global Logger

	while 1 {

		set el [string first "\n" $txt]
		if { $el < 0 } {
			set el [string length $txt]
		}
		incr el -1
		set out [string range $txt 0 $el]
		incr el +2
		set txt [string range $txt $el end]
		addText $Logger "$out"
		endLine $Logger
		if { $txt == "" } {
			return
		}
	}
}

proc conerror { code } {

    switch -- $code {

	0 { return "protocol violation" }
	1 { return "node number out of range" }
	2 { return "unimplelented function" }
	3 { return "node has no UART" }
	4 { return "already connected to this component" }
	5 { return "node has no PINS module" }
	6 { return "node has no SENSORS module" }
	7 { return "server-side timeout" }
	8 { return "module has a non-socket interface" }
	9 { return "no leds module at this node" }
       10 { return "unexpected disconnection" }
       11 { return "request line too long" }
       12 { return "invalid request" }
    }

    return "error code $code (unknown)"
}

## binary encoding/decoding ###################################################

proc tohex { b } {

	scan $b %c v
	return [format %02x $v]
}
	
proc htodec { d } {
#
# Converts a hex digit to decimal
#
	global HDIGS

	if ![info exists HDIGS(0)] {
		# initialize
		set HDIGS(0) 0
		set HDIGS(1) 1
		set HDIGS(2) 2
		set HDIGS(3) 3
		set HDIGS(4) 4
		set HDIGS(5) 5
		set HDIGS(6) 6
		set HDIGS(7) 7
		set HDIGS(8) 8
		set HDIGS(9) 9
		set HDIGS(a) 10
		set HDIGS(b) 11
		set HDIGS(c) 12
		set HDIGS(d) 13
		set HDIGS(e) 14
		set HDIGS(f) 15
		set HDIGS(A) 10
		set HDIGS(B) 11
		set HDIGS(C) 12
		set HDIGS(D) 13
		set HDIGS(E) 14
		set HDIGS(F) 15
	}

	if ![info exists HDIGS($d)] {
		return -1
	}

	return $HDIGS($d)
}

proc abinB { s b } {
#
# append one binary byte to string s
#
	upvar $s str
	append str [binary format c $b]
}

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

proc dbinB { s } {
#
# decode one binary byte from string s
#
	upvar $s str
	if { $str == "" } {
		return -1
	}
	binary scan $str c val
	set str [string range $str 1 end]
	return [expr ($val & 0x000000ff)]
}

proc dbinS { s } {
#
# decode one binary short int from string s
#
	upvar $s str
	if { [string length $str] < 2 } {
		return -1
	}
	binary scan $str S val
	set str [string range $str 2 end]
	return [expr ($val & 0x0000ffff)]
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

proc valsid { } {

	global StatId

	if [catch { expr $StatId } num] {
		log "request ignored: illegal node Id '$StatId'"
		return 1
	}
	set StatId $num
	return 0
}

proc doConnect { } {
#
# Handle a new connection request
#
	global Option StatId AGENT_MAGIC

	switch $Option {

		"UART (ascii)"	{
			if [valsid] { return }
			uartHandler $StatId a
		}

		"UART (hex)"	{
			if [valsid] { return }
			uartHandler $StatId h
		}

		"SENSORS"	{
			if [valsid] { return }
			sensorsHandler $StatId
		}

		"PINS"	{
			if [valsid] { return }
			pinsHandler $StatId
		}

		"LEDS"	{ 
			if [valsid] { return }
			ledsHandler $StatId
		}

		"ROAMER" {

			moveHandler
		}

		"PANEL" {

			panelHandler 
		}

		"CLOCK"	{

			clockHandler
		}

		default {
			log "request ignored: $Option unimplemented"
		}
	}
}

proc validSid { v } {

	if { $v == "" } {
		return 1
	}
	if { [string length $v] > 6 } {
		return 0
	}
	if { $v == "0" } {
		return 1
	}
	return [regexp "^\[1-9\]\[0-9\]*$" $v]
}

wm title . "VUEE udaemon"

frame .top -borderwidth 10
pack .top -side top -expand 0 -fill x
button .top.quit -text "Quit" -command { destroy .top }
button .top.connect -text "Connect" -command doConnect

tk_optionMenu .top.select Option \
	"UART (ascii)" "UART (hex)" "SENSORS" "PINS" "LEDS" "ROAMER" "PANEL" \
		"CLOCK"

label .top.l -text "Node Id:"
entry .top.stat -width 6 -relief sunken -textvariable StatId \
	-validate key -vcmd {validSid %P} -invcmd bell

pack .top.quit .top.connect .top.select -side right
pack .top.l .top.stat -side left

bind .top.stat <Return> doConnect
bind .top <Destroy> { exit }
bind . <Destroy> { exit }

focus .top.stat

frame .logger

set Logger [text .logger.t -width 64 -height 10 \
	-borderwidth 2 -relief raised -setgrid true -wrap none \
	-yscrollcommand {.logger.scrolly set} \
	-xscrollcommand {.logger.scrollx set} \
	-font {-family courier -size 9} \
	-exportselection 1 \
	-state normal]

scrollbar .logger.scrolly -command {.logger.t yview}
scrollbar .logger.scrollx -orient horizontal -command {.logger.t xview}

pack .logger.scrolly -side right -fill y
pack .logger.scrollx -side bottom -fill x

pack .logger.t -side left -fill both -expand true

pack .logger -side top -fill both -expand true

$Logger delete 1.0 end
$Logger configure -state disabled

bind . <Destroy> { exit }

if { [info tclversion] < 8.5 } {

	alert "This program requires Tcl/Tk version 8.5 or higher!"
	exit 99
}

catch { close stdin }
catch { close stdout }
catch { close stderr }

vwait forever
