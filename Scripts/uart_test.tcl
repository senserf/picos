#!/usr/bin/tclsh

set OUAB	0
set EXAB	0

proc bcode { ch } {

	binary scan $ch c res
	# note that res is signed 
	return [expr ($res & 0xff)]
}

proc codeb { ch } {

	return [binary format c [expr $ch & 0xff]] 
}

proc hcode { ch } {

	return [format %02X $ch]
}

proc dumph { msg } {

	set len [string length $msg]

	for { set i 0 } { $i < $len } { incr i } {
		set b [hcode [bcode [string index $msg $i]]]
		puts -nonewline "$b"
	}

	puts ""

	# now for the ASCII dump

	for { set i 0 } { $i < $len } { incr i } {
		set b [expr [bcode [string index $msg $i]] & 0xff]
		if { $b > 0x20 && $b < 0x80 } {
			set b "^[codeb $b]"
		} else {
			set b "^."
		}
		puts -nonewline $b
	}
	puts ""
}

proc inicrc { } {

	global crctab

	set i 0
	foreach val {
    		0x0000  0x1021  0x2042  0x3063  0x4084  0x50a5  0x60c6  0x70e7
    		0x8108  0x9129  0xa14a  0xb16b  0xc18c  0xd1ad  0xe1ce  0xf1ef
    		0x1231  0x0210  0x3273  0x2252  0x52b5  0x4294  0x72f7  0x62d6
    		0x9339  0x8318  0xb37b  0xa35a  0xd3bd  0xc39c  0xf3ff  0xe3de
    		0x2462  0x3443  0x0420  0x1401  0x64e6  0x74c7  0x44a4  0x5485
    		0xa56a  0xb54b  0x8528  0x9509  0xe5ee  0xf5cf  0xc5ac  0xd58d
    		0x3653  0x2672  0x1611  0x0630  0x76d7  0x66f6  0x5695  0x46b4
    		0xb75b  0xa77a  0x9719  0x8738  0xf7df  0xe7fe  0xd79d  0xc7bc
    		0x48c4  0x58e5  0x6886  0x78a7  0x0840  0x1861  0x2802  0x3823
    		0xc9cc  0xd9ed  0xe98e  0xf9af  0x8948  0x9969  0xa90a  0xb92b
    		0x5af5  0x4ad4  0x7ab7  0x6a96  0x1a71  0x0a50  0x3a33  0x2a12
    		0xdbfd  0xcbdc  0xfbbf  0xeb9e  0x9b79  0x8b58  0xbb3b  0xab1a
    		0x6ca6  0x7c87  0x4ce4  0x5cc5  0x2c22  0x3c03  0x0c60  0x1c41
    		0xedae  0xfd8f  0xcdec  0xddcd  0xad2a  0xbd0b  0x8d68  0x9d49
    		0x7e97  0x6eb6  0x5ed5  0x4ef4  0x3e13  0x2e32  0x1e51  0x0e70
    		0xff9f  0xefbe  0xdfdd  0xcffc  0xbf1b  0xaf3a  0x9f59  0x8f78
    		0x9188  0x81a9  0xb1ca  0xa1eb  0xd10c  0xc12d  0xf14e  0xe16f
    		0x1080  0x00a1  0x30c2  0x20e3  0x5004  0x4025  0x7046  0x6067
    		0x83b9  0x9398  0xa3fb  0xb3da  0xc33d  0xd31c  0xe37f  0xf35e
    		0x02b1  0x1290  0x22f3  0x32d2  0x4235  0x5214  0x6277  0x7256
    		0xb5ea  0xa5cb  0x95a8  0x8589  0xf56e  0xe54f  0xd52c  0xc50d
    		0x34e2  0x24c3  0x14a0  0x0481  0x7466  0x6447  0x5424  0x4405
    		0xa7db  0xb7fa  0x8799  0x97b8  0xe75f  0xf77e  0xc71d  0xd73c
    		0x26d3  0x36f2  0x0691  0x16b0  0x6657  0x7676  0x4615  0x5634
    		0xd94c  0xc96d  0xf90e  0xe92f  0x99c8  0x89e9  0xb98a  0xa9ab
    		0x5844  0x4865  0x7806  0x6827  0x18c0  0x08e1  0x3882  0x28a3
    		0xcb7d  0xdb5c  0xeb3f  0xfb1e  0x8bf9  0x9bd8  0xabbb  0xbb9a
    		0x4a75  0x5a54  0x6a37  0x7a16  0x0af1  0x1ad0  0x2ab3  0x3a92
    		0xfd2e  0xed0f  0xdd6c  0xcd4d  0xbdaa  0xad8b  0x9de8  0x8dc9
    		0x7c26  0x6c07  0x5c64  0x4c45  0x3ca2  0x2c83  0x1ce0  0x0cc1
    		0xef1f  0xff3e  0xcf5d  0xdf7c  0xaf9b  0xbfba  0x8fd9  0x9ff8
    		0x6e17  0x7e36  0x4e55  0x5e74  0x2e93  0x3eb2  0x0ed1  0x1ef0
	} {
		set crctab($i) [expr $val]
		incr i
	}
}

