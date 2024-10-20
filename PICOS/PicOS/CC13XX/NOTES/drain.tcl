#!/bin/sh
########\
exec tclsh "$0" "$@"

# bit rate
set brate	50000.0
# Xmt drain uA
set xdrain	20000.0
# Idle drain
set idrain	5.0
# Xmt startup s
set soffset	0.005
# Startup drain
set sdrain	3000.0
# Frame length (bytes): sync + length + SID + NID + MAC
set frame	[expr { 4 + 1 + 2 + 4 + 4 }]
# Packet length (bytes)
set plength	[expr { 2 + 2 + 3 }]
# Packets in ping
set pping	[expr { 8 * 4 }]
# Inter-ping intervals (sec)
set ipint	30.0
# Battery capacity
set bcap	250000.0

###############################################################################

proc ptime { pl } {

	global brate

	return [expr { ($pl * 8.0) / $brate }]
}

proc pingt { } {

	global pping frame plength

	# ping length in bytes
	set pinlen [expr { $pping * ( $plength + $frame ) }]

	return [ptime $pinlen]
}

proc live { cur } {

	global bcap

	set h [expr { $bcap / $cur }]
	set d [expr { $h / 24.0 }]

	puts "Days: $d"
}

set tp [pingt]
set tt [expr { $tp + $soffset }]
# Average ping drain
set apd [expr { ($soffset * $sdrain + $tp * $xdrain) / $tt }]


set itime [expr { $ipint - $tt }]

set curr [expr { ($itime * $idrain + $tt * $apd) / $ipint }]

puts "Curr: $curr"

live $curr
