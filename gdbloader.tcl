#!/bin/sh
########\
exec tclsh85 "$0" "$@"

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
	# one have to fight when glueing together Windows and Cygwin stuff;
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

proc handle_args { } {

	global ST argv

	set ST(PDE) ""
	set ST(PAR) ""

	if { [lindex $argv 0] == "-D" } {
		# explicit device, this will be ignored under Cygwin
		set ST(PDE) [lindex $argv 1]
		set argv [lrange $argv 2 end]
	}

	# the remaining parameters are passed to gdbproxy
	set ST(PAR) [concat [list "msp430"] $argv]
}

handle_args

## Commands
set ProxyCmd "msp430-gdbproxy"
set GdbCmd "msp430-gdb"
## Only this works for reflashing the FET for now
set ProxyRefArgs "msp430 --update-usb-fet TIUSB"

## Duplicate Exit Avoidance Flag
set DEAF 0

## State:
##	BLA: blackout (waiting for something unable to respond)
##	OFF: proxy not running
##	PSU: proxy starting up
##	PUP: proxy up, no gdb (i.e., image not selected)
##	GSU: gdb starting up
##	GUP: gdb up, image selected, no command to gdb
##	ILO: image being loaded
##	REF: reflashing FET
##
## Do we want more? Like "node running"? Manual commands will screw it up
## anyway (unless we carefully monitor what is going on - we can do it later,
## if necessary, but not right away). We can use drastic actions to clear
## before button commands, e.g., kill before loading.
set ST(STA) "BLA"

proc oneof { st args } {

	foreach a $args {
		if { $a == $st } {
			return 1
		}
	}

	return 0
}

## File to upload (selected)
set ST(IMF) ""

## Last selected file
set ST(LUP) ""

## Proxy callback to detect timeouts + timeout values
set CB(PRO) ""
## Startup timeout
set CB(PRO,TS) [expr 1000 * 3]
## Reprogram timeout
set CB(PRO,TR) [expr 1000 * 10]

## GDB callback
set CB(GDB) ""
## Startup timeout
set CB(GDB,TS) [expr 1000 * 4]
## Load timeout
set CB(GDB,TL) [expr 1000 * 3]

###############################################################################

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

	tk_dialog [cw].alert "Attention!" "${msg}!" "" 0 "OK"
}

proc confirm { msg } {

	return [tk_dialog [cw].confirm "Warning!" $msg "" 0 "NO" "YES"]
}

proc kill_win_proc_by_name { name } {
#
# A desperate tool to kill something we have spawned, which has escaped, like
# gdbproxy, for instance
#
	global ST

	if { $ST(SYS) == "L" } {
		return
	}

	if [catch { exec ps -W } pl] {
		return
	}

	foreach ln [split $pl "\n"] {
		if ![regexp "(\[0-9\]+).*:..:..(.*)" $ln jnk pid cmd] {
			continue
		}
		if { [string first $name $cmd] >= 0 } {
			catch { exec kill -f $pid }
		}
	}
}

###############################################################################

proc delay { msec } {
#
# A variant of "after" admitting events while waiting
#
	global ST

	if { [info exists ST(DEL)] && $ST(DEL) != "" } {
		catch { after cancel $ST(DEL) }
	}

	set ST(DEL) [after $msec delay_trigger]
	vwait ST(DEL)
	catch { unset ST(DEL) }
}

proc delay_trigger { } {

	global ST

	if ![info exists ST(DEL)] {
		return
	}

	set ST(DEL) ""
}

proc terminate { } {

	global DEAF

	if $DEAF { return }
	term_stop 0
	term_stop 1
	set DEAF 1
	exit 0
}

###############################################################################

proc term_addtxt { t txt } {

	global Term

	set t $Term($t)

	$t configure -state normal
	$t insert end $txt
	$t configure -state disabled
	$t yview -pickplace end
}

proc term_endline { tt } {

	global Term

	set t $Term($tt)

	$t configure -state normal
	$t insert end "\n"

	# lines that are removed off the top
	while 1 {
		set ix [$t index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix < 1024 } {
			break
		}
		$t delete 1.0 2.0
	}

	$t configure -state disabled
	$t yview -pickplace end
}

proc term_dspline { tt ln } {

	global Term

	term_addtxt $tt $ln
	term_endline $tt
}

###############################################################################

