#!/usr/local/bin/tclsh
# This program combines a ns input file and a post-assign IR file
# to add the ip and ip-mac sections to the ir file.


set scriptdir [file dirname [info script]]
set updir [file dirname $scriptdir]
#set mactable "$updir/switch_tools/intel510/macslist"
source "$scriptdir/libir.tcl"
load $updir/../lib/sql.so
set DB [sql connect]
sql selectdb $DB tbdb

namespace import TB_LIBIR::ir

if {[llength $argv] != 2} {
    puts stderr "Syntax: $argv0 <irfile> <nsfile>"
    exit 1
}

set irfile [lindex $argv 0]
set nsfile [lindex $argv 1]


# Extract testbed commands
set tbcommands [exec $scriptdir/extract_tb.tcl < $nsfile]

# Read ir file
ir read $irfile

# Parse testbed commands
foreach cmd [split $tbcommands "\n"] {
    set c [lindex $cmd 0]
    switch -- $c {
	"set-ip" {
	    set node [lindex $cmd 1]
	    set ip [lindex $cmd 2]
	    set IP($node) $ip
	}
	"set-ip-interface" {
	    set node [lindex $cmd 1]
	    set dst [lindex $cmd 2]
	    set ip [lindex $cmd 3]
	    set IP($node:$dst) $ip
	}
    }
}

# Read in link map
if {! [ir exists /virtual/links]} {
    puts stderr "IR does not contain virtual/links section."
    exit 1
}
foreach pair [ir get /virtual/links] {
    set vlinkmap([lindex $pair 0]) [lindex $pair 1]
}

# Read in mac map
if {! [ir exists /vlan]} {
    puts stderr "IR does not contain vlan section."
    exit 1
}
foreach vlan [ir get /vlan] {
    set vlanmap([lindex $vlan 0]) [lrange $vlan 1 end]
}

# Read in node map
if {! [ir exists /virtual/nodes]} {
    puts stderr "IR does not contain virtual/nodes section."
    exit 1
}
foreach pair [ir get /virtual/nodes] {
    set vnodemap([lindex $pair 0]) [lindex $pair 1]
    set rvnodemap([lindex $pair 1]) [lindex $pair 0]
}
# Parse mac list
# This is pretty dumb but is quick to do
sql query $DB "select node_id,MAC from interfaces"
while {[set row [sql fetchrow $DB]] != {}} {
    set node [lindex $row 0]
    set mac [lindex $row 1]
    if {[info exists rvnodemap($node)]} {
	lappend MACTABLE($rvnodemap($node)) $mac
    }
}

# Loop through virtual links
if {! [ir exists /topology/links]} {
    puts stderr "IR does not contain topology/links section."
    exit 1
}

proc subset {A B} {
    set ret {}
    foreach element $A {
	if {[lsearch $B $element] != -1} {
	    lappend ret $element
	}
    }
    return $ret
}

proc get_macs {l} {
    global vlanmap
    
    set i 0
    set macs {}
    while {[info exists vlanmap($l-$i)]} {
	set macs [concat $macs $vlanmap($l-$i)]
	incr i
    }

    return $macs
}

