#!/usr/local/bin/tclsh
######################################################################
# gentestbed.tcl
#
# gentestbed.tcl <switches> <nodesperswitch> <linkspernode> <internode> <interswitch>
######################################################################

if {[llength $argv] != 5} {
    puts "Syntax: gentestbed.tcl <switches> <nodesperswitch> <linkspernode> <internode> <interswitch>"
    exit 1
}

set i 0
foreach a {switches nodes links inode iswitch} {
    upvar 0 $a v
    set v [lindex $argv $i]
    incr i
}

for {set s 0} {$s < $switches} {incr s} {
    puts "node switch_$s switch"
    for {set n 0} {$n < $nodes} {incr n} {
	puts "node node_${s}_$n pc"
	puts "link node_${s}_$n switch_$s $inode $links"
    }
}
for {set s 0} {$s < $switches} {incr s} {
    for {set s2 $s} {$s2 < $switches} {incr s2} {
	if {$s != $s2} {
	    puts "link switch_${s} switch_${s2} 1 $iswitch"
	}
    }
}

