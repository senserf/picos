package provide log 1.0
###############################################################################
# Log functions. Copyright (C) 2008-13 Olsonet Communications Corporation.
###############################################################################

namespace eval LOGGING {

variable Log

proc log_open { { fname "" } { maxsize "" } { maxvers "" } } {

	variable Log

	if { $fname == "" } {
		if ![info exists Log(FN)] {
			set Log(FN) "log"
		}
	} else {
		set Log(FN) $fname
	}

	if { $maxsize == "" } {
		if ![info exists Log(MS)] {
			set Log(MS) 5000000
		}
	} else {
		set Log(MS) $maxsize
	}

	if { $maxvers == "" } {
		if ![info exists Log(MV)] {
			set Log(MV) 4
		}
	} else {
		set Log(MV) $maxvers
	}

	if [info exists Log(FD)] {
		# close previous log
		catch { close $Log(FD) }
		unset Log(FD)
	}

	if [catch { file size $Log(FN) } fs] {
		# not present
		if [catch { open $Log(FN) "w" } fd] {
			error "cannot open log file $Log(FN), $fd"
		}
		# empty log
		set Log(SZ) 0
	} else {
		# log file exists
		if [catch { open $Log(FN) "a" } fd] {
			error "cannot open log file $Log(FN), $fd"
		}
		set Log(SZ) $fs
	}
	set Log(FD) $fd
	set Log(CD) 0
}

proc rotate { } {

	variable Log

	catch { close $Log(FD) }
	unset Log(FD)

	for { set i $Log(MV) } { $i > 0 } { incr i -1 } {
		set tfn "$Log(FN).$i"
		set ofn $Log(FN)
		if { $i > 1 } {
			append ofn ".[expr $i - 1]"
		}
		catch { file rename -force $ofn $tfn }
	}

	catch { log_open }
}

proc outlm { m } {

	variable Log

	catch {
		puts $Log(FD) $m
		flush $Log(FD)
	}

	incr Log(SZ) [string length $m]
	incr Log(SZ)

	if { $Log(SZ) >= $Log(MS) } {
		rotate
	}
}

proc log { m } {

	variable Log 

	if ![info exists Log(FD)] {
		# no log file
		return
	}

	set sec [clock seconds]
	set day [clock format $sec -format %d]
	set hdr [clock format $sec -format "%H:%M:%S"]

	if { $day != $Log(CD) } {
		# day change
		set today "Today is "
		append today [clock format $sec -format "%h $day, %Y"]
		if { $Log(CD) == 0 } {
			# startup
			outlm "$hdr #### $today ####"
		} else {
			outlm "00:00:00 #### BIM! BOM! $today ####"
		}
		set Log(CD) $day
	}

	outlm "$hdr $m"
}

namespace export log*

### end of LOGGING namespace ##################################################

}

namespace import ::LOGGING::log*
