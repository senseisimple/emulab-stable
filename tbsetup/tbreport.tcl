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
    set updir [file dirname $scriptdir]
} else {
    # install tree
    set updir [file dirname $scriptdir]/lib
    set scriptdir [file dirname $scriptdir]/lib/tbsetup
}


source $scriptdir/ir/libir.tcl
load $updir/lib/sql.so
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
    if {[info exists ipmap($b:$a)]} {
	lappend ipmap($b:$a) $ip
    } else {
	set ipmap($a:$b) $ip
    }
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
foreach link [array names ipmap] {
    set t [split $link :]
    puts [format "%-20s %-15s %-20s %-15s" [lindex $t 0] \
	      [lindex $ipmap($link) 0] \
	      [lindex $t 1] [lindex $ipmap($link) 1]]
    if {$verbose == 1} {
	set smac $macmap([lindex $ipmap($link) 0])
	set dmac $macmap([lindex $ipmap($link) 1])
	sql query $DB "select IFC from interfaces where mac = \"$smac\""
	set sifc [sql fetchrow $DB]
	sql endquery $DB
	sql query $DB "select IFC from interfaces where mac = \"$dmac\""
	set difc [sql fetchrow $DB]
	sql endquery $DB
	puts [format "%20s %-15s %20s %-15s" /dev/eth$sifc $smac /dev/eth$difc $dmac]
    }
}

