#!/usr/local/bin/tclsh

if {[file dirname [info script]] == "."} {
    set updir ".."
} else {
    set updir [file dirname [file dirname [info script]]]
}
set snmpit "$updir/switch_tools/intel510/snmpit"

if {$argc != 1} {
    puts stderr "Syntax: $argv0 <ir-file>"
    exit 1
}

set irFile [lindex $argv 0]
set t [split $nsFile .]
set prefix [join [lrange $t 0 [expr [llength $t] - 2]] .]
set logFile "$prefix.log"

if {[catch "open $logFile w" logFp]} {
    puts stderr "Could not open $logFile for writing."
    exit 1
}

puts "Input: $irFile"
puts "Log: $logFile"

if {! [file exists $irFile]} {
    puts stderr "$irFile does not exist"
    exit 1
}

puts "Setting up VLANs"

if {[catch "exec $snmpit $irFile >@ $logFp 2>@ $logFp" err]} {
    puts stderr "Error running $smpit. ($err)"
    exit 1
}

puts "PLACEHOLDER - Verifying virtual network."
puts "PLACEHOLDER - Copying disk images."
puts "PLACEHOLDER - Booting for the first time."
puts "PLACEHOLDER - Verifyin OS functionality."
puts "PLACEHOLDER - Setting up interfaces."
puts "PLACEHOLDER - Installing secondary pacakages."
puts "PLACEHOLDER - Rebooting."
puts "Testbed ready for use."

