#!/bin/sh
# This is temporary
########\
exec wish85 C:/cygwin/home/pawel/SOFTWARE/PIP/pip.tcl "$@"

package require Tk
package require Ttk

proc cygfix { } {
#
# Issue a dummy reference to a file path to trigger a possible DOS-path
# warning, after which things should continue without any further warnings.
# This first reference must be caught as otherwise it would abort the script.
#
	catch { exec ls [pwd] }
}

###############################################################################

# will be set by deploy
set PicOSPath	""
set DefProjDir	""

set EditCommand "elvis -f ram -m -G x11 -font 9x15"

## File types to be listed in the Files view
set LFTypes {
	{ Headers { "\\.h$" "\\.ch$" } { Header { ".h" ".ch" } } }
	{ Sources { "\\.cc?$" "\\.asm$" } { Source { ".cc" } } }
	{ Options { "^options\[_a-z\]*\\.sys$" } { Options { ".sys" } } }
	{ XMLData { "\\.xml$" } { XMLData { ".xml" } } }
}

## Directory names to be ignored in the project's directory:
## strict names, patterns (case ignored)
set IGDirs {
	{ "CVS" "VUEE_TMP" }
	{ "^ktmp" "junk" "attic" "ossi" "\\~\\$" }
}

## Dictionary of configuration items (to be searched for in config.prj) + their
## default values
set CFItems { "CPU" "MSP430" "MB" 0 "BO" "" } 

## List of legal CPU types
set CPUTypes { MSP430 eCOG }

## List of last projects
set LProjects ""

## Program running in term
set TCMD(FD) ""
set TCMD(BF) ""
set TCMD(BL) 0
set TCMD(CB) ""
set TCMD(CL) 0

set P(SSL) ""
set P(SSV) ""

## double exit avoidance flag
set REX	0

###############################################################################

proc log { m } {

	puts $m
}

###############################################################################

proc xq { pgm { pargs "" } } {
#
# A flexible exec (or so I hope)
#
	set ef [auto_execok $pgm]
	if ![file executable $ef] {
		set ret [eval [list exec] [list sh] [list $ef] $pargs]
	} else {
		set ret [eval [list exec] [list $ef] $pargs]
	}
	return $ret
}

proc alert { msg } {

	tk_dialog .alert "Attention!" "${msg}!" "" 0 "OK"
}

proc confirm { msg } {

	return [tk_dialog .alert "Warning!" $msg "" 0 "NO" "YES"]
}

proc trunc_fname { n fn } {
#
# Truncate a file name to be displayed
#
	if { $fn == "" } {
		return "---"
	}

	set ln [string length $fn]
	if { $ln > $n } {
		set fn "...[string range $fn end-[expr $n - 3] end]"
	}
	return $fn
}

proc term_addtxt { txt } {

	global Term

	$Term configure -state normal
	$Term insert end "$txt"
	$Term configure -state disabled
	$Term yview -pickplace end
}

proc term_endline { } {

	global TCMD Term

	$Term configure -state normal
	$Term insert end "\n"

	while 1 {
		set ix [$Term index end]
		set ix [string range $ix 0 [expr [string first "." $ix] - 1]]
		if { $ix <= 1024 } {
			break
		}
		# delete the topmost line if above limit
		$Term delete 1.0 2.0
	}

	$Term configure -state disabled
	# make sure the last line is displayed
	$Term yview -pickplace end
	# BOL flag
	set TCMD(BL) 0
}

proc term_dspline { ln } {
#
# Write a line to the terminal
#
	global TCMD

	if $TCMD(BL) {
		term_endline
	}
	term_addtxt $ln
	term_endline
}

proc term_output { } {

	global TCMD

	if [catch { read $TCMD(FD) } chunk] {
		# assume EOF
		do_stop_term
		return
	}

	if [eof $TCMD(FD)] {
		do_stop_term
		return
	}

	if { $chunk == "" } {
		return
	}

	append TCMD(BF) $chunk
	# look for CR+LF, LF+CR, CR, LF; if there is only one of those at the
	# end, ignore it for now and keep for posterity
	set sl [string length $TCMD(BF)]

	while { [regexp "\[\r\n\]" $TCMD(BF) m] } {
		set el [string first $m $TCMD(BF)]
		if { $el == 0 } {
			# first character
			if { $sl < 2 } {
				# have to leave it and wait
				return
			}
			# check the second one
			set n [string index $TCMD(BF) 1]
			if { $m == "\r" && $n == "\n" || \
			     $m == "\n" && $n == "\r"    } {
				# two-character EOL
				set TCMD(BF) [string range $TCMD(BF) 2 end]
				incr sl -2
			} else {
				set TCMD(BF) [string range $TCMD(BF) 1 end]
				incr sl -1
			}
			# complete previous line
			term_endline
			set TCMD(BL) 0
			continue
		}
		# send the preceding string to the terminal
		term_addtxt [string range $TCMD(BF) 0 [expr $el - 1]]
		incr sl -$el
		set TCMD(BL) 1
		set TCMD(BF) [string range $TCMD(BF) $el end]
	}

	if { $TCMD(BF) != "" } {
		term_addtxt $TCMD(BF)
		set TCMD(BL) 1
		set TCMD(BF) ""
	}
}

