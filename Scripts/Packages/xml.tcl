package provide xml 1.0
###############################################################################
# Mini XML parser. Copyright (C) 2008-12 Olsonet Communications Corporation.
###############################################################################

### Last modified PG111008A ###

namespace eval XML {

proc xstring { s } {
#
# Extract a possibly quoted string
#
	upvar $s str

	if { [xspace str] != "" } {
		error "illegal white space"
	}

	set c [string index $str 0]
	if { $c == "" } {
		error "empty string illegal"
	}

	if { $c != "\"" } {
		# no quote; this is formally illegal in XML, but let's be
		# pragmatic
		regexp "^\[^ \t\n\r\>\]+" $str val
		set str [string range $str [string length $val] end]
		return [xunesc $val]
	}

	# the tricky way
	if ![regexp "^.(\[^\"\]*)\"" $str match val] {
		error "missing \" in string"
	}
	set str [string range $str [string length $match] end]

	return [xunesc $val]
}

proc xunesc { str } {
#
# Remove escapes from text
#
	regsub -all "&amp;" $str "\\&" str
	regsub -all "&quot;" $str "\"" str
	regsub -all "&lt;" $str "<" str
	regsub -all "&gt;" $str ">" str
	regsub -all "&nbsp;" $str " " str

	return $str
}

proc xspace { s } {
#
# Skip white space
#
	upvar $s str

	if [regexp -indices "^\[ \t\r\n\]+" $str ix] {
		set ix [lindex $ix 1]
		set match [string range $str 0 $ix]
		set str [string range $str [expr $ix + 1] end]
		return $match
	}

	return ""
}

proc xcmnt { s } {
#
# Skip a comment
#
	upvar $s str

	set sav $str

	set str [string range $str 4 end]
	set cnt 1

	while 1 {
		set ix [string first "-->" $str]
		set iy [string first "<!--" $str]
		if { $ix < 0 } {
			error "unterminated comment: [string range $sav 0 15]"
		}
		if { $iy > 0 && $iy < $ix } {
			incr cnt
			set str [string range $str [expr $iy + 4] end]
		} else {
			set str [string range $str [expr $ix + 3] end]
			incr cnt -1
			if { $cnt == 0 } {
				return
			}
		}
	}
}

proc xftag { s } {
#
# Find and extract the first tag in the string
#
	upvar $s str

	set front ""

	while 1 {
		# locate the first tag
		set ix [string first "<" $str]
		if { $ix < 0 } {
			set str "$front$str"
			return ""
		}
		append front [string range $str 0 [expr $ix - 1]]
		set str [string range $str $ix end]
		# check for a comment
		if { [string range $str 0 3] == "<!--" } {
			# skip the comment
			xcmnt str
			continue
		}
		set et ""
		if [regexp -nocase "^<(/)?\[a-z:_\]" $str ix et] {
			# this is a tag
			break
		}
		# skip the thing and keep going
		append front "<"
		set str [string range $str 1 end]
	}

	if { $et != "" } {
		set tm 1
	} else {
		set tm 0
	}

	if { $et != "" } {
		# terminator, skip the '/', so the text is positioned at the
		# beginning of keyword
		set ix 2
	} else {
		set ix 1
	}

	# starting at the keyword
	set str [string range $str $ix end]

	if ![regexp -nocase "^(\[a-z0-9:_\]+)(.*)" $str ix kwd str] {
		# error
		error "illegal tag: [string range $str 0 15]"
	}

	set kwd [string tolower $kwd]

	# decode the attributes
	set attr ""
	array unset atts

	while 1 {
		xspace str
		if { $str == "" } {
			error "unterminated tag: <$et$kwd"
		}
		set c [string index $str 0]
		if { $c == "/" } {
			# self-terminating
			if { $tm != 0 || [string index $str 1] != ">" } {
				error "broken self-terminating tag:\
					<$et$kwd ... [string range $str 0 15]"
			}
			set str [string range $str 2 end]
			return [list 2 $front $kwd $attr]
		}
		if { $c == ">" } {
			# done
			set str [string range $str 1 end]
			# term preceding_text keyword attributes
			return [list $tm $front $kwd $attr]
		}
		# this must be a keyword
		if ![regexp -nocase "^(\[a-z\]\[a-z0-9_\]*)=" $str match atr] {
			error "illegal attribute: <$et$kwd ... [string range \
				$str 0 15]"
		}
		set atr [string tolower $atr]
		if [info exists atts($attr)] {
			error "duplicate attribute: <$et$kwd ... $atr"
		}
		set atts($atr) ""
		set str [string range $str [string length $match] end]
		if [catch { xstring str } val] {
			error "illegal attribute value: \
				<$et$kwd ... $atr=[string range $str 0 15]"
		}
		lappend attr [list $atr $val]
	}
}

proc xadv { s kwd } {
#
# Returns the text + the list of children for the current tag. A child looks
# like this:
#
#	text:		<"" the_text>
#	element:	<tag attributes children_list>
#
	upvar $s str

	set chd ""

	while 1 {
		# locate the nearest tag
		set tag [xftag str]
		if { $tag == "" } {
			# no more
			if { $kwd != "" } {
				error "unterminated tag: <$kwd ...>"
			}

			if { $str != "" } {
				# a tailing text item
				lappend chd [list "" $str]
				return $chd
			}
		}

		set md [lindex $tag 0]
		set fr [lindex $tag 1]
		set kw [lindex $tag 2]
		set at [lindex $tag 3]

		if { $fr != "" } {
			# append a text item
			lappend chd [list "" $fr]
		}

		if { $md == 0 } {
			# opening, not self-closing
			set cl [xadv str $kw]
			# inclusion ?
			set tc [list $kw $at $cl]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} elseif { $md == 2 } {
			# opening, self-closing
			set tc [list $kw $at ""]
			if ![xincl str $tc] {
				lappend chd $tc
			}
		} else {
			# closing
			if { $kw != $kwd } {
				error "mismatched tag: <$kwd ...> </$kw>"
			}
			# we are done with the tag
			return $chd
		}
	}
}

proc xincl { s tag } {
#
# Process an include tag
#
	set kw [lindex $tag 0]

	if { $kw != "include" && $kw != "xi:include" } {
		return 0
	}

	set fn [sxml_attr $tag "href"]

	if { $fn == "" } {
		error "href attribute of <$kw ...> is empty"
	}

	if [catch { open $fn "r" } fd] {
		error "cannot open include file $fn: $fd"
	}

	if [catch { read $fd } fi] {
		catch { close $fd }
		error "cannot read include file $fn: $fi"
	}

	# merge it
	upvar $s str

	set str $fi$str

	return 1
}

proc sxml_parse { s } {
#
# Builds the XML tree from the provided string
#
	upvar $s str

	set v [xadv str ""]

	return [list root "" $v]
}

proc sxml_name { s } {

	return [lindex $s 0]
}

proc sxml_txt { s } {

	set txt ""

	foreach t [lindex $s 2] {
		if { [lindex $t 0] == "" } {
			append txt [lindex $t 1]
		}
	}

	return $txt
}

proc sxml_snippet { s } {

	if { [lindex $s 0] != "" } {
		return ""
	}

	return [lindex $s 1]
}

proc sxml_attr { s n { e "" } } {

	if { $e != "" } {
		# flag to tell the difference between an empty attribute and
		# its complete lack
		upvar $e ef
		set ef 0
	}

	if { [lindex $s 0] == "" } {
		# this is a text
		return ""
	}

	set al [lindex $s 1]
	set n [string tolower $n]
	foreach a $al {
		if { [lindex $a 0] == $n } {
			if { $e != "" } {
				set ef 1
			}
			return [lindex $a 1]
		}
	}
	return ""
}

proc sxml_children { s { n "" } } {

	# this is automatically null for a text
	set cl [lindex $s 2]

	if { $n == "+" } {
		# all including text
		return $cl
	}

	set res ""

	if { $n == "" } {
		# tagged elements only
		foreach c $cl {
			if { [lindex $c 0] != "" } {
				lappend res $c
			}
		}
		return $res
	} else {
		# all with the given tag name
		foreach c $cl {
			if { [lindex $c 0] == $n } {
				lappend res $c
			}
		}
	}

	return $res
}

proc sxml_child { s n } {

	# null for a text
	set cl [lindex $s 2]

	foreach c $cl {
		if { [lindex $c 0] == $n } {
			return $c
		}
	}

	return ""
}

proc sxml_fctl { s } {
#
# A secret (public) function: first character, lower case
#
	return [string tolower [string index $s 0]]
}

proc sxml_yes { item attr } {
#
# A useful shortcut
#
	if { [sxml_fctl [sxml_attr $item $attr]] == "y" } {
			return 1
	}
	return 0
}

namespace export sxml_*

### end of XML namespace ######################################################

}

namespace import ::XML::*
