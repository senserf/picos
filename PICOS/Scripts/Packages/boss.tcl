package provide boss 1.0

###############################################################################
# BOSS ########################################################################
###############################################################################

namespace eval BOSS {

###############################################################################
# ISO 3309 CRC + supplementary stuff needed by the protocol module ############
###############################################################################

variable CRCTAB {
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

variable B

# abort function (in case of internal fatal error, considered impossible)
set B(ABR) ""

# diag output function
set B(DGO) ""

# character zero (aka NULL)
set B(ZER) [format %c [expr 0x00]]

# the ACK flag
set B(ACK) [expr 0x04]

# direct packet type
set B(MAD) [expr 0xAC]

# acked packet type
set B(MAP) [expr 0xAD]

# preamble byte
set B(IPR) [format %c [expr 0x55]]

# diag preamble (for packet modes) = ASCII DLE
set B(DPR) [format %c [expr 0x10]]

# output busy flag (current message pending) + output event variable; also
# used as main event trigger for all vital events
set B(OBS) 0

# output queue
set B(OQU) ""

# reception automaton state
set B(STA) 0

# reception automaton remaining byte count
set B(CNT) 0

# send callback
set B(SCB) ""

# long retransmit interval
set B(RTL) 2048

# short retransmit interval
set B(RTS) 250

# function to call on packet reception
set B(DFN) "bo_nop"

# function to call on UART close (which can happen asynchronously)
set B(UCF) ""

# low-level reception timer
set B(TIM)  ""

# packet timeout (msec), once reception has started
set B(PKT) 80

###############################################################################

proc bo_nop { args } { }

proc boss_chks { wa } {

	variable CRCTAB

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

proc bo_abort { msg } {

	variable B

	if { $B(ABR) != "" } {
		$B(ABR) $msg
		exit 1
	}

	catch { puts stderr $msg }
	exit 1
}

proc bo_diag { } {

	variable B

	if { [string index $B(BUF) 0] == $B(ZER) } {
		# binary
		set ln [string range $B(BUF) 3 5]
		binary scan $ln cuSu lv code
		set ln "\[[format %02x $lv] -> [format %04x $code]\]"
	} else {
		# ASCII
		set ln "[string trim $B(BUF)]"
	}

	if { $B(DGO) != "" } {
		$B(DGO) $ln
	} else {
		puts "DIAG: $ln"
		flush stdout
	}
}

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::BOSS::$fun"
}

proc bo_emu_readable { fun } {
#
# Emulates auto read on readable UART
#
	variable B

	if [$fun] {
		# a void call, increase the timeout
		if { $B(ROT) < $B(RMX) } {
			incr B(ROT)
		}
	} else {
		set B(ROT) 0
	}

	set B(ONR) [after $B(ROT) "[lize bo_emu_readable] $fun"]
}

proc bo_fnwrite { msg } {
#
# Writes a packet to the UART
#
	variable B

	set ln [string length $msg]
	if { $ln > $B(MPL) } {
		bo_abort "assert fnwrite: $ln > max ($B(MPL))"
	}

	if { $ln < 2 } {
		bo_abort "assert fnwrite: length ($ln) < 2"
	}

	if [expr $ln & 1] {
		# need a filler zero byte
		append msg $B(ZER)
		incr ln -1
	} else {
		incr ln -2
	}

	if [catch {
		puts -nonewline $B(SFD) \
			"$B(IPR)[binary format c $ln]$msg[binary format s\
				[boss_chks $msg]]"
		flush $B(SFD)
	}] {
		boss_close "BOSS write error"
	}
}

proc bo_send { } {
#
# Callback for sending packets out
#
	variable B
	
	# cancel the callback, in case called explicitly
	if { $B(SCB) != "" } {
		catch { after cancel $B(SCB) }
		set B(SCB) ""
	}

	if !$B(OBS) {
		# just in case, this is a consistency invariant
		set B(OUT) ""
	}

	set len [string length $B(OUT)]
	# if len is nonzero, an outgoing message is pending; its length
	# has been checked already (and is <= MAXPL)

	set flg [expr $B(CUR) | ( $B(EXP) << 1 )]

	if { $len == 0 } {
		# ACK only
		set flg [expr $flg | $B(ACK)]
	}

	bo_fnwrite "[binary format cc $B(MAP) $flg]$B(OUT)"

	set B(SCB) [after $B(RTL) [lize bo_send]]
}

proc bo_write { msg { byp 0 } } {
#
# Write out a message
#
	variable B

	set lm [expr $B(MPL) - 2]

	if { [string length $msg] > $lm } {
		# truncate the message to size, probably a bad idea
		set msg [string range $msg 0 [expr $lm - 1]]
	}

	if $byp {
		# immediate output, direct protocol
		bo_fnwrite "[binary format cc $B(MAD) 0]$msg"
		return
	}

	if $B(OBS) {
		bo_abort "assert write: output busy"
	}

	set B(OUT) $msg
	set B(OBS) 1

	bo_send
}

proc bo_timeout { } {

	variable B

	if { $B(TIM) != "" } {
		bo_rawread 1
		set B(TIM) ""
	}
}

proc bo_rawread { { tm 0 } } {
#
# Called whenever data is available on the UART (mode S)
#
	variable B
#
#  STA = 0  -> Waiting for preamble
#        1  -> Waiting for the length byte
#        2  -> Waiting for (CNT) bytes until the end of packet
#        3  -> Waiting for end of DIAG preamble
#        4  -> Waiting for EOL until end of DIAG
#        5  -> Waiting for (CNT) bytes until the end of binary diag
#
	set chunk ""
	set void 1

	while 1 {

		if { $chunk == "" } {

			if [catch { read $B(SFD) } chunk] {
				# nonblocking read, ignore errors
				set chunk ""
			}

			if { $chunk == "" } {
				# check for timeout
				if $tm {
					# reset
					set B(STA) 0
				} elseif { $B(STA) != 0 } {
					# something has started, set up timer
					# if not running already
					if { $B(TIM) == "" } {
						set B(TIM) \
				            	    [after $B(PKT) \
							[lize bo_timeout]]
					}
				}
				return $void
			}
			# there is something to process, cancel timeout
			if { $B(TIM) != "" } {
				catch { after cancel $B(TIM) }
				set B(TIM) ""
			}
			set void 0
		}

		set bl [string length $chunk]

		switch $B(STA) {

		0 {
			# Look up the preamble byte in the received string
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $B(IPR) } {
					# preamble found
					set B(STA) 1
					break
				}
				if { $c == $B(DPR) } {
					# diag preamble
					set B(STA) 3
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
			if { [expr $bl & 1] || $bl > [expr $B(MPL) - 2] } {
				# reset
				set B(STA) 0
				continue
			}
			# how many bytes to expect
			set B(CNT) [expr $bl + 4]
			set B(BUF) ""
			# found
			set B(STA) 2
		}

		2 {
			# packet reception, filling the buffer
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}

			# end of packet, reset
			set B(STA) 0

			if { $bl == $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				# we have a complete buffer
				bo_receive
				continue
			}

			# merged packets
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			bo_receive
		}

		3 {
			# waiting for the end of a diag header
			set chunk [string trimleft $chunk $B(DPR)]
			if { $chunk != "" } {
				set B(BUF) ""
				# look at the first byte of diag
				if { [string index $chunk 0] == $B(ZER) } {
					# a binary diag, length == 7
					set B(CNT) 7
					set B(STA) 5
				} else {
					# ASCII -> wait for NL
					set B(STA) 4
				}
			}
		}

		4 {
			# waiting for NL ending a diag
			set c [string first "\n" $chunk]
			if { $c < 0 } {
				append B(BUF) $chunk
				set chunk ""
				continue
			}

			append B(BUF) [string range $chunk 0 $c]
			set chunk [string range $chunk [expr $c + 1] end]
			# reset
			set B(STA) 0
			bo_diag
		}

		5 {
			# waiting for CNT bytes of binary diag
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}
			# reset
			set B(STA) 0
			append B(BUF) [string range $chunk 0 [expr $B(CNT) - 1]]
			set chunk [string range $chunk $B(CNT) end]
			bo_diag
		}

		default {
			set B(STA) 0
		}
		}
	}
}

