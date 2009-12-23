#!/bin/sh
###########################\
exec tclsh "$0" "$@"

#####################################
# UART front for various UART modes #
#####################################

###############################################################################
#
# Note: I have decided to put everything into a single file, as keeping the
# multiple "pseudo-modules" is messy. You should either do it right (meaning
# install everything properly as Tcl packages), or keep the structure as
# simple as possible (meaning, preferably, in one piece)
#
###############################################################################

proc abt { ln } {

	puts stderr $ln
	exit 99
}

proc no_function { args } {

	abt "undefined plugin function called"
}

if { [info tclversion] < 8.5 } {
	abt "This script requires Tcl 8.5 or newer!"
}

###############################################################################
# Determine whether we are on UNIX or Windows #################################
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

###############################################################################

# for tracing and debugging
set DB(SFD)	""
set DB(LEV)	0

# diag preamble (for packet modes)
set CH(DPR)	[format %c [expr 0x54]]

# character zero
set CH(ZER)	[format %c [expr 0x00]]

# maximum message length (for packet modes)
set PM(MPL)	82

# outgoing message
set ST(OUT)	""

# send callback
set ST(SCB)	""

# low-level reception timer
set ST(TIM)  ""

# reception automaton initial state
set ST(STA) 0

# reception automaton remaining byte count
set ST(CNT) 0

# input buffer
set ST(BUF) ""

# app-level input function
set ST(DFN)	"no_function"

# app-level output function
set ST(OFN)	"no_function"

# echo flag
set ST(ECO)	0

# previous command
set ST(PCM)	""

# packet timeout (msec), once reception has started
set IV(PKT)	80

# short retransmit interval
set IV(RTS)	250

# long retransmit interval
set IV(RTL)	2000

###############################################################################
# ISO 3309 CRC ################################################################
###############################################################################

set CRCTAB {
    0x0000  0x1021  0x2042  0x3063  0x4084  0x50a5  0x60c6  0x70e7
    0x8108  0x9129  0xa14a  0xb16b  0xc18c  0xd1ad  0xe1ce  0xf1ef
    0x1231  0x0210  0x3273  0x2252  0x52b5  0x4294  0x72f7  0x62d6
    0x9339  0x8318  0xb37b  0xa35a  0xd3bd  0xc39c  0xf3ff  0xe3de
    0x2462  0x3443  0x0420  0x1401  0x64e6  0x74c7  0x44a4  0x5485
    0xa56a  0xb54b  0x8528  0x9509  0xe5ee  0xf5cf  0xc5ac  0xd58d
    0x3653  0x2672  0x1611  0x0630  0x76d7  0x66f6  0x5695  0x46b4
    0xb75b  0xa77a  0x9719  0x8738  0xf7df  0xe7fe  0xd79d  0xc7bc
    0x48c4  0x58e5  0x6886  0x78a7  0x0840  0x1861  0x2802  0x3823
    0xc9cc  0xd9ed  0xe98e  0xf9af  0x8948  0x9969  0xa90a  0xb92b
    0x5af5  0x4ad4  0x7ab7  0x6a96  0x1a71  0x0a50  0x3a33  0x2a12
    0xdbfd  0xcbdc  0xfbbf  0xeb9e  0x9b79  0x8b58  0xbb3b  0xab1a
    0x6ca6  0x7c87  0x4ce4  0x5cc5  0x2c22  0x3c03  0x0c60  0x1c41
    0xedae  0xfd8f  0xcdec  0xddcd  0xad2a  0xbd0b  0x8d68  0x9d49
    0x7e97  0x6eb6  0x5ed5  0x4ef4  0x3e13  0x2e32  0x1e51  0x0e70
    0xff9f  0xefbe  0xdfdd  0xcffc  0xbf1b  0xaf3a  0x9f59  0x8f78
    0x9188  0x81a9  0xb1ca  0xa1eb  0xd10c  0xc12d  0xf14e  0xe16f
    0x1080  0x00a1  0x30c2  0x20e3  0x5004  0x4025  0x7046  0x6067
    0x83b9  0x9398  0xa3fb  0xb3da  0xc33d  0xd31c  0xe37f  0xf35e
    0x02b1  0x1290  0x22f3  0x32d2  0x4235  0x5214  0x6277  0x7256
    0xb5ea  0xa5cb  0x95a8  0x8589  0xf56e  0xe54f  0xd52c  0xc50d
    0x34e2  0x24c3  0x14a0  0x0481  0x7466  0x6447  0x5424  0x4405
    0xa7db  0xb7fa  0x8799  0x97b8  0xe75f  0xf77e  0xc71d  0xd73c
    0x26d3  0x36f2  0x0691  0x16b0  0x6657  0x7676  0x4615  0x5634
    0xd94c  0xc96d  0xf90e  0xe92f  0x99c8  0x89e9  0xb98a  0xa9ab
    0x5844  0x4865  0x7806  0x6827  0x18c0  0x08e1  0x3882  0x28a3
    0xcb7d  0xdb5c  0xeb3f  0xfb1e  0x8bf9  0x9bd8  0xabbb  0xbb9a
    0x4a75  0x5a54  0x6a37  0x7a16  0x0af1  0x1ad0  0x2ab3  0x3a92
    0xfd2e  0xed0f  0xdd6c  0xcd4d  0xbdaa  0xad8b  0x9de8  0x8dc9
    0x7c26  0x6c07  0x5c64  0x4c45  0x3ca2  0x2c83  0x1ce0  0x0cc1
    0xef1f  0xff3e  0xcf5d  0xdf7c  0xaf9b  0xbfba  0x8fd9  0x9ff8
    0x6e17  0x7e36  0x4e55  0x5e74  0x2e93  0x3eb2  0x0ed1  0x1ef0
}

