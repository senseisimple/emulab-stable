#!/usr/local/bin/tclsh

set sdir [file dirname [info script]]
if {[file dirname [info script]] == "."} {
    load ../lib/sql.so
} else {
    load [file dirname [file dirname [info script]]]/lib/sql.so
}

if {[llength $argv] != 1} {
    puts stderr "Syntax: $argv0 <node#>"
    exit 1
}
set node [lindex $argv 0]

set DB [sql connect]
sql selectdb $DB tbdb

sql query $DB "select * from ssh_host_keys where node_id=\"tbpc$node\""
set key [sql fetchrow $DB]
if {$key != ""} {
    puts [exec echo [lindex $key 1] | $sdir/key8]
}

