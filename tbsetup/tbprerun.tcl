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

proc unlock {} {
    global lockfile
    outs "Unlocking the world!"
    if {! [file exists $lockfile]} {
	outs stderr "Error: World already unlocked - DB may be corrupted."
    } else {
	if {[catch "file delete -force $lockfile" err]} {
	    outs stderr "Error unlocking world ($err)"
	}
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
    set scriptdir [file dirname $scriptdir]/lib/tbsetup
}

set lockfile "/usr/testbed/locks/tblock"
set ns2ir "$scriptdir/ns2ir/parse.tcl"
set postparse "$scriptdir/ns2ir/postparse.tcl"
set assign "$scriptdir/ir/assign.tcl"
set handle_ip "$scriptdir/ir/handle_ip.tcl"
set handle_os "$scriptdir/ir/handle_os.tcl"
set avail "$updir/db/avail"
set ptopgen "$updir/db/ptopgen"
set ptopfile "/tmp/testbed[pid].ptop"
set reserve "$updir/db/nalloc"
set libir "$scriptdir/ir/libir.tcl"

source $libir
namespace import TB_LIBIR::ir

if {$argc != 3} {
    puts stderr "Syntax: $argv0 <pid> <eid> <ns-file>"
    exit 1
}

set nsFile [lindex $argv 2]
set t [split $nsFile .]
set pid [lindex $argv 0]
set eid [lindex $argv 1]
set id "$pid-$eid"
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
if {[catch "exec $ns2ir $id $nsFile $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error parsing ns input. ($err)"
    exit 1
}
if {! [file exists $irFile]} {
    outs stderr "$irFile not generated.  Make sure you have a 'run' command in your ns file."
    exit 1
}

outs "Post Parsing ns input."
if {[catch "exec $postparse $nsFile $irFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error post parsing ns input. ($err)"
    exit 1
}

outs "Locking the world!"
set wait {15 30 60 600 600}
set waiti 0
while {[catch "open $lockfile {WRONLY CREAT EXCL}" lockfp]} {
    if {$wait == [llength $waiti]} {
	outs stderr "Giving up on locking.  If no other tbprerun is running then remove $lockfile manually."
	exit 1
    }
    set delay [lindex $wait $waiti]
    incr waiti
    outs "World is locked.  Waiting $delay seconds"
    after [expr $delay * 1000]
}
close $lockfp

outs "Determining available resources."
if {[catch "exec $avail | $ptopgen > $ptopfile 2>@ $logFp" err]} {
    outs stderr "Error determining available resources. ($err)"
    unlock
    exit 1
}

outs "Allocating resources - This may take a while."
if {[catch "exec $assign $irFile $ptopfile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error allocating resources.  See $logFile and assign.log for more info."
    unlock
    exit 1
}

ir read $irFile
set nodemap [ir get /virtual/nodes]
set machines {}
foreach pair $nodemap {
    lappend machines [lindex $pair 1]
}

outs "Reserving resources."
if {[catch "exec $reserve $id $machines >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error reserving resources. ($err)"
    unlock
    exit 1
}

unlock

outs "Allocating IP addresses."
if {[catch "exec $handle_ip $irFile $nsFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error allocating IP addresses. ($err)"
    exit 1
}

outs "Parsing OS information."
if {[catch "exec $handle_os $irFile $nsFile >@ $logFp 2>@ $logFp" err]} {
    outs stderr "Error parsing OS information. ($err)"
    exit 1
}

outs "Setup finished - $irFile generated."


