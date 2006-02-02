#!/usr/bin/tclsh
################

proc main { } {

	if [catch { open "Image" "r" } fd] {
		puts "Cannot open 'Image': $fd"
		exit 99
	}

	fconfigure $fd -translation binary
	fconfigure stdout -buffering none
	fconfigure stderr -buffering none

	set BIN [read -nonewline $fd]

	close $fd

	while 1 {

		puts -nonewline\
			"\nEnter default ESN (hex) to locate it in the image: "

		set line ""
		set st [gets stdin line]
		if { $st < 0 } {
			exit 0
		}

		regsub -all "\[ \t\]+" $line "" line
		set line [string tolower $line]
		if { [string first "0x" $line] != 0 } {
			set line "0x$line"
		}
		if [catch { expr $line } val] {
			puts stderr "Illegal hex number: '$line'"
			continue
		}

		# locate the value in the Image (little endian)
		set val [binary format i $line]

		set st [string first $val $BIN]
		if { $st < 0 } {
			puts stderr "ESN '$line' not found in the image"
			continue
		}

		set BEFORE [string range $BIN 0 [expr $st - 1]]
		set AFTER  [string range $BIN [expr $st + 4] end]

		# check if it occurs only once
		if { [string first $val $AFTER] >= 0 } {
			puts stderr "ESN '$line' occurs more than once in image"
			continue
		}

		# OK, move to the second stage
		break
	}

	unset BIN

	while 1 { 
		puts -nonewline "Enter new ESN, 'q' to quit: "
		set line ""
		set st [gets stdin line]
		if { $st < 0 } {
			exit 0
		}
		regsub -all "\[ \t\]+" $line "" line
		set line [string tolower $line]
		if { [string index $line 0] == "q" } {
			exit 0
		}
		if { [string first "0x" $line] != 0 } {
			set line "0x$line"
		}
		if [catch { expr $line } val] {
			puts stderr "Illegal hex number: '$line'"
			continue
		}

		if [catch { open "/tmp/an43copy__pg" "w" } fd] {
			puts stderr "Cannot open temporary file: $fd"
			exit 99
		}

		fconfigure $fd -translation binary

		puts -nonewline $fd $BEFORE
		puts -nonewline $fd [binary format i $line]
		puts -nonewline $fd $AFTER

		close $fd

		if [catch {
			exec msp430-objcopy -O ihex /tmp/an43copy__pg Image.a43
		} fd] {
			puts stderr "Conversion failed: $fd"
			exit 99
		}
		puts "New image written to Image.a43"

		file delete -force "/tmp/an43copy__pg"
	}
}

main