proc run_reflash { } {
#
# Reprogram the FET
#
	global ST ProxyCmd ProxyRefArgs CB

	if { $ST(STA) != "OFF" } {
		# ignore, state must be OFF
		return
	}

	term_dspline 0 "--REFLASHING FET"

	if [run_term_command 0 $ProxyCmd $ProxyRefArgs read_proxy_line \
		proxy_exits] {
			# failed
			return
	}

	set CB(PRO) [after $CB(PRO,TR) proxy_startup_timeout]

	new_state "REF"
}

proc run_proxy { dev } {
#
# Try to start the proxy for the specified device
#
	global ST ProxyCmd CB

	if { $ST(STA) != "OFF" } {
		# ignore, state must be OFF
		return
	}

	term_dspline 0 "--STARTING PROXY"

	set al $ST(PAR)

	if { $dev != "" } {
		lappend al $dev
	}

	if [run_term_command 0 $ProxyCmd $al read_proxy_line proxy_exits] {
		# failed
		return
	}

	# set up a callback to detect a failure
	set CB(PRO) [after $CB(PRO,TS) proxy_startup_timeout]

	# state: starting up
	new_state "PSU"
}

proc proxy_startup_timeout { } {

	global CB

	set CB(PRO) ""
	term_stop 0
	term_dspline 0 "--PROXY RESPONSE TIMEOUT!"
	new_state "OFF"
}

proc run_term_command { t kmd al inp clo } {
#
# Run a terminal command:
#
#	- terminal
#	- command
#	- argument list
#	- input function
#	- closing function
#
	global Term ST

	if { $Term($t,P) != "" } {
		# pipe already set up; perhaps this should be an alert?
		term_dspline $t "--TERMINAL BUSY!"
		return 1
	}

	set ef [auto_execok $kmd]
	if { $ef == "" } {
		term_dspline $t "--CANNOT RUN $cmd (PGM NOT FOUND)!"
		return 1
	}
	if [file executable $ef] {
		set cmd "[list $ef]"
	} else {
		set cmd "[list sh] [list $ef]"
	}

	foreach a $al {
		append cmd " [list $a]"
	}

	# stderr to stdout
	append cmd " 2>@1"

	if [catch { open "|$cmd" "r+" } fd] {
		term_dspline $t "--FAILED TO START $cmd ($fd)"
		return 1
	}

	set Term($t,P) $fd
	set Term($t,I) $inp
	set Term($t,C) $clo
	set Term($t,K) $kmd

	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "term_input $t"

	return 0
}

proc term_input { t } {

	global Term

	if [catch { read $Term($t,P) } chunk] {
		# assume EOF
		term_stop $t
		return
	}

	if [eof $Term($t,P)] {
		term_stop $t
		return
	}

	if { $chunk == "" } {
		return
	}

	append Term($t,B) $chunk

	# split into lines
	while 1 {
		set el [string first "\n" $Term($t,B)]
		if { $el < 0 } {
			return
		}
		# get hold of the line; in case we are on Cygwin, trim out the
		# CR characters
		set ln [string trim [string range $Term($t,B) 0 [expr $el - 1]]\
			"\r"]
		set Term($t,B) [string range $Term($t,B) [expr $el + 1] end]
		term_dspline $t $ln
		if { $Term($t,I) != "" } {
			$Term($t,I) $ln
		}
	}
}

proc term_stop { t } {
#
# Executed when a terminal program stops
#
	global Term

	if { $Term($t,P) != "" } {
		kill_pipe $Term($t,P) $Term($t,K)
		set Term($t,P) ""
		set Term($t,I) ""
		if { $Term($t,C) != "" } {
			$Term($t,C)
			set Term($t,C) ""
		}
		term_dspline $t "--TERMINATED"
		delay 400
	}
}

proc kill_pipe { fd nam } {
#
# Kills the process on the other end of our pipe
#
	if { $fd == "" || [catch { pid $fd } pp] || $pp == "" } {
		return
	}
	foreach p $pp {
		catch { exec kill -f -$sig $p }
	}
	kill_win_proc_by_name $nam
	catch { close $fd }
}

###############################################################################