###############################################################################

proc read_piprc { } {
#
# Read the rc file
#
	global env

	if ![info exists env(HOME)] {
		return ""
	}

	if [catch { open [file join $env(HOME) ".piprc"] "r" } fd] {
		# cannot open rc file
		return ""
	}

	if [catch { read $fd } rf] {
		catch { close $fd }
		return ""
	}

	catch { close $fd }

	return $rf
}

proc write_piprc { f } {
#
# Write the rc file
#
	global env

	if ![info exists env(HOME)] {
		return
	}

	if [catch { open [file join $env(HOME) ".piprc"] "w" } fd] {
		# cannot open rc file
		return
	}

	catch { puts -nonewline $fd $f }
	catch { close $fd }
}

proc get_last_project_list { } {
#
# Retrieve the list of last projects from .piprc
#
	global LProjects

	set rc [read_piprc]
	if [catch { dict get $rc "LPROJECTS" } lpr] {
		set lpr ""
	}

	set LProjects $lpr
	catch { mk_file_menu }
}

proc upd_last_project_list { } {
#
# Update the last projects list in .piprc
#
	global LProjects

	set rc [read_piprc]
	catch { dict set rc "LPROJECTS" $LProjects }
	write_piprc $rc
	catch { mk_file_menu }
}

###############################################################################

proc gfl_tree { } {
#
# Fill/update the treeview file list with files
#
	global LFTypes MKRECV P

	array unset MKRECV
	set fl [gfl_all_rec .]
	# we don't need this any more
	array unset MKRECV

	set tv $P(FL)

	# remove all nodes in treeview; will fill it from scratch
	$tv delete [$tv children {}]

	foreach t $LFTypes {
		set h [lindex $t 0]
		set l [gfl_spec $fl $h]
		set id [$tv insert {} end -text ${h}: -values [list $h "c"]]
		if [info exists P(FL,c,$h)] {
			set of 1
		} else {
			set of 0
		}
		$tv item $id -open $of
		# tree, parent, list, path so far
		gfl_tree_pop $tv $id $l ""
	}
}

proc gfl_tree_pop { tv node lst path } {
#
# Populate the list of children of the given node with the current contents
# of the list
#
	global P

	foreach t [lindex $lst 0] {
		# the directories
		set n [lindex $t 0]
		# augmented path
		set p [file join $path $n]
		set id [$tv insert $node end -text $n -values [list $p "d"]]
		# check if should be open or closed
		if [info exists P(FL,d,$p)] {
			# open
			set of 1
		} else {
			set of 0
		}
		$tv item $id -open $of
		# add recursively the children
		gfl_tree_pop $tv $id [lindex $t 1] $p
	}
	# now for the files
	foreach t [lindex $lst 1] {
		set p [file join $path $t]
		$tv insert $node end -text $t -values [list $p "f"]
	}
}

proc gfl_all_rec { path } {
#
# The recursive part of gfl_tree; returns a two-element list { dirs files } or
# NULL is there is nothing more below this point
#
	global MKRECV IGDirs LFTypes

	if [catch { exec ls $path } sdl] {
		# doesn't exist?
		return ""
	}

	set dirs ""
	set fils ""

	# flag == at least one file present here or down from here
	set fp 0

	foreach f $sdl {
		set p [file normalize [file join $path $f]]
		if { $p == "" } {
			# just in case
			continue
		}
		if [file isdirectory $p] {
			# should we ignore it
			if [info exists MKRECV($p)] {
				# avoid loops
				continue
			}
			set MKRECV($p) ""
			if { [lsearch -exact [lindex $IGDirs 0] $f] >= 0 } {
				continue
			}
			set k 0
			foreach m [lindex $IGDirs 1] {
				if [regexp -nocase $m $f] {
					set k 1
					break
				}
			}
			if $k {
				continue
			}

			set rfl [gfl_all_rec [file join $path $f]]
			# if rfl is NULL, it means that no more files are
			# present down the tree

			if { $rfl != "" } {
				set fp 1
				lappend dirs [list $f $rfl]
			}
			# ignore otherwise
			continue
		}
		# a regular file
		foreach t $LFTypes {
			set k 0
			foreach p [lindex $t 1] {
				if [regexp $p $f] {
					lappend fils [list [lindex $t 0] $f]
					set k 1
					set fp 1
					break
				}
			}
			if $k {
				break
			}
		}
	}

	if $fp {
		return [list $dirs $fils]
	} else {
		# no file below this point
		return ""
	}
}

