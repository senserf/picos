package provide oss_u_xrs 1.0

package require oss_u 1.0

#
# XRS
#

namespace eval OSSUAB {

variable ST
variable IV

# the AB counters
variable AB

set AB(EXP)	0
set AB(CUR)	0
set AB(MAG)	[expr 0xAB]

# long retransmission delay
set IV(long)	2000
# short retransmission delay
set IV(short)	250

set ST(OUT)	""
# send callback
set ST(SCB)	""
# input interceptor
set ST(DFN)	""

# maximum packet length
set PM(MPL)	56

proc u_ab_setif { { dfun "" } { mpl "" } } {
#
# Start/resume the protocol
#
	variable ST

	if { $dfun != "" && [string range $dfun 0 1] != "::" } {
		set dfun "::$dfun"
	}

	set ST(DFN) $dfun

	if { $dfun != "" } {
		# install our input interceptor
		::OSSU::u_setif ::OSSUAB::u_ab_rcv $mpl
		# start the sender callback
		sendit
		if { $mpl != "" } {
			variable PM
			# the specified value is the same as for the PHY,
			# so it excludes the CRC, but includes everything
			# else
			set PM(MPL) [expr $mpl - 4]
		}
	} else {
		# remove the input interceptor
		::OSSU::u_setif
	}
}

proc u_ab_rcv { msg } {
#
# Called whenever a message is received
#
	variable ST
	variable AB
	variable IV

	set len [string length $msg]
	if { $len < 4 } {
		# ignore it
		return
	}

	# extract the header
	binary scan $msg cccc cu ex ma le

	if { [expr $ma & 0xff] != $AB(MAG) } {
		# wrong magic
		return
	}
	set ex [expr $ex & 0xff]

	if { $ST(OUT) != "" } {
		# we have an outgoing message
		if { $ex != $AB(CUR) } {
			# expected != our current, we are done with this
			# message
			set ST(OUT) ""
			set AB(CUR) $ex
		}
	} else {
		# no outgoing message, set current to expected
		set AB(CUR) $ex
	}

	if { $len == 4 } {
		# empty message, treat as pure ACK and ignore
		return
	}

	set cu [expr $cu & 0x00ff]
	if { $cu != $AB(EXP) } {
		# not what we expect, speed up the NAK
		catch { after cancel $ST(SCB) }
		set ST(SCB) [after $IV(short) ::OSSUAB::sendit]
		return
	}

	if { $ST(DFN) == "" } {
		# no receive function present
		return
	}

	set le [expr $le & 0x00ff]
	if { $le > [expr $len - 4] } {
		# consistency check
		return
	}

	# receive it
	$ST(DFN) [string range $msg 4 [expr 3 + $le]]

	# update expected
	set AB(EXP) [expr ( $AB(EXP) + 1 ) & 0x00ff]

	# force an ACK
	sendit
}

proc sendit { } {
#
# Callback for sending messages out
#
	variable ST
	variable IV
	variable AB

	# cancel the callback, in case called explicitly
	if { $ST(SCB) != "" } {
		catch { after cancel $ST(SCB) }
		set ST(SCB) ""
	}

	if { $ST(DFN) == "" } {
		# we are switched off, just go away
		return
	}

	# if len > 0, an outgoing message is pending; note: its length has been
	# checked already (and is <= MAXPL)
	set len [string length $ST(OUT)]

	::OSSU::u_fnwrite \
		"[binary format cccc $AB(CUR) $AB(EXP) $AB(MAG) $len]$ST(OUT)"

	set ST(SCB) [after $IV(long) ::OSSUAB::sendit]
}

proc u_ab_write { msg } {
#
# Send out a message
#
	variable ST

	if { $ST(OUT) != "" } {
		return 0
	}

	variable PM

	if { [string length $msg] > $PM(MPL) } {
		set msg [string range $msg 0 [expr $PM(MPL) - 1]]
	}

	set ST(OUT) $msg
	sendit
	return 1
}

proc u_ab_ready { } {
#
# Check if can accept a new message
#
	variable ST

	if { $ST(OUT) == "" } {
		return 1
	} else {
		return 0
	}
}

proc wcancel { } {
#
	variable ST

	set ST(OUT) $ST(OUT)
}

proc u_ab_wait { tm } {
#
# Wait for output available or timeout
#
	variable ST

	if { $ST(OUT) == "" } {
		return 0
	}

	if { $tm != "" } {
		set tm [after $tm ::OSSUAB::wcancel]
	}

	vwait ::OSSUAB::ST(OUT)

	catch { after cancel $tm }

	if { $ST(OUT) == "" } {
		return 0
	}
	return 1
}

namespace export u_ab_*

### end of OSSUAB namespace ###################################################

}

namespace import ::OSSUAB::*
