#!/usr/local/bin/tclsh
# Just parses the ir and prints out a basic human readible report.

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
    set updir [file dirname $scriptdir]/lib
} else {
    # install tree
    set updir [file dirname $scriptdir]/lib
    set scriptdir [file dirname $scriptdir]/lib/tbsetup
}


source $scriptdir/ir/libir.tcl
load $updir/sql.so
set DB [sql connect]
sql selectdb $DB tbdb

namespace import TB_LIBIR::ir

if {[llength $argv] == 0 || [llength $argv] > 2} {
    puts stderr "Syntax: $argv0 \[-v\] <irfile>"
    exit 1
}

set verbose 0
if {[llength $argv] == 2} {
    if {[lindex $argv 0] != "-v"} {
	puts stderr "Bad option: [lindex $argv 0]"
	exit 1
    }
    set verbose 1
    set file [lindex $argv 1]
} else {
    set file [lindex $argv 0]
}
ir read $file

if {! [ir exists /virtual/nodes]} {
    puts stderr "IR file incomplete - no /virtual/nodes - run tbprerun"
    exit 1
}
set vnodes [ir get /virtual/nodes]
foreach pair $vnodes {
    set vnodemap([lindex $pair 0]) [lindex $pair 1]
    set rvnodemap([lindex $pair 1]) [lindex $pair 0]
}

if {! [ir exists /ip/map]} {
    puts stderr "IR file incomplete - no /ip/map - run tbprerun"
    exit 1
}
set ips [ir get /ip/map]
foreach line $ips {
    set a [lindex $line 0]
    set b [lindex $line 1]
    set ip [lindex $line 2]
#    if {[info exists ipmap($b:$a)]} {
#	lappend ipmap($b:$a) $ip
#    } else {
	set ipmap($a:$b) $ip
#    }
}

set macs [ir get /ip/mac]
foreach mac $macs {
    set macmap([lindex $mac 1]) [lindex $mac 0]
}

proc get_ip {node} {
    regexp {[0-9]+} $node num
    return "155.99.214.1$num"
}
puts "Nodes"
puts "Virtual Node         Physical Node        IP Address"
puts "-------------------- -------------------- ----------------"
foreach node [lsort [array names vnodemap]] {
    puts [format "%-20s %-20s %-16s" $node $vnodemap($node) \
	      [get_ip $vnodemap($node)]]
}

puts ""
puts "Links"
puts "Source               IP               Destination         IP"
puts "-------------------- --------------- -------------------- ----------------"
foreach line [ir get /virtual/links] {
    set link [lindex $line 0]
    set nodes [lrange $line 1 end]
    set t [split [lindex $nodes 0] -]
    set snode $rvnodemap([lindex $t 0])
    set sifc [lindex $t 1]
    if {[llength $nodes] == 2} {
	set t [lindex $nodes 1]
	set delay {}
    } else {
	set t [lindex $nodes 3]
	set delay [lrange $nodes 1 2]
    }
    set t [split $t -]
    set dnode $rvnodemap([lindex $t 0])
    set difc [lindex $t 1]
    set sip $ipmap($snode:$dnode)
    set dip $ipmap($dnode:$snode)
    
    puts [format "%-20s %-15s %-20s %-15s" $snode $sip $dnode $dip]

    if {$verbose == 1} {
	set smac $macmap($sip)
	set dmac $macmap($dip)
	puts [format "%20s %-15s %20s %-15s" /dev/eth$sifc $smac /dev/eth$difc $dmac]
	if {$delay != {}} {
	    set a [lindex $delay 0]
	    set b [lindex $delay 1]
	    set delaynode [lindex [split $a -] 0]
	    set delayifc1 [lindex [split $a -] 1]
	    set delayifc2 [lindex [split $b -] 1]
	    sql query $DB "select delay,bandwidth,lossrate from delays where node_id=\"$delaynode\" and card0=$delayifc1 and card1=$delayifc2"
	    set delayinfo [sql fetchrow $DB]
	    sql endquery $DB
	    if {$delayinfo == {}} {
		sql query $DB "select delay,bandwidth,lossrate from delays where node_id=\"$delaynode\" and card0=$delayifc2 and card1=$delayifc1"
		set delayinfo [sql fetchrow $DB]
		sql endquery $DB
	    }
	    set delay [lindex $delayinfo 0]
	    set bandwidth [lindex $delayinfo 1]
	    set lossrate [lindex $delayinfo 2]
	    puts [format "   Delay %-10s /dev/eth$delayifc1 /dev/eth$delayifc2 Delay: %4d BW: %5.1f Loss: %.3f" $rvnodemap($delaynode) $delay $bandwidth $lossrate]
	}
    }
}

