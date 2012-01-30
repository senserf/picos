#!/bin/sh
##########################\
exec tclsh85 "$0" "$@"

############################################################################
# Generates copies of an Image file with modified node ID                  #
#                                                                          #
# Copyright (C) Olsonet Communications, 2008-2012 All Rights Reserved      #
############################################################################

set PM(VER)	1.3.8

proc usage { } {

	global argv0

	puts stderr "Usage: $argv0 \[-C \[configfile\]\]"
	exit 99
}

set ST(WSH) [info tclversion]

if { $ST(WSH) < 8.5 } {
	puts stderr "$argv0 requires Tcl/Tk 8.5 or newer!"
	exit 99
}

###############################################################################
# Determine whether we are on UNIX or Windows #################################
###############################################################################

if [catch { exec uname } ST(SYS)] {
	set ST(SYS) "W"
} elseif [regexp -nocase "linux" $ST(SYS)] {
	set ST(SYS) "L"
} elseif [regexp -nocase "cygwin" $ST(SYS)] {
	set ST(SYS) "C"
} else {
	set ST(SYS) "W"
}
if { $ST(SYS) != "L" } {
	# sanitize arguments; here you have a sample of the magnitude of
	# stupidity I have to fight when glueing together Windows and Cygwin
	# stuff; the last argument (sometimes!) has a CR character appended
	# at the end, and you wouldn't believe how much havoc that can cause
	set u [string trimright [lindex $argv end]]
	if { $u == "" } {
		set argv [lreplace $argv end end]
	} else {
		set argv [lreplace $argv end end $u]
	}
	unset u
}

set PM(PWD) [file normalize [pwd]]
set MWSG ""
# list of modes for Image files and the associated file name extensions
set PM(MDS) { "IHEX" "ELF" }
set PM(MDS,IHEX) { ".a43" {{ "Intel Hex" {*.a43}} { "All" {*} }}}
set PM(MDS,ELF) { "" {{ "All" {*} }}}

## double exit avoidance flag
set DEAF 0

proc terminate { } {

	global DEAF

	if $DEAF { return }

	set DEAF 1

	exit 0
}

###############################################################################
# Make sure we run as wish ####################################################
###############################################################################

if [catch { package require Tk $ST(WSH) } ] {
	puts stderr "Cannot find Tk version $ST(WSH) matching the Tcl version"
	exit 99
}

proc iscwd { dir } {

	global PM

	if { [file normalize $dir] == $PM(PWD) } {
		return 1
	}

	return 0
}

proc suff { } {
#
# Returns the currently effective file suffix
#
	global DEFS PM
	return [lindex $PM(MDS,$DEFS(MOD)) 0]
}

proc ftps { } {
#
# Returns the currently effective set of filetypes for file open dialog
#
	global DEFS PM
	return [lindex $PM(MDS,$DEFS(MOD)) 1]
}

proc reset_file_name_mode { } {
#
# Executed by setdefs and after a change of the file name mode (ihex/ELF) to
# (re) initialize the file view
#
	global DEFS PM IFN TFN

	foreach t { SRD TRD } i { IFN TFN } {
		if { $DEFS($t) != "" } {
			# check if exists
			if ![file isdirectory $DEFS($t)] {
				set DEFS($t) ""
			}
		}
		if { $DEFS($t) == "" } {
			set DEFS($t) $PM(DEP)
		}
		set DEFS($t) [file normalize $DEFS($t)]
		if { $DEFS($i) == "" || [file extension $DEFS($i)] != [suff] } {
			if { $i == "IFN" } {
				set DEFS($i) "Image[suff]"
			} else {
				set DEFS($i) "Image_nnnn[suff]"
			}
		}
		# check if the input image file is present
		if { $i == "IFN" && 
		    ![file isfile [file join $DEFS($t) $DEFS($i)]] } {
			# sorry, try to look something up
			if { [catch { glob -directory $DEFS($t) -tails \
			     "Image*[suff]" } fl] || $fl == "" } {
				# this won't work, anyway, but we need some
				# filler
				set DEFS($i) "Image[suff]"
			} else {
				set fl [lsort $fl]
				set fi ""
				foreach f $fl {
					# eliminate clones
					if ![regexp -nocase \
					     "_.*\[a-z0-9\]_\[0-9\]+" $f] {
						set fi $f
						break
					}
				}
				if { $fi == "" } {
					# still need the filler
					set fi "Image[suff]"
				}
				set DEFS($i) $fi
			}
		}

		if [iscwd $DEFS($t)] {
			# use local name
			set $i $DEFS($i)
		} else {
			# use full path
			set $i [file join $DEFS($t) $DEFS($i)]
		}
	}
}