proc bo_receive { } {
#
# Handle a received packet
#
	variable B
	
	# dmp "RCV" $B(BUF)

	# validate CRC
	if [boss_chks $B(BUF)] {
		return
	}

	# strip off the checksum
	set msg [string range $B(BUF) 0 end-2]
	set len [string length $msg]

	if { $len < 2 } {
		# ignore it
		return
	}

	# extract the header
	binary scan $msg cucu pr fg

	# trim the message
	set msg [string range $msg 2 end]

	if { $pr == $B(MAD) } {
		# direct, receive right away, nothing else to do
		$B(DFN) $msg
		# count received messages as a form of heartbeat
		return
	}

	if { $pr != $B(MAP) } {
		# wrong magic
		return
	}

	set cu [expr $fg & 1]
	set ex [expr ($fg & 2) >> 1]
	set ac [expr $fg & 4]

	if $B(OBS) {
		# we have an outgoing message
		if { $ex != $B(CUR) } {
			# expected != our current, we are done with this
			# packet
			set B(OUT) ""
			set B(CUR) $ex
			set B(OBS) 0
		}
	} else {
		# no outgoing message, set current to expected
		set B(CUR) $ex
	}

	if $ac {
		# treat as pure ACK and ignore
		return
	}

	if { $cu != $B(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $B(SCB) }
		set B(SCB) [after $B(RTS) [lize bo_send]]
		return
	}

	# receive it
	$B(DFN) $msg

	# update expected
	set B(EXP) [expr 1 - $B(EXP)]

	# force an ACK
	bo_send
}

