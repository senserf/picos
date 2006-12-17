#!/usr/bin/tclsh
#
# Create a matching ....._undef.h file
#
	set fn [lindex $argv 0]

	if ![regexp "^(.+)\\.h$" $fn junk fr] {
		abt "input file name must look like 'xxxx.h'"
	}

	set tfn "${fr}_undef.h"

	set sfd [open $fn "r"]
	set tfd [open $tfn "w"]

	puts $tfd "\#ifndef __picos_${fr}_undef_h__"
	puts $tfd "\#define __picos_${fr}_undef_h__"

	puts $tfd "// Created automatically, do not edit!!!"

	while { [gets $sfd line] >= 0 } {
		set line [string trim $line]
		if ![regexp "^\#\[ \t\]*define\[ \t\]+(\[^ \t()\]+)" $line junk\
		    sym] {
			continue
		}
		puts $tfd "\#undef $sym"
	}
	
	puts $tfd "\#endif"
