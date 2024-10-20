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
