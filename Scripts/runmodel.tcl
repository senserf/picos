#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
########\
exec wish85 "$0" "$@"

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

proc kill_cygwin { plist } {
#
# Cygwin kill
#
	set ef [auto_execok "./BIN/kill.exe"]
	if { $ef == "" } {
		return 0
	}

	foreach p $plist {
		if [catch { eval [list exec] [list $ef] "-f $p" } err] {
			return 0
		}
		term_output "Killed (cyg): $p"
	}

	return 1
}

proc kill_windows { plist } {
#
# Windows kill
#
	foreach p $plist {
		if ![catch { eval exec "taskkill" "/F /PID $p" } err] {
			term_output "Killed (win): $p"
		}
	}
}

proc kill_pipe { fd } {
#
# Kills the process on the other end of our pipe
#
	if { $fd == "" || [catch { pid $fd } pp] || $pp == "" } {
		return
	}

	# try the cygwin kill first, probably isn't going to work, we are on
	# Windows
	if ![kill_cygwin $pp] {
		kill_windows $pp
	}
	catch { close $fd }
}

proc run { } {

	global WI FI OFD UDE PIP BUF Running VUEPARS UDEPARS

	if $Running {
		stop_term
		return
	}

	if { $FI(DS) == "" } {
		alert "No data file selected"
		return
	}

	# do we have an output file
	catch { close $OFD }
	set OFD ""
	if { $FI(OS) != "" } {
		if [catch { open $FI(OS) "a" } OFD] {
			alert "Cannot open the output file $FI(OS), $OFD"
			set OFD ""
			return
		}
	}

	set ef [auto_execok "./BIN/side.exe"]
	if { $ef == "" } {
		alert "Cannot execute the simulator"
	}

	set tdir "DATAFILES"

	set fl [glob -nocomplain -tails -directory DATAFILES "supp_node*.xml"]

	set sl ""
	set sn 0
	set ar [list $FI(DS) +]
	foreach f $fl {
		if [regexp "^supp_node_(.+).xml" $f jnk suf] {
			lappend sl $suf
			continue
		}
		if { $f == "supp_node.xml" } {
			set sn 1
			break
		}
	}

	if { $sn || $sl != "" || $VUEPARS != "" } {
		lappend ar "--"
		if $sn {
			lappend ar "-n"
			lappend ar [file join $tdir "supp_node.xml"]
		} else {
			foreach s $sl {
				lappend ar "-n"
				lappend ar $s
				lappend ar [file join $tdir "supp_node_$s.xml"]
			}
		}
	}

	foreach s $VUEPARS {
		lappend ar $s
	}

	set cmd "[list $ef] -e"

	foreach k $ar {
		lappend cmd $k
	}

	lappend cmd "2>@1"

	if [catch { open "|$cmd" "r" } fd] {
		alert "Error starting simulator, $fd"
	}

	set PIP $fd
	set BUF ""

	fconfigure $fd -blocking 0 -buffering none -eofchar "" -translation lf
	fileevent $fd readable "term_output"

	set Running 1
	update_run_button

	after 2000

	set ef [auto_execok "./BIN/udaemon.exe"]
	if { $ef == "" } {
		alert "Cannot execute udaemon"
	}

	set ar [list [list $ef] "-T"]

	foreach s $UDEPARS {
		lappend ar $s
	}

	set pf [file join "DATAFILES" "uplug.tcl"]
	if ![file isfile $pf] {
		set pf [file join "DATAFILES" "shared_plug.tcl"]
		if ![file isfile $pf] {
			set pf ""
		}
	}

	if { $pf != "" } {
		lappend ar "-P"
		lappend ar $pf
	}

	append ar " 2>@1"

	if [catch { open "|$ar" "r" } fd] {
		alert "Cannot start udaemon: $fd"
		return
	}

	set UDE $fd

	fconfigure $fd -blocking 0 -buffering none -eofchar ""
	fileevent $fd readable "udaemon_output"
}

proc stop_term { } {

	global UDE PIP OFD Running

	if { $UDE != "" } {
		kill_pipe $UDE
		set UDE ""
	}

	if { $PIP != "" } {
		kill_pipe $PIP
		set PIP ""
	}

	if { $OFD != "" } {
		catch { close $OFD }
		set OFD ""
	}

	set Running 0
	update_run_button
}

proc update_run_button { } {

	global WI Running

	if $Running {
		set t "Stop"
	} else {
		set t "Run"
	}

	$WI(RU) configure -text $t
}

proc udaemon_output { } {

	global UDE

	if { [catch { read $UDE } dummy] || [eof $UDE] } {
		stop_term
	}
}