proc setdefs { } {
#
# Set (default) paths to all relevant places
#
	global env argv PM ST DEFS

	# default configuration file name
	set PM(CFN) "config.gen"

	# locate the home directory
	if { $ST(SYS) == "L" } {
		# Linux: go straight to HOME
		if [info exists env(HOME)] {
			set PM(HOM) $env(HOME)
		} else {
			set PM(HOM) $PM(PWD)
		}
	} else {
		while 1 {
			if [info exists env(HOME)] {
				# cygwin?
				set PM(HOM) $env(HOME)
				break
			}
			if ![info exists env(HOMEPATH)] {
				set PM(HOM) $PM(PWD)
				break
			}
			set dfn [file join $env(HOMEPATH) "Desktop"]
			if [file isdirectory $dfn] {
				set PM(HOM) $dfn
				break
			}
			set dfn [file join $env(HOMEPATH) "Documents"]
			if [file isdirectory $dfn] {
				set PM(HOM) $dfn
				break
			}
			if [file isdirectory $env(HOMEPATH)] {
				set PM(HOM) $env(HOMEPATH)
				break
			}
			set PM(HOM) $PM(PWD)
			break
		}
	}
	
	set u [lsearch -exact $argv "-C"]
	if { $u >= 0 } {
		# config file name
		set v [lindex $argv [expr $u + 1]]
		if { $v != "" } {
			set PM(CFN) $v
			set v [expr $u + 1]
		} else {
			set v $u
		}
		# with -C, the default path for Image files is the current
		# directory
		set PM(DEP) $PM(PWD)
		# in case we ever want to handle other arguments
		set argv [lreplace $argv $u $v]
	} else {
		# determine the location of the config file
		if { $ST(SYS) == "L" } {
			set PM(CFN) [file join $PM(HOM) ".genimagerc"]
		} else {
			set dfn ""
			if [info exists env(APPDATA)] {
				set dfn [file join $env(APPDATA) "genimage"]
				if ![file isdirectory $dfn] {
					set dfn ""
				}
			}
			if { $dfn == "" } {
				set dfn $PM(HOM)
			}
			set PM(CFN) [file join $dfn $PM(CFN)]
		}
		# the default path to Image files is HOME
		set PM(DEP) $PM(HOM)
	}

	set PM(CFN) [file normalize $PM(CFN)]

	foreach t { SRD TRD IFN TFN IDN PFX FRM CNT MOD } {
		## SRD - source directory
		## TRD - target directory
		## IFN - input file name
		## TFN - target file name (pattern)
		## IDN - cookie
		## PFX - target cookie replacement prefix
		## FRM - starting counter value
		## CNT - number of copies
		## MOD - file mode (IHEX, ELF)
		set DEFS($t) ""
	}

	while 1 {
		if [catch { open $PM(CFN) "r" } dfd] {
			# no config file
			break
		}
		if [catch { read -nonewline $dfd } defs] {
			# cannot read
			catch { close $dfd }
			break
		}
		catch { close $dfd }
		foreach d $defs {
			set key [lindex $d 0]
			if { $key != "" } {
				set DEFS($key) [lindex $d 1]
			}
		}
		break
	}

	# make sure the mode is sane
	set i 1
	foreach t $PM(MDS) {
		if { $DEFS(MOD) == $t } {
			set i 0
			break
		}
	}

	if $i {
		set DEFS(MOD) [lindex $PM(MDS) 0]
	}

	reset_file_name_mode

	if { [verify_idn $DEFS(IDN) 8] == "" } {
		set DEFS(IDN) "BACADEAD"
	}

	if { [verify_idn $DEFS(PFX) 4] == "" } {
		set DEFS(PFX) "BACA"
	}

	if { [verify_pnm $DEFS(FRM)] == "" } {
		set DEFS(FRM) 1
	}

	if { [verify_pnm $DEFS(CNT)] == "" } {
		set DEFS(CNT) 1
	}
}

