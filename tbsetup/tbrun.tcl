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
set snmpit "$updir/switch_tools/intel510/snmpit"

if {$argc != 1} {
    puts stderr "Syntax: $argv0 <ir-file>"
    exit 1
}

set irFile [lindex $argv 0]
set t [split $irFile .]
set prefix [join [lrange $t 0 [expr [llength $t] - 2]] .]
set logFile "$prefix.log"

if {[catch "open $logFile a+" logFp]} {
    puts stderr "Could not open $logFile for writing."
    exit 1
}

outs "Input: $irFile"
outs "Log: $logFile"

if {! [file exists $irFile]} {
    outs stderr "$irFile does not exist"
    exit 1
}

outs "Beginning Testbed run for $irFile. [clock format [clock seconds]]"

outs "Setting up VLANs"

if {[catch "exec $snmpit -f $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error running $snmpit. ($err)"
    exit 1
}

outs "PLACEHOLDER - Verifying virtual network."
outs "PLACEHOLDER - Copying disk images."
outs "PLACEHOLDER - Booting for the first time."
outs "PLACEHOLDER - Verifyin OS functionality."
outs "PLACEHOLDER - Setting up interfaces."
outs "PLACEHOLDER - Installing secondary pacakages."
outs "PLACEHOLDER - Rebooting."
outs "Testbed ready for use."

