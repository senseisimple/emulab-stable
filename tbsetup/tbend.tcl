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

load $updir/lib/sql.so

if {$argc != 3 && $argc != 2} {
    puts stderr "Syntax: $argv0 <pid> <eid>"
    exit 1
}
if {$argc == 3} {
    puts stderr "Warning: Ignoring IR file argument.
}

set DB [sql connect]
sql selectdb $DB tbdb

set pid [lindex $argv 0]
set eid [lindex $argv 1]
set id "$pid-$eid"
set logFile "${id}_end.log"

if {[catch "open $logFile a+" logFp]} {
    puts stderr "Could not open $logFile for writing."
    exit 1
}

outs "Log: $logFile"

outs ""
outs "Ending Testbed run for $id. [clock format [clock seconds]]"

outs "Unallocating resources"

sql query $DB "select node_id from reserved where eid=\"$eid\" and pid=\"$pid\""
while {[set machine [sql fetchrow $DB]] != {}} {
    lappend machines $machine
}

if {[catch "exec $nfree $pid $eid >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error freeing resources. ($err)"
    exit 1
}

outs "Resetting VLANs"
if {[catch "exec $resetvlans $machines >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error reseting vlans ($err)"
    exit 1
}
