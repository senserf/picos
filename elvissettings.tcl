#!/bin/sh
##########################\
exec tclsh85 "$0" "$@"

##############################################
# Creates a list of color settings for elvis #
##############################################

package require Tk

## Font options
set FontSizes { 5x7 5x8 6x9 6x10 6x12 6x13 7x13 7x14 8x13 9x15 9x18 10x20 }

## Attributes
set Attribs { bold italic underlined boxed }

## The order of colors
set CList { normal idle bottom lnum showmode ruler selection hlsearch
	cursor tool toolbar scroll scrollbar status statusbar comment
	string keyword function number prep prepquote other variable fixed
	libt argument hexheading linenumber formatted link spell }

## The defaults

set Values(normal)	{ "" #00FF00 "" #000000 }	;# green on black
set Values(idle)	{ normal }			;# green on black
set Values(bottom)	{ normal }			;# green on black
set Values(lnum)	{ normal }
set Values(showmode)	{ normal }
set Values(ruler)	{ normal }
set Values(selection)	{ "" "" "" #D2B48c }		;# on tan
set Values(hlsearch)	{ "" "" "" "" boxed }
##
set Values(cursor)	{ "" #FF0000 "" #FFFF00 }	;# red on yellow
set Values(tool)	{ "" #000000 "" #BFBFBF }	;# black on gray75
set Values(toolbar)	{ "" #FFFFFF "" #666666 }	;# white on gray40
set Values(scroll)	{ tool }
set Values(scrollbar)	{ toolbar }
set Values(status)	{ tool }
set Values(statusbar)	{ toolbar }
##
set Values(comment)	{ "" #006400 #90EE90 "" italic };# #dark or light green
set Values(string)	{ "" #8B5A2B #FFA54F }		;# tan4 or tan1
set Values(keyword)	{ "" "" "" "" bold }
set Values(function)	{ "" #8B0000 #FFC0CB }
set Values(number)	{ "" #00008B #ADD8E6 }
set Values(prep)	{ number "" "" "" bold }
set Values(prepquote)	{ string }
set Values(other)	{ keyword }
set Values(variable)	{ "" #262626 #EEE8AA }
set Values(fixed)	{ "" #595959 #CCCCCC }
set Values(libt)	{ keyword "" "" "" italic }
set Values(argument)	{ "" #00FF00 #006400 "" bold }
set Values(hexheading)	{ "" #B3B3B3 }
set Values(linenumber)	{ "" #BEBEBE }
##
set Values(formatted)	{ normal }
set Values(link)	{ "" #0000FF #ADD8E6 "" underlined }
set Values(spell)	{ "" "" "" #FFC0CB }
##
set Values(font)	"9x15"
set Values(commands)	""

set DEAF 0

###############################################################################
# Implements a scrolled frame #################################################
###############################################################################

if {[info exists ::scrolledframe::version]} { return }
  namespace eval ::scrolledframe \
  {
  # beginning of ::scrolledframe namespace definition

    package require Tk 8.4
    namespace export scrolledframe

  # ==============================
  #
  # scrolledframe
  set version 0.9.1
  set (debug,place) 0
  #
  # a scrolled frame
  #
  # (C) 2003, ulis
  #
  # NOL licence (No Obligation Licence)
  #
  # Changes (C) 2004, KJN
  #
  # NOL licence (No Obligation Licence)
  # ==============================
  #
  # Hacked package, no documentation, sorry
  # See example at bottom
  #
  # ------------------------------
  # v 0.9.1
  #  automatic scroll on resize
  # ==============================

    package provide Scrolledframe $version

    # --------------
    #
    # create a scrolled frame
    #
    # --------------
    # parm1: widget name
    # parm2: options key/value list
    # --------------
    proc scrolledframe {w args} \
    {
      variable {}
      # create a scrolled frame
      frame $w
      # trap the reference
      rename $w ::scrolledframe::_$w
      # redirect to dispatch
      interp alias {} $w {} ::scrolledframe::dispatch $w
      # create scrollable internal frame
      frame $w.scrolled -highlightt 0 -padx 0 -pady 0
      # place it
      place $w.scrolled -in $w -x 0 -y 0
      if {$(debug,place)} { puts "place $w.scrolled -in $w -x 0 -y 0" } ;#DEBUG
      # init internal data
      set ($w:vheight) 0
      set ($w:vwidth) 0
      set ($w:vtop) 0
      set ($w:vleft) 0
      set ($w:xscroll) ""
      set ($w:yscroll) ""
      set ($w:width)    0
      set ($w:height)   0
      set ($w:fillx)    0
      set ($w:filly)    0
      # configure
      if {$args != ""} { uplevel 1 ::scrolledframe::config $w $args }
      # bind <Configure>
      bind $w <Configure> [namespace code [list resize $w]]
      bind $w.scrolled <Configure> [namespace code [list resize $w]]
      # return widget ref
      return $w
    }

    # --------------
    #
    # dispatch the trapped command
    #
    # --------------
    # parm1: widget name
    # parm2: operation
    # parm2: operation args
    # --------------
    proc dispatch {w cmd args} \
    {
      variable {}
      switch -glob -- $cmd \
      {
        con*    { uplevel 1 [linsert $args 0 ::scrolledframe::config $w] }
        xvi*    { uplevel 1 [linsert $args 0 ::scrolledframe::xview  $w] }
        yvi*    { uplevel 1 [linsert $args 0 ::scrolledframe::yview  $w] }
        default { uplevel 1 [linsert $args 0 ::scrolledframe::_$w    $cmd] }
      }
    }

    # --------------
    # configure operation
    #
    # configure the widget
    # --------------
    # parm1: widget name
    # parm2: options
    # --------------
    proc config {w args} \
    {
      variable {}
      set options {}
      set flag 0
      foreach {key value} $args \
      {
        switch -glob -- $key \
        {
          -fill   \
          {
            # new fill option: what should the scrolled object do if it is
	    # smaller than the viewing window?
            if {$value == "none"} {
               set ($w:fillx) 0
               set ($w:filly) 0
            } elseif {$value == "x"} {
               set ($w:fillx) 1
               set ($w:filly) 0
            } elseif {$value == "y"} {
               set ($w:fillx) 0
               set ($w:filly) 1
            } elseif {$value == "both"} {
               set ($w:fillx) 1
               set ($w:filly) 1
            } else {
               error "invalid value: should be \"$w configure -fill value\",\
			where \"value\" is \"x\", \"y\", \"none\", or \"both\""
            }
            resize $w force
            set flag 1
          }
          -xsc*   \
          {
            # new xscroll option
            set ($w:xscroll) $value
            set flag 1
          }
          -ysc*   \
          {
            # new yscroll option
            set ($w:yscroll) $value
            set flag 1
          }
          default { lappend options $key $value }
        }
      }
      # check if needed
      if {!$flag || $options != ""} \
      {
        # call frame config
        uplevel 1 [linsert $options 0 ::scrolledframe::_$w config]
      }
    }

    # --------------
    # resize proc
    #
    # Update the scrollbars if necessary, in response to a change in either the
    # viewing window or the scrolled object.
    # Replaces the old resize and the old vresize
    # A <Configure> call may mean any change to the viewing window or the
    # scrolled object.
    # We only need to resize the scrollbars if the size of one of these objects
    # has changed.
    # Usually the window sizes have not changed, and so the proc will not
    # resize the scrollbars.
    # --------------
    # parm1: widget name
    # parm2: pass anything to force resize even if dimensions are unchanged
    # --------------
    proc resize {w args} \
    {
      variable {}
      set force [llength $args]

      set _vheight     $($w:vheight)
      set _vwidth      $($w:vwidth)
      # compute new height & width
      set ($w:vheight) [winfo reqheight $w.scrolled]
      set ($w:vwidth)  [winfo reqwidth  $w.scrolled]

      # The size may have changed, e.g. by manual resizing of the window
      set _height     $($w:height)
      set _width      $($w:width)
      set ($w:height) [winfo height $w] ;# gives the actual height
      set ($w:width)  [winfo width  $w] ;# gives the actual width

      if {$force || $($w:vheight) != $_vheight || $($w:height) != $_height} {
        # resize the vertical scroll bar
        yview $w scroll 0 unit
        # yset $w
      }

      if {$force || $($w:vwidth) != $_vwidth || $($w:width) != $_width} {
        # resize the horizontal scroll bar
        xview $w scroll 0 unit
        # xset $w
      }
    } ;# end proc resize

    # --------------
    # xset proc
    #
    # resize the visible part
    # --------------
    # parm1: widget name
    # --------------
    proc xset {w} \
    {
      variable {}
      # call the xscroll command
      set cmd $($w:xscroll)
      if {$cmd != ""} { catch { eval $cmd [xview $w] } }
    }

    # --------------
    # yset proc
    #
    # resize the visible part
    # --------------
    # parm1: widget name
    # --------------
    proc yset {w} \
    {
      variable {}
      # call the yscroll command
      set cmd $($w:yscroll)
      if {$cmd != ""} { catch { eval $cmd [yview $w] } }
    }

    # -------------
    # xview
    #
    # called on horizontal scrolling
    # -------------
    # parm1: widget path
    # parm2: optional moveto or scroll
    # parm3: fraction if parm2 == moveto, count unit if parm2 == scroll
    # -------------
    # return: scrolling info if parm2 is empty
    # -------------
    proc xview {w {cmd ""} args} \
    {
      variable {}
      # check args
      set len [llength $args]
      switch -glob -- $cmd \
      {
        ""      {set args {}}
        mov*    \
        { if {$len != 1} { error "wrong # args: should be \"$w xview moveto\
			fraction\"" } }
        scr*    \
        { if {$len != 2} { error "wrong # args: should be \"$w xview scroll\
			count unit\"" } }
        default \
        { error "unknown operation \"$cmd\": should be empty,\
			moveto or scroll" }
      }
      # save old values:
      set _vleft $($w:vleft)
      set _vwidth $($w:vwidth)
      set _width  $($w:width)
      # compute new vleft
      set count ""
      switch $len \
      {
        0       \
        {
          # return fractions
          if {$_vwidth == 0} { return {0 1} }
          set first [expr {double($_vleft) / $_vwidth}]
          set last [expr {double($_vleft + $_width) / $_vwidth}]
          if {$last > 1.0} { return {0 1} }
          return [list $first $last]
        }
        1       \
        {
          # absolute movement
          set vleft [expr {int(double($args) * $_vwidth)}]
        }
        2       \
        {
          # relative movement
          foreach {count unit} $args break
          if {[string match p* $unit]} { set count [expr {$count * 9}] }
          set vleft [expr {$_vleft + $count * 0.1 * $_width}]
        }
      }
      if {$vleft + $_width > $_vwidth} { set vleft [expr {$_vwidth - $_width}] }
      if {$vleft < 0} { set vleft 0 }
      if {$vleft != $_vleft || $count == 0} \
      {
        set ($w:vleft) $vleft
        xset $w
        if {$($w:fillx) && ($_vwidth < $_width || $($w:xscroll) == "") } {
          # "scrolled object" is not scrolled, because it is too small or
	  # because no scrollbar was requested
          # fillx means that, in these cases, we must tell the object what
	  # its width should be
          place $w.scrolled -in $w -x [expr {-$vleft}] -width $_width
          if {$(debug,place)} { puts "place $w.scrolled -in $w -x\
		[expr {-$vleft}] -width $_width" } ;#DEBUG
        } else {
          place $w.scrolled -in $w -x [expr {-$vleft}] -width {}
          if {$(debug,place)} { puts "place $w.scrolled -in $w -x\
		[expr {-$vleft}] -width {}" } ;#DEBUG
        }

      }
    }

    # -------------
    # yview
    #
    # called on vertical scrolling
    # -------------
    # parm1: widget path
    # parm2: optional moveto or scroll
    # parm3: fraction if parm2 == moveto, count unit if parm2 == scroll
    # -------------
    # return: scrolling info if parm2 is empty
    # -------------
    proc yview {w {cmd ""} args} \
    {
      variable {}
      # check args
      set len [llength $args]
      switch -glob -- $cmd \
      {
        ""      {set args {}}
        mov*    \
        { if {$len != 1} { error "wrong # args: should be \"$w yview moveto\
		fraction\"" } }
        scr*    \
        { if {$len != 2} { error "wrong # args: should be \"$w yview scroll\
		count unit\"" } }
        default \
        { error "unknown operation \"$cmd\": should be empty,\
		moveto or scroll" }
      }
      # save old values
      set _vtop $($w:vtop)
      set _vheight $($w:vheight)
  #    set _height [winfo height $w]
      set _height $($w:height)
      # compute new vtop
      set count ""
      switch $len \
      {
        0       \
        {
          # return fractions
          if {$_vheight == 0} { return {0 1} }
          set first [expr {double($_vtop) / $_vheight}]
          set last [expr {double($_vtop + $_height) / $_vheight}]
          if {$last > 1.0} { return {0 1} }
          return [list $first $last]
        }
        1       \
        {
          # absolute movement
          set vtop [expr {int(double($args) * $_vheight)}]
        }
        2       \
        {
          # relative movement
          foreach {count unit} $args break
          if {[string match p* $unit]} { set count [expr {$count * 9}] }
          set vtop [expr {$_vtop + $count * 0.1 * $_height}]
        }
      }
      if {$vtop + $_height > $_vheight} { set vtop [expr {$_vheight - $_height}] }
      if {$vtop < 0} { set vtop 0 }
      if {$vtop != $_vtop || $count == 0} \
      {
        set ($w:vtop) $vtop
        yset $w
        if {$($w:filly) && ($_vheight < $_height || $($w:yscroll) == "")} {
          # "scrolled object" is not scrolled, because it is too small or
	  # because no scrollbar was requested
          # filly means that, in these cases, we must tell the object what its
	  # height should be
          place $w.scrolled -in $w -y [expr {-$vtop}] -height $_height
          if {$(debug,place)} { puts "place $w.scrolled -in $w -y\
		[expr {-$vtop}] -height $_height" } ;#DEBUG
        } else {
          place $w.scrolled -in $w -y [expr {-$vtop}] -height {}
          if {$(debug,place)} { puts "place $w.scrolled -in $w -y\
		[expr {-$vtop}] -height {}" } ;#DEBUG
        }
      }
    }

  # end of ::scrolledframe namespace definition
  }

  package require Scrolledframe
  namespace import ::scrolledframe::scrolledframe

###############################################################################
# End scrolled frame ##########################################################
###############################################################################

proc alert { msg } {

	tk_dialog .alert "Attention!" "${msg}!" "" 0 "OK"
}

proc advlist { src n item } {

	set ix 0
	set tar ""
	foreach it $src {
		if { $ix == $n } {
			set it $item
		}
		lappend tar $it
		incr ix
	}

	while { $ix <= $n } {

		if { $ix != $n } {
			lappend tar ""
		} else {
			lappend tar $item
		}

		incr ix
	}

	return $tar
}

proc valcol { c } {
#
# Validates a color
#
	set c [string toupper [string trim $c]]
	if { [string length $c] != 7 || ![regexp "^#\[0-9A-F\]+$" $c] } {
		error "illegal color $c"
	}
	return $c
}

proc read_old_conf { } {

	global Values FontSizes Attribs

	set cs [read stdin]
	set er ""

	foreach c $cs {

		set f [lindex $c 0]
		set l [lindex $c 1]

		if { $f == "font" } {
			if { [lsearch -exact $FontSizes $l] < 0 } {
				lappend er "illegal font size: $l"
			} else {
				set Values(font) $l
			}
			continue
		}

		if { $f == "commands" } {
			# this one is not verified
			set Values(commands) $l
			continue
		}

		if ![info exists Values($f)] {
			# ignore garbage
			lappend er "unknown face $f"
			continue
		}

		set ll [llength $l]

		lassign $l like color alter backgr
		# the list of attributes
		set l [lrange $l 4 end]

		if { $like != "" && ($like == $f ||
		    ![info exists Values($like)]) } {
			lappend er "illegal like redirection $f -> $like"
			continue
		}

		set bad 0
		foreach a { color alter backgr } {
			eval "set cc $[subst $a]"
			if { $cc != "" } {
				if [catch { valcol $cc } cc] {
					lappend er $cc
					set bad 1
					break
				}
				set $a $cc
			}
		}

		if $bad {
			continue
		}

		set al ""

		set bad 0
		foreach a $l {

			if { [lsearch -exact $Attribs $a] < 0 } {
				lappend er "illegal attribute $a"
				set bad 1
				break
			}

			lappend al $a
		}

		if $bad {
			continue
		}

		set Values($f) [concat [list $like $color $alter $backgr] $al]
	}

	return $er
}

proc color_pick { ix wi } {
#
# Pick a color, modify the entry, repaint the widget
#
	global Values CList

	set cn ""
	# last two characters of the widget name determine the color type
	regexp "..$" $wi cn

	# the face
	set f [lindex $CList $ix]

	# the list of values
	set vals $Values($f)

	set ci [lsearch -exact { co al bg } $cn]
	if { $ci < 0 } {
		# impossible
		return
	}

	# index into vals
	incr ci

	set col [lindex $vals $ci]
	if { $col == "" } {
		set col #000000
	}

	set col [tk_chooseColor -initialcolor $col -title \
	    "Choose [lindex { "" foreground alternate background } $ci] color"]

	# replace the color in the list
	set Values($f) [advlist $vals $ci $col]

	if { $col == "" } {
		$wi configure -bg white -text "none"
	} else {
		$wi configure -bg $col -text ""
	}
}

proc do_cancel { } {

	global DEAF

	if $DEAF {
		return
	}

	catch { close stdout }
	catch { destroy . }
	set DEAF 1
	exit 0
}

proc update_attributes { } {

	global ST Attribs CList Values

	set ix 0
	foreach f $CList {
		# truncate on attributes
		set vals [lrange [advlist $Values($f) 4 ""] 0 3]
		set lik $ST($ix,LIK)
		if ![info exists Values($lik)] {
			set lik ""
		}
		set vals [advlist $vals 0 $lik]
		foreach cd $Attribs {
			if $ST($ix,$cd) {
				lappend vals $cd
			}
		}
		set Values($f) $vals
		incr ix
	}

	# extra commands
	set Values(commands) [string trim [$ST(EC) get 0.0 end]]
}

proc do_done { } {

	global CList Values

	update_attributes

	set out ""

	foreach f $CList {
		lappend out [list $f $Values($f)]
	}

	foreach f { font commands } {
		lappend out [list $f $Values($f)]
	}

	puts -nonewline $out
	flush stdout

	do_cancel
}

proc mk_main_window { } {

	global ST Values CList Attribs FontSizes

	wm title . "Elvis configuration"

	set wn ""

	set cf "$wn.cf"

	labelframe $cf -text "Color scheme" -padx 4 -pady 4
	pack $cf -side top -expand yes -fill both

	scrolledframe $cf.sf -height 360 -width 360 \
        	-xscrollcommand "$cf.hs set" \
		-yscrollcommand "$cf.vs set" \
		-fill none
    	scrollbar $cf.vs -command "$cf.sf yview"
	scrollbar $cf.hs -command "$cf.sf xview" -orient horizontal
	grid $cf.sf -row 0 -column 0 -sticky nsew
	grid $cf.vs -row 0 -column 1 -sticky ns
	grid $cf.hs -row 1 -column 0 -sticky ew
	grid rowconfigure $cf 0 -weight 1
	grid columnconfigure $cf 0 -weight 1

	set cf "$cf.sf.scrolled"

	#######################################################################

	set rc 0
	label $cf.hfa -text "Face"
	grid $cf.hfa -column 0 -row $rc -sticky w -padx 4
	##
	label $cf.hlt -text "Linked to"
	grid $cf.hlt -column 1 -row $rc -sticky w -padx 4
	##
	label $cf.hco -text "Base color"
	grid $cf.hco -column 2 -row $rc -sticky w -padx 4
	##
	label $cf.hal -text "Alt color"
	grid $cf.hal -column 3 -row $rc -sticky w -padx 4
	##
	label $cf.hbg -text "Bgr color"
	grid $cf.hbg -column 4 -row $rc -sticky w -padx 4
	##
	label $cf.hxb -text "B"
	grid $cf.hxb -column 5 -row $rc -sticky w
	##
	label $cf.hxi -text "I"
	grid $cf.hxi -column 6 -row $rc -sticky w
	##
	label $cf.hxu -text "U"
	grid $cf.hxu -column 7 -row $rc -sticky w
	##
	label $cf.hxo -text "O"
	grid $cf.hxo -column 8 -row $rc -sticky w

	#######################################################################

	set ix 0
	foreach f $CList {

		incr rc
		set vlist $Values($f)

		# element prefix for this row
		set p $cf.e$rc

		set el [label ${p}fa -text $f]
		grid $el -column 0 -row $rc -sticky w -padx 4

		# like
		set val [lindex $vlist 0]

		# selection: CList - current
		set sel "- [join [lreplace $CList $ix $ix]]"

		if { $val == "" } {
			set val "-"
		}
		set ST($ix,LIK) $val
		set el "${p}lt"
		eval "set em \[tk_optionMenu $el ST($ix,LIK) $sel\]"
		grid $el -column 1 -row $rc -sticky we -padx 4

		set ci 1
		foreach cd { co al bg } {

			set val [lindex $vlist $ci]
			# now this becomes the column number
			incr ci

			if { $val == "" } {
				set txt "none"
				set col "#FFFFFF"
			} else {
				set txt ""
				set col $val
			}
			set el "${p}$cd"
			button $el -text $txt -bg $col \
				-command "color_pick $ix $el"
			grid $el -column $ci -row $rc -sticky we -padx 4
		}

		# attributes
		set val [lrange $vlist 4 end]
		foreach cd $Attribs cu { xb xi xu xo } {
			incr ci
			if { [lsearch -exact $val $cd] >= 0 } {
				# present
				set ST($ix,$cd) 1
			} else {
				set ST($ix,$cd) 0
			}
			set el "${p}$cu"
			checkbutton $el -state normal -variable ST($ix,$cd)
			grid $el -column $ci -row $rc -sticky w
		}
		incr ix
	}

	#######################################################################

	set cf "$wn.ff"
	frame $cf
	pack $cf -side top -expand no -fill x

	set lf $cf.lf
	frame $lf
	pack $lf -side left -expand no -fill y

	labelframe $lf.fs -text "Font size" -padx 4 -pady 4
	pack $lf.fs -side top -expand no -fill none

	set el $lf.fs.fo
	set sel [join $FontSizes]
	eval "set em \[tk_optionMenu $el Values(font) $sel\]"
	pack $el -side top -expand no -fill x

	button $lf.cb -text "Cancel" -command do_cancel
	pack $lf.cb -side top -expand no -fill x

	button $lf.db -text "Done" -command do_done
	pack $lf.db -side top -expand no -fill x

	##
	set tf $cf.rf
	labelframe $tf -text "Extra commands" -padx 4 -pady 4
	pack $tf -side left -expand yes -fill both

	set ST(EC) $tf.t

	text $tf.t -font {-family courier -size 9} -state normal \
		-yscrollcommand "$tf.scrolly set" \
		-exportselection yes -width 54 -height 2

	scrollbar $tf.scrolly -command "$tf.t yview"

	pack $tf.t -side left -expand yes -fill both
	pack $tf.scrolly -side left -expand no -fill y

	$ST(EC) insert end $Values(commands)

	bind $ST(EC) <ButtonRelease-1> "tk_textCopy $ST(EC)"
	bind $ST(EC) <ButtonRelease-2> "tk_textPaste $ST(EC)"

	bind . <Destroy> "do_cancel"
}

set er [read_old_conf]

mk_main_window

if { $er != "" } {
	alert "Errors in old config data (items ignored): [join $er ,]"
}

vwait forever
