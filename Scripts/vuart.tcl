package provide vuart 1.0
#####################################################################
# This is a package for initiating direct VUEE-UART communication.  #
# Copyright (C) 2012 Olsonet Communications Corporation.            #
#####################################################################

namespace eval VUART {

variable VU

proc abin_S { s h } {
#
# append one short int to string s (in network order)
#
	upvar $s str
	append str [binary format S $h]
}

proc abin_I { s l } {
#
# append one 32-bit int to string s (in network order)
#
	upvar $s str
	append str [binary format I $l]
}

proc dbin_Q { s } {
#
# decode return code from string s
#
	upvar $s str
	if { $str == "" } {
		return -1
	}
	binary scan $str I val
	set str [string range $str 4 end]
	return [expr $val & 0xff]
}

proc init { abv } {

	variable VU

	# callback
	set VU(CB) ""

	# error status
	set VU(ER) ""

	# socket file descriptor
	set VU(FD) ""

	# abort variable
	set VU(AV) $abv
}

proc abtd { } {

	variable VU
	upvar #0 $VU(AV) abv

	return $abv
}

proc kick { } {

	variable VU

	catch {
		upvar #0 $VU(AV) abv
		set abv $abv
	}
}

proc cleanup { { ok 0 } } {

	variable VU

	stop_cb

	if !$ok {
		catch { close $VU(FD) }
	}
	array unset VU
}

proc stop_cb { } {

	variable VU

	if { $VU(CB) != "" } {
		catch { after cancel $VU(CB) }
		set VU(CB) ""
	}
}

proc sock_flush { } {
#
# Completes the flush for an async socket
#
	variable VU

	stop_cb

	if [abtd] {
		return
	}

	if [catch { flush $VU(FD) } err] {
		set VU(ER) "Write to VUEE failed: $err"
		kick
		return
	}

	if [fblocked $VU(FD)] {
		# keep trying while blocked
		set VU(CB) [after 200 ::VUART::sock_flush]
		return
	}

	# done
	kick
}

proc wkick { } {
#
# Wait for a kick (or abort)
#
	variable VU
	uplevel #0 "vwait $VU(AV)"
}

proc sock_read { } {

	variable VU

	if { $VU(SS) == 0 } {
		# expecting the initial code
		if { [catch { read $VU(FD) 4 } res] || $res == "" } {
			# disconnection
			set VU(ER) "Connection refused"
			kick
			return
		}

		set code [dbin_Q res]
		if { $code != 129 } {
			# wrong code
			set VU(ER) "Connection refused by VUEE, code $code"
			kick
			return 
		}

		set VU(SS) 1
		set VU(SI) ""
	}

	# read the signature message

	while 1 {
		if [catch { read $VU(FD) 1 } res] {
			set VU(ER) "Connection broken during handshake"
			kick
			return
		}
		if { $res == "" } {
			return
		}
		if { $res == "\n" } {
			# done
			break
		}
		append VU(SI) $res
	}

	# verify the signature
	kick
	if ![regexp "^P (\[0-9\]+) (\[0-9\]+) (\[0-9\]+) <(\[^ \]*)>:" $VU(SI) \
	    mat nod hos tot tna] {
		set VU(ER) "Illegal node signature: $sig"
		return
	}

	# connection OK

	set VU(SI) [list $nod $hos $tot $tna]
}

proc vuart_conn { ho po no abvar { sig "" } } {
#
# Connect to UART at the specified host, port, node; abvar is the abort 
# variable (set to 1 <from the outside> to abort the connection in progress)
#
	variable VU

	init $abvar

	if [abtd] {
		cleanup
		error "Preaborted"
	}

	# these actions cannot block
	if [catch { socket -async $ho $po } sfd] {
		cleanup
		error "Connection failed: $sfd"
	}

	set VU(FD) $sfd

	if [catch { fconfigure $sfd -blocking 0 -buffering none \
    	    -translation binary -encoding binary } erc] {
		cleanup
		error "Connection failed: $erc"
	}

	# prepare a request
	set rqs ""
	abin_S rqs 0xBAB4
	abin_S rqs 1
	abin_I rqs $no
	abin_I rqs 0

	if [catch { puts -nonewline $sfd $rqs } erc] {
		cleanup
		error "Write to VUEE failed: $erc"
	}

	after 200 ::VUART::sock_flush
	wkick

	if [abtd] {
		cleanup
		error "Aborted"
	}

	set err $VU(ER)
	if { $err != "" } {
		cleanup
		error $err
	}

	# wait for a reply
	set VU(SS) 0
	fileevent $VU(FD) readable ::VUART::sock_read

	wkick

	if [abtd] {
		cleanup
		error "Aborted"
	}

	set err $VU(ER)
	if { $err != "" } {
		cleanup
		error $err
	}

	set fd $VU(FD)

	if { $sig != "" } {
		upvar $sig s
		set s $VU(SI)
	}

	cleanup 1

	fileevent $fd readable {}

	return $fd
}

namespace export vuart_conn

### end of VUART namespace ####################################################
}

namespace import ::VUART::vuart_conn

###############################################################################