proc boss_reset { } {
#
# Protocol reset
#
	variable B

	# queue of outgoing messages
	set B(OQU) ""

	# current outgoing message
	set B(OUT) ""

	# output busy flag
	set B(OBS) 0

	# expected
	set B(EXP) 0

	# current
	set B(CUR) 0

	if { $B(SCB) != "" } {
		# abort the callback
		catch { after cancel $B(SCB) }
		set B(SCB) ""
	}
}

proc boss_init { ufd mpl { clo "" } { emu 0 } } {
#
# Initialize: 
#
#	ufd - UART descriptor
#	mpl - max packet length
#	clo - function to call on UART close (can happen asynchronously)
#	emu - emulate 'readable'
#
	variable B

	boss_reset

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(SFD) $ufd
	set B(MPL) $mpl

	set B(UCF) $clo

	# User packet length: in the S mode, the Network ID field is used by
	# the protocol
	set B(UPL) [expr $B(MPL) - 2]

	fconfigure $B(SFD) -buffering full -translation binary

	# start the write callback for the persistent stream
	bo_send

	# insert the auto-input handler
	if $emu {
		# the readable flag doesn't work for UART on some Cygwin
		# setups
		set B(ROT) 1
		set B(RMX) $emu
		bo_emu_readable "[lize bo_rawread]"
	} else {
		# do it the easy way
		fileevent $B(SFD) readable "[lize bo_rawread]"
	}
}

proc boss_oninput { { fun "" } } {
#
# Declares a function to be called when a packet is received
#
	variable B

	if { $fun == "" } {
		set fun "bo_nop"
	} else {
		set fun [gize $fun]
	}

	set B(DFN) $fun
}

proc boss_trigger { } {
	set ::BOSS::B(OBS) $::BOSS::B(OBS)
}

proc boss_wait { } {

	variable B

	if !$B(OBS) {
		# not busy
		if { $B(OQU) != "" } {
			# something in queue
			if { $B(SFD) != "" } {
				bo_write [lindex $B(OQU) 0]
				set B(OQU) [lrange $B(OQU) 1 end]
			} else {
				# drop everything, this is just a precaution
				set B(OQU) ""
			}
		}
	}

	vwait ::BOSS::B(OBS)
}

proc boss_stop { } {
#
# Stop the protocol
#
	variable B

	foreach cb { "SCB" "TIM" } {
		# kill callbacks
		if { $B($cb) != "" } {
			catch { after cancel $B($cb) }
			set B($cb) ""
		}
	}

	boss_reset

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""
}

proc boss_close { { err "" } } {
#
# Close the UART (externally or internally, which can happen asynchronously)
#
	variable B

	if { [info exist B(ONR)] && $B(ONR) != "" } {
		# we have been emulating 'readable', kill the callback
		catch { after cancel $B(ONR) }
		unset B(ONR)
	}

	if { $B(UCF) != "" } {
		# any extra function to call?
		set bucf $B(UCF)
		# prevents recursion loops
		set B(UCF) ""
		$bucf $err
	}

	catch { close $B(SFD) }

	set B(SFD) ""

	# stop the protocol
	boss_stop

	boss_oninput

	boss_trigger
}

proc boss_send { buf { urg 0 } } {
#
# This is the user-level output function
#
	variable B

	if { $B(SFD) == "" } {
		# ignore if disconnected, this shouldn't happen
		return
	}

	# dmp "SND<$urg>" $buf

	if $urg {
		bo_write $buf 1
		return
	}

	lappend B(OQU) $buf

	boss_trigger
}

proc boss_timeouts { slow { fast 0 } } {
#
# Sets the timeouts:
#
#	slow	periodic ACKS
#	fast	nack after unexpected packet
#
	variable B

	if { $slow == 0 } {
		set slow 1000000000
	}
	set B(RTL) $slow
	if $fast {
		set B(RTS) $fast
	}

	if { $B(SCB) != "" } {
		bo_send
	}
}

namespace export boss_*

}

namespace import ::BOSS::*
