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
set handle_ip "$updir/ir/handle_ip.tcl"
set avail "$updir/db/avail"
set ptopgen "$updir/db/ptopgen"
set ptopfile "$updir/ir/testbed.ptop"
set reserve "$updir/db/nalloc"
set libir "$updir/ir/libir.tcl"

source $libir
namespace import TB_LIBIR::ir

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

outs "Determining available resources."
if {[catch "exec $avail type=pc ver extras | $ptopgen > $ptopfile 2>@ $logFp" err]} {
    outs stderr "Error determining available resources. ($err)"
    exit 1
}

outs "Allocating resources - This may take a while."
if {[catch "exec $assign $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error allocating resources. ($err)"
    exit 1
}

ir read $irFile
set nodemap [ir get /virtual/nodes]
set machines {}
foreach pair $nodemap {
    lappend machines [lindex $pair 1]
}

outs "Reserving resources."
if {[catch "exec $reserve $prefix $machines >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error reserving resources. ($err)"
    exit 1
}

outs "Allocating IP addresses."
if {[catch "exec $handle_ip $irFile $nsFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error allocating IP addresses. ($err)"
    exit 1
}

outs "Setup finished - $irFile generated."


