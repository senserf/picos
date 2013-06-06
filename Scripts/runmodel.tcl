#!/bin/sh
########\
exec tclsh85 "$0" "$@"

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

proc xq { pgm { pargs "" } } {
#
# Execute program
#
	set ef [auto_execok $pgm]
	set re [eval [list exec] [list $ef] $pargs]
	return $re
}

proc iflist { } {
#
# Prepare a list of input file candidates
#
	set fl [glob -nocomplain "*.xml"]
	set re ""

	foreach f $fl {
		if [catch { open $f "r" } fd] {
			continue
		}
		if [catch { read $fd } cn] {
			catch { close $fd }
			continue
		}
		if { [string first "<network" $cn] >= 0 } {
			lappend re $f
		}
	}

	return $re
}

proc soutput { } {
#
# Selects the output file
#
	global OFN

	set fl [tk_getSaveFile -parent . -defaultextension "txt" \
		-initialfile $OFN \
		-title "Output file name:"]

	if { $fl == "" } {
		return
	}

	set OFN $fl
}

proc run { } {

	global IFN OFN

	if [catch { xq side.exe [list $IFN $OFN "&"] } er] {
		alert "Cannot start SIDE: $er"
		return
	}

	after 2000

	if [catch { xq udaemon.exe [list "-T" "&"] } er] {
		alert "Cannot start udaemon: $er"
		return
	}

	exit 0
}

proc start { } {

	global IFN OFN

	wm title . "Model selection"

	set ifl [iflist]
	if { $ifl == "" } {
		alert "No data files found"
		exit 1
	}

	set IFN [lindex $ifl 0]
	set OFN "output.txt"

	set u [frame .u]
	pack $u -side top -expand y -fill both

	#######################################################################

	label $u.il -text "Select data file:" -anchor w
	grid $u.il -column 0 -row 0 -padx 4 -pady 2 -sticky w

	eval "tk_optionMenu $u.im IFN $ifl"
	grid $u.im -column 1 -row 0 -padx 4 -pady 2 -sticky we

	#######################################################################

	label $u.ol -text "Select output file:" -anchor w
	grid $u.ol -column 0 -row 1 -padx 4 -pady 2 -sticky w

	button $u.om -textvariable OFN -command "soutput"
	grid $u.om -column 1 -row 1 -padx 4 -pady 2 -sticky we

	#######################################################################

	set d [frame .d]
	pack $d -side top -expand n -fill x

	button $d.c -text "Cancel" -command "exit 0"
	pack $d.c -side left -expand n

	button $d.g -text "Run model" -command "run"
	pack $d.g -side right -expand n

	#######################################################################
}

start