proc term_output { { line "" } } {

	global PIP OFD BUF

	if { $line != "" } {

		set chunk "$line\n"

	} else {

		if [catch { read $PIP } chunk] {
			stop_term
			return
		}

		if [eof $PIP] {
			stop_term
			return
		}

		if { $chunk == "" } {
			return
		}

		if { $OFD != "" } {
			catch { puts -nonewline $OFD $chunk }
		}
	}

	append BUF $chunk
	# look for CR+LF, LF+CR, CR, LF; if there is only one of those at the
	# end, ignore it for now and keep for posterity
	set sl [string length $BUF]

	while { [regexp "\[\r\n\]" $BUF m] } {
		set el [string first $m $BUF]
		if { $el == 0 } {
			# first character
			if { $sl < 2 } {
				# have to leave it and wait
				return
			}
			# check the second one
			set n [string index $BUF 1]
			if { $m == "\r" && $n == "\n" || \
			     $m == "\n" && $n == "\r"    } {
				# two-character EOL
				set BUF [string range $BUF 2 end]
				incr sl -2
			} else {
				set BUF [string range $BUF 1 end]
				incr sl -1
			}
			# complete previous line
			term_endline
			continue
		}
		# send the preceding string to the terminal
		term_addtxt [string range $BUF 0 [expr $el - 1]]
		incr sl -$el
		set BUF [string range $BUF $el end]
	}

	if { $BUF != "" } {
		term_addtxt $BUF
		set BUF ""
	}
}

proc term_addtxt { txt } {

	global WI

	set t $WI(CO)

	$t configure -state normal
	$t insert end $txt
	$t configure -state disabled
	$t yview -pickplace end
}

proc term_endline { } {

	global WI

	set t $WI(CO)

	$t configure -state normal
	$t insert end "\n"

	while 1 {
		set ix [$t index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= 4096 } {
			break
		}
		# delete the topmost line if above limit
		$t delete 1.0 2.0
	}

	$t configure -state disabled
	# make sure the last line is displayed
	$t yview -pickplace end
}

proc get_data_files { } {

	return [lsort [concat [glob -nocomplain "*.xml"] \
		[glob -nocomplain "*.txt"]]]
}

proc select_data_file { } {

	global WI

	set fl [get_data_files]

	if { $fl == "" } {
		alert "No data files available"
		return
	}

	set r "$WI(DS)._sdm"

	catch { destroy $r }

	set m [menu $r -tearoff 0]

	foreach f $fl {
		$m add command -label $f -command [list use_data_file $f]
	}

	tk_popup $m [winfo rootx $WI(DS)] [winfo rooty $WI(DS)]
}

proc use_data_file { f } {

	global WI FI

	$WI(DS) configure -text [button_file_name $f]
	set FI(DS) $f
}

proc select_output_file { } {

	global WI FI

	set fl [tk_getSaveFile \
		-parent . \
		-defaultextension ".txt" \
		-initialdir . \
		-title "Output file name:"]

	set FI(OS) $fl

	if { $fl == "" } {
		set fl "None"
	} else {
		set fl [button_file_name $fl]
	}

	$WI(OS) configure -text $fl
}

proc button_file_name { f } {

	set ll [string length $f]

	if { $ll > 24 } {
		set f [string range $f end-23 end]
	}

	return $f
}

proc clear_term { } {

	global WI

	set t $WI(CO)

	$t configure -state normal
	$t delete 1.0 end
	$t configure -state disabled
}

proc start { } {

	global WI FI Running

	wm title . "Runmodel"

	set uf [frame .uf]
	pack $uf -side top -fill both -expand yes
	set t [text $uf.t \
			-yscrollcommand "$uf.scroly set" \
			-setgrid true \
        		-width 80 -height 24 -wrap char \
			-font {-family courier -size 10} \
			-exportselection 1 \
			-state normal]
	set WI(CO) $t
	$t delete 1.0 end
	set s [scrollbar $uf.scroly -command "$t yview"]
	pack $s -side right -fill y
	pack $t -expand yes -fill both

	frame .bf -borderwidth 0
	pack .bf -side top -expand no -fill both

	label .bf.dl -text "Data: "
	pack .bf.dl -side left -expand no -fill y

	set df [lindex [get_data_files] 0]
	if { $df == "" } {
		set FI(DS) ""
		set df "Select"
	} else {
		set FI(DS) $df
		set df [button_file_name $df]
	}

	set WI(DS) [button .bf.db -text $df -justify left \
		-command select_data_file]
	pack .bf.db -side left -expand no -fill y

	label .bf.ol -text " Output: "
	pack .bf.ol -side left -expand no -fill y

	set FI(OS) ""
	set WI(OS) [button .bf.ob -text "None" -justify left \
		-command select_output_file]
	pack .bf.ob -side left -expand no -fill y

	set WI(RU) [button .bf.rb -text "Run" -command run]
	pack .bf.rb -side right -expand no -fill y

	button .bf.rc -text "Clear" -command clear_term
	pack .bf.rc -side right -expand no -fill y

	set Running 0

	bind . <Destroy> kill_me
}

proc kill_me { } {

	catch { stop_term }
	exit 0
}

set UDE ""
set PIP ""

set VUEPARS ""
set UDEPARS ""

start
