#!/usr/local/bin/tclsh

# checkowner.tcl <pid> <eid> <nodes...>
# This script verifies that all nodes in <nodes...> belong to the specified
# pid and eid.  It prints out error messages to stdout and exit's with the
# number of incorrect nodes.

### Bootstrapping code.  The whole purpose of this is to find the
# directory containing the script.
set file [info script]
while {![catch "file readlink $file" newfile]} {
    set file $newfile
}
set scriptdir [file dirname $file]
if {$scriptdir == "."} {set scriptdir [pwd]}
###
set updir [file dirname $scriptdir]
if {[file exists $updir/lib/sql.so]} {
    # dev tree
    load $updir/lib/sql.so
} else {
    # install tree
    load $updir/sql.so
}

set DB [sql connect]
sql selectdb $DB tbdb

if {$argc < 3} {
    puts stderr "Syntax: $argv0 <pid> <eid> <nodes...>"
    exit 1
}

set pid [lindex $argv 0]
set eid [lindex $argv 1]
set errcount 0
foreach node [lrange $argv 2 end] {
    sql query $DB "select pid,eid from reserved where node_id=\"$node\""
    set t [sql fetchrow $DB]
    sql endquery $DB
    if {$t == {}} {
	puts "$node: not reserved"
	incr errcount
    } elseif {[lindex $t 0] != $pid || [lindex $t 1] != $eid} {
	puts "$node: eid: ($eid $pid) != ($t)"
	incr errcount
    }
}

exit $errcount