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

if {[catch "exec ssh -n -l root 155.99.214.1$node cat /etc/ssh/ssh_host_key | $sdir/key7" hostkey]} {
    puts stderr "ERROR: Could not get hostkey ($hostkey)"
    exit 1
}

sql query $DB "select * from ssh_host_keys where node_id=\"tbpc$node\""
if {[sql fetchrow $DB] == ""} {
    sql exec $DB "insert into ssh_host_keys values (\"tbpc$node\", '$hostkey')"
} else {
    sql exec $DB "update ssh_host_keys set ssh_key='$hostkey' where node_id=\"tbpc$node\""
}

