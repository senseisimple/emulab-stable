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

### Bootstrapping code.  The whole purpose of this is to find the
# directory containing the script.
set file [info script]
while {![catch "file readlink $file" newfile]} {
    set file $newfile
}
set scriptdir [file dirname $file]
if {$scriptdir == "."} {set scriptdir [pwd]}
###
if {[file exists $scriptdir/ns2ir]} {
    # development tree
    set updir [file dirname $scriptdir]
} else {
    # install tree
    set updir [file dirname $scriptdir]/lib
    set scriptdir [file dirname $scriptdir]/lib/tbsetup
}

set nfree "$updir/db/nfree"
set libir "$scriptdir/ir/libir.tcl"
set resetvlans "$scriptdir/resetvlans.tcl"

source $libir
namespace import TB_LIBIR::ir

if {$argc != 1} {
    puts stderr "Syntax: $argv0 <ir-file>"
    exit 1
}

set nsFile [lindex $argv 0]
set t [split $nsFile .]
set prefix [join [lrange $t 0 [expr [llength $t] - 2]] .]
set irFile "$prefix.ir"
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

outs ""
outs "Ending Testbed run for $irFile. [clock format [clock seconds]]"

ir read $irFile

outs "Unallocating resources"

set nodemap [ir get /virtual/nodes]
set machines {}
foreach pair $nodemap {
    lappend machines [lindex $pair 1]
}

if {[catch "exec $nfree $prefix $machines >@ $logFp 2>@ $logFp err"]} {
    outs stderr "Error freeing resources. ($err)"
    exit 1
}

outs "Resetting VLANs"
if {[catch "exec $resetvlans $machines >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error reseting vlans ($err)"
    exit 1
}