proc verify_pnm { s } {
#
# Integer >= 1
#
	if [catch { expr $s } v] {
		return ""
	}

	if [regexp -nocase "\[-.e\]" $s] {
		return ""
	}

	if { $v < 1  || $v > 65535 } {
		return ""
	}

	return $v
}

proc verify_idn { s n } {
#
# A string of bytes represented as hex digits in little endian
#
	set ln [string length $s]

	if { $ln != $n } {
		return ""
	}

	set res ""

	while { $ln } {
		set b [string toupper [string range $s 0 1]]
		set s [string range $s 2 end]
		if ![regexp "\[0-9A-F\]\[0-9A-F\]" $b] {
			return ""
		}
		if { $res == "" } {
			set res $b
		} else {
			set res "$b $res"
		}
		incr ln -2
	}
	return $res
}

proc idn_to_bin { idn } {
#
# Transform a cookie string (4 bytes HEX, back to back) into a binary sequence
# of the respective bytes
#
	return [binary format H8 $idn]
}

proc increment { b } {
#
# Increments the hex string
#
	set r ""

	set more 1

	foreach v $b {

		if $more {
			set v [expr 0x$v + 1]
			if { $v < 256 } {
				set more 0
			} else {
				set v 0
			}
			set v [format "%02X" $v]
		}
		lappend r $v
	}

	return $r
}

proc checksum { s } {

	if { [expr [string length $s] & 1] != 0 } {
		return ""
	}

	set sum 0

	while { $s != "" } {
		set b [string range $s 0 1]
		set s [string range $s 2 end]
		if [catch { expr 0x$b + $sum } sum] {
			return ""
		}
	}

	return [format %02X [expr (0x100 - ( $sum & 0xff )) & 0xff]]
}

proc ferror { n s } {

	error "Image format error, line $n: $s!"
}

proc cerror { n s } {

	error "Image checksum error, line $n: $s!"
}

###############################################################################
# HEX file processing #########################################################
###############################################################################

proc read_source_IHEX { fn } {
#
# Reads the source image file
#
	set chunks ""
	set chunk ""
	set caddr -1

	if [catch { open $fn "r" } fd] {
		error "Cannot open image file $fn: $fd"
	}

	if [catch { read $fd } fc] {
		catch { close $fd }
		error "Cannot read image file $fn: $fc"
	}

	catch { close $fd }

	# preprocess into chunks
	set fc [split $fc "\n"]
	set linenum 1
	foreach ln $fc {
		set ln [string toupper [string trim $ln]]
		if ![regexp "^:(..)(....)(..)(.*)(..)" $ln jk bc ad rt rec ch] {
			incr linenum
			continue
		}
		set chk [checksum "$bc$ad$rt$rec"]
		if { $chk == "" } {
			ferror $linenum $ln
		}
		if { [catch { expr 0x$rt } rt] || \
		     [catch { expr 0x$bc } bc] || \
		     [catch { expr 0x$ad } ad] || \
		     [catch { expr 0x$ch } ch]  } {
			    ferror $linenum $ln
		}
		set ll [string length $rec]
		if { $ll != [expr $bc * 2] } {
			ferror $linenum $ln
		}
		if { $ch != [expr 0x$chk] } {
			cerror $linenum $ln
		}
		if { $rt != 0 } {
			# not a data record
			if { $chunk != "" } {
				# pending data record
				lappend chunks [list $start 0 $chunk]
				set chunk ""
				set caddr -1
			}
			lappend chunks [list $ad $rt $rec]
		} else {
			# data record
			if { $ad != $caddr } {
				# start a new chunk
				if { $chunk != "" } {
					lappend chunks \
						[list $start 0 $chunk]
				}
				set start $ad
				set chunk $rec
				set caddr [expr $ad + $bc]
			} else {
				append chunk $rec
				set caddr [expr $caddr + $bc]
			}
		}
		incr linenum
	}

	if { $chunk != "" } {
		lappend chunks [list $start 0 $chunk]
	}

	return $chunks
}

proc locate_id_IHEX { im idn } {
#
# Locates the tag in the image
#
	set cn -1
	set ix -1
	foreach ch $im {
		incr ix
		if { [lindex $ch 1] != 0 } {
			# not a data record
			continue
		}
		set chunk [lindex $ch 2]
		set loc [string first $idn $chunk] 
		if { $loc < 0 } {
			lappend new $ch
			continue
		}
		if { $cn >= 0 || [string last $idn $chunk] != $loc } {
			error "Image error, multiple occurrences of the cookie!"
		}
		set cn $ix
		set po $loc
	}

	if { $cn < 0 } {
		error "Cookie not found in the image file!"
	}

	return [list $cn $po]
}

