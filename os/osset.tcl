#!/usr/local/bin/tclsh

source [file dirname [info script]]/oslib.tcl
namespace import TB_OS::os

if {[llength $argv] < 2} {
    puts stderr "Syntax: $arg0 <node> <base> <deltas...>"
    exit 1
}

os init

set db [os db]

set node [lindex $argv 0]
set base [lindex $argv 1]
set deltas [lrange $argv 2 end]

# Check arguments
if {[os querydb $node] == {}} {
    puts stderr "Could not find node: $node"
    exit 1
}
if {[os querybase $base] == {}} {
    puts stderr "Unknown base: $base"
    exit 1
}
foreach delta $deltas {
    if {[os querydelta $delta] == {}} {
	puts stderr "Unknown delta: $delta"
	exit 1
    }
}

# Set base
sql exec $db "update SW_table set image_id = \"$base\" where node_id = \"$node\""

# Clear deltas
sql exec $db "delete from fixed_list where node_id = \"$node\""

# Add new deltas
foreach delta $deltas {
    sql exec $db "insert into fixed_list values ( \"$node\", \"$delta\" )"
}

os end

