#!/usr/local/bin/tclsh

# This takes a list of machine names and removes all VLANs containing
# only a subset of those machines.

set snmpit [file dirname [info script]]/snmpit

if {[catch "open \"|$snmpit -l\" r" vlanFp]} {
    puts stderr "Error running $snmpit -l ($vlanFp)"
    exit 1
}

foreach arg $argv {
    set machines($arg) {}
}

set toremove {}

while {[gets $vlanFp line] >= 0} {
    set id [lindex $line 0]
    if {! [regexp {[0-9]+} $id]} {continue}
    if {[lindex $line 1] == "Control"} {continue}
    if {[lindex $line 1] == "System"} {continue}
    set remove 1
    foreach member [lrange $line 2 end] {
	set mname [lindex [split $member :] 0]
	if {! [info exists machines($mname)]} {
	    set remove 0
	    break
	}
    }
    if {$remove == 1} {lappend toremove $id}
}

puts "Removing VLANs: $toremove"

if {$toremove != {}} {
    if {[catch "exec $snmpit -debug -u -r $toremove >@ stdout 2>@ stderr" err]} {
	puts stderr "Error running $snmpit -u -r ($err)"
	exit 1
    }
}

puts "VLANs removed."
