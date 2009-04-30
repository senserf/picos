package provide sdb 1.0

#############################################################################
#                                                                           #
# This is a trivial database of sensor samples whereby each dayload of data #
# takes one file whose name looks like YYYYMMDD.                            #
#                                                                           #
# Copyright (C) 2009 Olsonet Communications Corporation                     #
#                                                                           #
#############################################################################

namespace eval SDB {

variable DB

set DB(DP)	""
set DB(CU)	""
set DB(HR)	[expr 24 * 3600]
set DB(YC)	""

variable PT

# database index
set PT(DI)	"2\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]\[0-9\]$"

proc db_init { dnam } {
#
# The argument is a directory name that must exist
#
	variable DB

	if ![file isdirectory $dnam] {
		error "db_init: argument not a valid directory path"
	}

	set DB(DP) $dnam
}

proc db_close { } {
#
	db_stop
	set DB(DP) ""
}

proc db_add { dt data } {
#
# Add an entry to the current list of entries
#
	variable DB

	if { $DB(DP) == "" } {
		error "db_add: database not opened"
	}

	if [catch { open [file join $DB(DP) $dt] "a" } fd] {
		error "db_add: cannot open daily record: $fd"
	}

	if [catch { puts $fd $data } er] {
		catch { close $fd }
		error "db_add: cannot write: $fd"
	}

	catch { close $fd }
}

proc db_stop { } {
#
# Stop reading daily record
#
	variable DB

	if { $DB(CU) != "" } {
		catch { close $DB(CU) }
		set DB(CU) ""
	}
}

proc db_ttods { tm } {

	if [catch { clock format $tm -format %Y%m%d } res] {
		error "db_ttods: illegal number of seconds"
	}
	return $res
}

proc db_dstot { ds } {

	if [catch { clock scan "${ds}T000000" } sec] {
		error "db_dstot: bad day index: $ds"
	}

	return $sec
}

proc fyear { ni max } {
#
# Find the nearest next record by year
#
	variable DB
	variable PT

	# the year
	set yy [string range $ni 0 3]

	while { "${yy}0101" <= $max } {

		if { $DB(YC) == "" || $DB(YC,Y) != $yy } {
			# year not in the cache
			set DB(YC) ""
			set DB(YC,Y) $yy
			if [catch { glob -directory $DB(DP) -nocomplain -tails \
			    ${yy}* } yc] {
				# not found
				set yc ""
			}
			foreach y $yc {
				# cleanup the list
				if ![regexp $PT(DI) $y y] {
					continue
				}
				lappend DB(YC) $y
			}
			set DB(YC) [lsort $DB(YC)]
		}
		foreach y $DB(YC) {
			if { $y >= $ni } {
				if { $y > $max } {
					return ""
				} else {
					return $y
				}
			}
		}
		# not found, increment the year
		incr yy
	}

	return ""
}

proc db_start { dt { next "" } { max 20300101 }} {
#
# Start reading daily record
#
	variable DB

	db_stop

	if { $next == "+" } {
		# we are asked to increment
		set ni [db_ttods [expr [db_dstot $dt] + $DB(HR)]]
		if { $ni > $max } {
			# no more
			return ""
		}

		# naively try to open (expecting success in normal circumstances
		if ![catch { open [file join $DB(DP) $ni] "r" } fd] {
			# success
			set DB(CU) $fd
			return $ni
		}

		# we have failed, there is a hole, try by the year
		set ni [fyear $ni $max]

		if { $ni == "" } {
			return ""
		}

		# this must succeed
		if ![catch { open [file join $DB(DP) $ni] "r" } fd] {
			# success
			set DB(CU) $fd
			return $ni
		}

		error "db_start: cannot open record $ni: $fd"
	}

	if [catch { open [file join $DB(DP) $dt] "r" } fd] {
		# assume it doesn't exist
		return ""
	}

	set DB(CU) $fd
	return $dt
}

proc db_get { } {
#
# Next record at cursor
#
	variable DB

	if { $DB(CU) == "" } {
		return ""
	}

	while 1 {
		if [catch { gets $DB(CU) line } cnt] {
			error "db_get: cannot read: $cnt"
		}
		if { $cnt < 0 } {
			db_stop
			return ""
		}
		set line [string trim $line]
		if { $line != "" } {
			return $line
		}
	}
}

namespace export db_*

### end of SDB namespace ######################################################
}

namespace import ::SDB::db_*

###############################################################################
