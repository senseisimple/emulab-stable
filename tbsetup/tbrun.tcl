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

proc readfifo {fp {prefix {}}} {
    while {[gets $fp line] >= 0} {
	outs $prefix$line
    }
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
    set scriptdir [file dirname $scriptdir]/lib
    set bindir [file dirname $scriptdir]/bin
    set execdir [file dirname $scriptdir]/libexec
}

set snmpit "$bindir/snmpit"
set libir "$scriptdir/ir/libir.tcl"
set os_setup "$execdir/os_setup"

source $libir
namespace import TB_LIBIR::ir

if {$argc != 3} {
    puts stderr "Syntax: $argv0 <pid> <eid> <ir-file>"
    exit 1
}

# ignore pid and eid for the moment.
set pid [lindex $argv 0]
set eid [lindex $argv 1]
set irFile [lindex $argv 2]
set t [split $irFile .]
set prefix [join [lrange $t 0 [expr [llength $t] - 2]] .]
set logFile "$prefix.log"

if {[catch "open $logFile a+" logFp]} {
    puts stderr "Could not open $logFile for writing."
    exit 1
}

set tmpio "/tmp/[pid].tmp"
if {[catch "exec touch $tmpio"]} {
    outs stderr "Could not create $tmpio for IO redirection."
    exit 1
}
if {[catch "open $tmpio r" tmpioFP]} {
    outs stderr "Could not open $tmpio for IO redirection. ($tmpio)"
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
if {[catch "exec $snmpit -debug -u -f $irFile >@ $logFp 2> $tmpio" err]} {
    readfifo $tmpioFP "SNMPIT: "
    outs stderr "Error running $snmpit. ($err)"
    exit 1
}
readfifo $tmpioFP "SNMPIT: "

outs "Resetting OS and rebooting."
if {[catch "exec $os_setup $pid $eid $irFile >@ stdout 2>@ $logFp" err]} {
    outs stderr "Error running $os_setup. ($err)"
    exit 1
}

outs "Testbed ready for use."

close $tmpioFP
file delete -force $tmpio
exit 0