proc crc { str } {

	global crctab

	set len [string length $str]
	set chs 0

	while { $str != "" } {
		set b0 [string index $str 0]
		set b1 [string index $str 1]
		set str [string range $str 2 end]
		# this assumes little endian architecture
		set val [expr [bcode $b0] | ( [bcode $b1] << 8 )]
		set chs [expr \
         (($chs << 8) & 0xffff) ^ ($crctab([expr ($chs >> 8) ^ ($val >>   8)]))]
		set chs [expr \
         (($chs << 8) & 0xffff) ^ ($crctab([expr ($chs >> 8) ^ ($val & 0xff)]))]
	}
	return $chs
}

proc openser { n } {

	global UART

	set UART [open COM$n: RDWR]
	fconfigure $UART -mode 9600,n,8,1 -handshake none -translation binary \
		-blocking 0
}

proc rdb { sec } {

	global UART

	set tim [expr [clock seconds] + $sec + 1]

	while 1 {
		while { [catch { read $UART 1 } by] } {
			if { [clock seconds] >= $tim } {
				return ""
			}
			after 50
		}
		if { $by != "" } {
			return $by
		}
		after 50
		if { [clock seconds] >= $tim } {
			return ""
		}
	}
}

proc snb { b } {

	global UART
	puts -nonewline $UART $b
	flush $UART
}

proc send { msg } {

	global OUAB EXAB UART

	fconfigure $UART -blocking 1

	set len [string length $msg]
	if { $len == 0 } {
		# this is an ACK
		set b 1
	} else {
		set b $OUAB
	}
	set b [expr $b | ($EXAB << 1)]

	set out [binary format cc $b $len]
	append out $msg
	if [expr ($len & 1)] {
		append out "x"
	}
	set b [crc $out]
	append out [binary format cc [expr $b & 0xff] [expr $b >> 8]]
	set len [string length $out]
	puts "SENDING:"
	dumph $out
	for { set i 0 } { $i < $len } { incr i } {
		snb [string index $out $i]
	}

	fconfigure $UART -blocking 0
}

proc tmout { mes } {

	puts "TIMEOUT:"
	if { $mes != "" } {
		dumph $mes
	} else {
		puts "NO MESSAGE"
	}
}
	
proc rcv { sec ret } {

	global OUAB EXAB

	set mes ""

	if { $ret < 2 } {
		set ret 2
	}
	if { $sec < 1 } {
		set sec 1
	}

	set tim [expr [clock seconds] + $ret]

	while 1 {

		if { [clock seconds] >= $tim } {
			tmout $mes
			return ""
		}
		
		set b [rdb $sec]
		if { $b == "" } {
			tmout $mes
			return ""
		}
		set rmes $b
		set b [bcode $b]
		set MB [expr $b & 1]
		set AB [expr ($b >> 1) & 1]
		if { [expr $b & 0xfc] != 0 } {
			continue
		}
		set b [rdb $sec]
		if { $b == "" } {
			tmout $mes
			return ""
		}
		append rmes $b
		set len [bcode $b]
		if { $len > 128 } {
			continue
		}
		set mes ""
		set ext [expr $len & 1]
		set bcn $len
		while { $bcn > 0 } {
			set b [rdb $sec]
			if { $b == "" } {
				tmout $mes
				return ""
			}
			append rmes $b
			append mes $b
			incr bcn -1
		}
		if $ext {
			set b [rdb $sec]
			if { $b == "" } {
				tmout $mes
				return ""
			}
			append rmes $b
			append mes $b
		}
		set b [rdb $sec]
		if { $b == "" } {
			tmout $mes
			return ""
		}
		append rmes $b
		set b [rdb $sec]
		if { $b == "" } {
			tmout $mes
			return ""
		}
		append rmes $b
		break
	}

	#puts "RECEIVED:"
	#dumph $rmes
	#dumph $mes

	# examine the checksum
	set c [crc $rmes]
	if { $c != 0 } {
		puts "BAD CRC: $c"
		return ""
	}
	puts -nonewline "CRC OK, "

	# process the header
	if { $AB != $OUAB } {
		puts -nonewline "ACKN: $OUAB -> $AB"
		set OUAB $AB
	} else {
		puts -nonewline "ACKR: $OUAB == $AB"
	}

	if { $len != 0 } {
		# this is a message
		if { $MB == $EXAB } {
			set EXAB [expr 1 - $EXAB]
			puts ", MSGN: $MB -> $EXAB"
			flush stdout
			return $mes
		} else {
			puts ", MSGR: $MB != $EXAB"
		}
	} else {
		puts ""
	}
	flush stdout
	return ""
}

