#!/usr/local/bin/tclsh

source [file dirname [info script]]/oslib.tcl
namespace import TB_OS::os

if {[llength $argv] == 0 || [llength $argv] > 2} {
    puts stderr "Syntax: $argv0 \[-v\[v\]\] <node>"
    exit 1
}

os init

set verbose 0
if {[llength $argv] == 2} {
    switch -- [lindex $argv 0] {
	"-v" {set verbose 1}
	"-vv" {set verbose 2}
	default {
	    puts stderr "Unrecognized option: [lindex $argv 0]"
	    exit 1
	}
    }
    set node [lindex $argv 1]
} else {
    set node [lindex $argv 0]
}

if {[catch "os querynode $node" state]} {
    puts stderr "ERROR querying node: $state"
    exit 1
}
set bstate [os querybase [lindex $state 0]]
puts "base: [lindex $state 0] ([lindex $bstate 3])"
if {$verbose > 0} {
    puts "os: [lindex $bstate 0]"
    puts "ver: [lindex $bstate 1]"
    puts "extras: [lindex $bstate 2]"
    puts ""
}
puts "deltas: [lindex $state 1]"

if {$verbose > 0} {
    foreach delta [lindex $state 1] {
	set info [os querydelta $delta]
	puts ""
	puts "delta: $delta"
	puts "name: [lindex $info 0]"
	puts "desc: [lindex $info 1]"
	if {$verbose > 1} {
	    puts "rpms: [lindex $info 2]"
	}
    }
}

os end