proc gfl_spec { fl ft } {
#
# Given a combined global list of all qualified files, extracts from it a
# class-specific sublist; this time the directories and files at each level
# are sorted by name; note that they are sorted independently (the dirs are
# supposed to go first when the thing is displayed)
#
	set dirs ""
	set fils ""

	# flag == at least one file present here or down from here
	set fp 0

	foreach t [lindex $fl 0] {
		# directories first; the name
		set n [lindex $t 0]
		set rfl [gfl_spec [lindex $t 1] $ft]
		if { $rfl != "" } {
			set fp 1
			lappend dirs [list $n $rfl]
		}
	}
	set dirs [lsort -index 0 $dirs]

	foreach t [lindex $fl 1] {
		# files
		if { [lindex $t 0] != $ft } {
			# wrong type
			continue
		}
		# just the name
		lappend fils [lindex $t 1]
		set fp 1
	}

	if !$fp {
		return ""
	}

	set fils [lsort $fils]

	return [list $dirs $fils]
}

proc gfl_open { tree node } {
#
# Keep track of which ones are open and which ones are closed; we will need it
# for updates
#
	global P

	set t [$tree set $node type]

	if { $t == "c" || $t == "d" } {
		# mark it as open
		set P(FL,$t,[$tree set $node filename]) ""
	}
}

proc gfl_close { tree node } {

	global P

	set t [$tree set $node type]

	if { $t == "c" || $t == "d" } {
		array unset P "FL,$t,[$tree set $node filename]"
	}
}

###############################################################################

proc edit_file { fn } {

	global EFDS EFST EditCommand

	if [catch { open "|$EditCommand [list $fn]" "r" } fd] {
		alert "Cannot start text editor: $fd"
		return
	}

	set EFDS($fd) $fn
	# file status
	set EFST($fd,M) 0
	# PID (unknown yet)
	set EFST($fd,P) ""

	log "Editing file: $fn"

	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "edit_status_read $fd"
}

proc edit_status_read { fd } {

	global EFDS EFST

	if [catch { gets $fd line } val] {
		# aborted
		log "Edit session aborted"
		edit_close $fd 1
		return
	}

	if { $val < 0 } {
		# finished normally
		edit_close $fd 0
		return
	}

	set line [string trim $line]

	if ![regexp "BST: (\[0-9\])" $line jnk st] {
		# ignore any other lines except the first PID
		if { $EFST($fd,P) == "" && [regexp "PID: (\[0-9\]+)" $line jnk \
		     st] } {
			set EFST($fd,P) $st
			log "Edit process ID: $st"
		}
		return
	}

	if { $st != $EFST($fd,M) } {
		log "Edit status change for $EFDS($fd): $EFST($fd,M) -> $st"
		set EFST($fd,M) $st
	}
}

proc edit_close { fd ab } {

	global EFST EFDS

	catch { close $fd }

	if [info exists EFDS($fd)] {
		# not an external kill, soft close
		if $ab {
			set ab "aborted"
		} else {
			set ab "closed"
		}
		log "Edit session $EFDS($fd) $ab"
		array unset EFST "$fd,*"
		unset EFDS($fd)
		# redo the file list
		gfl_tree
	}
}

proc edit_unsaved { } {

	global EFST EFDS

	set nf 0
	set ul ""

	foreach fd [array names EFDS] {
		if { $EFST($fd,M) != 0 } {
			incr nf
			append ul "$EFDS($fd), "
		}
	}

	if { $nf == 0 } {
		# no unsaved files
		return 0
	}

	regsub ", $" $ul "" ul

	if { $nf == 1 } {
		alert "Unsaved file: $ul. Please save the file or close the\
			editing session and try again."
	} else {
		# more than one unsaved file
		alert "There are $nf unsaved files: $ul. Please save them or\
			close the editing sessions and try again."
	}

	# terminate the editing sessions

	return 1
}

proc edit_kill { { fp "" } } {

	global EFDS EFST

	foreach fd [array names EFDS] {
		if { $fp != "" && $EFDS($fd) != $fp } {
			# not this file
			continue
		}
		unset EFDS($fd)
		set pid $EFST($fd,P)
		array unset EFST($fd,*)
		if { $pid != "" } {
			log "Killing edit process: $pid"
			if [catch { exec kill -QUIT $pid } err] {
				log "Cannot kill: $err"
			}
		}
	}
}

proc file_is_edited { fn { m 0 } } {

	global EFDS EFST

	foreach fd [array names EFDS] {
		if { $EFDS($fd) == $fn } {
			if { $m == 0 || $EFST($fd,M) } {
				return 1
			}
		}
	}

	return 0
}

proc open_for_edit { x y } {

	global P EFDS

	set tv $P(FL)
	set node [$tv identify item $x $y]

	if { $node == "" } {
		return
	}

	set vs [$tv item $node -values]

	if { [lindex $vs 1] != "f" } {	
		# not a file
		return
	}

	set fp [file normalize [lindex $vs 0]]
	if [file_is_edited $fp] {
		alert "The file is already being edited"
		return
	}
	edit_file $fp
}