proc dump { len sec } {

	if { $sec < 1 } {
		set sec 1
	}

	set fill 0
	while 1 {

		set b [rdb $sec]
		if { $b == "" } {
			set b " ====="
		} else {
			set c [expr [bcode $b] & 0xff]
			set b " [hcode $c]<"
			if { $c > 0x20 && $c < 0x80 } {
				append b [codeb $c]
			} else {
				append b "."
			}
			append b ">"
		}

		if { $fill == 8 } {
			puts ""
			set fill 0
		}

		puts -nonewline $b
		incr fill
		flush stdout
	
		if { $len > 0 } {
			incr len -1
			if { $len == 0 } {
				break
			}
		}
	}
	puts "\nEND"
}

proc badcmd { } {

	puts "bad command"
}

proc getline { } {

	return [gets stdin]
}

proc testloop { n } {

	global OUAB EXAB

	set OUAB 0
	set EXAB 0
	set chrs "abcdefghijklmnopqrstuvwxyz01234567890"

	if { $n < 1 } {
		set n 1
	}

	set k 0
	set l [string length $chrs]
	while 1 {
		set msg ""
		for { set i 0 } { $i < $n } { incr i } {
			append msg [string index $chrs [expr ($i + $k) % $l]]
		}

		set cab $OUAB
		while 1 {
			if { $cab != $OUAB } {
				break
			}
			after 500
			send $msg
			for { set j 0 } { $j < 10 } { incr j } {
				set rmsg [rcv 2 10]
				if { $rmsg != "" } {
					break
				}
			}
			puts "RCVSUCCESS::::"
			puts $rmsg
			send ""
		}
		puts "ACKNOWLEDGED!!!!!!!!"
		after 1000
		incr k
	}
}
		
proc main { } {

	global OUAB EXAB

	inicrc
	openser 7

	while 1 {
		set line [getline]
		set cmd [string tolower [string index $line 0]]
		set line [string trim [string range $line 1 end]]

		if { $cmd == "b" } {
			if ![regexp "^(\[01\]) *(\[01\]) *$" $line junk m a] {
				puts "OUAB $OUAB, EXAB $EXAB"
				continue
			}
			set OUAB $m
			set EXAB $a
			continue
		}

		if { $cmd == "s" } {
			send $line
			continue
		}

		if { $cmd == "r" } {
			set ret 10
			set sec 1
			if { [regexp "^\[0-9\]+" $line val] && \
			    ![catch { expr $val } val] } {
				set ret $val
				if { [regexp " (\[0-9\]+)" $line junk val] && \
			    	    ![catch { expr $val } val] } {
					set sec $val
				}
			}
			rcv $sec $ret
			continue
		}

		if { $cmd == "d" } {
			set sec 2
			set ret 256
			if { [regexp "^\[0-9\]+" $line val] && \
			    ![catch { expr $val } val] } {
				set ret $val
				if { [regexp " (\[0-9\]+)" $line junk val] && \
			    	    ![catch { expr $val } val] } {
					set sec $val
				}
			}
			dump $ret $sec
			continue
		}

		if { $cmd == "t" } {
			testloop 24
		}
		badcmd
	}
}

main
