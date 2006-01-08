# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
# Ditto, another hack class.
Class LLink -superclass NSObject

SimplexLink instproc init {link dir} {
    $self set mylink $link
    $self set mydir $dir
}
SimplexLink instproc queue {} {
    $self instvar mylink
    $self instvar mydir

    set myqueue [$mylink set ${mydir}queue]
    return $myqueue
}
LLink instproc init {lan node} {
    $self set mylan  $lan
    $self set mynode $node
}
LLink instproc queue {} {
    $self instvar mylan
    $self instvar mynode

    set port [$mylan get_port $mynode]
    
    return [$mylan set linkq([list $mynode $port])]
}
# Don't need any rename procs since these never use their own name and
# can not be generated during Link creation.

Queue instproc init {link type node} {
    $self set mylink $link
    $self set mynode $node
    
    # These control whether the link was created RED or GRED. It
    # filters through the DB.
    $self set gentle_ 0
    $self set red_ 0

    $self set traced 0
    $self set trace_type "header"
    $self set trace_expr {}
    $self set trace_snaplen 0
    $self set trace_endnode 0

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
	    $link mustdelay
	} elseif {$type == "GRED"} {
	    set red_ 1
	    set gentle_ 1
	    $link mustdelay
	} elseif {$type != "DropTail"} {
	    punsup "Link type $type, using DropTail!"
	}
    }
}

Queue instproc rename {old new} {
    $self instvar mylink

    $mylink rename_queue $old $new
}

Queue instproc rename_lanlink {old new} {
    $self instvar mylink

    set mylink $new
}

Queue instproc get_link {} {
    $self instvar mylink

    return $mylink
}

Queue instproc agent_name {} {
    $self instvar mylink
    $self instvar mynode

    return "$mylink-$mynode"
}

# Turn on tracing.
Queue instproc trace {{ttype "header"} {texpr ""}} {
    $self instvar traced
    $self instvar trace_expr
    $self instvar trace_type
    
    if {$texpr == ""} {
	set texpr {}
    }

    set traced 1
    set trace_type $ttype
    set trace_expr $texpr
}

#
# A queue is associated with a node on a link. Return that node.
# 
Queue instproc get_node {} {
    $self instvar mynode

    return $mynode
}

Link instproc init {s nodes bw d type} {
    $self next $s $nodes $bw $d $type

    set src [lindex $nodes 0]
    set dst [lindex $nodes 1]

    $self set src_node $src
    $self set dst_node $dst

    # The default netmask, which the user may change (at his own peril).
    $self set netmask "255.255.255.0"

    var_import GLOBALS::new_counter
    set q1 q[incr new_counter]
    
    Queue to$q1 $self $type $src
    Queue from$q1 $self $type $dst

    $self set toqueue to$q1
    $self set fromqueue from$q1
}