proc tree_menu { x y X Y } {

	global P

	set tv $P(FL)

	# first check if there's a selection
	set fl ""
	foreach t [$tv selection] {
		# make sure we only look at file items
		set vs [$tv item $t -values]
		if { [lindex $vs 1] == "f" } {
			lappend fl [lindex $vs 0]
		}
	}
	if { $fl == "" } {
		# no selection, check if we are pointing at some file
		set t [$tv identify item $x $y]
		if { $t == "" } {
			return
		}
		set vs [$tv item $t -values]
		if { [lindex $vs 1] == "f" } {
			lappend fl [lindex $vs 0]
		}
	}

	# create the menu
	catch { destroy .popm }
	set m [menu .popm -tearoff 0]

	set P(ML) $fl

	if { $fl == "" } {
		set st "disabled"
	} else {
		set st "normal"
	}

	$m add command -label "Edit" -command open_multiple -state $st
	$m add command -label "Delete" -command delete_multiple -state $st
	$m add command -label "New File" -command "new_file $x $y"


	tk_popup .popm $X $Y
}

proc open_multiple { } {

	global P

	if ![info exists P(ML)] {
		# the list of files
		return
	}

	set fl $P(ML)

	if { [llength $fl] == 1 } {
		set fp [file normalize [lindex $fl 0]]
		if [file_is_edited $fp] {
			alert "The file is already being edited"
		} else {
			edit_file $fp
		}
		return
	}

	set el ""
	foreach f $fl {
		set fp [file normalize $f]
		if ![file_is_edited $fp] {
			lappend el $fp
		}
	}

	if { $el == "" } {
		alert "All these files are already being edited"
		return
	}

	foreach fp $el {
		edit_file $fp
	}
}

proc delete_multiple { } {

	global P EFST EFDS

	if ![info exists P(ML)] {
		# the list of files
		return
	}

	set fl $P(ML)

	# check for edited and modified

	set msg "Are you sure you want to delete "

	if { [llength $fl] < 2 } {
		append msg "this file: [lindex $fl 0]"
	} else {
		append msg "these files: "
		append msg [join $fl ", "]
	}
	append msg "?"

	if ![confirm $msg] {
		return
	}

	# check for modification
	set mf ""

	foreach f $fl {
		if [file_is_edited [file normalize $f] 1] {
			lappend mf $f
		}
	}

	if { $mf != "" } {
		set msg "File"
		if { [llength $mf] > 1 } {
			append msg "s: "
			append msg [join $mf ", "]
			append msg " have"
		} else {
			append msg ": [lindex $mf 0] has"
		}
		append msg " been edited and modified but not yet saved. If you\
			proceed, the edit sessions will be closed and the\
			changes will be discarded."
		if ![confirm $msg] {
			return
		}
	}

	# delete
	foreach f $fl {
		set fp [file normalize $f]
		edit_kill $fp
		catch { file delete -force -- $fp }
	}

	# redo the tree view
	gfl_tree
}

proc new_file { x y } {

	global P LFTypes

	set tv $P(FL)

	set t [$tv selection]

	if { [llength $t] != 1 } {
		# use the pointer
		set t [$tv identify item $x $y]
	} else {
		# use the selection
		set t [lindex $t 0]
	}

	# the defaults
	set dir "."
	set typ Sources
	set ext ""

	while 1 {
		# this loop is merely to facilitate selection from a messy set
		# of options
		if { $t == "" } {
			# don't know
			break
		}
		set vs [$tv item $t -values]
		set tp [lindex $vs 1]
		if { $tp == "d" } {
			# use this directory and look up the parent class
			set dir [lindex $vs 0]
			while 1 {
				set t [$tv parent $t]
				if { $t == "" } {
					# will redo for "unknown"
					break
				}
				if { [lindex [$tv item $t -values] 0] == "c" } {
					# the top, i.e., the class node
					break
				}
			}
			# redo for unknown or class
			continue
		}

		set fn [lindex $vs 0]

		if { $tp == "c" } {
			# class, force the suffix
			set typ $fn
			break
		}

		# file
		set ext [file extension $fn]
		set dir [file dirname $fn]

		# determine types from the class
		set t [$tv parent $t]
	}

	set fo 1
	foreach t $LFTypes {
		if { [lindex $t 0] == $typ } {
			set fo 0
			break
		}
	}

	if $fo {
		# impossible
		set t [lindex $LFTypes 1]
	}

	set typ [list [lindex $t 2]]
	if { $ext == "" } {
		set ext [lindex [lindex [lindex $typ 0] 1] 0]
	}

	set dir [file normalize $dir]

	while 1 {

		set fn [tk_getSaveFile \
				-defaultextension $ext \
				-filetypes $typ \
				-initialdir $dir \
				-title "New file"]

		if { $fn == "" } {
			# cancelled
			return
		}

		set fn [file normalize $fn]

		# check if falls under the project
		set fo 0
		foreach t $LFTypes {
			foreach p [lindex $t 1] {
				if [regexp $p $fn] {
					set fo 1
					break
				}
			}
			if $fo {
				break
			}
		}
		if !$fo {
			alert "Illegal file extension"
			continue
		}
		set cd [file normalize [pwd]]
		set fk $fn
		set fo 0
		while 1 {
			set fl [file normalize [file dirname $fk]]
			if { $fl == $fk } {
				break
			}
			if { $fl == $cd } {
				set fo 1
				break
			}
			set fk $fl
		}

		if !$fo {
			alert "This file is located outside the project's\
				directory"
			continue
		}

		if [file exists $fn] {
			alert "This file already exists"
			continue
		}

		break
	}

	catch { exec touch $fn }
	gfl_tree
	edit_file $fn
}

