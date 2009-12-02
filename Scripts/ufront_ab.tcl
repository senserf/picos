#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#
# UART front for XRS protocol
#

proc abt { ln } {

	puts stderr $ln
	exit 99
}

proc msource { f } {
#
# Intelligent 'source'
#
	if ![catch { uplevel #0 source $f } ] {
		# found it right here
		return
	}

	set dir "Scripts"
	set lst ""

	for { set i 0 } { $i < 10 } { incr i } {
		set dir "../$dir"
		set dno [file normalize $dir]
		if { $dno == $lst } {
			# no progress
			break
		}
		if ![catch { uplevel #0 source [file join $dir $f] } ] {
			# found it
			return
		}
	}

	# failed
	abt "Cannot locate file $f 'sourced' by the script."
}

msource oss_u.tcl
msource oss_u_ab.tcl

package require oss_u_ab 1.0

###############################################################################

proc uget { msg } {
#
# Receive a normal command response line from the board
#
	puts $msg
}

proc uget_bin { msg } {
#
# For binary messages
#
	set len [string length $msg]

	set enc "Received: $len < "

	# encode the bytes in hex
	for { set i 0 } { $i < $len } { incr i } {
		binary scan [string index $msg $i] c byte
		append enc [format "%02x " $byte]
	}

	append enc ">"

	puts $enc
}

proc inline { } {
#
# Preprocess a line input from the keyboard
#
	global ST

	if [catch { gets stdin line } stat] {
		# ignore any errors (can they happen at all?)
		return ""
	}

	if { $stat < 0 } {
		# end of file
		exit 0
	}

	set line [string trim $line]

	if { $line == "" } {
		# ignore empty lines
		return ""
	}

	if { $line == "!!" } {
		# previous command
		if { $ST(PCM) == "" } {
			puts "no previous rejected command"
			return
		}
		set line $ST(PCM)
		set ST(PCM) ""
	}

	if ![u_ab_ready] {
		puts "board busy"
		set ST(PCM) $line
		return ""
	}

	if { [string index $line 0] == "!" } {
		# not for the board
		if [icmd [string trimleft \
		    [string range $line 1 end]]] {
			# failed
			set ST(PCM) $line
		}
		return ""
	}

	return $line
}

proc sget { } {
#
# STDIN becomes readable
#
	global ST mpl
	set line [inline]
	if { $line != "" } {
		if $ST(ECO) {
			puts $line
		}
		set ln [expr [string length $line] + 1]
		if { $ln > $mpl } {
			puts "Error: line longer than $mpl characters"
			return
		}
		u_ab_write $line
	}
}

proc suball { line pars vals } {
#
# Scans the string substituting all occurrences of parameters with their values
#
	foreach par $pars {

		regsub -all $par $line [lindex $vals 0] line
		set vals [lrange $vals 1 end]

	}

	return $line

}

proc macsub { line } {
#
# Macro substitution (no recursion: one scan per macro in definition order)
#
	global BMAC

	set ml $BMAC(+)

	foreach m $ml {

		set ol ""
		set ll [string length $m]
		set ma [lindex $BMAC($m) 0]
		set pl [lindex $BMAC($m) 1]

		while 1 {

			set x [string first $m $line]
			if { $x < 0 } {
				break
			}

			append ol [string range $line 0 [expr $x - 1]]
			set line [string range $line [expr $x + $ll] end]

			if { $pl != "" } {
				# parameters are expected
				if { [string index $line 0] != "(" } {
					# ignore, assume not a macro call
					continue
				}
				# get the actual parameters
				set al ""
				set line [string range $line 1 end]
				while 1 {
					set c [string index $line 0]
					if { $c == ")" } {
						# all done
						break
					}
					if { $c == "" } {
						error "illegal macro reference"
					}
					if { $c == "," } {
						# empty parameter
						lappend al ""
						set line \
						    [string range $line 1 end]
						continue
					}
					# starts a new parameter
					regexp "^\[^),\]+" $line c
					lappend al $c
					set line [string range $line \
						[string length $c] end]
					if { [string index $line 0] == "," } {
						set line \
						    [string range $line 1 end]
					}
				}
				# skip the closing parenthesis
				set line [string range $line 1 end]
				append ol [suball $ma $pl $al]
			} else {
				append ol $ma
			}
		}
		append ol $line
		set line $ol
	}

	return $line
}

proc sget_bin { } {
#
# The binary variant
#
	global ST BMAC mpl

	set line [inline]
	if { $line == "" } {
		return
	}

	if [catch { macsub $line } line] {
		puts "Error: $line"
		return
	}

	set out ""
	set eco ""
	set oul 0

	while 1 {
		set line [string trimleft $line]
		if { $line == "" } {
			break
		}
		while 1 {
			if [regexp -nocase "^\\\$(\[0-9a-f\]+)" $line mat num] {
				set num "0x$num"
				break
			}
			if [regexp -nocase "^x(\[0-9a-f\]+)" $line mat num] {
				set num "0x$num"
				break
			}
			if [regexp -nocase "^\\0x\[0-9a-f\]+" $line mat] {
				set num $mat
				break
			}
			if [regexp "^\[0-9\]+" $line mat] {
				set num $mat
				regsub "^0+" $num "" num
				if { $num == "" } {
					set num 0
				}
				break
			}
			puts "Error: illegal number format:\
				[string range $line 0 5] ..."
			return
		}

		if [catch { expr $num } val] {
			puts "Error: illegal numerical value $num"
			return
		}

		if { $val > 255 } {
			puts "Error: value $val ($num) is over 255"
			return
		}

		set line [string range $line [string length $mat] end]

		if $ST(ECO) {
			append eco [format "%02x " $val]
		}

		append out [binary format c $val]
		incr oul
	}

	if $ST(ECO) {
		puts "Sending: < $eco>"
	}
		
	if { $oul > $mpl } {
		puts "Error: block longer than $mpl bytes"
		return
	}

	u_ab_write $out
}

###############################################################################

proc icmd { cmd } {
#
# A command addressed to us
#
	global ST

	if { $cmd == "e" || $cmd == "echo" } {
		if { $ST(ECO) } {
			puts "Echo is now off"
			set ST(ECO) 0
		} else {
			puts "Echo is now on"
			set ST(ECO) 1
		}
		return 1
	}
	puts "Command $cmd unimplemented"
	return 0
}

###############################################################################

proc readmac { } {
#
# Read the macro file for binary mode
#
	global BMAC bin

	set bfd ""
	set BMAC(+) ""
	if { $bin == "+" } {
		# try the deafult
		if [catch { open "bin.mac" r } bfd] {
			# nothing there
			return
		}
	}

	if { $bfd == "" && [catch { open $bin "r" } bfd] } {
		abt "Cannot open the macro file $bin: $bfd"
	}

	set lines [split [read $bfd] "\n"]

	catch { close $bfd }

	# parse the macros
	set ML ""

	foreach ln $lines {

		set ln [string trim $ln]
		if { $ln == "" || [string index $ln 0] == "#" } {
			# ignore
			continue
		}

		if ![regexp -nocase "^(\[_a-z\]\[a-z0-9_\]*)(.*)" $ln j nm t] {
			abt "Illegal macro name in this line: $ln"
		}

		if [info exists BMAC($nm)] {
			abt "Duplicate macro name $nm in this line: $ln"
		}

		set t [string trimleft $t]
		set d [string index $t 0]
		set p ""

		if { $d == "(" } {
			# this is a parameterized macro
			set t [string range $t 1 end]
			# parse the parameters
			while 1 {
				set t [string trimleft $t]
				set d [string index $t 0]
				if { $d == "," } {
					set t [string range $t 1 end]
					continue
				}
				if { $d == ")" } {
					set t [string trimleft \
						[string range $t 1 end]]
					break
				}
				if ![regexp -nocase \
				    "^(\[a-z\]\[a-z0-9_\]*)(.*)" $t j pm t] {
					abt "Illegal macro parameter in this\
						line: $ln"
				}
				set t [string trimleft $t]
				if [info exists parms($pm)] {
					abt "Duplicate macro parameter in this\
						line: $ln"
				}
				set parms($pm) ""
				lappend p $pm
			}
			array unset parms
		}

		# now the macro body
		if { [string index $t 0] != "=" } {
			abt "Illegal macro definition in this line: $ln"
		}

		set t [string trimleft [string range $t 1 end]]
		regexp "^\"(.*)\"$" $t j t
		if { $t == "" } {
			abt "Empty macro in this line: $ln"
		}

		set BMAC($nm) [list $t $p]
		lappend ML $nm
	}
	# list of names
	set BMAC(+) $ML
}

proc usage { } {

	global argv0

	abt "Usage: $argv0 -p port/dev \[-s speed (def 9600)\]\
		 \[-l blen (def 62)\] \[-b \[macro_file\]\]"
}

set ST(ECO) 0

set prt ""
set spd ""
set mpl ""
set bin ""

while 1 {

	set par [lindex $argv 0]

	if { $par == "" } {
		break
	}

	if ![regexp "^-(\[a-z\])$" $par jnk par] {
		usage
	}

	set argv [lrange $argv 1 end]

	set val [lindex $argv 0]

	if ![regexp "^-(\[a-z\])$" $val jnk par] {
		set argv [lrange $argv 1 end]
	} else {
		set val ""
	}

	if { $par == "p" && $prt == "" && $val != "" } {
		set prt $val
		continue
	}

	if { $par == "s" && $spd == "" } {
		if { [catch { expr $val } spd] || $spd <= 0 } {
			usage
		}
		continue
	}

	if { $par == "l" && $mpl == "" } {
		if { [catch { expr $val } mpl] || $mpl <= 0 || $mpl > 250 } {
			usage
		}
		continue
	}

	if { $par == "b" && $bin == "" } {
		if { $val == "" } {
			set bin "+"
		} else {
			set bin $val
		}
		continue
	}

	usage
}

if { $prt == "" } {
	set prt "CNCA0"
}

if { $spd == "" } {
	set spd 115200
}

if { $mpl == "" } {
	set mpl 56
}

if [catch { u_start $prt $spd "" } err] {
	abt $err
}

fconfigure stdin -buffering line -blocking 0 -eofchar ""

if { $bin != "" } {

	readmac

	u_ab_setif uget_bin [expr $mpl + 4]
	fileevent stdin readable sget_bin

} else {

	u_ab_setif uget [expr $mpl + 4]
	fileevent stdin readable sget
}

# u_settrace 7 dump.txt

vwait None

###############################################################################
