#!/usr/local/bin/tclsh

source [file dirname [info script]]/oslib.tcl
namespace import TB_OS::os

if {[llength $argv] != 1} {
    puts stderr "Syntax: $argv0 <node>"
    exit 1
}

os init

set dbstate [os querydb $argv]
if {[catch "os querynode $argv" nodestate]} {
    puts stderr "Error querying node: $argv"
    exit -1
}

set ret 0
if {[lindex $dbstate 0] != [lindex $nodestate 0]} {
    puts "OS mismatch: DB: [lindex $dbstate 0] Node: [lindex $nodestate 0]"
    set ret 1
}

foreach delta [lindex $nodestate 1] {
    set nodedelta($delta) 1
}
foreach delta [lindex $dbstate 1] {
    if {! [info exists nodedelta($delta)]} {
	puts "Node missing delta: $delta"
	set ret 1
    } else {
	unset nodedelta($delta)
    }
}
foreach delta [array names nodedelta] {
    puts "Node has extra delta: $delta"
    set ret 1
}

os end

exit $ret

