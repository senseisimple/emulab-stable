#!/usr/local/bin/tclsh

source [file dirname [info script]]/oslib.tcl
namespace import TB_OS::os

if {[llength $argv] == 0 || [llength $argv] > 2} {
    puts stderr "Syntax: $argv0 <node>"
    exit 1
}

os init

if {[catch "os querynode $argv" state]} {
    puts stderr "ERROR querying node: $state"
    exit 1
}
set db [os db]

# Set base
sql exec $db "update SW_table set image_id = \"[lindex $state 0]\" where node_id = \"$argv\""

# Clear deltas
sql exec $db "delete from fixed_list where node_id = \"$argv\""

# Add new deltas
foreach delta [lindex $state 1] {
    sql exec $db "insert into fixed_list values ( \"$argv\", \"$delta\" )"
}

os end

