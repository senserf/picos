package provide autoconnect 1.0


package require unames

###############################################################################
# Autoconnect #################################################################
###############################################################################

namespace eval AUTOCONNECT {

variable ACB

array set ACB { CLO "" CBK "" }

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

proc autocn_start { op cl hs hc cc { po "" } { dl "" } { lf "" } } {
#
# op - open function
# cl - close function
# hs - command to issue handshake message
# hc - handshake condition (if true, handshake has been successful)
# cc - connection condition (if false, connection has been broken)
# po - poll function (to kick the node if no heartbeat)
# dl - explicit device list
# lf - log function
# 
	variable ACB

	set ACB(OPE) [gize $op]
	set ACB(CLO) [gize $cl]
	set ACB(HSH) [gize $hs]
	set ACB(HSC) [gize $hc]
	set ACB(CNC) [gize $cc]
	set ACB(POL) [gize $po]
	set ACB(DLI) $dl
	set ACB(LDF) $lf

	set ACB(DVL) -1

	if { $ACB(LDF) != "" } {
		$ACB(LDF) "ACN START: $ACB(OPE) $ACB(CLO) $ACB(HSH) $ACB(HSC)\
			$ACB(CNC) $ACB(POL) $ACB(DLI)"
	}

	# last device scan time
	set ACB(LDS) 0

	# automaton state
	set ACB(STA) "L"

	# hearbeat variables
	set ACB(RRM) -1
	set ACB(LRM) -1

	if { $ACB(CBK) != "" } {
		# a precaution
		catch { after cancel $ACB(CBK) }
	}
	set ACB(CBK) [after 10 [lize ac_callback]]
}

proc autocn_stop { } {
#
# Stop the callback and disconnect
#
	variable ACB

	if { $ACB(CBK) != "" } {
		catch { after cancel $ACB(CBK) }
		set ACB(CBK) ""
	}

	if { $ACB(CLO) != "" } {
		$ACB(CLO)
	}
}

proc ac_again { d } {

	variable ACB

	set ACB(CBK) [after $d [lize ac_callback]]
}

proc ac_callback { } {
#
# This callback checks if we are connected and if not tries to connect us
# automatically to the node
#
	variable ACB

	if { $ACB(LDF) != "" } {
		$ACB(LDF) "AC S=$ACB(STA)"
	}

	if { $ACB(STA) == "R" } {
		# CONNECTED
		if ![$ACB(CNC)] {
			# just got disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		# we are connected, check for heartbeat
		if { $ACB(RRM) == $ACB(LRM) } {
			# stall?
			if { $ACB(POL) != "" } { $ACB(POL) }
			set ACB(STA) "T"
			set ACB(FAI) 0
			ac_again 1000
			return
		}
		set ACB(RRM) $ACB(LRM)
		# we are still connected, no need to panic
		ac_again 2000
		return
	}

	if { $ACB(STA) == "T" } {
		# HEARTBEAT FAILURE
		if ![$ACB(CNC)] {
			# disconnected
			set ACB(STA) "L"
			ac_again 100
			return
		}
		if { $ACB(RRM) == $ACB(LRM) } {
			# count failures
			if { $ACB(FAI) == 5 } {
				set ACB(STA) "L"
				$ACB(CLO)
				ac_again 100
				return
			}
			incr ACB(FAI)
			if { $ACB(POL) != "" } { $ACB(POL) }
			ac_again 600
			return
		}
		set ACB(RRM) $ACB(LRM)
		set ACB(STA) "R"
		ac_again 2000
		return
	}
	
	#######################################################################

	if { $ACB(STA) == "L" } {
		# CONNECTING
		set tm [clock seconds]
		if { $tm > [expr $ACB(LDS) + 5] } {
			# last scan for new devices was done more than 5 sec
			# ago, rescan
			if { $ACB(DLI) != "" } {
				set ACB(DVS) ""
				foreach d $ACB(DLI) {
					if [catch { expr $d } n] {
						lappend ACB(DVS) $d
					} else {
						lappend ACB(DVS) \
							[unames_ntodev $d]
					}
				}
			} else {
				unames_scan
				set ACB(DVS) [lindex [unames_choice] 0]
			}
			set dvl [llength $ACB(DVS)]
			if { $dvl != $ACB(DVL) } {
				set ACB(DVL) $dvl
			}
			set ACB(LDS) $tm
			if { $ACB(LDF) != "" } {
				$ACB(LDF) "AC RESCAN: $ACB(DVS), $ACB(DVL)"
			}
		}
		# index into the device table
		set ACB(CUR) 0
		set ACB(STA) "N"
		ac_again 250
		return
	}

	#######################################################################

	if { $ACB(STA) == "N" } {
		# TRYING NEXT DEVICE
		if { $ACB(LDF) != "" } {
			$ACB(LDF) "AC N CUR=$ACB(CUR), DVL=$ACB(DVL)"
		}
		# try to open a new UART
		if { $ACB(CUR) >= $ACB(DVL) } {
			if { $ACB(DVL) == 0 } {
				# no devices
				set ACB(LDS) 0
				ac_again 1000
				return
			}
			set ACB(STA) "L"
			ac_again 100
			return
		}

		set dev [lindex $ACB(DVS) $ACB(CUR)]
		incr ACB(CUR)
		if { [$ACB(OPE) $dev] == 0 } {
			ac_again 100
			return
		}

		$ACB(HSH)

		set ACB(STA) "C"
		ac_again 2000
		return
	}

	if { $ACB(STA) == "C" } {
		# WAITING FOR HANDSHAKE
		if [$ACB(HSC)] {
			# established, assume connection OK
			set ACB(STA) "R"
			ac_again 2000
			return
		}
		# sorry, try another one
		if { $ACB(LDF) != "" } {
			$ACB(LDF) "AC C -> CLOSING"
		}
		$ACB(CLO)
		set ACB(STA) "N"
		ac_again 100
		return
	}

	set ACB(STA) "L"
	ac_again 1000
}

namespace export autocn_*

}

namespace import ::AUTOCONNECT::*