proc modify_image_IHEX { im org s } {
#
# Substitute the new string in the image
#
	set new ""

	set wh [lindex $org 0]
	set ix [lindex $org 1]

	set ch [lindex $im $wh]
	set ad [lindex $ch 0]
	set rc [lindex $ch 2]

	set rl [string length $s]

	set rc "[string range $rc 0 [expr $ix - 1]]$s[string range $rc \
		[expr $ix + $rl] end]"

	set im [lreplace $im $wh $wh [list $ad 0 $rc]]

	return $im
}

proc write_image_IHEX { im fn } {

	if [catch { open $fn "w" } fd] {
		error "Cannot open $fn for writing: $fd!"
	}

	foreach ch $im {
		set ad [lindex $ch 0]
		set rt [lindex $ch 1]
		set rc [lindex $ch 2]
		while 1 {
			set ln [string length $rc]
			if { $ln > 32 } {
				set ln 32
			}
			set bc [expr $ln / 2]
			set bk [format "%02X%04X%02X" $bc $ad $rt]
			append bk [string range $rc 0 [expr $ln - 1]]
			append bk [checksum $bk]
			if [catch {
				puts $fd ":$bk"
			} er] {
				error "Cannot write to $fn:$er!"
			}
			set rc [string range $rc $ln end]
			if { $rc == "" } {
				break
			}
			set ad [expr $ad + $bc]
		}
	}
	catch { close $fd }
}

###############################################################################
# ELF file processing #########################################################
###############################################################################

proc read_source_ELF { fn } {
#
# Reads the source image file
#
	if [catch { open $fn "r" } fd] {
		error "Cannot open image file $fn: $fd"
	}

	fconfigure $fd -encoding binary -translation binary

	if [catch { read $fd } fc] {
		catch { close $fd }
		error "Cannot read image file $fn: $fc"
	}

	catch { close $fd }

	return $fc
}

proc locate_id_ELF { im idn } {
#
# Locates the tag in the image
#
	# translate idn to binary
	set idb [idn_to_bin $idn]
	# first location
	set loc [string first $idb $im]
	if { $loc < 0 } {
		error "Cookie not found in the image file!"
	}
	if { [string last $idb $im] != $loc } {
		error "Image error, multiple occurrences of the cookie!"
	}
	return $loc
}
	
proc modify_image_ELF { im org s } {
#
# Substitute the new string in the image
#
	set s [idn_to_bin $s]
	set len [string length $s]
	return [string replace $im $org [expr $org + $len - 1] $s]
}

proc write_image_ELF { im fn } {

	if [catch { open $fn "w" } fd] {
		error "Cannot open $fn for writing: $fd!"
	}

	fconfigure $fd -encoding binary -translation binary
	puts -nonewline $fd $im
	catch { close $fd }
}

###############################################################################
###############################################################################
###############################################################################

proc click_file_mode { menb n } {
#
# Click on the file mode change button
#
	global DEFS PM

	set fm [lindex $PM(MDS) $n]
	if { $fm == "" } {
		return
	}

	if { $fm != $DEFS(MOD) } {
		set DEFS(MOD) $fm
		$menb configure -text $fm
		reset_file_name_mode
	}
}
			
proc mkfname { ofn s } {
#
# New file name
#
	set t ""
	foreach q $s {
		set t "${q}$t"
	}
	set rfn "[file rootname $ofn]"

	regsub -nocase "_\[a-z0-9.\]*$" $rfn "" rfn

	append t "_[format %05u [expr 0x$t & 0x0000ffff]]"

	if { $rfn == $ofn } {
		return "${rfn}_$t"
	}

	return "${rfn}_$t[file extension $ofn]"
}

###############################################################################

proc savdefs { } {
#
# Save the defaults
#
	global PM env DEFS

	if [catch { open $PM(CFN) "w" } dfd] {
		# cannot write
		return
	}
	set w ""
	foreach d [array names DEFS] {
		lappend w [list $d $DEFS($d)]
	}

	catch { puts -nonewline $dfd $w }
	catch { close $dfd }
}