set ip_section {}
set ip_mac_section {}
foreach linkline [ir get /topology/links] {
    set link [lindex $linkline 0]
    set src [lindex $linkline 1]
    set dst [lindex $linkline 3]

    # Look for an assigned ip
    if {[info exists IP($src:$dst)]} {
	lappend ip_section [list $src $dst $IP($src:$dst)]
	lappend ip_mac_section [list [subset $MACTABLE($src) [get_macs $link]] $IP($src:$dst)]
	set ips_assigned($IP($src:$dst)) 1
	lappend ips_node($src) $IP($src:$dst)
    } elseif {[info exists IP($src)]} {
	if {[info exists single_node($src)]} {
	    puts stderr "Can not use set-ip on nodes with multiple links ($src)"
	    exit 1
	}
	set single_node($src) 1
	lappend ip_section [list $src $dst $IP($src)]
	lappend ip_mac_section [list [subset $MACTABLE($src) [get_macs $link]] $IP($src)]
	set ips_assigned($IP($src)) 1
	lappend ips_node($src) $IP($src)
    } else {
	if {[info exists to_assign($link)]} {
	    set to_assign($link) [list $src $dst 1]
	} else {
	    set to_assign($link) [list $src $dst 0]
	}
    }
    if {[info exists IP($dst:$src)]} {
	lappend ip_section [list $dst $src $IP($dst:$src)]
	lappend ip_mac_section [list [subset $MACTABLE($dst) [get_macs $link]] $IP($dst:$src)]
	set ips_assigned($IP($dst:$src)) 1
	lappend ips_node($dst) $IP($dst:$src)
    } elseif {[info exists IP($dst)]} {
	if {[info exists single_node($dst)]} {
	    puts stderr "Can not use set-ip on nodes with multiple links ($dst)"
	    exit 1
	}
	set single_node($dst) 1
	lappend ip_section [list $dst $src $IP($dst)]
	lappend ip_mac_section [list [subset $MACTABLE($dst) [get_macs $link]] $IP($dst)]
	set ips_assigned($IP($dst)) 1
	lappend ips_node($dst) $IP($dst)
    } else {
	if {[info exists to_assign($link)]} {
	    set to_assign($link) [list $dst $src 1]
	} else {
	    set to_assign($link) [list $dst $src 0]
	}
    }
}

# Assign all unassigned nodes
set ip_base "192.168"

proc find_free_ip {subnet} {
    global ips_assigned
    for {set i 1} {$i < 250} {incr i} {
	if {! [info exists ips_assigned($subnet.$i)]} {
	    set ips_assigned($subnet.$i) 1
	    return $subnet.$i
	}
    }
    return {}
}

proc get_subnet {ip} {
    return [join [lrange [split $ip .] 0 2] .]
}
proc find_free_subnet {} {
    global ips_assigned ip_base

    foreach ip [array names ips_assigned] {
	set used([get_subnet $ip]) 1
    }
    for {set i 1} {$i < 250} {incr i} {
	if {! [info exists used($ip_base.$i)]} {return "$ip_base.$i"}
    }
    return {}
}

foreach left [array names to_assign] {
    set node [lindex $to_assign($left) 0]
    set dst [lindex $to_assign($left) 1]
    set both [lindex $to_assign($left) 2]

    if {$both == 1} {
	set subnet [find_free_subnet]
	set ipA [find_free_ip $subnet]
	set ipB [find_free_ip $subnet]
	lappend ip_section [list $node $dst $ipA]
	lappend ip_mac_section [list [subset $MACTABLE($node) [get_macs $left]] $ipA]
	lappend ip_section [list $dst $node $ipB]
	lappend ip_mac_section [list [subset $MACTABLE($dst) [get_macs $left]] $ipB]
	lappend ips_node($node) $ipA
	lappend ips_node($dst) $ipB
    } else {
	if {[info exists IP($dst:$node)]} {
	    set subnet [get_subnet $IP($dst:$node)]
	} elseif {[info exists IP($dst)]} {
	    set subnet [get_subnet $IP($dst)]
	} else {
	    set subnet [find_free_subnet]
	}
	set ip [find_free_ip $subnet]
	lappend ip_section [list $node $dst $ip]
	lappend ip_mac_section [list [subset $MACTABLE($node) [get_macs $left]] $ip]
	lappend ips_node($node) $ip
}
}

# Output
if {[catch "open $irfile a" fp]} {
    puts stderr "Can not open $irfile for writing ($fp)"
    exit 1
}
puts $fp "START ip"
puts $fp "START map"
foreach line $ip_section {
    puts $fp $line
}
puts $fp "END map"
puts $fp "START mac"
foreach line $ip_mac_section {
    set mac [lindex $line 0]
    set ip [lindex $line 1]
    sql exec $DB "update interfaces set IP = '$ip' where MAC = '$mac'"
    puts $fp $line
}
puts $fp "END mac"
puts $fp "END ip"

close $fp