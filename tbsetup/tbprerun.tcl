#!/usr/local/bin/tclsh

if {[file dirname [info script]] == "."} {
    set updir ".."
} else {
    set updir [file dirname [file dirname [info script]]]
}
set ns2ir "$updir/ir/ns2ir/parse.tcl"
set assign "$updir/ir/assign.tcl"

if {$argc != 1} {
    puts stderr "Syntax: $argv0 <ns-file>"
    exit 1
}

set nsFile [lindex $argv 0]
set t [split $nsFile .]
set prefix [join [lrange $t 0 [expr [llength $t] - 2]] .]
set irFile "$prefix.ir"
set logFile "$prefix.log"

if {[catch "open $logFile w" logFp]} {
    puts stderr "Could not open $logFile for writing."
    exit 1
}

puts "Input: $nsFile"
puts "Output: $irFile"
puts "Log: $logFile"

if {! [file exists $nsFile]} {
    puts stderr "$nsFile does not exist"
    exit 1
}

puts "Parsing ns input."
if {[catch "exec $ns2ir $nsFile $irFile >@ $logFp 2>@ $logFp" err]} {
    puts stderr "Error parsing ns input. ($err)"
    exit 1
}
if {! [file exists $irFile]} {
    puts stderr "$irFile not generated.  Make sure you have a 'run' command in your ns file."
    exit 1
}

puts "PLACEHOLDER - Determining available resources."

puts "Allocating resources - This may take a while."
if {[catch "exec $assign $irFile >@ $logFp 2>@ $logFp" err]} {
    puts stderr "Error allocating resources. ($err)"
    exit 1
}

puts "PLACEHODLER - Reserving resources."

puts "Setup finished - $irFile generated."


