package provide fesc 1.0

###############################################################################
# FESC ########################################################################
###############################################################################

# This is the F-esc mode packet interface as in Alphanet 1.5

namespace eval FESC {

variable B

# character zero (aka NULL)
set B(ZER) [format %c [expr 0x00]]

# STX, DLE, ETX
set B(IPR) [format %c [expr 0x02]]
set B(DLE) [format %c [expr 0x10]]
set B(ETX) [format %c [expr 0x03]]

# diag preamble (for packet modes) = ASCII DLE
set B(DPR) [format %c [expr 0x10]]

# initial parity
set B(PAI) [expr { 0x02 + 0x03 }]

###############################################################################

proc fe_diag { } {

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

	return "::FESC::$fun"
}

proc fe_emu_readable { fun } {
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

	set B(ONR) [after $B(ROT) "[lize fe_emu_readable] $fun"]
}

proc fe_write { msg } {

	variable B

	set out $B(IPR)
	set par [expr { (-$B(PAI)) & 0xFF }]
	set lm [string length $msg]

	if { $lm > $B(MPL) } {
		# truncate to size
		set lm $B(MPL)
	}

	for { set i 0 } { $i < $lm } { incr i } {
		set c [string index $msg $i]
		if { $c == $B(IPR) || $c == $B(ETX) || $c == $B(DLE) } {
			# need to escape
			append out $B(DLE)
		}
		append out $c
		# parity
		binary scan $c cu v
		set par [expr { ($par - $v) & 0xFF }]
	}

	set par [binary format c $par]
	if { $par == $B(IPR) || $par == $B(ETX) || $par == $B(DLE) } {
		set par "$B(DLE)$par"
	}

	# complete
	append out "${par}$B(ETX)"

	if [catch {
		puts -nonewline $B(SFD) $out
		flush $B(SFD)
	}] {
		fesc_close "FESC write error"
	}
}

proc fe_rawread { } {
#
# Called whenever data is available on the UART
#
	variable B
#
#  STA = 0  -> Waiting for STX
#        1  -> Waiting for payload bytes + ETX
#        2  -> Waiting for end of DIAG preamble
#        3  -> Waiting for EOL until end of DIAG
#        4  -> Waiting for (CNT) bytes until the end of binary diag
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
				return $void
			}

			set void 0
		}

		set bl [string length $chunk]

		switch $B(STA) {

		0 {
			# waiting for STX
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if { $c == $B(DPR) } {
					# diag preamble
					set B(STA) 2
					break
				}
				if { $c == $B(IPR) } {
					# STX
					set B(STA) 1
					set B(ESC) 0
					set B(BUF) ""
					set B(BFL) 0
					# initialize the parity byte
					set B(PAR) $B(PAI)
					break
				}
			}
			if { $i == $bl } {
				set chunk ""
				continue
			}


			# remove the parsed out portion of the input string
			incr i
			set chunk [string range $chunk $i end]
		}

		1 {
			# parse the chunk for escapes
			for { set i 0 } { $i < $bl } { incr i } {
				set c [string index $chunk $i]
				if !$B(ESC) {
					if { $c == $B(ETX) } {
						# that's it
						set B(STA) 0
						set chunk [string range $chunk \
							[expr { $i + 1 }] end]
						if { $B(BFL) > $B(MPM) ||
						     $B(PAR) } {
							# too long or parity
							break
						}
						# remove the parity byte
						$B(DFN) [string range \
							$B(BUF) 0 end-1]
						break
					}
					if { $c == $B(IPR) } {
						# reset
						set B(STA) 0
						set chunk [string range $chunk \
							$i end]
						break
					}
					if { $c == $B(DLE) } {
						# escape
						set B(ESC) 1
						continue
					}
				} else {
					set B(ESC) 0
				}
				if { $B(BFL) < $B(MPM) } {
					append B(BUF) $c
					binary scan $c cu v
					set B(PAR) [expr { ($B(PAR) + $v)
						& 0xFF }]
				}
				incr B(BFL)
			}
			if $B(STA) {
				set chunk ""
			}
		}


		2 {
			# waiting for the first non-DLE byte
			set chunk [string trimleft $chunk $B(DPR)]
			if { $chunk != "" } {
				set B(BUF) ""
				if { [string index $chunk 0] == $B(ZER) } {
					# a binary diag, length == 7
					set B(CNT) 7
					set B(STA) 4
				} else {
					# ASCII -> wait for NL
					set B(STA) 3
				}
			}
		}

		3 {
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
			fe_diag
		}

		4 {
			# waiting for CNT bytes of binary diag
			if { $bl < $B(CNT) } {
				append B(BUF) $chunk
				set chunk ""
				incr B(CNT) -$bl
				continue
			}
			# reset
			set B(STA) 0
			append B(BUF) [string range $chunk 0 \
				[expr $B(CNT) - 1]]

			set chunk [string range $chunk $B(CNT) end]
			fe_diag
		}

		default {
			set B(STA) 0
		}
		}
	}
}

proc fesc_init { ufd mpl { inp "" } { dia "" } { clo "" } { emu 0 } } {
#
# Initialize: 
#
#	ufd - UART descriptor
#	mpl - max packet length
#	inp - function to be called on user input
#	dia - function to be called to present a diag message
#	clo - function to call on UART close (can happen asynchronously)
#	emu - emulate 'readable'
#
	variable B

	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(SFD) $ufd
	set B(MPL) $mpl
	set B(MPM) [expr { $mpl + 1 }]

	set B(UCF) $clo

	set B(DFN) $inp
	set B(DGO) $dia

	fconfigure $B(SFD) -buffering full -translation binary

	if $emu {
		# the readable flag doesn't work for UART on some Cygwin
		# setups
		set B(ROT) 1
		set B(RMX) $emu
		fe_emu_readable [lize fe_rawread]
	} else {
		# do it the easy way
		fileevent $B(SFD) readable [lize fe_rawread]
	}
}

proc fesc_close { { err "" } } {
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
	set B(STA) 0
	set B(CNT) 0
	set B(BUF) ""

	set B(DFN) ""
	set B(DGO) ""
}

proc fesc_send { buf } {
#
# This is the user-level output function
#
	variable B

	if { $B(SFD) == "" } {
		# ignore if disconnected, this shouldn't happen
		return
	}

	fe_write $buf
}

namespace export fesc_*

}

namespace import ::FESC::*