###############################################################################

proc val_prj_dir { dir } {
#
# Validate the formal location of a project directory
#
	global PicOSPath

	set apps [file normalize [file join $PicOSPath Apps]]

	while 1 {
		set d [file normalize [file dirname $dir]]
		if { $d == $dir } {
			# no change
			alert "This directory won't do! A project directory\
				must be a proper subdirectory of $apps"
			return 0
		}
		if { $d == $apps } {
			# OK
			return 1
		}
		set dir $d
	}
}

proc prj_name { dir } {

	global PicOSPath

	return [string trim [string range $dir [string length \
		[file normalize [file join $PicOSPath Apps]]] end] "/"]
}

proc val_prj_incomp { } {
#
# Just an alert
#
	alert "Inconsistent contents of the project's directory: app.cc cannot\
		coexist with app_... files"
}

proc val_prj_exists { dir } {
#
# Check if the directory contains an existing project that appears to be making
# sense
#
	global P

	if [catch { exec ls $dir } fl] {
		alert "Cannot access the project's directory $dir, $fl"
		return 0
	}

	set pl ""
	set es 0

	foreach fn $fl {
		if { $fn == "app.cc" } {
			if { $pl != "" } {
				val_prj_incomp
				return 0
			}
			set es 1
			continue
		}
		if { $fn == "app.c" } {
			alert "This looks like a legacy praxis: file app.c is\
				incompatible with our projects, please convert\
				manually and try again"
			return 0
		}
		if [regexp "^app_(\[a-zA-Z0-9\]+)\\.cc$" $fn jnk pn] {
			if $es {
				val_prj_incomp
				return 0
			}
			lappend pl $pn
		}
	}

	if { !$es && $pl == "" } {
		alert "There is nothing resmbling a PicOS project in this\
			directory"
		return 0
	}

	# time to close the previous project and assume the new one

	if [catch { cd $dir } err] {
		alert "Cannot move to the project's directory $dir, $err"
		return 0
	}

	array unset P "FL,d,*"
	array unset P "FL,c,*"

	set P(PL) $pl

	wm title . "Project: [prj_name $dir]"

	gfl_tree

	return 1
}

proc close_project { } {

	global P

	if [edit_unsaved] {
		# no
		return 1
	}

	edit_kill

	if $P(AC) {
		# this is used to tell if a project is currently opened;
		# perhaps we should've checked it before edit_unsaved?
		set P(AC) 0
		set P(CO) ""
	}

	return 0
}

proc open_project { { which -1 } } {

	global P DefProjDir PicOSPath LProjects

	if [close_project] {
		# no
		return
	}

	if { $which < 0 } {

		# open file

		while 1 {
			set dir [tk_chooseDirectory \
					-initialdir $DefProjDir \
					-parent . \
					-title "Project directory"]

			if { $dir == "" } {
				# cancelled
				return
			}

			# probably already normalized, but it doesn't hurt to
			# make sure
			set dir [file normalize $dir]

			if ![val_prj_dir $dir] {
				# formally illegal
				continue
			}

			# this must be an existing project, do some rudimentary
			# checks
			if [val_prj_exists $dir] {
				break
			}
		}

	} else {

		while 1 {

			set dir [lindex $LProjects $which]
			if { $dir == "" } {
				alert "No such project"
				break
			}

			set dir [file normalize $dir]

			if ![val_prj_dir $dir] {
				set dir ""
				break
			}

			if ![val_prj_exists $dir] {
				set dir ""
			}
			break
		}

		if { $dir == "" } {
			# cannot open, remove the entry from LProjects
			set LProjects [lreplace $LProjects $which $which]
			upd_last_project_list
			return
		}
	}

	# add to LProjects

	set lp ""
	lappend lp $dir
	set nc 1
	foreach p $LProjects {
		if { $nc >= 6 } {
			# no more than 6
			break
		}
		if { $p == "" } {
			continue
		}
		set p [file normalize $p]
		if { $p == $dir } {
			continue
		}
		lappend lp $p
		incr nc
	}
	set LProjects $lp
	upd_last_project_list
	setup_project
	mk_build_menu
}

proc get_config { } {
#
# Reads the project configuration from config.prj
#
	global CFItems P

	# start from the dictionary of defaults
	set P(CO) $CFItems

	if [catch { open "config.prj" "r" } fd] {
		return
	}

	if [catch { read $fd } pf] {
		catch { close $fd }
		alert "Cannot read config.prj, $pf"
		return
	}

	catch { close $fd }

	set D [dict create]

	foreach { k v } $pf {
		if { $k == "" || ![dict exists $P(CO) $k] } {
			alert "Illegal contents of config.prj ($vp), file\
				ignored"
			return
		}
		dict set D $k $v
	}

	set P(CO) [dict merge $P(CO) $D]
}

proc set_config { } {
#
# Saves the project configuration
#
	global P

	if [catch { open "config.prj" "w" } fd] {
		alert "Cannot open config.prj for writing, $fd"
		return
	}

	if [catch { puts -nonewline $fd $P(CO) } er] {
		catch { close $fd }
		alert "Cannot write to config.prj, $er"
		return
	}

	catch { close $fd }
}

