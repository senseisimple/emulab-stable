#!/usr/local/bin/tclsh

proc outs {args} {
    global logFp
    if {[llength $args] == 1} {
	set out stdout
	set s [lindex $args 0]
    } else {
	set out [lindex $args 0]
	set s [lindex $args 1]
    }
    
    puts $out $s
    puts $logFp $s
}

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

outs "Input: $nsFile"
outs "Output: $irFile"
outs "Log: $logFile"
outs ""
outs "Beginning Testbed pre run for $nsFile. [clock format [clock seconds]]"

if {! [file exists $nsFile]} {
    outs stderr "$nsFile does not exist"
    exit 1
}

outs "Parsing ns input."
if {[catch "exec $ns2ir $nsFile $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error parsing ns input. ($err)"
    exit 1
}
if {! [file exists $irFile]} {
    outs stderr "$irFile not generated.  Make sure you have a 'run' command in your ns file."
    exit 1
}

outs "PLACEHOLDER - Determining available resources."

outs "Allocating resources - This may take a while."
if {[catch "exec $assign $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error allocating resources. ($err)"
    exit 1
}

outs "PLACEHODLER - Reserving resources."

outs "Setup finished - $irFile generated."