LanLink instproc init {s nodes bw d type} {
    var_import GLOBALS::new_counter

    # This is a list of {node port} pairs.
    $self set nodelist {}

    # The simulator
    $self set sim $s

    # By default, a local link
    $self set widearea 0

    # Default type is a plain "ethernet". User can change this.
    $self set protocol "ethernet"

    # Colocation is on by default, but this only applies to emulated links
    # between virtual nodes anyway.
    $self set trivial_ok 1

    # Allow user to control whether link gets a linkdelay, if link is shaped.
    # If not shaped, and user sets this variable, a link delay is inserted
    # anyway on the assumption that user wants later control over the link.
    # Both lans and links can get linkdelays.     
    $self set uselinkdelay 0

    # Allow user to control if link is emulated.
    $self set emulated 0

    # Allow user to turn off actual bw shaping on emulated links.
    $self set nobwshaping 0

    # mustdelay; force a delay (or linkdelay) to be inserted. assign_wrapper
    # is free to override this, but not sure why it want to! When used in
    # conjunction with nobwshaping, you get a delay node, but with no ipfw
    # limits on the bw part, and assign_wrapper ignores the bw when doing
    # assignment.
    $self set mustdelay 0

    # Allow user to turn on veth devices on emulated links.
    $self set useveth 0

    # XXX Allow user to set the accesspoint.
    $self set accesspoint {}

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

    # The default netmask, which the user may change (at his own peril).
    $self set netmask "255.255.255.0"

    # Make sure BW is reasonable. 
    # XXX: Should come from DB instead of hardwired max.
    # Measured in kbps
    set maxbw 1000000

    # XXX skip this check for a simulated lanlink even if it
    # causes nse to not keep up with real time. The actual max
    # for simulated links will be added later
    if { [$self set simulated] != 1 && $bw > $maxbw } {
	perror "Bandwidth requested ($bw) exceeds maximum of $maxbw kbps!"
	return
    }

    # Virt lan settings, for the entire lan
    $self instvar settings

    # And a two-dimenional arrary for per-member settings.
    # TCL does not actually have multi-dimensional arrays though, so its faked.
    $self instvar member_settings

    # Now we need to fill out the nodelist
    $self instvar nodelist

    # r* indicates the switch->node chars, others are node->switch
    $self instvar bandwidth
    $self instvar rbandwidth
    $self instvar ebandwidth
    $self instvar rebandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss
    $self instvar cost
    $self instvar linkq

    $self instvar iscloud
    $self set iscloud 0

    foreach node $nodes {
	set nodepair [list $node [$node add_lanlink $self]]
	set bandwidth($nodepair) $bw
	set rbandwidth($nodepair) $bw
	# Note - we don't set defaults for ebandwidth and rebandwidth - lack
	# of an entry for a nodepair indicates that they should be left NULL
	# in the output.
	set delay($nodepair) [expr $d / 2.0]
	set rdelay($nodepair) [expr $d / 2.0]
	set loss($nodepair) 0
	set rloss($nodepair) 0
	set cost($nodepair) 1
	lappend nodelist $nodepair

	set lq q[incr new_counter]
	Queue lq$lq $self $type $node
	set linkq($nodepair) lq$lq
    }
}

#
# Set the mustdelay flag.
#
LanLink instproc mustdelay {} {
    $self instvar mustdelay
    set mustdelay 1
}

#
# Set up tracing.
#
Lan instproc trace {{ttype "header"} {texpr ""}} {
    $self instvar nodelist
    $self instvar linkq

    foreach nodeport $nodelist {
	set linkqueue $linkq($nodeport)
	$linkqueue trace $ttype $texpr
    }
}

Link instproc trace {{ttype "header"} {texpr ""}} {
    $self instvar toqueue
    $self instvar fromqueue
    
    $toqueue trace $ttype $texpr
    $fromqueue trace $ttype $texpr
}

Lan instproc trace_snaplen {len} {
    $self instvar nodelist
    $self instvar linkq

    foreach nodeport $nodelist {
	set linkqueue $linkq($nodeport)
	$linkqueue set trace_snaplen $len
    }
}

Link instproc trace_snaplen {len} {
    $self instvar toqueue
    $self instvar fromqueue
    
    $toqueue set trace_snaplen $len
    $fromqueue set trace_snaplen $len
}

Lan instproc trace_endnode {onoff} {
    $self instvar nodelist
    $self instvar linkq

    foreach nodeport $nodelist {
	set linkqueue $linkq($nodeport)
	$linkqueue set trace_endnode $onoff
    }
}