proc setup_project { } {
#
# Set up the project's parameters and build the dynamic menus; assumes we are
# in the project's directory
#
	global P

	get_config
	set P(AC) 1

	# enable menus ....
}

###############################################################################

proc set_menu_button { w tx ltx cmd } {
#
# Set up a menu button with some initial text tx and the list of options ltx
#
	if { [lsearch -exact $ltx $tx] < 0 } {
		set tx "---"
	}

	menubutton $w -text $tx -direction right -menu $w.m -relief raised
	menu $w.m -tearoff 0

	set n 0
	foreach t $ltx {
		$w.m add command -label $t -command "$cmd $w $n"
		incr n
	}
}

proc board_list { cpu } {

	global PicOSPath

	set dn [file join $PicOSPath PicOS $cpu BOARDS]
	set fl [glob -nocomplain -tails -directory $dn *]

	set r ""
	foreach f $fl {
		if [file isdirectory [file join $dn $f]] {
			lappend r $f
		}
	}
	return $r
}

proc do_board_selection { } {
#
# Execute CPU and board selection from Configuration menu
#
	global P

	if !$P(AC) {
		return
	}

	# copy the relevant parameters to the working array
	foreach k { "MB" "BO" "CPU" } {
		set P(BS,$k)  [dict get $P(CO) $k]
	}

	while 1 {
		# the event variable
		mk_board_selection_window
		set P(BS,EV) 0
		vwait P(BS,EV)
		if { $P(BS,EV) < 0 } {
			# cancellation
			stop_board_selection
			return
		}
		if { $P(BS,EV) == 1 } {
			# accepted; copy the options
			foreach k { "MB" "BO" "CPU" } {
				dict set P(CO) $k $P(BS,$k)
			}
			stop_board_selection
			set_config
			return
		}
		# redo
	}
}

proc done_board_selection_click { v } {

	global P

	if { [info exists P(BS,EV)] && $P(BS,EV) == 0 } {
		set P(BS,EV) $v
	}
}

proc stop_board_selection { } {
#
# Terminate the board selection window and clean up things
#
	global P

	if [info exists P(BS,WI)] {
		catch { destroy $P(BS,WI) }
	}
	array unset P "BS,*"
	mk_build_menu
}

proc multiple_check_click { } {
#
# The "multiple" selection has changed
#
	global P

	# just redo the window
	set P(BS,EV) 2
}

proc cpu_selection_click { w n } {
#
# A different CPU has been selected
#
	global P CPUTypes

	set t [$w.m entrycget $n -label]
	$w configure -text $t
	set P(BS,CPU) $t
}

proc board_selection_click { w n } {
#
# A board has been selected
#
	global P

	# the board number
	set nb 0
	regexp "\[0-9\]+$" $w nb

	set t [$w.m entrycget $n -label]
	$w configure -text $t

	set P(BS,BO) [lreplace $P(BS,BO) $nb $nb $t]
}

proc mk_board_selection_window { } {
#
# Open the board selection window
#
	global P CPUTypes

	set w ".bsel"

	# the usual precaution
	catch { destroy $w }

	set P(BS,WI) $w

	toplevel $w

	wm title $w "Board selection"

	# make it modal
	catch { grab $w }

	set f "$w.main"

	frame $f
	pack $f -side top -expand y -fill both

	# column number for the grid
	set cn 0
	set rn 0

	### CPU selection #####################################################

	set rm [expr $rn + 1]

	label $f.cpl -text "CPU"
	grid $f.cpl -column $cn -row $rn -sticky nw -padx 1 -pady 1

	set_menu_button $f.cpb $P(BS,CPU) $CPUTypes cpu_selection_click
	grid $f.cpb -column $cn -row $rm -sticky nw -padx 1 -pady 1

	### Multiple boards/single board ######################################

	if { $P(PL) != "" } {
		# we have a multi-program case, so the "Multiple" checkbox
		# is needed
		incr cn
		label $f.mbl -text "Multiple"
		grid $f.mbl -column $cn -row $rn -sticky nw -padx 1 -pady 1
		checkbutton $f.mbc -variable P(BS,MB) \
			-command "multiple_check_click"
		grid $f.mbc -column $cn -row $rm -sticky nw -padx 1 -pady 1
	}

	# the list of available boards
	set boards [board_list $P(BS,CPU)]

	if $P(BS,MB) {
		# multiple
		set nb 0
		set tb ""
		set lb ""
		foreach suf $P(PL) {
			set bn [lindex $P(BS,BO) $nb]
			if { $bn == "" } {
				if { $lb != "" } {
					set bn $lb
				} else {
					set bn "---"
				}
			} else {
				set lb $bn
			}
			incr cn
			label $f.bl$nb -text "Board ($suf)"
			grid $f.bl$nb -column $cn -row $rn -sticky nw \
				-padx 1 -pady 1
			set_menu_button $f.bm$nb $bn $boards \
				board_selection_click
			grid $f.bm$nb -column $cn -row $rm -sticky nw \
				-padx 1 -pady 1
			incr nb
			lappend tb $bn
		}
		set P(BS,BO) $tb
	} else {
		# single board
		incr cn
		set bn [lindex $P(BS,BO) 0]
		label $f.bl0 -text "Board"
		grid $f.bl0 -column $cn -row $rn -sticky nw -padx 1 -pady 1
		set_menu_button $f.bm0 $bn $boards board_selection_click
		grid $f.bm0 -column $cn -row $rm -sticky nw -padx 1 -pady 1
	}

	incr cn

	# the done button
	button $f.don -text "Done" -width 7 \
		-command "done_board_selection_click 1"
	grid $f.don -column $cn -row $rn -sticky nw -padx 1 -pady 1

	button $f.can -text "Cancel" -width 7 \
		-command "done_board_selection_click -1"
	grid $f.can -column $cn -row $rm -sticky nw -padx 1 -pady 1

	bind $w <Destroy> "done_board_selection_click -1"
}
	
