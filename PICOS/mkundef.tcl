#!/usr/bin/tclsh
#
# Create a matching ....._undef.h file
#
	set fn [lindex $argv 0]

	if { $fn == "" } {
		set fn "stdattr.h"
	}

	if ![regexp "^(.+)\\.h$" $fn junk fr] {
		abt "input file name must look like 'xxxx.h'"
	}

	set tfn "${fr}_undef.h"

	set sfd [open $fn "r"]
	set tfd [open $tfn "w"]

	puts $tfd "// Created automatically, do not edit!!!"

	puts $tfd "\#ifdef __picos_${fr}_h__"
	puts $tfd "\#undef __picos_${fr}_h__"

	while { [gets $sfd line] >= 0 } {
		set line [string trim $line]
		if ![regexp "^\#\[ \t\]*define\[ \t\]+(\[^ \t()\]+)" $line junk\
		    sym] {
			continue
		}
		puts $tfd "\#undef $sym"
	}
	
	puts $tfd "\#endif"
