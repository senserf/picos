package provide tooltips 1.0

###############################################################################
# TOOLTIPS ####################################################################
###############################################################################

namespace eval TOOLTIPS {

variable ttps

proc tip_init { { fo "" } { wi "" } { bg "" } { fg "" } } {
#
# Font, wrap length, i.e., width (in pixels)
#
	variable ttps

	if { $fo == "" } {
		set fo "TkSmallCaptionFont"
	}

	if { $wi == "" } {
		set wi 320
	}

	if { $bg == "" } {
		set bg "lightyellow"
	}

	if { $fg == "" } {
		set fg "black"
	}

	set ttps(FO) $fo
	set ttps(WI) $wi
	set ttps(BG) $bg
	set ttps(FG) $fg

}

proc tip_set { w t } {

	bind $w <Any-Enter> [list after 200 [list tip_show %W $t]]
	bind $w <Any-Leave> [list after 500 [list destroy %W.ttip]]
	bind $w <Any-KeyPress> [list after 500 [list destroy %W.ttip]]
	bind $w <Any-Button> [list after 500 [list destroy %W.ttip]]
}

proc tip_show { w t } {

	global tcl_platform
	variable ttps

	set px [winfo pointerx .]
	set py [winfo pointery .]

	if { [string match $w* [winfo containing $px $py]] == 0 } {
                return
        }

	catch { destroy $w.ttip }

	set scrh [winfo screenheight $w]
	set scrw [winfo screenwidth $w]

	set tip [toplevel $w.ttip -bd 1 -bg black]

	wm geometry $tip +$scrh+$scrw
	wm overrideredirect $tip 1

	if { $tcl_platform(platform) == "windows" } {
		wm attributes $tip -topmost 1
	}

	pack [label $tip.label -bg $ttps(BG) -fg $ttps(FG) -text $t \
		-justify left -wraplength $ttps(WI) -font $ttps(FO)]

	set wi [winfo reqwidth $tip.label]
	set hi [winfo reqheight $tip.label]

	set xx [expr $px - round($wi / 2.0)]

	if { $py > [expr $scrh / 2.0] } {
		# lower half
		set yy [expr $py - $hi - 10]
	} else {
		set yy [expr $py + 10]
	}

	if  { [expr $xx + $wi] > $scrw } {
		set xx [expr $scrw - $wi]
	} elseif { $xx < 0 } {
		set xx 0
	}

	wm geometry $tip [join "$wi x $hi + $xx + $yy" {}]

        raise $tip

        bind $w.ttip <Any-Enter> { destroy %W }
        bind $w.ttip <Any-Leave> { destroy %W }
}

namespace export tip_*

}

namespace import ::TOOLTIPS::*
