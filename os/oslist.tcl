#!/usr/local/bin/tclsh

source [file dirname [info script]]/oslib.tcl
namespace import TB_OS::os

if {[llength $argv] > 2} {
    puts stderr "Syntax: $argv0 [-v] \[<base>\]"
    exit 1
}

set verbose 0
if {[lindex $argv 0] == "-v"} {
    set verbose 1
    set argv [lrange $argv 1 end]
}
os init

if {$argv == {}} {
    set bases [os listbases]
    set fmt_str "%-3s %-40s %-10s %-4s %-10s"
    puts [format $fmt_str "Id" "Description" "OS" "Ver" "Extras"]
    foreach base $bases {
	set info [os querybase $base]
	puts [format $fmt_str $base [lindex $info 3] [lindex $info 0] [lindex $info 1] [lindex $info 2]]
	if {$verbose == 1} {
	    puts "  File: [lindex $info 3]"
	}
    }
} else {
    set deltas [os listdeltas $argv]
    set fmt_str "%-10s %-10s %-50s"
    puts [format $fmt_str "Id" "Name" "Description"]
    foreach delta $deltas {
	set info [os querydelta $delta]
	puts [format $fmt_str $delta [lindex $info 0] [lindex $info 1]]
	if {$verbose == 1} {
	    puts "  Files: [lindex $info 2]"
	}
    }
}

os end