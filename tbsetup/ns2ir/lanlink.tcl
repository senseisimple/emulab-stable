# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

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
# This class is a hack.  It's sole purpose is to associate to a Link
# and a direction for accessing the Queue class.
Class SimplexLink -superclass NSObject

SimplexLink instproc init {link dir} {
    $self set mylink $link
    $self set mydir $dir
}
SimplexLink instproc queue {} {
    $self instvar mylink
    $self instvar mydir
    return [$mylink set ${mydir}queue]
}
# Don't need any rename procs since these never use their own name and
# can not be generated during Link creation.

Queue instproc init {link type dir} {
    $self set mylink $link
    

    # direction is either "to" indicating src to dst or "from" indicating
    # the dst to src.  I.e. to dst or from dst.
    $self set direction $dir

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
	    punsup "Link type $type, using DropTail!"
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

# Hacky. Need to create an association bewteen the queue direction
# and a dummynet pipe. This should happen later on, but I do not
# have time right now to make all the changes. Instead, convert
# "to" to "pipe0" and "from" to "pipe1".
Queue instproc get_pipe {} {
    $self instvar direction

    if {$direction == "to"} {
	set pipe "pipe0"
    } else {
	set pipe "pipe1"
    }
    return $pipe
}

Link instproc init {s nodes bw d type} {
    $self next $s $nodes $bw $d $type

    set src [lindex $nodes 0]
    set dst [lindex $nodes 1]

    $self set src_node $src
    $self set dst_node $dst

    var_import GLOBALS::new_counter
    set q1 q[incr new_counter]
    
    Queue to$q1 $self $type to
    Queue from$q1 $self $type from

    $self set toqueue to$q1
    $self set fromqueue from$q1
}

LanLink instproc init {s nodes bw d type} {
    # This is a list of {node port} pairs.
    $self set nodelist {}

    # The simulator
    $self set sim $s

    # By default, a local link
    $self set widearea 0

    # A simulated lanlink unless we find otherwise
    $self set simulated 1
    # Figure out if this is a lanlink that has at least
    # 1 non-simulated node in it. 
    foreach node $nodes {
	if { [$node set simulated] == 0 } {
	    $self set simulated 0
	    break
	}
    }
    

    # Make sure BW is reasonable. 
    # XXX: Should come from DB instead of hardwired max.
    # Measured in kbps
    set maxbw 100000

    # XXX skip this check for a simulated lanlink even if it
    # causes nse to not keep up with real time. The actual max
    # for simulated links will be added later
    if { [$self set simulated] != 1 && $bw > $maxbw } {
	perror "Bandwidth requested ($bw) exceeds maximum of $maxbw kbps!"
	return
    }

    # Now we need to fill out the nodelist
    $self instvar nodelist

    # r* indicates the switch->node chars, others are node->switch
    $self instvar bandwidth
    $self instvar rbandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss
    $self instvar cost

    foreach node $nodes {
	set nodepair [list $node [$node add_lanlink $self]]
	set bandwidth($nodepair) $bw
	set rbandwidth($nodepair) $bw
	set delay($nodepair) [expr $d / 2.0]
	set rdelay($nodepair) [expr $d / 2.0]
	set loss($nodepair) 0
	set rloss($nodepair) 0
	set cost($nodepair) 1
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
    $self instvar widearea
    set isremote 0

    # Determined a subnet (if possible) and any used IP addresses in it.
    # ips is a set which contains all used IP addresses in this LanLink.
    set subnet {}
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set port [lindex $nodeport 1]
	set ip [$node ip $port]
	set isremote [expr $isremote + [$node set isremote]]
	if {$ip != {}} {
	    if {$isremote} {
		perror "Not allowed to specify IP subnet of a remote link!"
	    }
	    set subnet [join [lrange [split $ip .] 0 2] .]
	    set ips($ip) 1
	}
    }
    if {$isremote && [$self info class] != "Link"} {
	perror "Not allowed to use a remote node in lan $self!"
	return
    }
    set widearea $isremote

    # If we couldn't find a subnet we ask the Simulator for one.
    if {$subnet == {}} {
	if {$isremote} {
	    set subnet [$sim get_subnet_remote]
	} else {
	    set subnet [$sim get_subnet]
	}
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

#
# Return the subnet of a lan. Actually, just return one of the IPs.
#
LanLink instproc get_subnet {} {
    $self instvar nodelist

    set nodeport [lindex $nodelist 0]
    set node [lindex $nodeport 0]
    set port [lindex $nodeport 1]

    return [$node ip $port]
}

#
# Set the routing cost for all interfaces on this LAN
#
LanLink instproc cost {c} {
    $self instvar nodelist
    $self instvar cost

    foreach nodeport $nodelist {
	set cost($nodeport) $c
    }
}


Link instproc rename {old new} {
    $self next $old $new

    $self instvar toqueue
    $self instvar fromqueue
    $toqueue rename_lanlink $old $new
    $fromqueue rename_lanlink $old $new
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
    $self instvar rbandwidth
    $self instvar rdelay
    $self instvar rloss
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
	set rbandwidth($newnodeport) $rbandwidth($nodeport)
	set rdelay($newnodeport) $rdelay($nodeport)
	set rloss($newnodeport) $rloss($nodeport)
	unset bandwidth($nodeport)
	unset delay($nodeport)
	unset loss($nodeport)
	unset rbandwidth($nodeport)
	unset rdelay($nodeport)
	unset rloss($nodeport)
    }
    set nodelist $newnodelist
}

Link instproc updatedb {DB} {
    $self next $DB
    $self instvar toqueue
    $self instvar fromqueue
    $self instvar nodelist
    $self instvar src_node
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	if {$node == $src_node} {
	    set linkqueue $toqueue
	} else {
	    set linkqueue $fromqueue
	}
	set limit_ [$linkqueue set limit_]
	set maxthresh_ [$linkqueue set maxthresh_]
	set thresh_ [$linkqueue set thresh_]
	set q_weight_ [$linkqueue set q_weight_]
	set linterm_ [$linkqueue set linterm_]
	set queue-in-bytes_ [$linkqueue set queue-in-bytes_]
	if {${queue-in-bytes_} == "true"} {
	    set queue-in-bytes_ 1
	} elseif {${queue-in-bytes_} == "false"} {
	    set queue-in-bytes_ 0
	}
	set bytes_ [$linkqueue set bytes_]
	if {$bytes_ == "true"} {
	    set bytes_ 1
	} elseif {$bytes_ == "false"} {
	    set bytes_ 0
	}
	set mean_pktsize_ [$linkqueue set mean_pktsize_]
	set red_ [$linkqueue set red_]
	if {$red_ == "true"} {
	    set red_ 1
	} elseif {$red_ == "false"} {
	    set red_ 0
	}
	set gentle_ [$linkqueue set gentle_]
	if {$gentle_ == "true"} {
	    set gentle_ 1
	} elseif {$gentle_ == "false"} {
	    set gentle_ 0
	}
	set wait_ [$linkqueue set wait_]
	set setbit_ [$linkqueue set setbit_]
	set droptail_ [$linkqueue set drop-tail_]
	
	set nodeportraw [join $nodeport ":"]
	sql exec $DB "update virt_lans set q_limit=$limit_, q_maxthresh=$maxthresh_, q_minthresh=$thresh_, q_weight=$q_weight_, q_linterm=$linterm_, q_qinbytes=${queue-in-bytes_}, q_bytes=$bytes_, q_meanpsize=$mean_pktsize_, q_wait=$wait_, q_setbit=$setbit_, q_droptail=$droptail_, q_red=$red_, q_gentle=$gentle_ where pid=\"$pid\" and eid=\"$eid\" and vname=\"$self\" and member=\"$nodeportraw\""
    }
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
    $self instvar cost
    $self instvar widearea
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    foreach nodeport $nodelist {
	set nodeportraw [join $nodeport ":"]
	sql exec $DB "insert into virt_lans (pid,eid,vname,member,delay,rdelay,bandwidth,rbandwidth,lossrate,rlossrate,cost,widearea) values (\"$pid\",\"$eid\",\"$self\",\"$nodeportraw\",$delay($nodeport),$rdelay($nodeport),$bandwidth($nodeport),$rbandwidth($nodeport),$loss($nodeport),$rloss($nodeport),$cost($nodeport),$widearea)"
    }
}