proc terminate { { f "" } } {

	global REX

	if $REX { return }

	set REX 1

	if { $f == "" && [edit_unsaved] } {
		return
	}

	edit_kill
	close_project
	exit 0
}

###############################################################################

proc run_term_command { cmd al } {
#
# Run a command in term window
#
	global TCMD

	if { $TCMD(FD) != "" } {
		error "Already running a command, abort first"
	}

	set ef [auto_execok $cmd]
	if { $ef == "" } {
		error "Cannot execute $cmd"
	}

	if [file executable $ef] {
		set cmd "[list $ef]"
	} else {
		set cmd "[list sh] [list $ef]"
	}

	foreach a $al {
		append cmd " [list $a]"
	}

	# stderr to stdout
	append cmd " 2>@1"

	if [catch { open "|$cmd" "r" } fd] {
		error "Cannot execute $cmd, $fd"
	}

	# command started
	set TCMD(FD) $fd
	set TCMD(BF) ""
	mark_running 1

	fconfigure $fd -blocking 0 -buffering none
	fileevent $fd readable "term_output"

	# enable abort command
}

proc do_abort_term { } {

	global TCMD

	if { $TCMD(FD) != "" } {
		catch { close $TCMD(FD) }
		set TCMD(FD) ""
		set TCMD(BF) ""
		term_dspline "--ABORTED--"
		mark_running 0
	}
}

proc do_stop_term { } {

	global TCMD

	if { $TCMD(FD) != "" } {
		catch { close $TCMD(FD) }
		set TCMD(FD) ""
		set TCMD(BF) ""
	}
	mark_running 0
	term_dspline "--DONE--"
}

###############################################################################

proc mk_file_menu { } {
#
# Create the File menu of the project window; it must be done dynamically,
# because it depends on the list of recently opened projects
#
	global LProjects

	set m .menu.file

	$m delete 0 end

	$m add command -label "Open project ..." -command "open_project"
	$m add command -label "New project ..." -command "new_project"
	$m add separator

	if { $LProjects != "" } {
		set ix 0
		foreach p $LProjects {
			# this is a full file path, use the last so many
			# characters
			set p [trunc_fname 64 $p]
			$m add command -label $p -command "open_project $ix"
			incr ix
		}
		$m add separator
	}

	$m add command -label "Quit" -command "terminate"
	$m add separator
	$m add command -label "New file ..." -command "new_file"
	$m add command -label "New directory ..." -command "new_directory"
}

proc mk_build_menu { } {
#
# Create the build menu, which depends on the board selection and stuff
#
	global P

	set m .menu.build

	$m delete 0 end

	if !$P(AC) {
		# no project
		return
	}

	if { $P(CO) == "" } {
		# no project, no selection, no build
		return
	}

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]

	if { $mb != "" && $bo != "" } {
		# we do have mkmk
		if $mb {
			set bi 0
			foreach b $bo {
				set suf [lindex $P(PL) $bi]
				$m add command -label "mkmk $b $suf" \
					-command "do_mkmk_node $bi"
				incr bi
			}
			$m add separator
			set bi 0
			foreach b $bo {
				set suf [lindex $P(PL) $bi]
				$m add command -label "make -f Makefile_$suf" \
					-command "do_make_node $bi"
				incr bi
			}
			$m add separator
		} else {
			$m add command -label "mkmk $bo" -command "do_mkmk_node"
			$m add command -label "make" -command "do_make_node"
		}
	}

	$m add command -label "VUEE" -command "do_make_vuee"
	$m add separator
	$m add command -label "Abort" -command "do_abort_term"
}

proc mark_running_tm { } {

	global P TCMD

	set P(SSV) [format "%3d" $TCMD(CL)]
}

proc mark_running { stat } {

	global P TCMD

	if $stat {
		# running
		if { $TCMD(CB) != "" } {
			# the callback is active
			return
		}
		set TCMD(CL) 0
		set P(SSL) "Running: "
		mark_running_tm
		set TCMD(CB) [after 1000 mark_running_cb]
		return
	}

	if { $TCMD(CB) != "" } {
		after cancel $TCMD(CB)
		set TCMD(CB) ""
	}

	set P(SSL) "Idle:"
}