proc read_proxy_line { ln } {

	global ST CB

	if { $ST(STA) == "PSU" } {
		# proxy starting up, expect a connection message
		if { [string first ": waiting on TCP port" $ln] > 0 } {
			# got connected
			term_dspline 0 "--PROXY CONNECTION ESTABLISHED"
			catch { after cancel $CB(PRO) }
			set CB(PRO) ""
			new_state "PUP"
			# what if we see something like this in state > 1?
		}
		return
	}

	if { $ST(STA) == "ILO" } {
		# loading image
		if { [string first ": MSP430_Memory(WRITE)" $ln] > 0 } {
			# reset GDB timeout
			catch { after cancel $CB(GDB) }
			set CB(GDB) [after $CB(GDB,TL) gdb_load_abort]
		}
		return
	}

	if { $ST(STA) == "REF" } {
		# reflashing
		if { [string first "programmed" $ln] > 0 } {
			# reset the timeout
			catch { after cancel $CB(PRO) }
			set CB(PRO) [after $CB(PRO,TR) proxy_startup_timeout]
		} elseif { [string first "omplete" $ln] > 0 } {
			# done
			catch { after cancel $CB(PRO) }
			set CB(PRO) ""
			term_dspline 0 "--REPROGRAMMING COMPLETE"
			term_stop 0
			new_state "OFF"
		}
	}
}

proc proxy_exits { } {

	global ST CB

	if { $CB(PRO) != "" } {
		catch { after cancel $CB(PRO) }
		set CB(PRO) ""
	}
	# will kill gdb as well
	new_state "OFF"
}

###############################################################################

proc image_selection_click { } {
#
# Handles an event in the image selection list
#
	global ST

	if { $ST(IMF) != $ST(LUP) } {
		# a change of the image kills GDB
		if [oneof $ST(STA) "GSU" "GUP" "ILO"] {
			term_dspline 1 "--IMAGE CHANGE, GDB ABORTED"
			term_stop 1
			new_state "PUP"
		}
		set ST(LUP) $ST(IMF)
	}
}

proc start_gdb { } {
#
# Selects an image to load and starts gdb
#
	global ST GdbCmd CB

	if ![oneof $ST(STA) "PUP" "GUP"] {
		return
	}

	# if GDB is running, kill it first
	term_stop 1

	if { $ST(IMF) == "" } {
		term_dspline 1 "--NO IMAGES TO UPLOAD!"
		return
	}

	term_dspline 1 "--RUNNING GDB FOR $ST(IMF)"

	if [run_term_command 1 $GdbCmd [list $ST(IMF)] \
	    read_gdb_line gdb_exits] {
		# failed
		return
	}

	# set up a callback to detect a failure
	set CB(GDB) [after $CB(GDB,TS) gdb_startup_timeout]

	# state: starting up
	new_state "GSU"
}

proc gdb_startup_timeout { } {

	global ST CB

	set CB(GDB) ""
	term_stop 1
	term_dspline 1 "--GDB STARTUP TIMEOUT!"
	# FIXME: perhaps this should be smarter, e.g., detect if
	# the proxy is still around
	new_state "PUP"
}

proc read_gdb_line { ln } {

	global ST CB

	if { $ST(STA) == "GSU" } {
		# starting up, look for this:
		if { [string first "in _reset_vector__ ()" $ln] > 0 } {
			# got connected
			catch { after cancel $CB(GDB) }
			set CB(GDB) ""
			new_state "GUP"
			term_dspline 1 "--GDB CONNECTED TO DEVICE"
		}
		return
	}

	if { $ST(STA) == "ILO" } {
		# loading an image
		if { [string first "ransfer rate" $ln] > 0 } {
			# loading completed
			catch { after cancel $CB(GDB) }
			set CB(GDB) ""
			new_state "GUP"
			term_dspline 1 "--PROGRAM LOADED SUCCESSFULLY"
		}
		return
	}


	# MORE:
}

proc gdb_exits { } {

	global ST CB Term

	if { $Term(0,P) == "" } {
		new_state "OFF"
	} else {
		new_state "PUP"
	}

	if { $CB(GDB) != "" } {
		catch { after cancel $CB(GDB) }
		set CB(GDB) ""
	}
}

proc get_image_list { } {
#
# Builds the list of loadable images for selection
#
	if [catch { glob "Image*" } til] {
		set til ""
	}

	set iml ""
	foreach im $til {
		# choose qualifying files (ignore .a43 files)
		if { [string first "." $im] < 0 } {
			lappend iml $im
		}
	}

	return [lsort $iml]
}

proc load_image { } {

	global ST CB

	if { $ST(STA) != "GUP" || $ST(IMF) == "" } {
		return
	}

	# send INT signal to gdb to get it to the proper state
	interrupt_gdb

	# issue the load command
	gdb_cmd "load $ST(IMF)"

	# change the state to loading
	new_state "ILO"

	# timeout callback
	set CB(GDB) [after $CB(GDB,TL) gdb_load_abort]
}