###############################################################################

proc disable_go { } {
	global GOBUT
	$GOBUT configure -state disabled
}

proc enable_go { } {
	global GOBUT
	$GOBUT configure -state normal
}

proc cw { } {
#
# Returns the window currently in focus or null if this is the root window
#
	set w [focus]
	if { $w == "." } {
		set w ""
	}

	return $w
}

proc alert { msg } {

	tk_dialog [cw].alert "Attention!" $msg "" 0 "OK"
}

proc mk_mess_window { w h } {
#
# Progress message
#
	global WMSG

	set WMSG "[cw].msg"

	toplevel $WMSG

	wm title $WMSG "Progress report"

	label $WMSG.t -width $w -height $h -borderwidth 2 -state normal
	pack $WMSG.t -side top -fill x -fill y
	button $WMSG.c -text "Cancel" -command cancel_gen
	pack $WMSG.c -side top
	bind $WMSG <Destroy> cancel_gen
	$WMSG.t configure -text ""
	raise $WMSG
}

proc cancelled { } {

	global WMSG

	if { $WMSG == "" } {
		return 1
	}

	if ![winfo exists $WMSG] {
		set WMSG ""
		return 1
	}
	return 0
}

proc outpgs { msg } {
#
# Progress message, detect cancellation
#
	global WMSG

	if [cancelled] {
		return 1
	}

	catch { $WMSG.t configure -text $msg }

	return 0
}

proc cancel_gen { } {

	global EWAIT WMSG

	catch { destroy $WMSG }
	set WMSG ""
	incr EWAIT
}

proc blink { } {

	global EWAIT

	incr EWAIT
}

proc change_ifn { } {
#
# Input name selection
#
	global DEFS IFN

	set fn [tk_getOpenFile \
			-defaultextension [suff] \
			-initialdir $DEFS(SRD) \
			-filetypes [ftps] \
			-title "Source Image File"]

	if { $fn == "" } {
		# no change
		return
	}

	set DEFS(SRD) [file dirname $fn]
	set DEFS(IFN) [file tail $fn]
	if [iscwd $DEFS(SRD)] {
		# local name will do
		set IFN $DEFS(IFN)
	} else {
		set IFN $fn
	}
}

proc change_trd { } {
#
# Target directory selection
#
	global DEFS TFN

	set fn [tk_getSaveFile \
		-initialdir $DEFS(TRD) \
		-initialfile $DEFS(TFN) \
		-title "Target files"]

	if { $fn == "" } {
		# no change
		return
	}

	set DEFS(TRD) [file dirname $fn]
	set DEFS(TFN) [file tail $fn]
	if [iscwd $DEFS(TRD)] {
		set TFN $DEFS(TFN)
	} else {
		set TFN $fn
	}
}

proc generate { } {
#
# Do it
# 
	global TFN IFN DEFS EWAIT

	if { $IFN == "" } {
		alert "Input file has not been specified!"
		return
	}

	if { $TFN == "" } {
		alert "Destination file(s) unknown!"
		return
	}

	set str [verify_pnm $DEFS(FRM)]
	if { $str == "" } {
		alert "The FROM number is illegal, must be something integer\
			greater than zero and less than 65536!"
		return
	}

	set cnt [verify_pnm $DEFS(CNT)]
	if { $cnt == "" } {
		alert "The COUNT is illegal, must be something integer\
			greater than zero and less than 65536!"
		return
	}

	if { [expr $str + $cnt] > 65536 } {
		alert "FROM + COUNT is greater than 65536, which is illegal,\
			as the maximum node number is 65535!"
		return
	}

	set idn [verify_idn $DEFS(IDN) 8]
	if { $idn == "" } {
		alert "The cookie is illegal, must be an 8-digit hex number!"
		return
	}

	set pfx [verify_idn $DEFS(PFX) 4]
	if { $pfx == "" } {
		alert "The prefix is illegal, must be a 4-digit hex number!"
		return
	}

	if [catch { read_source_$DEFS(MOD) $IFN } image] {
		alert $image
		return
	}

	# transform str into hex string
	set frm [verify_idn "$DEFS(PFX)[format %04X $str]" 8]
	if { $frm == "" } {
		# impossible
		alert "Internal error converting node number!"
		return
	}

	if [catch { locate_id_$DEFS(MOD) $image [join $idn ""] } org] {
		alert $org
		return
	}

	disable_go
	mk_mess_window 32 1

	while { $cnt } {

		if [outpgs "Doing image $str, $cnt to go ..."] {
			# cancelled
			break
		}

		# wait for the message window to update
		after 100 blink
		tkwait variable EWAIT
		if [cancelled] {
			# in case
			break
		}

		set image [modify_image_$DEFS(MOD) $image $org [join $frm ""]]

		set nfn [mkfname $TFN $frm]
		if [catch { write_image_$DEFS(MOD) $image $nfn } err] {
			cancel_gen
			alert "Cannot write to $nfn: $err!"
			enable_go
			return
		}
		set frm [increment $frm]
		incr cnt -1
		incr str
	}
	cancel_gen
	enable_go

	if { $cnt == 0 } {
		alert "All done!"
		savdefs
	}
}