proc mark_running_cb { } {

	global TCMD P

	incr TCMD(CL)
	mark_running_tm
	set TCMD(CB) [after 1000 mark_running_cb]
}

proc mk_project_window { } {

	global P Term

	# no project active
	set P(AC) 0
	# no configuration
	set P(CO) ""

	menu .menu -tearoff 0

	#######################################################################

	set m .menu.file
	menu $m -tearoff 0

	.menu add cascade -label "File" -menu $m -underline 0

	mk_file_menu

	#######################################################################

	set m .menu.config
	menu $m -tearoff 0

	.menu add cascade -label "Configuration" -menu $m -underline 0
	$m add command -label "CPU+Board ..." -command "do_board_selection"

	#######################################################################

	set m .menu.build
	menu $m -tearoff 0

	.menu add cascade -label "Build" -menu $m -underline 0

	mk_build_menu

	#######################################################################

	. configure -menu .menu

	#######################################################################

	panedwindow .pane
	pack .pane -side top -expand y -fill both

	frame .pane.left
	pack .pane.left -side left -expand y -fill both

	mark_running 0

	frame .pane.left.sf
	pack .pane.left.sf -side top -expand n -fill x

	label .pane.left.sf.ss -textvariable P(SSL) -background white \
		-justify left -anchor w
	pack .pane.left.sf.ss -side left -expand y -fill x

	label .pane.left.sf.sv -textvariable P(SSV) -background white \
		-justify right -anchor e
	pack .pane.left.sf.sv -side right -expand y -fill x

	set cn .pane.left

	#######################################################################

	ttk::treeview $cn.tree 	-columns { filename type } \
				-displaycolumns {} \
				-yscroll "$cn.vsb set" \
				-xscroll "$cn.hsb set" \
				-show tree

	pack $cn.tree -side top -expand y -fill both

	ttk::scrollbar $cn.vsb -orient vertical -command "$cn.tree yview"
	ttk::scrollbar $cn.hsb -orient horizontal -command "$cn.tree xview"

	$cn.tree column \#0 -stretch 1 -width 120

	bind $cn.tree <<TreeviewOpen>> { gfl_open %W [%W focus] }
	bind $cn.tree <<TreeviewClose>> { gfl_close %W [%W focus] }

	lower [ttk::frame $cn.dummy]
	pack $cn.dummy -expand y -fill both
	grid $cn.tree $cn.vsb -sticky nsew -in $cn.dummy
	grid $cn.hsb -sticky nsew -in $cn.dummy
	grid columnconfigure $cn.dummy 0 -weight 1
	grid rowconfigure $cn.dummy 0 -weight 1

	set P(FL) $cn.tree

	bind $P(FL) <Double-1> "open_for_edit %x %y"
	bind $P(FL) <ButtonPress-3> "tree_menu %x %y %X %Y"
	bind $P(FL) <ButtonPress-2> "tree_menu %x %y %X %Y"

	#######################################################################

	set w .pane.right
	frame $w
	pack $w -side right -expand y -fill both -anchor e

	set Term $w.t

	text $Term

	$Term configure \
		-yscrollcommand "$w.scroly set" \
		-setgrid true \
        	-width 80 -height 24 -wrap char \
		-font {-family courier -size 10} \
		-exportselection 1 \
		-state normal

	$Term delete 1.0 end

	scrollbar $w.scroly -command "$w.t yview"
	# scrollbar $w.scrolx -orient horizontal -command "$w.t xview"
	pack $w.scroly -side right -fill y
	# pack $w.scrolx -side bottom -fill x
	pack $Term -expand yes -fill both

	#######################################################################

	# make it a paned window, so the tree view area can be easily resized
	.pane add .pane.left .pane.right

	bind . <Destroy> "terminate -force"
}

proc do_mkmk_node { { bi 0 } } {

	global P

	set al ""

	set mb [dict get $P(CO) "MB"]
	set bo [dict get $P(CO) "BO"]

	if $mb {
		lappend al [lindex $bo $bi]
		lappend al [lindex $P(PL) $bi]
	} else {
		lappend al $bo
	}

	if [catch { run_term_command "mkmk" $al } err] {
		alert $err
	}
}

proc do_make_node { { bi 0 } } {

	global P

	set mb [dict get $P(CO) "MB"]

	set al ""

	if $mb {
		# the index makes sense
		lappend al "-f"
		lappend al "Makefile_[lindex $P(PL) $bi]"
	}

	if [catch { run_term_command "make" $al } err] {
		alert $err
	}
}

###############################################################################

cygfix
	
###############################################################################

if { $PicOSPath == "" } {
	if [catch { xq picospath } PicOSPath] {
		puts stderr "cannot locate PicOS path: $PicOSPath"
		exit 99
	}
	set PicOSPath [file normalize $PicOSPath]
}
	
if { $DefProjDir == "" } {
	set DefProjDir [file join $PicOSPath Apps VUEE]
}

get_last_project_list

mk_project_window

###############################################################################

vwait forever
