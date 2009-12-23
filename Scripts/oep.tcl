package require oss_u_xrs 1.0

package provide oep 1.0

namespace eval OEP {

variable CH
variable ST
variable IV
variable PM
variable PT

# timeout while waiting for the first empty chunk
set IV(ECH)	2000

# retransmission interval 
set IV(RET)	500

# timeout while waiting for next chunk of a round
set IV(CHK)	1000

# packet space for outgoing chunks
set IV(CSP)	16

# number of retransmission before declaring failure
set PM(RET)	8

# maximum packet length - CRC; equals the length of a complete non-empty
# preamble packet (this must be the same as OEP_MAXRAWPL in oep.h): 60
set PM(CPL)	[expr 4 + 2 + 54]

# chunk (body) length
set PM(CHS)	54

# packet types
set PT(GO)	[expr 0x11]
set PT(CH)	[expr 0x12]

proc ini_rcv { nc } {
#
# Initialize the chunk array for reception
#
	variable CH
	variable ST

	for { set ic 0 } { $ic < $nc } { incr ic } {
		# mark chunks as absent
		set CH($ic) ""
	}

	# total chunks
	set ST(NCK) $nc

	# remaining chunks
	set ST(RCK) $nc
}

proc ini_snd { blk } {
#
# Initialize the chunk array for sending
#
	variable CH
	variable ST
	variable PM

	set cn 0
	set len [string length $blk]

	while 1 {
		set cs [expr $cn * $PM(CHS)]
		if { $cs > $len } {
			break
		}
		set ce [expr $cs + $PM(CHS) - 1]
		set CH($cn) [string range $blk $cs $ce]
		while { $ce >= $len } {
			# for the last chunk, even it up
			append CH($cn) \x00
			incr ce -1
		}
		incr cn
	}

	set ST(NCK) $cn
}

proc clean_rcv { } {
#
# Clean the chunk array after a reception (possibly aborted)
#
	variable CH
	variable ST

	array unset CH
	# ST is fully dynamic and scratch
	array unset ST

	# remove our interceptor
	u_setif
}

proc clean_snd { } {
#
# Clean after transmission
#
	variable CH
	variable RC
	variable ST

	array unset CH
	array unset RC
	array unset ST

	u_setif
}

proc ptmout { } {
#
# Trigger timeout while waiting for initial empty chunk; this means reception
# handshake failure
#
	variable ST
	set ST(ADV) 1
}

proc ctmout { } {
#
# Trigger timeout while waiting for next chunk of a round; this means and of
# round (and is considered normal)
#
	variable ST
	set ST(ADV) 0
}

proc retrans { intv count } {
#
# Retransmits the packet count times at intv intervals, or until a reply is
# received
#
	variable ST

	# the first time
	u_write $ST(MSG)

	# attempt counter
	set ST(TRY) $count

	# the interval
	set ST(INT) $intv

	# timer callback
	set ST(CBA) [after $intv ::OEP::poll]

	# wait until resolved
	vwait ::OEP::ST(ADV)
}

proc poll { } {
#
# Retransmit the current poll packet
#
	variable ST

	if { $ST(TRY) == 0 } {
		# no more, failure
		set ST(ADV) 1
		unset ST(MSG)
		return
	}

	incr ST(TRY) -1

	u_write $ST(MSG)
	set ST(CBA) [after $ST(INT) ::OEP::poll]
}

proc oep_rcv { lid rqn nc } {
#
# Reception
#
	variable ST
	variable CH
	variable IV
	variable PM
	variable PT

	# session identifiers
	set ST(LID) $lid
	set ST(RQN) $rqn

	# initialize the chunk array
	ini_rcv $nc

	# install OEP interceptor to receive chunks
	u_setif ::OEP::oep_rcv_chunk $PM(CPL)

	# wait for an empty chunk
	set ST(CBA) [after $IV(ECH) ::OEP::ptmout]
	# ST(ADV) is used as the event trigger; nonzero means failure, zero
	# means proceed to next stage
	vwait ::OEP::ST(ADV)

	if $ST(ADV) {
		# failure: clear, set status, resume AB
		clean_rcv
		return ""
	}

	# next stage: start rounds
	while { [go_prompt] } {

		retrans $IV(RET) $PM(RET)

		if $ST(ADV) {
			clean_rcv
			return ""
		}
	}

	# all chunks received, send the stop packet

	set msg [binary format scc $ST(LID) $PT(GO) $ST(RQN)]
	u_frame msg
	# send it three times
	u_write $msg
	after 128
	u_write $msg
	after 128
	u_write $msg

	# done, collect all chunks
	set res ""
	for { set ic 0 } { $ic < $nc } { incr ic } {
		append res $CH($ic)
	}

	clean_rcv
	return $res
}

proc oep_rcv_chunk { msg } {

	variable PT
	variable ST
	variable PM
	variable CH
	variable IV

	set len [string length $msg]

	if { $len != $PM(CPL) && $len != 4 } {
		# illegal
		return
	}

	# decode the header
	binary scan $msg scc lid cmd rqn

	# I wish there was a way to avoid this sign nonsense
	set cmd [expr $cmd & 0xff]
	if { $cmd != $PT(CH) } {
		# ignore if not chunk packet
		return
	}

	set lid [expr $lid & 0xffff]
	set rqn [expr $rqn & 0xff]

	if { $lid != $ST(LID) || $rqn != $ST(RQN) } {
		# ignore if wrong session, shouldn't really happen in our case
		return
	}

	# OK, clear the timer
	catch { after cancel $ST(CBA) }

	if { $len == 4 } {
		# empty chunk, end of round; advance to next one
		set ST(ADV) 0
		return
	}

	# expecting more chunks
	set ST(CBA) [after $IV(CHK) ::OEP::ctmout]

	# chunk number
	binary scan [string range $msg 4 5] s cn
	set cn [expr $cn & 0xffff]

	if { $cn >= $ST(NCK) } {
		# illegal, ignore, shouldn't happen
		return
	}

	if { $CH($cn) == "" } {
		# missing
		set CH($cn) [string range $msg 6 end]
		# to go
		incr ST(RCK) -1
	}
}

proc go_prompt { } {
#
# Prepare a GO packet based on the list of chunks that are still missing
#
	variable ST
	variable CH
	variable PT

	if { $ST(RCK) == 0 } {
		# all done
		return 0
	}

	# start the packet
	set msg [binary format scc $ST(LID) $PT(GO) $ST(RQN)]

	set n 0
	set i 0
	set l $ST(NCK)

	while { $i != $l } {
		# opening a new range
		if { $CH($i) != "" } {
			incr i
			continue
		}

		if { $n == 22 } {
			# only one left
			append msg [binary format s $i]
			break
		}

		set s $i
		incr i

		while { $i != $l } {
			if { $CH($i) != "" } {
				break
			}
			incr i
		}

		set t [expr $i - 1]
		if { $s == $t } {
			# a singleton
			append msg [binary format s $s]
			incr n
			continue
		}

		# a range
		append msg "[binary format s $t][binary format s $s]"

		incr n 2

		if { $n == 23 } {
			break
		}
	}

	if { $n == 0 } {
		# impossible
		error "illegal go_prompt, no missing chunks"
	}

	u_frame msg
	set ST(MSG) $msg
	return 1
}

proc oep_snd { lid rqn blk {lpr 0.0} } {
#
# Transmission
#
	variable ST
	variable RC
	variable CH
	variable IV
	variable PT
	variable PM

	set ST(LID) $lid
	set ST(RQN) $rqn

	# initialize chunks
	ini_snd $blk


	# prepare an empty chunk message to have it handy
	set ecm [binary format scc $ST(LID) $PT(CH) $ST(RQN)]
	# framed version
	set ecf $ecm
	u_frame ecf

	while 1 {

		# start rounds, empty chunk goes first
		set ST(MSG) $ecf

		# install OEP interceptor to receive GO packets
		u_setif ::OEP::oep_rcv_go $PM(CPL)

		retrans $IV(RET) $PM(RET)

		if $ST(ADV) {
			clean_snd
			return 1
		}

		# received a GO
		if { $ST(RCK) == 0 } {
			# empty GO, we are done
			break
		}

		u_setif ::OEP::oep_rcv_null $PM(CPL)

		set cp 0
		while { $cp < $ST(RCK) } {
			# next chunk number
			set sp $RC($cp)
			incr cp
			if { $cp >= $ST(RCK) || $RC($cp) > $sp } {
				# a singleton
				set fp $sp
			} else {
				# a pair describing a range
				set fp $RC($cp)
				incr cp
			}
			while { $fp <= $sp } {
				if { $lpr == 0.0 || [expr rand ()] < $lpr } {
					# send the chunk
					u_fnwrite \
					    "$ecm[binary format s $fp]$CH($fp)"
				} 
				# else, we simulate a loss

				# delay for a bit in a manner that makes it
				# possible to empty the input buffer
				after $IV(CSP) ::OEP::ctmout
				vwait ::OEP::ST(ADV)
				incr fp
			}
		}
	}

	# done

	clean_snd
	return 0
}

proc oep_rcv_null { $msg } {

}

proc oep_rcv_go { msg } {
#
# Receive a GO packet
#
	variable ST
	variable RC
	variable PT

	set len [string length $msg]
	if { $len < 4 } {
		# illegal
		return
	}

	binary scan $msg scc lid cmd rqn
			
	set cmd [expr $cmd & 0xff]
	if { $cmd != $PT(GO) } {
		# ignore if not GO
		return
	}

	set lid [expr $lid & 0xffff]
	set rqn [expr $rqn & 0xff]

	if { $lid != $ST(LID) || $rqn != $ST(RQN) } {
		# ignore if wrong session, shouldn't really happen in our case
		return
	}

	# the number of entries
	set len [expr ($len - 4) / 2]
	set ST(RCK) $len

	set cp 4
	set ce 5
	for { set cn 0 } { $cn < $len } { incr cn } {
		binary scan [string range $msg $cp $ce] s ck
		if { $ck >= $ST(NCK) } {
			# sanity check, ignore the packet if chunk number is
			# off range
			return
		}
		set RC($cn) $ck
		incr cp 2
		incr ce 2
	}
			
	# OK, clear the timer and advance
	catch { after cancel $ST(CBA) }
	set ST(ADV) 0
}

namespace export oep_*

### end of OEP namespace ######################################################

}

namespace import ::OEP::*

###############################################################################