Link instproc trace_endnode {onoff} {
    $self instvar toqueue
    $self instvar fromqueue
    
    $toqueue set trace_endnode $onoff
    $fromqueue set trace_endnode $onoff
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
    $self instvar netmask
    set isremote 0
    set netmaskint [inet_atohl $netmask]

    # Determine a subnet (if possible) and any used IP addresses in it.
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
	    set ipint [inet_atohl $ip]
	    set subnet [inet_hltoa [expr $ipint & $netmaskint]]
	    set ips($ip) 1
	    $sim use_subnet $subnet $netmask
	}
    }
    if {$isremote && [$self info class] != "Link"} {
	perror "Not allowed to use a remote node in lan $self!"
	return
    }
    set widearea $isremote

    # See parse-ns if you change this! 
    if {$isremote && ($netmask != "255.255.255.248")} {
	puts stderr "Ignoring netmask for remote link; forcing 255.255.255.248"
	set netmask "255.255.255.248"
	set netmaskint [inet_atohl $netmask]
    }

    # If we couldn't find a subnet we ask the Simulator for one.
    if {$subnet == {}} {
	if {$isremote} {
	    set subnet [$sim get_subnet_remote]
	} else {
	    set subnet [$sim get_subnet $netmask]
	}
    }

    # Now we assign IP addresses to any node:port's without them.
    set ip_counter 2
    set subnetint [inet_atohl $subnet]
    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set port [lindex $nodeport 1]
	if {[$node ip $port] == {}} {
	    set ip {}
	    set max [expr ~ $netmaskint]
	    for {set i $ip_counter} {$i < $max} {incr i} {
		set nextip [inet_hltoa [expr $subnetint | $i]]
		
		if {! [info exists ips($nextip)]} {
		    set ip $nextip
		    set ips($ip) 1
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
# XXX - Set the accesspoint for the lan to node. This is temporary.
#
LanLink instproc set_accesspoint {node} {
    $self instvar accesspoint
    $self instvar nodelist

    foreach pair $nodelist {
	set n [lindex $pair 0]
	set p [lindex $pair 1]
	if {$n == $node} {
	    set accesspoint $node
	    return {}
	}
    }
    perror "set_accesspoint: No such node $node in lan $self."
}

#
# Set a setting for the entire lan.
#
LanLink instproc set_setting {capkey capval} {
    $self instvar settings

    set settings($capkey) $capval
}

#
# Set a setting for just one member of a lan
#
LanLink instproc set_member_setting {node capkey capval} {
    $self instvar member_settings
    $self instvar nodelist

    foreach pair $nodelist {
	set n [lindex $pair 0]
	set p [lindex $pair 1]
	if {$n == $node} {
	    set member_settings($node,$capkey) $capval
	    return {}
	}
    }
    perror "set_member_setting: No such node $node in lan $self."
}

#
# Return the subnet of a lan. Actually, just return one of the IPs.
#
LanLink instproc get_netmask {} {
    $self instvar netmask

    return $netmask
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

Link instproc rename_queue {old new} {
    $self next $old $new

    $self instvar toqueue
    $self instvar fromqueue

    if {$old == $toqueue} {
	set toqueue $new
    } elseif {$old == $fromqueue} {
	set fromqueue $new
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
    $self instvar rbandwidth
    $self instvar rdelay
    $self instvar rloss
    $self instvar linkq
    $self instvar accesspoint

    # XXX Temporary
    if {$accesspoint == $old} {
	set accesspoint $new
    }
    
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
	set linkq($newnodepair) linkq($nodeport)
	
	unset bandwidth($nodeport)
	unset delay($nodeport)
	unset loss($nodeport)
	unset rbandwidth($nodeport)
	unset rdelay($nodeport)
	unset rloss($nodeport)
	unset linkq($nodeport)
    }
    set nodelist $newnodelist
}

LanLink instproc rename_queue {old new} {
    $self instvar nodelist
    $self instvar linkq

    foreach nodeport $nodelist {
	set foo linkq($nodeport)
	
	if {$foo == $old} {
	    set linkq($nodeport) $new
	}
    }
}

Link instproc updatedb {DB} {
    $self instvar toqueue
    $self instvar fromqueue
    $self instvar nodelist
    $self instvar src_node
    $self instvar trivial_ok
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar bandwidth
    $self instvar rbandwidth
    $self instvar ebandwidth
    $self instvar rebandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss
    $self instvar cost
    $self instvar widearea
    $self instvar uselinkdelay
    $self instvar emulated
    $self instvar nobwshaping
    $self instvar useveth
    $self instvar sim
    $self instvar netmask
    $self instvar protocol
    $self instvar mustdelay

    if {$protocol != "ethernet"} {
	perror "Link must be an ethernet only, not a $protocol"
	return
    }

    $sim spitxml_data "virt_lan_lans" [list "vname"] [list $self]

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

	#
	# Note; we are going to deprecate virt_lans:member and virt_nodes:ips
	# Instead, store vnode,vport,ip in the virt_lans table. To get list
	# of IPs for a node, join virt_nodes with virt_lans. port number is
	# no longer required, but we maintain it to provide a unique key that
	# does not depend on IP address.
	#
	set port [lindex $nodeport 1]
	set ip [$node ip $port]

	set nodeportraw [join $nodeport ":"]

	set fields [list "vname" "member" "mask" "delay" "rdelay" "bandwidth" "rbandwidth" "lossrate" "rlossrate" "cost" "widearea" "emulated" "uselinkdelay" "nobwshaping" "usevethiface" "q_limit" "q_maxthresh" "q_minthresh" "q_weight" "q_linterm" "q_qinbytes" "q_bytes" "q_meanpsize" "q_wait" "q_setbit" "q_droptail" "q_red" "q_gentle" "trivial_ok" "vnode" "vport" "ip" "mustdelay"]

	# Treat estimated bandwidths differently - leave them out of the lists
	# unless the user gave a value - this way, they get the defaults if not
	# specified
	if { [info exists ebandwidth($nodeport)] } {
	    lappend fields "est_bandwidth"
	}

	if { [info exists rebandwidth($nodeport)] } {
	    lappend fields "rest_bandwidth"
	}
	
	# Tracing.
	if {[$linkqueue set traced] == 1} {
	    lappend fields "traced"
	    lappend fields "trace_type"
 	    lappend fields "trace_expr"
 	    lappend fields "trace_snaplen"
 	    lappend fields "trace_endnode"
	}

	set values [list $self $nodeportraw $netmask $delay($nodeport) $rdelay($nodeport) $bandwidth($nodeport) $rbandwidth($nodeport) $loss($nodeport) $rloss($nodeport) $cost($nodeport) $widearea $emulated $uselinkdelay $nobwshaping $useveth $limit_  $maxthresh_ $thresh_ $q_weight_ $linterm_ ${queue-in-bytes_}  $bytes_ $mean_pktsize_ $wait_ $setbit_ $droptail_ $red_ $gentle_ $trivial_ok $node $port $ip $mustdelay]

	if { [info exists ebandwidth($nodeport)] } {
	    lappend values $ebandwidth($nodeport)
	}

	if { [info exists rebandwidth($nodeport)] } {
	    lappend values $rebandwidth($nodeport)
	}

	# Tracing.
	if {[$linkqueue set traced] == 1} {
	    lappend values [$linkqueue set traced]
	    lappend values [$linkqueue set trace_type]
	    lappend values [$linkqueue set trace_expr]
	    lappend values [$linkqueue set trace_snaplen]
	    lappend values [$linkqueue set trace_endnode]
	}

	$sim spitxml_data "virt_lans" $fields $values
    }
}

Lan instproc updatedb {DB} {
    $self instvar nodelist
    $self instvar linkq
    $self instvar trivial_ok
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::GLOBALS::modelnet_cores
    var_import ::GLOBALS::modelnet_edges
    $self instvar bandwidth
    $self instvar rbandwidth
    $self instvar ebandwidth
    $self instvar rebandwidth
    $self instvar delay
    $self instvar rdelay
    $self instvar loss
    $self instvar rloss
    $self instvar cost
    $self instvar widearea
    $self instvar uselinkdelay
    $self instvar emulated
    $self instvar nobwshaping
    $self instvar useveth
    $self instvar sim
    $self instvar netmask
    $self instvar protocol
    $self instvar accesspoint
    $self instvar settings
    $self instvar member_settings
    $self instvar mustdelay

    if {$modelnet_cores > 0 || $modelnet_edges > 0} {
	perror "Lans are not allowed when using modelnet; just duplex links."
	return
    }

    $sim spitxml_data "virt_lan_lans" [list "vname"] [list $self]

    #
    # Upload lan settings and them per-member settings
    #
    foreach setting [array names settings] {
	set fields [list "vname" "capkey" "capval"]
	set values [list $self $setting $settings($setting)]
	
	$sim spitxml_data "virt_lan_settings" $fields $values
    }

    foreach nodeport $nodelist {
	set node [lindex $nodeport 0]
	set isvirt [$node set isvirt]
	set linkqueue $linkq($nodeport)
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
	
	#
	# Note; we are going to deprecate virt_lans:member and virt_nodes:ips
	# Instead, store vnode,vport,ip in the virt_lans table. To get list
	# of IPs for a node, join virt_nodes with virt_lans. port number is
	# no longer required, but we maintain it to provide a unique key that
	# does not depend on IP address.
	#
	set port [lindex $nodeport 1]
	set ip [$node ip $port]

	set nodeportraw [join $nodeport ":"]

	set is_accesspoint 0
	if {$node == $accesspoint} {
	    set is_accesspoint 1
	}

	set fields [list "vname" "member" "mask" "delay" "rdelay" "bandwidth" "rbandwidth" "lossrate" "rlossrate" "cost" "widearea" "emulated" "uselinkdelay" "nobwshaping" "usevethiface" "q_limit" "q_maxthresh" "q_minthresh" "q_weight" "q_linterm" "q_qinbytes" "q_bytes" "q_meanpsize" "q_wait" "q_setbit" "q_droptail" "q_red" "q_gentle" "trivial_ok" "protocol" "is_accesspoint" "vnode" "vport" "ip" "mustdelay"]

	# Treat estimated bandwidths differently - leave them out of the lists
	# unless the user gave a value - this way, they get the defaults if not
	# specified
	if { [info exists ebandwidth($nodeport)] } {
	    lappend fields "est_bandwidth"
	}

	if { [info exists rebandwidth($nodeport)] } {
	    lappend fields "rest_bandwidth"
	}

	# Tracing.
	if {[$linkqueue set traced] == 1} {
	    lappend fields "traced"
	    lappend fields "trace_type"
 	    lappend fields "trace_expr"
 	    lappend fields "trace_snaplen"
 	    lappend fields "trace_endnode"
	}
	
	set values [list $self $nodeportraw $netmask $delay($nodeport) $rdelay($nodeport) $bandwidth($nodeport) $rbandwidth($nodeport) $loss($nodeport) $rloss($nodeport) $cost($nodeport) $widearea $emulated $uselinkdelay $nobwshaping $useveth $limit_  $maxthresh_ $thresh_ $q_weight_ $linterm_ ${queue-in-bytes_}  $bytes_ $mean_pktsize_ $wait_ $setbit_ $droptail_ $red_ $gentle_ $trivial_ok $protocol $is_accesspoint $node $port $ip $mustdelay]

	if { [info exists ebandwidth($nodeport)] } {
	    lappend values $ebandwidth($nodeport)
	}

	if { [info exists rebandwidth($nodeport)] } {
	    lappend values $rebandwidth($nodeport)
	}

	# Tracing.
	if {[$linkqueue set traced] == 1} {
	    lappend values [$linkqueue set traced]
	    lappend values [$linkqueue set trace_type]
	    lappend values [$linkqueue set trace_expr]
	    lappend values [$linkqueue set trace_snaplen]
	    lappend values [$linkqueue set trace_endnode]
	}

	$sim spitxml_data "virt_lans" $fields $values

	foreach setting_key [array names member_settings] {
	    set foo      [split $setting_key ","]
	    set thisnode [lindex $foo 0]
	    set capkey   [lindex $foo 1]

	    if {$thisnode == $node} {
		set fields [list "vname" "member" "capkey" "capval"]
		set values [list $self $nodeportraw $capkey \
		                 $member_settings($setting_key)]
	
		$sim spitxml_data "virt_lan_member_settings" $fields $values
	    }
	}
    }
}

#
# Convert IP/Mask to an integer (host order)
#
proc inet_atohl {ip} {
    if {[scan $ip "%d.%d.%d.%d" a b c d] != 4} {
	perror "\[inet_atohl] Invalid ip $ip; cannot be converted!"
	return 0
    }
    return [expr ($a << 24) | ($b << 16) | ($c << 8) | $d]
}
proc inet_hltoa {ip} {
    set a [expr ($ip >> 24) & 0xff]
    set b [expr ($ip >> 16) & 0xff]
    set c [expr ($ip >> 8)  & 0xff]
    set d [expr ($ip >> 0)  & 0xff]

    return "$a.$b.$c.$d"
}
