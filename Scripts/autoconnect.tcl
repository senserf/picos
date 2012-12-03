package provide autoconnect 1.0

package require unames

###############################################################################
# Autoconnect #################################################################
###############################################################################

namespace eval AUTOCONNECT {

proc gize { fun } {

	if { $fun != "" && [string range $fun 0 1] != "::" } {
		set fun "::$fun"
	}

	return $fun
}

proc lize { fun } {

	return "::AUTOCONNECT::$fun"
}

proc autocn_heartbeat { } {
#
# Called to provide connection heartbeat, typically after a message reception
#
	variable ACB

	incr ACB(LRM)
}

proc autocn_start { op cl hs hc cc { po "" } } {
#
# op - open function
# cl - close function
# hs - command to issue handshake message
# hc - handshake condition (if true, handshake has been successful)
# cc - connection condition (if false, connection has been broken)
# po - poll function (to kick the node if no heartbeat)
# 
	variable ACB

	set ACB(OPE) [gize $op]
	set ACB(CLO) [gize $cl]
	set ACB(HSH) [gize $hs]
	set ACB(HSC) [gize $hc]
	set ACB(CNC) [gize $cc]
	set ACB(POL) [gize $po]

	# last device scan time
	set ACB(LDS) 0

	# automaton state
	set ACB(STA) "L"

	# hearbeat variables
	set ACB(RRM) -1
	set ACB(LRM) -1

	ac_callback
}

proc ac_callback { } {
#
# This callback checks if we are connected and if not tries to connect us
# automatically to the node
#
	variable ACB

	# trc "AC S=$ACB(STA)"
	if { $ACB(STA) == "R" } {
		if ![$ACB(CNC)] {
			set ACB(STA) "L"
			after 100 [lize ac_callback]
			return
		}
		# we are connected, check for heartbeat
		if { $ACB(RRM) == $ACB(LRM) } {
			# stall?
			if { $ACB(POL) != "" } { $ACB(POL) }
			set ACB(STA) "T"
			set ACB(FAI) 0
			after 1000 [lize ac_callback]
			return
		}
		set ACB(RRM) $ACB(LRM)
		# we are connected, no need to panic
		after 2000 [lize ac_callback]
		return
	}

	if { $ACB(STA) == "T" } {
		# counting heartbeat failures
		if ![$ACB(CNC)] {
			set ACB(STA) "L"
			after 100 [lize ac_callback]
			return
		}
		if { $ACB(RRM) == $ACB(LRM) } {
			if { $ACB(FAI) == 5 } {
				set ACB(STA) "L"
				$ACB(CLO)
				after 100 [lize ac_callback]
				return
			}
			incr ACB(FAI)
			if { $ACB(POL) != "" } { $ACB(POL) }
			after 600 [lize ac_callback]
			return
		}
		set ACB(RRM) $ACB(LRM)
		set ACB(STA) "R"
		after 2000 [lize ac_callback]
		return
	}
	
	#######################################################################

	if { $ACB(STA) == "L" } {
		# Main loop
		set tm [clock seconds]
		if { $tm > [expr $ACB(LDS) + 5] } {
			# last scan for new devices was done more than 5 sec
			# ago, rescan
			unames_scan
			set ACB(DVS) [lindex [unames_choice] 0]
			set ACB(DVL) [llength $ACB(DVS)]
			set ACB(LDS) $tm
			# trc "AC RESCAN: $ACB(DVS), $ACB(DVL)"
		}
		# index into the device table
		set ACB(CUR) 0
		set ACB(STA) "N"
		after 250 [lize ac_callback]
		return
	}

	#######################################################################

	if { $ACB(STA) == "N" } {
		# trc "AC N CUR=$ACB(CUR), DVL=$ACB(DVL)"
		# try to open a new UART
		if { $ACB(CUR) >= $ACB(DVL) } {
			if { $ACB(DVL) == 0 } {
				# no devices
				set ACB(LDS) 0
				after 1000 [lize ac_callback]
				return
			}
			set ACB(STA) "L"
			after 100 [lize ac_callback]
			return
		}

		set dev [lindex $ACB(DVS) $ACB(CUR)]
		incr ACB(CUR)
		if { [$ACB(OPE) $dev] == 0 } {
			after 100 [lize ac_callback]
			return
		}

		$ACB(HSH)

		set ACB(STA) "C"
		after 2000 [lize ac_callback]
		return
	}

	if { $ACB(STA) == "C" } {
		# check if handshake established
		if [$ACB(HSC)] {
			# yep, assume connection OK
			set ACB(STA) "R"
			after 2000 [lize ac_callback]
			return
		}
		# sorry, try another one
		# trc "AC C -> CLOSING"
		$ACB(CLO)
		set ACB(STA) "N"
		after 100 [lize ac_callback]
		return
	}

	set ACB(STA) "L"
	after 1000 [lize ac_callback]
}

namespace export autocn_*

}

namespace import ::AUTOCONNECT::*

###############################################################################
###############################################################################
