#!/bin/sh
#
#	Copyright 2002-2020 (C) Olsonet Communications Corporation
#	Programmed by Pawel Gburzynski & Wlodek Olesinski
#	All rights reserved
#
#	This file is part of the PICOS platform
#
#
#########\
exec tclsh86 `cygpath -a -w $0` $@

set AGENT_PATH "C:\\ti\\ccsv7\\ccs_base\\common\\uscif\\gdb_agent_console.exe"

proc abt { m } {

	puts stderr $m
	exit 1
}

proc run_agent { } {

	global argv AGENT_PATH AFD

	set bf [lindex $argv 0]
	if { $bf == "" } {
		set bf "gdbproxyconf.dat"
	}

	set bw [exec cygpath -a -w $bf]

	puts "running: $AGENT_PATH $bw"

	if [catch { open "|[list $AGENT_PATH $bw]" "r" } AFD] {
		abt "cannot start agent: $AFD"
	}

	fconfigure $AFD -blocking 0 -buffering none
	fileevent $AFD readable "agent_output"
}

proc agent_output { } {

	global AFD

	if { [catch { read $AFD } buf] || [eof $AFD] } {
		# done
		puts "agent terminated"
		exit 0
	}

	puts $buf
}

proc user_input { } {

	global AFD

	if [catch { pid $AFD } pp] {
		puts "no agent process to kill"
		return
	}

	foreach p $pp {
		puts "killing agent process $p"
		if [catch { exec kill -f -KILL $p } err] {
			puts "cannot kill: $err"
		}
	}

	exit 0
}

proc main { } {

	run_agent

	fconfigure stdin -blocking 0 -buffering line
	fileevent stdin readable "user_input"

	vwait forever
}

main









exec $ap $bw >&@ stdout
