#!/usr/local/bin/tclsh8.4

set y 24
set yi 18
set filei 0
set fp [open labelsf${filei}.cse w]
while {[gets stdin line] >= 0} {
    set id [lindex $line 0]
    set length [lindex $line 1]
    set type [lindex $line 2]
    puts $fp "text 30 $y -text [format %06d $id] -font {Courier 12}"
    puts $fp "text 78 $y -text [format %03d $length] -font {Courier 12}"
    puts $fp "text 108 $y -text $type -font {Courier 12}"
    if {$id % 56 == 0} {
	set y 24
	close $fp
	incr filei
	set fp [open labelsf${filei}.cse w]
    } else {
	incr y $yi
    }
}
