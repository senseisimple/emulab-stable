######################################################################
# lanlink.tcl
#
# This defines the LanLink class and its two children Lan and Link.  
# Lan and Link make no changes to the parent and exist purely to
# distinguish between the two in type checking of arguments.  A LanLink
# contains a number of node:port pairs as well as the characteristics
# bandwidth, delay, and loss rate.
######################################################################

Class LanLink -superclass NSObject
Class Link -superclass LanLink
Class Lan -superclass LanLink

LanLink instproc init {s nodes bw d} {
    # This is a list of {node port} pairs.
    $self set nodelist {}

    # The simulator
    $self set sim $s

    # Now we need to fill out the nodelist
    $self instvar nodelist
    $self instvar bandwidth
    $self instvar delay
    $self instvar loss
    foreach node $nodes {
	set nodepair [list $node [$node add_lanlink $self]]
	set bandwidth($nodepair) $bw
	set delay($nodepair) [expr $d / 2.0]
	set loss($nodepair) 0
	lappend nodelist $nodepair
    }
}

# get_port <node>
# This takes a node and returns the port that the node is connected
# to the LAN with.  If a node is in a LAN multiple times for some
# reason then this only returns the first.
LanLink instproc get_port {node} {
    $self instvar nodelist
    foreach pair $nodelist {
	set n [lindex $pair 0]
	set p [lindex $pair 1]
	if {$n == $node} {return $p}
    }
    return {}
}

# fill_ips
# This fills out the IP addresses (see README).  It determines a
# subnet, either from already assigned IPs or by asking the Simulator
# for one, and then fills out unassigned node:port's with free IP
# addresses.
LanLink instproc fill_ips {} {
    $self instvar nodelist
    $self instvar sim

    # Determined a subnet (if possible) and any used IP addresses in it.
    # ips is a set which contains all used IP addresses in this LanLink.
    set subnet {}
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set port [lindex $nodeport 1]
	set ip [$node ip $port]
	if {$ip != {}} {
	    set subnet [join [lrange [split $ip .] 0 2] .]
	    set ips($ip) 1
	}
    }

    # If we couldn't find a subnet we ask the Simulator for one.
    if {$subnet == {}} {
	set subnet [$sim get_subnet]
    }

    # Now we assign IP addresses to any node:port's without them.
    set ip_counter 2
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set port [lindex $nodeport 1]
	if {[$node ip $port] == {}} {
	    set ip {}
	    for {set i $ip_counter} {$i < 255} {incr i} {
		if {! [info exists ips($subnet.$i)]} {
		    set ip $subnet.$i
		    set ips($subnet.$i) 1
		    set ip_counter [expr $i + 1]
		    break
		}
	    }
	    if {$ip == {}} {
		perror "Ran out of IP addresses in subnet $subnet."
		set ip "255.255.255.255"
	    }
	    $node ip $port $ip
	}
    }
}

# The following methods are for renaming objects (see README).
LanLink instproc rename {old new} {
    $self instvar nodelist
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	$node rename_lanlink $old $new
    }
    [$self set sim] rename_lanlink $old $new
}
LanLink instproc rename_node {old new} {
    $self instvar nodelist
    $self instvar bandwidth
    $self instvar delay
    $self instvar loss
    set newnodelist {}
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set port [lindex $nodeport 1]
	set newnodeport [list $new $port]
	if {$node == $old} {
	    lappend newnodelist $newnodeport
	} else {
	    lappend newnodelist $nodeport
	}
	set bandwidth($newnodeport) $bandwidth($nodeport)
	set delay($newnodeport) $delay($nodeport)
	set loss($newnodeport) $loss($nodeport)
	unset bandwidth($nodeport)
	unset delay($nodeport)
	unset loss($nodeport)
    }
    set nodelist $newnodelist
}

# updatedb DB
# This adds a row to the virt_lans table.
LanLink instproc updatedb {DB} {
    $self instvar nodelist
    $self instvar bandwidth
    $self instvar delay
    $self instvar loss
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    foreach nodeport $nodelist {
	set nodeportraw [join $nodeport ":"]
	sql exec $DB "insert into virt_lans (pid,eid,vname,member,delay,bandwidth,lossrate) values (\"$pid\",\"$eid\",\"$self\",\"$nodeportraw\",$delay($nodeport),$bandwidth($nodeport),$loss($nodeport))"
    }
}