###############################################################################

catch { close stdin }
## stdout needed by PIP, stderr useful for errors
## catch { close stdout }
## catch { close stderr }

set EWAIT 0

setdefs

wm title . "Flash Image Generator $PM(VER)"

labelframe .fls -text "Files" -padx 4 -pady 4
pack .fls -side top -expand 1 -fill x

##

set w ".fls"

label $w.ifl -text "Input file:"
grid $w.ifl -column 0 -row 0 -sticky w -padx 4

entry $w.ifn -width 58 -textvariable IFN
grid $w.ifn -column 1 -row 0 -sticky we -padx 4 

button $w.ifb -text "Change" -command "change_ifn"
grid $w.ifb -column 2 -row 0 -sticky w -padx 4

##

label $w.odl -text "Output file(s): "
grid $w.odl -column 0 -row 1 -sticky w -padx 4

entry $w.odn -width 58 -textvariable TFN
grid $w.odn -column 1 -row 1 -sticky we -padx 4

button $w.odb -text "Change" -command "change_trd"
grid $w.odb -column 2 -row 1 -sticky w -padx 4

##

grid columnconfigure $w 1 -weight 10

##

frame .sel
pack .sel -side top -fill x

##

labelframe .sel.mo -text "File type" -padx 4 -pady 4
pack .sel.mo -side left -expand 0 -fill both

##

set w ".sel.mo"
label $w.sel -text "Select: "
pack $w.sel -side left -expand 0 -padx 4

menubutton $w.men -text $DEFS(MOD) -direction right -menu $w.men.m \
	-relief raised
pack $w.men -side left -expand 0
menu $w.men.m -tearoff 0
set n 0
foreach t $PM(MDS) {
	$w.men.m add command -label $t -command "click_file_mode $w.men $n"
	incr n
}

##

labelframe .sel.nn -text "Node numbers" -padx 4 -pady 4
pack .sel.nn -side left -expand 1 -fill both

##

set w ".sel.nn"

label $w.frl -text "From: "
pack $w.frl -side left -expand 0 -padx 4

entry $w.frn -width 6 -textvariable DEFS(FRM)
pack $w.frn -side left -expand 0 -padx 4

##

label $w.pad -text "  "
pack $w.pad -side left -expand 1 -fill x

##

label $w.cnl -text "How many: "
pack $w.cnl -side left -expand 0 -padx 4

entry $w.cnn -width 6 -textvariable DEFS(CNT)
pack $w.cnn -side left -expand 0 -padx 4

##

labelframe .sel.co -text "Cookie" -padx 4 -pady 4
pack .sel.co -side left -expand 0 -fill both

##

set w ".sel.co"

entry $w.co -width 10 -textvariable DEFS(IDN)
pack $w.co -side top -padx 4

##

labelframe .sel.pr -text "Prefix" -padx 4 -pady 4
pack .sel.pr -side left -expand 0 -fill both

##

set w ".sel.pr"

entry $w.pr -width 5 -textvariable DEFS(PFX)
pack $w.pr -side top -padx 4

##

frame .act -pady 4 -padx 4 -bd 2
pack .act -side top -expand 1 -fill x

##

set w ".act"

button $w.run -text "Generate" -command "generate"
pack $w.run -side left -padx 4

set GOBUT "$w.run"

button $w.q -text "Exit" -command "terminate"
pack $w.q -side right -padx 4

bind . <Destroy> { terminate }