proc run_node { } {

	global ST

	if { $ST(STA) != "GUP" } {
		return
	}

	gdb_cmd "monitor reset"
	delay 300
	gdb_cmd "c"
}

proc stop_node { } {

	global ST CB

	if ![oneof $ST(STA) "GUP" "ILO"] {
		return
	}

	if { $ST(STA) == "ILO" } {
		gdb_load_abort "BY USER"
	} else {
		interrupt_gdb
	}
	new_state "GUP"
}

proc interrupt_gdb { } {
#
# Sends an INT signal to GDB to move it to a known state
#
	global Term

	if { $Term(1,P) == "" || [catch { pid $Term(1,P) } ppl] } {
		# no GDB
		return
	}

	foreach p $ppl {
		catch { exec kill -INT $p }
	}

	delay 300
}

proc gdb_cmd { cmd } {
#
# Issue a command to gdb
#
	global Term

	if { $Term(1,P) == "" } {
		term_dspline 1 "--NO GDB, COMMAND IGNORED: $cmd"
		return
	}

	term_dspline 1 "--CMD: $cmd"

	catch { puts $Term(1,P) $cmd }
}

proc gdb_load_abort { { res "TIMEOUT" } } {
#
# Abort loading on timeout
#
	global CB

	new_state "GUP"
	catch { after cancel $CB(GDB) }
	set CB(GDB) ""
	term_dspline 1 "--LOADING ABORTED ($res)!"
	interrupt_gdb
}

proc gdb_user_input { tw } {
#
# Manually entered command line to gdb
#
	global ST

	set tx ""
	regexp "\[^\r\n\]+" [$tw get 0.0 end] tx
	$tw delete 0.0 end

	if { $ST(STA) != "GUP" } {
		return
	}

	gdb_cmd $tx
}

###############################################################################

proc new_state { s } {
#
# Sets the state
#
	global ST Term

	# the buttons frame
	set f $ST(BUT)

	# the new status
	set ST(STA) $s

	if { ![oneof $s "BLA" "GSU" "GUP" "ILO"] && $Term(1,P) != "" } {
		term_dspline 1 "--GDB ABORTED!"
		term_stop 1
	}

	if { $s == "OFF" && $Term(0,P) != "" } {
		term_dspline 0 "--PROXY ABORTED!"
		term_stop 0
	}

	foreach b { im lo ru st qu re up } {
		# disable all buttons
		$f.$b configure -state disabled
	}

	if { $s == "BLA" } {
		# that's it
		return
	}

	# these two are (normally) always enabled
	$f.qu configure -state normal
	$f.re configure -state normal

	if [oneof $s "OFF" "PSU" "GSU"] {
		# proxy not running or gdb starting up, you cannot do anything
		$f.up configure -state normal
		return
	}

	if { $s == "PUP" } {
		$f.im configure -state normal
		$f.up configure -state normal
		return
	}

	if { $s == "GUP" } {
		$f.lo configure -state normal
		$f.ru configure -state normal
		$f.st configure -state normal
		$f.up configure -state normal
		return
	}

	# "ILO" "REF" (image being loaded or reflashing FET);
	# wait until done; stop aborts
	$f.st configure -state normal
}

proc reprogram { } {

	global ST

	if { $ST(STA) == "ILO" } {
		# impossible
		term_dspline 0 "--IMAGE BEING LOADED, WAIT!"
		return
	}

	if ![confirm "Are you absolutely sure?"] {
		return
	}

	term_stop 1
	term_stop 0

	delay 500

	run_reflash

	if { $ST(STA) != "OFF" } {
		vwait ST(STA)
	} elseif { $ST(SYS) == "L" } {
		alert "On Linux, the programmer must be connected as\
			/dev/ttyUSB0 for reprogramming to succeed!\
			Check if this is the case"]
	}
}

proc restart { } {

	global ST

	term_stop 1
	term_stop 0

	delay 500

	if { $ST(SYS) != "L" } {
		# Cygwin - no device
		run_proxy ""
		if { $ST(STA) != "OFF" } {
			vwait ST(STA)
		}
	} else {
		# Linux
		set dev $ST(PDE)
		if { $dev == "" || [regexp -nocase "auto" $dev] } {
			# locate all USB ttys and scan them
			if { [catch { glob "/dev/ttyUSB*" } tl] || $tl == "" } {
				term_dspline 0 "--NO PROXY DEVICE FOUND"
				return
			}
		} else {
			set tl [list $dev]
		}
		
		foreach d $tl {
			term_dspline 0 "--TRYING DEVICE: $d"
			run_proxy $d
			if { $ST(STA) == "OFF" } {
				# failed to initialize
				continue
			}
			vwait ST(STA)
			if { $ST(STA) == "PUP" } {
				break
			}
		}
	}

	if { $ST(STA) == "OFF" } {
		term_dspline 0 "--CANNOT START PROXY"
	}
}

