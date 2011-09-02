package provide oss_u 1.0

#
# UART interface for packet protocols: N and P
#
namespace eval OSSU {

variable ST
variable CH
variable DB
variable IV
variable PM

# tracing
set DB(SFD)	""
set DB(LEV)	0

# packet preamble (in "P" mode, it is a string, i.e., multiple characters)
set CH(IPR)		""

# diag preamble: ASCII DLE
set CH(DPR)		[format %c [expr 0x10]]

# character zero
set CH(ZER)		[format %c [expr 0x00]]

# Packet timeout, once reception has started
set IV(PKT)		80

# Maximum packet/line length (this one always related to pure payload)
set PM(MPL)		80

# ISO 3309 CRC table
variable CRCTAB	{
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

proc u_systype { } {
#
# Return the system type, like in Linux, Windows ...
#
	variable SysType

	if [info exists SysType] {
		return $SysType
	}

	if [catch { exec uname } un] {
		set SysType "W"
	} elseif [regexp -nocase "linux" $un] {
		set SysType "L"
	} elseif [regexp -nocase "cygwin" $un] {
		set SysType "C"
	} else {
		set SysType "W"
	}

	return $SysType
}

proc u_cdevl { pi } {
#
# Returns the candidate list of devices to open based on the port identifier
#
	if { [regexp "^\[0-9\]+$" $pi] && ![catch { expr $pi } pn] } {
		# looks like a number
		if { $pn < 10 } {
			# use internal Tcl COM id, which is faster
			set wd "COM${pn}:"
		} else {
			set wd "\\\\.\\COM$pn"
		}
		return [list $wd "/dev/ttyUSB$pn" "/dev/tty$pn"]
	}

	# not a number
	return [list $pi "\\\\.\\$pi" "/dev/$pi" "/dev/tty$pi"]
}

proc u_start { udev speed mod { mpl $PM(MPL) } } {
#
# Initialize UART
#
	variable ST
	variable CH
	variable PM

	if { $udev == "" } {
		error "u_start: UART device must be specified"
	}

	set devlist [u_cdevl $udev]

	set fail 1

	if { [u_systype] == "L" } {
		set accs { RDWR NOCTTY NONBLOCK }
	} else {
		set accs "r+"
	}

	foreach udev $devlist {
		if ![catch { open $udev $accs } ST(SFD)] {
			set fail 0
			break
		}

	}

	if $fail {
		error "u_start: cannot open UART, device(s) $devlist"
	}

	# initialize status
	set ST(TIM)  ""
	# reception automaton state
	set ST(STA) 0
	set ST(CNT) 0
	# input buffer
	set ST(BUF) ""

	# set the mode
	set mod [string toupper [string index $mod 0]]

	set bf "full"
	set tr "binary"

	if { $mod == "P" } {
		# persistent UART mode: four preambles
		set CH(IPR) ""
		append CH(IPR) [format %c [expr 0x00]]
		append CH(IPR) [format %c [expr 0x01]]
		append CH(IPR) [format %c [expr 0x02]]
		append CH(IPR) [format %c [expr 0x03]]
		set ST(MOD) "P"
		set rf "rawread_p"
	} elseif { $mod == "N" } {
		# Non-persistent UART, e.g., XRS
		set CH(IPR) [format %c [expr 0x55]]
		set rf "rawread_n"
	} else {
		# Direct/line
		set CH(IPR) ""
		set mod "D"
		set bf "line"
		set tr "{ auto crlf }"
		set rf "rawread_d"
	}
	
	set ST(MOD) $mod
	set PM(MPL) $mpl

	if [catch { fconfigure $ST(SFD) -mode "$speed,n,8,1" -handshake none \
	    -buffering $bf -translation $tr -blocking 0 -eofchar "" } err] {
		error "u_start: cannot configure UART: $err"
	}

	fileevent $ST(SFD) readable ::OSSU::$rf
}

proc u_setif { { dfun "" } { mpl "" } } {
#
# Reset the input function
#
	variable ST
	variable CH

	if { $dfun != "" } {
		if { [string range $dfun 0 1] != "::" } {
			set dfun "::$dfun"
		}
		if { $mpl != "" } {
			set PM(MPL) $mpl
		}
	}

	# null dfun disables
	set ST(DFN) $dfun
}

proc u_settrace { lev { file "" } } {
#
# Set tracing parameters
#
	variable DB

	set DB(LEV) $lev

	if { $lev == 0 } {
		# closing
		if { $DB(SFD) != "" } {
			catch { close $DB(SFD) }
			set DB(SFD) ""
		}
		return
	}

	if { $DB(SFD) == "" } {
		# open
		if { $file != "" } {
			if [catch { open $file "w" } sfd] {
				return
			}
			set DB(SFD) $sfd
		}
	}
}
			
proc u_frame { m { ou 0 } { ex 0 } } {
#
# Frame the message in place
#
	upvar $m msg
	variable CH
	variable PM
	variable ST

	set ln [string length $msg]
	if { $ln > $PM(MPL) } {
		error "u_frame: message too long: $ln > $PM(MPL)"
	}

	if { $ST(MOD) == "D" } {
		return
	}

	if { $ST(MOD) == "P" } {
		# persistent UART mode
		if [expr $ln & 1] {
			# odd length, append a dummy byte, but leave the				# length as it was
			append msg \x00
		}
		# checksum coverage
		set msg "[binary format cc [expr ($ex << 1) | $ou] $ln]$msg"
		set msg "$msg[binary format s [chks $msg]]"
		return
	}

	# N mode
	if { $ln < 2 } {
		error "u_frame: message too short ($ln)"
	}

	if [expr $ln & 1] {
		# odd length, append NULL
		append msg \x00
		# do not count the LID, which we use explicitly
		incr ln -1
	} else {
		incr ln -2
	}

	set msg "$CH(IPR)[binary format c $ln]$msg[binary format s [chks $msg]]"
}

proc u_write { msg } {
#
# Send a packet to UART (exported)
#
	variable ST
	variable DB

	if { $DB(LEV) > 1 } {
		dump "W" $msg
	}

	if { $ST(MOD) == "D" } {
		puts $ST(SFD) $msg
	} else {
		puts -nonewline $ST(SFD) $msg
		flush $ST(SFD)
	}
}

proc u_fnwrite { msg { ou 0 } { ex 0 } } {
#
# Frame and write in one step
#
	variable ST
	variable CH

	u_frame msg $ou $ex
	u_write $msg
}

###############################################################################

proc chks { wa } {

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
	
proc rawread_n { } {
#
# Called whenever data is available on the UART (mode N)
#
	variable ST
	variable PM
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
					variable DB
					if { $DB(LEV) > 2 } {
						trc "packet timeout $ST(STA),
							$ST(CNT)"
					}
					# reset
					catch { after cancel $ST(TIM) }
					set ST(TIM) ""
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					variable IV
					set ST(TIM) \
					     [after $IV(PKT) ::OSSU::rawread_n]
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
			variable CH
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
			# expecting the length byte
			binary scan [string index $chunk 0] cu bl
			set chunk [string range $chunk 1 end]
			if { [expr $bl & 1] || $bl > $PM(MPL) } {
				variable DB
				if { $DB(LEV) > 2 } {
					trc "illegal packet length $bl"
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
				runit
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			runit
		}

		3 {
			# waiting for the end of a diag header
			variable CH
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
			outdiag
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
			outdiag
		}

		default {
			variable DB
			if { $DB(LEV) > 0 } {
				trc "illegal state in rawread: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc rawread_p { } {
#
# Called whenever data is available on the UART (mode P)
#
	variable ST
	variable PM
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
					variable DB
					if { $DB(LEV) > 2 } {
						trc "packet timeout $ST(STA),
							$ST(CNT)"
					}
					# reset
					catch { after cancel $ST(TIM) }
					set ST(TIM) ""
					set ST(STA) 0
				} elseif { $ST(STA) != 0 } {
					# something has started, set up timer
					variable IV
					set ST(TIM) \
					     [after $IV(PKT) ::OSSU::rawread_p]
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
			variable CH
			# P-mode
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
			# P-mode UART
			if { $bl > $PM(MPL) } {
				variable DB
				if { $DB(LEV) > 2 } {
					trc "illegal packet length $bl"
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
				runit
				continue
			}

			# merged packets
			append ST(BUF) [string range $chunk 0 \
				[expr $ST(CNT) - 1]]
			set chunk [string range $chunk $ST(CNT) end]
			runit
		}

		3 {
			# waiting for the end of a diag header
			variable CH
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
			outdiag
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
			outdiag
		}

		default {
			variable DB
			if { $DB(LEV) > 0 } {
				trc "illegal state in rawread: $ST(STA)"
			}
			set ST(STA) 0
		}
		}
	}
}

proc rawread_d { } {
#
# Called whenever data is available on the UART (mode D)
#
	variable ST
	variable PM
#
# We are line-buffered, so we can read one line at a time
#
	if { [catch { gets $ST(SFD) chunk } sta] || $sta < 0 } {
		return
	}

	if { $ST(DFN) != "" } {
		$ST(DFN) $chunk
	}
}

proc runit { } {

	variable ST
	variable DB

	if { $DB(LEV) > 1 } {
		# dump the packet
		dump "R" $ST(BUF)
	}

	# validate CRC
	if [chks $ST(BUF)] {
		if { $DB(LEV) > 2 } {
			trc "illegal checksum, packet ignored"
		}
		return
	}

	# user handler present?
	if { $ST(DFN) == "" } {
		# no, nothing else to do
		return
	}

	if { $ST(MOD) == "P" } {
		# extract the preamble
		binary scan [lindex $ST(BUF) 0] cu pre
		set ST(BUF) [string range $ST(BUF) 2 end-$ST(RPL)]
		$ST(DFN) $ST(BUF) [expr $pre & 1] [expr ($pre & 2) >> 1]
	} else {
		# strip off the checksum
		set ST(BUF) [string range $ST(BUF) 0 end-2]
		$ST(DFN) $ST(BUF)
	}
}

proc outdiag { } {

	variable ST
	variable CH

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

namespace export u_*

### end of OSSU namespace #####################################################

}

namespace import ::OSSU::*

###############################################################################
