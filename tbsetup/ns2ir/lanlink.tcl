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
Class Queue -superclass NSObject

Queue instproc init {link type} {
    $self set mylink $link
    
    # These control whether the link was created RED or GRED. It
    # filters through the DB.
    $self set gentle_ 0
    $self set red_ 0

    #
    # These are NS variables for queues (with NS defaults).
    #
    $self set limit_ 50
    $self set maxthresh_ 15
    $self set thresh_ 5
    $self set q_weight_ 0.002
    $self set linterm_ 10
    $self set queue-in-bytes_ 0
    $self set bytes_ 0
    $self set mean_pktsize_ 500
    $self set wait_ 1
    $self set setbit_ 0
    $self set drop-tail_ 1

    if {$type != {}} {
	$self instvar red_
	$self instvar gentle_
	
	if {$type == "RED"} {
	    set red_ 1
	} elseif {$type == "GRED"} {
	    set red_ 1
	    set gentle_ 1
	} elseif {$type != "DropTail"} {
	    puts stderr "Unsupported: Link type $type, using DropTail."
	}
    }
}

Queue instproc rename_lanlink {old new} {
    $self instvar mylink

    set mylink $new
}

Queue instproc get_link {} {
    $self instvar mylink

    return $mylink
}

LanLink instproc queue {} {
    $self instvar linkqueue

    return $linkqueue
}

LanLink instproc init {s nodes bw d type} {
    # This is a list of {node port} pairs.
    $self set nodelist {}

    # The simulator
    $self set sim $s

    # Now we need to fill out the nodelist
    $self instvar nodelist

    var_import GLOBALS::new_counter
    set q1 q[incr new_counter]

    Queue $q1 $self $type

    # For now, a single queue for the link. Makes no sense for lans.
    $self set linkqueue $q1

    # r* indicates the switch->node chars, others are node->switch
    $self instvar bandwidth
    $self instvar rbandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss

    foreach node $nodes {
	set nodepair [list $node [$node add_lanlink $self]]
	set bandwidth($nodepair) $bw
	set rbandwidth($nodepair) $bw
	set delay($nodepair) [expr $d / 2.0]
	set rdelay($nodepair) [expr $d / 2.0]
	set loss($nodepair) 0
	set rloss($nodepair) 0
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
    $self instvar linkqueue
    $linkqueue rename_lanlink $old $new
    
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
    $self instvar rbandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss
    $self instvar linkqueue
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    # For now, the return params are the same.
    set limit_ [$linkqueue set limit_]
    set maxthresh_ [$linkqueue set maxthresh_]
    set thresh_ [$linkqueue set thresh_]
    set q_weight_ [$linkqueue set q_weight_]
    set linterm_ [$linkqueue set linterm_]
    set queue-in-bytes_ [$linkqueue set queue-in-bytes_]
    set bytes_ [$linkqueue set bytes_]
    set mean_pktsize_ [$linkqueue set mean_pktsize_]
    set red_ [$linkqueue set red_]
    set gentle_ [$linkqueue set gentle_]
    set wait_ [$linkqueue set wait_]
    set setbit_ [$linkqueue set setbit_]
    set droptail_ [$linkqueue set drop-tail_]

    foreach nodeport $nodelist {
	set nodeportraw [join $nodeport ":"]
	sql exec $DB "insert into virt_lans (pid,eid,vname,member,delay,rdelay,bandwidth,rbandwidth,lossrate,rlossrate,q_limit,q_maxthresh,q_minthresh,q_weight,q_linterm,q_qinbytes,q_bytes,q_meanpsize,q_wait,q_setbit,q_droptail,q_red,q_gentle) values (\"$pid\",\"$eid\",\"$self\",\"$nodeportraw\",$delay($nodeport),$rdelay($nodeport),$bandwidth($nodeport),$rbandwidth($nodeport),$loss($nodeport),$rloss($nodeport),$limit_,$maxthresh_,$thresh_,$q_weight_,$linterm_,${queue-in-bytes_},$bytes_,$mean_pktsize_,$wait_,$setbit_,$droptail_,$red_,$gentle_)"
    }
}