proc mk_main_window { } {

	global Term ST

	frame .p
	pack .p -side left -expand y -fill both

	set pan .p.pane
	panedwindow $pan

	pack $pan -side top -expand y -fill both

	# gdbproxy output term ################################################

	set wl $pan.left
	frame $wl
	pack $wl -side left -expand y -fill both -anchor w

	set gt $wl.t
	text $gt

	$gt configure \
		-yscrollcommand "$wl.scroly set" \
		-setgrid true \
        	-width 32 -height 24 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$gt delete 1.0 end
	scrollbar $wl.scroly -command "$gt yview"
	pack $wl.scroly -side right -fill y
	pack $gt -side top -expand yes -fill both

	bind $gt <ButtonRelease-1> "tk_textCopy $gt"

	# window
	set Term(0) $gt
	# pipe
	set Term(0,P) ""
	# buffer
	set Term(0,B) ""
	# line input function
	set Term(0,I) ""
	# function invoked when closing
	set Term(0,C) ""

	# gdb output term #####################################################

	set wr $pan.right
	frame $wr
	pack $wr -side left -expand y -fill both -anchor w

	set lt $wr.t
	text $lt

	$lt configure \
		-yscrollcommand "$wr.scroly set" \
		-setgrid true \
		-width 54 \
        	-wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$lt delete 1.0 end
	scrollbar $wr.scroly -command "$lt yview"
	pack $wr.scroly -side right -fill y
	pack $lt -side top -expand yes -fill both

	bind $lt <ButtonRelease-1> "tk_textCopy $lt"

	set Term(1) $lt
	set Term(1,P) ""
	set Term(1,B) ""
	set Term(1,I) ""
	set Term(1,C) ""

	set en $wr.u
	text $en -height 1 -width 54 -font {-family courier -size 10}
	pack $en -side top -expand yes -fill x

	bind $en <ButtonRelease-1> "tk_textCopy $en"
	bind $en <ButtonRelease-2> "tk_textPaste $en"
	bind $en <Return> "gdb_user_input $en"
	bind $en <Control-c> "interrupt_gdb"

	$pan add $pan.left $pan.right

	# buttons
	set f .but
	frame $f
	pack $f -side left -expand no -fill both

	set ST(BUT) $f

	# Select an Image file and start gdb for it
	button $f.im -text "Connect" -command "start_gdb"
	pack $f.im -side top -expand no -fill x -anchor n

	# Load the image into the node
	button $f.lo -text "Load" -command "load_image"
	pack $f.lo -side top -expand no -fill x -anchor n

	# Run the node
	button $f.ru -text "Run" -command "run_node"
	pack $f.ru -side top -expand no -fill x -anchor n

	# Stop the node
	button $f.st -text "Stop" -command "stop_node"
	pack $f.st -side top -expand no -fill x -anchor n

	#######################################################################

	frame $f.fsel
	pack $f.fsel -side top -expand no -fill x -anchor n

	set il [get_image_list]
	if { [llength $il] == 0 } {
		set ST(IMF) ""
		label $f.fsel.men -text "No images" -anchor "w"
	} else {
		set ST(IMF) [lindex $il 0]
		eval "set mn \[tk_optionMenu $f.fsel.men ST(IMF) [join $il]\]"
	}

	foreach it $il {
		$mn entryconfigure $it -command image_selection_click
	}

	pack $f.fsel.men -side top -expand no -fill x -anchor n

	#######################################################################

	button $f.qu -text "Quit" -command "terminate"
	pack $f.qu -side bottom -expand no -fill x -anchor s

	button $f.re -text "Restart" -command "restart"
	pack $f.re -side bottom -expand no -fill x -anchor s

	button $f.up -text "Update FET" -command "reprogram"
	pack $f.up -side bottom -expand no -fill x -anchor s

	bind . <Destroy> "terminate"
}

kill_win_proc_by_name $GdbCmd
kill_win_proc_by_name $ProxyCmd

mk_main_window

new_state "OFF"

restart

vwait forever