proc chks { wa } {

	global CRCTAB

	set nb [string length $wa]

	set chs 0

	while { $nb > 0 } {

		binary scan $wa su waw
		#set waw [expr $waw & 0x0000ffff]

		set wa [string range $wa 2 end]
		incr nb -2

		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw >>   8)]] )) & \
			0x0000ffff ]
		set chs [expr (($chs << 8) ^ \
		    ( [lindex $CRCTAB [expr ($chs >> 8) ^ ($waw & 0xff)]] )) & \
			0x0000ffff ]
	}

	return $chs
}

###############################################################################
# Plugin independent functions ################################################
###############################################################################

proc u_cdevl { pi } {
#
# Returns the candidate list of devices to open based on the port identifier
#
	global ST

	if { $ST(SYS) == "L" } {
		# Linux
		if [regexp "^\[0-9\]+$" $pi] {
			# a number
			return \
			    [list "/dev/ttyUSB$pn" "/dev/tty$pn" "/dev/pts/$pi]
		}
		if ![regexp "^/dev" $pi] {
			set pi "/dev/$pi"
			regsub -all "//" $pi "/" pi
		}
		return [list $pi]
	}

	if { [regexp "^\[0-9\]+$" $pi] && ![catch { expr $pi } pn] } {
		# looks like a (COM) number
		if { $pn < 10 } {
			# use internal Tcl COM id, which is faster
			set wd "COM${pn}:"
		} else {
			set wd "\\\\.\\COM$pn"
		}
		return [list $wd]
	}

	# not a number
	return [list $pi "\\\\.\\$pi"]
}

proc u_start { udev speed mpl plug ifun } {
#
# open the UART
#
	global ST CH PM PLUG

	set devlist [u_cdevl $udev]

	set fail 1
	foreach udev $devlist {
		if ![catch { open $udev "r+" } ST(SFD)] {
			set fail 0
			break
		}
	}

	if $fail {
		abt "u_start: cannot open UART: $devlist"
	}

	set PM(MPL) $mpl

	# the generic part of UART configuration
	if [catch { fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
	    -blocking 0 -eofchar "" } err] {
		abt "u_start: cannot configure UART: $err"
	}

	# the plugin
	set PF $PLUG($plug)

	# initialize
	[lindex $PF 0]

	set ST(DFN) $ifun
	set ST(OFN) [lindex $PF 2]

	fileevent $ST(SFD) readable [lindex $PF 1]
}
	
proc u_outdiag { } {

	global ST CH

	if { [string index $ST(BUF) 0] == $CH(ZER) } {
		# binary
		set ln [string range $ST(BUF) 3 5]
		binary scan $ln cuSu lv code
		#set lv [expr $lv & 0xff]
		#set code [expr $code & 0xffff]
		puts "DIAG: \[[format %02x $lv] -> [format %04x $code]\]"
	} else {
		# ASCII
		puts "DIAG: [string trim $ST(BUF)]"
	}
	flush stdout
}

proc u_ready { } {
#
# Check if can accept a new message
#
	global ST

	if { $ST(OUT) == "" } {
		return 1
	} else {
		return 0
	}
}

###############################################################################
# Plugin: mode == N ###########################################################
###############################################################################

proc x_frame_n { m } {
#
# Frame the message (N mode); when we get it here, the message contains the
# four XRS header bytes in front (they formally count as N-payload), but no
# checksum. This is exactly what is covered by MPL.
#
	upvar $m msg
	global CH PM DB

	set ln [string length $msg]

	if { $ln > $PM(MPL) } {
		if { $DB(LEV) > 0 } {
			trc "frame: message too long: $ln > $PM(MPL), truncated"
		}
		set ln $PM(MPL)
		set msg [string range $msg 0 [expr $ln - 1]
	}

	if { $ln < 2 } {
		if { $DB(LEV) > 0 } {
			trc "frame: message too short: $ln < 2, padded"
		}
		while { $ln < 2 } {
			append msg $CH(ZER)
			incr ln
		}
	} elseif [expr $ln & 1] {
		# odd length, append NULL
		append msg \x00
		# do not count the LID in the length fiels
		incr ln -1
	} else {
		incr ln -2
	}

	set msg "$CH(IPR)[binary format c $ln]$msg[binary format s [chks $msg]]"
}

proc x_write_n { msg } {
#
# Send a packet to UART
#
	global DB ST

	if { $DB(LEV) > 1 } {
		dump "W" $msg
	}

	puts -nonewline $ST(SFD) $msg
	flush $ST(SFD)
}

proc x_fnwrite_n { msg } {
#
# Frame and write in one step
#
	x_frame_n msg
	x_write_n $msg
}

proc x_receive_n { } {
#
# Handle received packet (internal for the plugin)
#
	global ST DB CH IV

	if { $DB(LEV) > 1 } {
		# dump the packet
		dump "R" $ST(BUF)
	}

	# validate CRC
	if [chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# strip off the checksum
	set msg [string range $ST(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 4 } {
		# ignore it, we need at least the N-type header
		return
	}

	# extract the header
	binary scan $msg cucucucu cu ex ma le

	if { [expr $ma & 0xff] != $CH(MAG) } {
		# wrong magic
		return
	}

	if { $ST(OUT) != "" } {
		# we have an outgoing message
		if { $ex != $CH(CUR) } {
			# expected != our current, we are done with this
			# message
			set ST(OUT) ""
			set CH(CUR) $ex
		}
	} else {
		# no outgoing message, set current to expected
		set CH(CUR) $ex
	}

	if { $len == 4 } {
		# empty message, treat as pure ACK and ignore
		return
	}

	if { $cu != $CH(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(RTS) x_send_n]
		return
	}

	if { $le > [expr $len - 4] } {
		# consistency check
		if { $DB(LEV) > 2 } {
			trc "receive: inconsistent XRS header, $le <-> $len"
		}
		return
	}

	# receive it
	$ST(DFN) [string range $msg 4 [expr 3 + $le]]

	# update expected
	set CH(EXP) [expr ( $CH(EXP) + 1 ) & 0x00ff]

	# force an ACK
	x_send_n
}

proc x_send_n { } {
#
# Callback for sending packets out
#
	global ST IV CH
	
	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	# if len > 0, an outgoing message is pending; note: its length has been
	# checked already (and is <= MAXPL)
	set len [string length $ST(OUT)]

	x_fnwrite_n\
	    "[binary format cccc $CH(CUR) $CH(EXP) $CH(MAG) $len]$ST(OUT)"

	set ST(SCB) [after $IV(RTL) x_send_n]
}

proc p_rawread_n { } {
#
# Called whenever data is available on the UART (mode N)
#
	global ST PM
#
#  STA = 0  -> Waiting for preamble
#        1  -> Waiting for the length byte
#        2  -> Waiting for (CNT) bytes until the end of packet
#        3  -> Waiting for end of DIAG preamble
#        4  -> Waiting for EOL until end of DIAG
#        5  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""

	while 1 {

		if { $chunk == "" } {

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if { $ST(TIM) != "" } {
					global DB
					if { $DB(LEV) > 2 } {
					  trc "rawread: packet timeout $ST(STA),
					    $ST(CNT)"
					}
					# reset
					catch { after cancel $ST(TIM) }
					set ST(TIM) ""
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					global IV
					set ST(TIM) \
					     [after $IV(PKT) p_rawread_n]
				}
				return
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $CH(IPR) } {
					# preamble found
					set ST(STA) 1
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
					break
				}
			}
			if { $i == $bl } {
				# not found, keep waiting
				set chunk ""
				continue
			}
			# found, remove the parsed portion and keep going
			set chunk [string range $chunk [expr $i + 1] end]
		}

		1 {
			# expecting the length byte (note that the byte
			# does not cover the statid field, so its range is
			# up to MPL - 2)
			binary scan [string index $chunk 0] cu bl
			set chunk [string range $chunk 1 end]
			if { [expr $bl & 1] || $bl > [expr $PM(MPL) - 2] } {
				global DB
				if { $DB(LEV) > 2 } {
					trc "rawread: illegal packet length $bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# how many bytes to expect
			set ST(CNT) [expr $bl + 4]
			set ST(BUF) ""
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				x_receive_n
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			x_receive_n
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			u_outdiag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			u_outdiag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc p_write_n { msg } {
#
# Send out a message
#
	global ST PM DB

	if { $ST(OUT) != "" } {
		# busy
		return 0
	}

	# we still need the four XRS header bytes in front
	set lm [expr $PM(MPL) - 4]
	if { [string length $msg] > $lm } {
		# this verification is probably redundant (we do check the
		# user payload size before submitting it), but it shouldn't
		# hurt
		if { $DB(LEV) > 0 } {
			trc "write: outgoing message truncated to $lm bytes"
		}
		set msg [string range $msg 0 [expr $lm - 1]]
	}

	set ST(OUT) $msg
	x_send_n
	return 1
}

proc p_init_n { } {
#
# Initialize
#
	global CH ST PM

	set CH(EXP) 0
	set CH(CUR) 0
	set CH(MAG) [expr 0xAB]
	set CH(IPR) [format %c [expr 0x55]]
	set ST(MOD) "N"

	# User packet length: in the N mode, the specified length (MPL) covers
	# the "Station ID", which is used for two AB control bytes (and it
	# doesn'c cover CRC - it never does). So the user length (true payload)
	# is MPL - 4 (two extra bytes are needed for "magic" and true payload
	# length.

	set PM(UPL) [expr $PM(MPL) - 4]

	fconfigure $ST(SFD) -buffering full -translation binary

	# start the write callback
	x_send_n
}

set PLUG(N) [list p_init_n p_rawread_n p_write_n]
set PLUG(X) $PLUG(N)

###############################################################################
# Plugin: mode == P ###########################################################
###############################################################################

proc x_frame_p { m cu ex } {
#
# Frame the message (P mode)
#
	upvar $m msg
	global CH PM

	set ln [string length $msg]
	if { $ln > $PM(MPL) } {
		# payload length
		if { $DB(LEV) > 0 } {
			trc "frame: message too long: $ln > $PM(MPL), truncated"
		}
		set ln $PM(MPL)
		set msg [string range $msg 0 [expr $ln - 1]
	}

	if [expr $ln & 1] {
		# odd length, append a dummy byte, but leave the				# length as it was
		append msg $CH(ZER)
	}

	# checksum coverage
	set msg "[binary format cc [expr ($ex << 1) | $cu] $ln]$msg"
	set msg "$msg[binary format s [chks $msg]]"
}

proc x_write_p { msg } {
#
# Send a packet to UART
#
	global DB ST

	if { $DB(LEV) > 1 } {
		dump "W" $msg
	}

	puts -nonewline $ST(SFD) $msg
	flush $ST(SFD)
}

proc x_fnwrite_p { msg cu ex } {
#
# Frame and write in one step
#
	x_frame_p msg $cu $ex
	x_write_p $msg
}

proc x_receive_p { } {
#
# Handle received packet
#
	global ST DB CH IV

	if { $DB(LEV) > 1 } {
		# dump the packet
		dump "R" $ST(BUF)
	}

	# validate CRC
	if [chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			trc "receive: illegal checksum, packet ignored"
		}
		return
	}

	# extract the preamble
	binary scan [lindex $ST(BUF) 0] cu pre
	# extract the payload and AB flags
	set msg [string range $ST(BUF) 2 end-$ST(RPL)]
	set cu [expr $pre & 1]
	set ex [expr ($pre & 2) >> 1]

	if { $ST(OUT) != "" } {
		# we have an outgoing message
		if { $ex != $CH(CUR) } {
			# expected != our current, so we are done with the
			# present message
			set ST(OUT) ""
			set CH(CUR) $ex
		}
	} else {
		# no outgoing message, set current to expected
		set CH(CUR) $ex
	}

	if { $msg == "" } {
		# a pure ACK
		return
	}

	if { $cu != $CH(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(RTS) x_send_p]
		return
	}

	# receive it
	$ST(DFN) $msg

	# new "expected"
	if $CH(EXP) {
		set CH(EXP) 0
	} else {
		set CH(EXP) 1
	}

	# force an ACK
	x_send_p
}

proc x_send_p { } {
#
# Callback for sending messages out
#
	global ST IV CH

	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	if { $ST(OUT) == "" } {
		# there is no outgoing message, just an ACK
		x_fnwrite_p "" 1 $CH(EXP)
	} else {
		x_fnwrite_p $ST(OUT) $CH(CUR) $CH(EXP)
	}

	set ST(SCB) [after $IV(RTL) x_send_p]
}

proc p_rawread_p { } {
#
# Called whenever data is available on the UART (mode P)
#
	global ST PM
#
#  STA = 0  -> Waiting for preamble
#        1  -> Waiting for the length byte
#        2  -> Waiting for (CNT) bytes until the end of packet
#        3  -> Waiting for end of DIAG preamble
#        4  -> Waiting for EOL until end of DIAG
#        5  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""

	while 1 {

		if { $chunk == "" } {

			if [catch { read $ST(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if { $ST(TIM) != "" } {
					global DB
					if { $DB(LEV) > 2 } {
					  trc "rawread: packet timeout $ST(STA),
					    $ST(CNT)"
					}
					# reset
					catch { after cancel $ST(TIM) }
					set ST(TIM) ""
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					global IV
					set ST(TIM) [after $IV(PKT) p_rawread_p]
				}
				return
			}
			# there is something to process, cancel timeout
			if { $ST(TIM) != "" } {
				catch { after cancel $ST(TIM) }
				set ST(TIM) ""
			}
		}

		set bl [string length $chunk]

		switch $ST(STA) {

		0 {
			# waiting for packet preamble
			global CH
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				# there are four preambles to choose from
				if { [string first $c $CH(IPR)] >= 0 } {
					# preamble found
					set ST(STA) 1
					# needed for checksum
					set ST(BUF) $c
					break
				}
				if { $c == $CH(DPR) } {
					# diag preamble
					set ST(STA) 3
					break
				}
			}
			if { $i == $bl } {
				# not found, keep waiting
				set chunk ""
				continue
			}
			# found, remove the parsed portion and keep going
			set chunk [string range $chunk [expr $i + 1] end]
		}

		1 {
			# expecting the length byte
			set c [string index $chunk 0]
			binary scan $c cu bl
			set chunk [string range $chunk 1 end]
			if { $bl > $PM(MPL) } {
				global DB
				if { $DB(LEV) > 2 } {
					trc "rawread: illegal packet length $bl"
				}
				# reset
				set ST(STA) 0
				continue
			}
			# include in message for checksum evaluation
			append ST(BUF) $c
			# adjust on odd length, but do not change formal length
			if [expr $bl & 1] {
				set ST(CNT) [expr $bl + 3]
				# flag == odd length
				set ST(RPL) 3
			} else {
				set ST(CNT) [expr $bl + 2]
				# flag == even length
				set ST(RPL) 2
			}
			# found
			set ST(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}

			# end of packet, reset
			set ST(STA) 0

			if { $bl == $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				x_receive_p
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			x_receive_p
		}

		3 {
			# waiting for the end of a diag header
			global CH
			set chunk [string trimleft $chunk $CH(DPR)]
			if { $chunk != "" } {
				set ST(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $CH(ZER) } {
					# a binary diag, length == 7
					set ST(CNT) 7
					set ST(STA) 5
				} else {
					# ASCII -> wait for NL
					set ST(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append ST(BUF) $chunk
				set chunk ""
				continue
			}

			append ST(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set ST(STA) 0
			u_outdiag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $ST(CNT) } {
				append ST(BUF) $chunk
				set chunk ""
				incr ST(CNT) -$bl
				continue
			}
			# reset
			set ST(STA) 0
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]

			set chunk [string range $chunk $ST(CNT) end]
			u_outdiag
		}

		default {
			global DB
			if { $DB(LEV) > 0 } {
				trc "rawread: illegal state: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc p_write_p { msg } {
#
# Send out a message
#
	global ST PM DB

	if { $ST(OUT) != "" } {
		return 0
	}

	set ll [string length $msg]

	if { $ll > $PM(MPL) } {
		if { $DB(LEV) > 0 } {
			trc "write: outgoing message truncated from $ll to\					$PM(MPL) bytes"
		}
		set msg [string range $msg 0 [expr $PM(MPL) - 1]]
	}

	set ST(OUT) $msg
	x_send_p
	return 1
}

proc p_init_p { } {
#
# Initialize
#
	global CH ST PM

	set CH(EXP) 0
	set CH(CUR) 0

	set CH(IPR) ""
	append CH(IPR) [format %c [expr 0x00]]
	append CH(IPR) [format %c [expr 0x01]]
	append CH(IPR) [format %c [expr 0x02]]
	append CH(IPR) [format %c [expr 0x03]]

	set ST(MOD) "P"

	# In this case, the user length is the same as MPL, as the PHY length
	# specification doesn't cover the header
	set PM(UPL) $PM(MPL)

	fconfigure $ST(SFD) -buffering full -translation binary

	# start the write callback
	x_send_p
}

set PLUG(P) [list p_init_p p_rawread_p p_write_p]

###############################################################################
# Plugin: mode == D ###########################################################
###############################################################################

proc p_rawread_d { } {
#
# Trivial in this case
#
	global ST PM DB

	if { [catch { read $ST(SFD) } sta] || $sta == "" } {
		return
	}

	append ST(BUF) $sta

	while 1 {

		set sta [string first "\n" $ST(BUF)]
		if { $sta < 0 } {
			return
		}

		set msg [string range $ST(BUF) 0 [expr $sta - 1]]
		set ST(BUF) [string range $ST(BUF) [expr $sta + 1] end]

		if { $DB(LEV) > 1 } {
			# dump the packet
			dump "R" $msg
		}

		$ST(DFN) $msg
	}
}

proc p_write_d { msg } {
#
# Send out a message
#
	global ST PM DB

	if { $DB(LEV) > 1 } {
		dump "W" $msg
	}

	catch { puts $ST(SFD) $msg }

	return 1
}

proc p_init_d { } {
#
# Initialize
#
	global CH ST PM

	set ST(MOD) "D"

	fconfigure $ST(SFD) -buffering line -translation { lf crlf }

	set PM(UPL) $PM(MPL)
}

set PLUG(D) [list p_init_d p_rawread_d p_write_d]

###############################################################################
# Tracing and debugging #######################################################
###############################################################################

proc dump { hdr buf } {

	set code ""
	set nb [string length $buf]
	binary scan $buf cu$nb code
	set ol ""
	foreach co $code {
		append ol [format " %02x" $co]
	}
	trc "$hdr:$ol"
}
	
proc d_settrace { lev { file "" } } {
#
# Set tracing parameters
#
	global DB

	if { $DB(SFD) != "" } {
		# close any previous file
		catch { close $DB(SFD) }
		set DB(SFD) ""
	}

	# new level
	set DB(LEV) $lev

	if { $lev == 0 } {
		return ""
	}

	# open
	if { $file != "" } {
		if [catch { open $file "w" } sfd] {
			set DB(LEV) 0
			return "Failed to open debug file, $sfd!"
		}
		set DB(SFD) $sfd
	}

	return ""
}

proc trc { msg } {

	variable DB

	if { $DB(SFD) == "" } {
		set str stdout
	} else {
		set str $DB(SFD)
	}

	catch { 
		puts $str $msg
		flush $str
	}
}

###############################################################################
###############################################################################
###############################################################################

proc inline { } {
#
# Preprocess a line input from the keyboard
#
	global ST

	if [catch { gets stdin line } stat] {
		# ignore errors (can they happen at all?)
		return ""
	}

	if { $stat < 0 } {
		# end of file
		exit 0
	}

	if { $line == "" } {
		# ignore empty lines
		return ""
	}

	# we used to trim here, but perhaps we shouldn't; let's try to avoid
	# any interpretation, because the user may actually want to send a
	# bunch of blanks

	# the trimmed version for special command check
	set ltr [string trim $line]

	if { $ltr == "!!" } {
		# previous command
		if { $ST(PCM) == "" } {
			puts "No previous command!"
			return
		}
		set line $ST(PCM)
	} elseif { [string index $ltr 0] == "!" } {
		# not for the board
		icmd [string trimleft [string range $ltr 1 end]]
		return ""
	} else {
		# last command
		set ST(PCM) $line
	}

	if ![u_ready] {
		puts "Board busy!"
		return ""
	}

	return $line
}

proc sget { } {
#
# STDIN becomes readable, i.e., a line of user input (ASCII mode)
#
	global ST PM

	set line [inline]

	if { $line == "" } {
		# busy, internal, or actually an empty line
		return
	}

	if $ST(ECO) {
		puts $line
	}

	set ln [expr [string length $line] + 1]

	if { $ln > $PM(UPL) } {
		puts "Error: line longer than max payload of $PM(UPL) chars"
		return
	}

	$ST(OFN) $line
}

proc sget_bin { } {
#
# The binary variant of sget
#
	global PM ST BMAC

	set line [inline]

	if { $line == "" } {
		return
	}

	if [catch { macsub $line } line] {
		# macro substitutions
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

		set val ""
		regexp "^(\[^ \t\]+)(.*)" $line mat val line

		if [catch { eval "expr $val" } val] {
			puts "Error: illegal expression: $val ...\
				[string range $line 0 5] ..."
			return
		}

		set val [expr $val & 0xff]

		if $ST(ECO) {
			append eco [format "%02x " $val]
		}

		append out [binary format c $val]
		incr oul
	}

	if $ST(ECO) {
		puts "Sending: < $eco>"
	}

	if { $oul > $PM(UPL) } {
		puts "Error: block longer than max payload of $PM(UPL) bytes"
		return
	}

	$ST(OFN) $out
}

proc uget { msg } {
#
# Receive a line from the board (ASCII mode - just show it)
#
	puts $msg
}

proc uget_bin { msg } {
#
# In binary mode
#
	set len [string length $msg]

	set enc "Received: $len < "

	# encode the bytes in hex
	for { set i 0 } { $i < $len } { incr i } {
		binary scan [string index $msg $i] cu byte
		append enc [format "%02x " $byte]
	}

	append enc ">"

	puts $enc
}

###############################################################################

proc icmd { cmd } {
#
# Handle internal commands
#
	global ST

	set par ""
	if ![regexp "^(\[^ \t\]+)(.*)" $cmd jnk cmd par] {
		# impossible
		puts "Empty command!"
		return
	}
	set par [string trim $par]

	if { $cmd == "e" || $cmd == "echo" } {

		if { $par == "" } {
			# toggle
			if $ST(ECO) {
				set par "off"
			} else {
				set par "on"
			}
		}

		if { $par == "on" } {
			puts "Echo is now on"
			set ST(ECO) 1
		} elseif { $par == "off" } {
			puts "Echo is now off"
			set ST(ECO) 0
		} else {
			puts "Illegal parameter, must be empty, on, or off!
		}
		return
	}

	if { $cmd == "t" || $cmd == "trace" } {

		set fnm ""
		if ![regexp "^(\[0-9\])(.*)" $par jnk lev fnm] {
			puts "Illegal parameters, must be level \[filename\]!"
			return
		}
		set fnm [string trim $fnm]

		set err [d_settrace $lev $fnm]
		if { $err == "" } {
			set err "OK"
		}
		puts $err
		return
	}

	puts "Unknown internal command $cmd!"
}

###############################################################################

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

proc readmac { bin } {
#
# Read the macro file for binary mode
#
	global BMAC

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

###############################################################################

proc usage { } {

	global argv0

	puts stderr "Usage: $argv0 args   where args can be:"
	puts stderr ""
	puts stderr "       -p port/dev   UART dev or COM number, required"
	puts stderr "       -s speed      UART speed, default is 9600"
	puts stderr "       -m x|p|d      mode: XRS, P, direct, default is d"
	puts stderr "       -l pktlen     max pkt len, default is 82"
	puts stderr "       -b \[file\]     binary mode (optional macro file)"
	puts stderr ""
	puts stderr "Note that pktlen should be the same as the length used"
	puts stderr "by the praxis in the respective argument of phys_uart."

	exit 99
}

proc initialize { } {

	global argv ST

	set prt ""
	set spd ""
	set mpl ""
	set bin ""
	set mod ""

	# parse command line arguments

	while 1 {

		set par [lindex $argv 0]

		if { $par == "" } {
			# done
			break
		}

		if ![regexp "^-(\[a-z\])$" $par jnk par] {
			# must be a flag
			usage
		}

		set argv [lrange $argv 1 end]

		# the following value
		set val [lindex $argv 0]

		if ![regexp "^-(\[a-z\])$" $val jnk par] {
			# if it looks like a flag, assume the value is empty
			set argv [lrange $argv 1 end]
		} else {
			set val ""
		}

		# the mode flag
		if { $par == "m" && $mod == "" && \
		    ($val == "x" || $val == "p" || $val == "d") } {
			set mod $val
			continue
		}

		# the UART device
		if { $par == "p" && $prt == "" && $val != "" } {
			set prt $val
			continue
		}

		# speed
		if { $par == "s" && $spd == "" } {
			if { [catch { expr $val } spd] || $spd <= 0 } {
				usage
			}
			continue
		}

		# length
		if { $par == "l" && $mpl == "" } {
			if { [catch { expr $val } mpl] || $mpl <= 0 || \
			    $mpl > 250 } {
				usage
			}
			# FIXME: the max may depend on the mode
			continue
		}

		# binary
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

	# cleanups and defaults
	if { $mod == "" } {
		set mod "d"
	}

	set mod [string toupper $mod]

	if { $prt == "" } {
		usage
	}

	if { $spd == "" } {
		set spd 9600
	}

	if { $mpl == "" } {
		set mpl 82
	}

	if { $bin != "" } {
		set ifu "uget_bin"
		set sfu "sget_bin"
		set ST(BIN) 1
		readmac $bin
	} else {
		set ifu "uget"
		set sfu "sget"
		set ST(BIN) 0
	}

	u_start $prt $spd $mpl $mod $ifu

	fconfigure stdin -buffering line -blocking 0 -eofchar ""
	fileevent stdin readable $sfu
}


initialize

vwait None
