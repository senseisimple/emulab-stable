# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# node.tcl
#
# This defines the Node class.  Instances of this class are created by
# the 'node' method of Simulator.  A Node is connected to a number of
# LanLinks.  Each such connection is associated with a virtual port,
# an integer.  Each virtual port also has an IP address.  Virtual
# ports start at 0 and go up continuously.  Besides the port
# information each node also has a variety of strings.  These strings
# are set by tb-* commands and dumped to the DB but are otherwise
# uninterpreted.
######################################################################

Class Node -superclass NSObject

Node instproc init {s} {
    $self set sim $s

    # portlist is a list of connections for the node.  It is sorted
    # by portnumber.  I.e. the ith element of portlist is the connection
    # on port i.
    $self set portlist {}

    # A list of agents attached to this node.
    $self set agentlist {}

    # A counter for udp/tcp portnumbers. Assign them in an increasing
    # fashion as agents are assigned to the node.
    $self set next_portnumber_ 5000

    # iplist, like portlist, is supported by portnumber.  An entry of
    # {} indicates an unassigned IP address for that port.
    $self set iplist {}

    # A route list. 
    $self instvar routelist
    array set routelist {}

    # The type of the node.
    $self set type "pc" 

    # Is remote flag. Used when we do IP assignment later.
    $self set isremote 0

    # Sorta ditto for virt.
    $self set isvirt 0

    # If hosting a virtual node (or nodes).
    $self set virthost 0

    # Sorta ditto for subnode stuff.
    $self set issubnode    0
    $self set subnodehost  0
    $self set subnodechild ""

    # If osid remains blank when updatedb is called it is changed
    # to the default OS based on it's type (taken from node_types
    # table).
    $self set osid ""

    # Start with an empty set of desires
    $self instvar desirelist
    array set desirelist {}

    # These are just various strings that we pass through to the DB.
    $self set cmdline ""
    $self set rpms ""
    $self set startup ""
    $self set deltas ""
    $self set tarfiles ""
    $self set failureaction "fatal"
    $self set fixed ""
    $self set nseconfig ""
    $self set realtime 0

    var_import ::GLOBALS::simulated
    if { $simulated == 1 } {
	$self set simulated 1
    } else {
	$self set simulated 0
	$self set realtime 0
    }
    $self set nsenode_vportlist {}
}

# The following procs support renaming (see README)
Node instproc rename {old new} {
    $self instvar portlist
    foreach object $portlist {
	$object rename_node $old $new
    }
    [$self set sim] rename_node $old $new
}

Node instproc rename_lanlink {old new} {
    $self instvar portlist
    set newportlist {}
    foreach node $portlist {
	if {$node == $old} {
	    lappend newportlist $new
	} else {
	    lappend newportlist $node
	}
    }
    set portlist $newportlist
}

# updatedb DB
# This adds a row to the virt_nodes table corresponding to this node.
Node instproc updatedb {DB} {
    $self instvar portlist
    $self instvar type
    $self instvar osid
    $self instvar cmdline
    $self instvar rpms
    $self instvar startup
    $self instvar deltas
    $self instvar iplist
    $self instvar tarfiles
    $self instvar failureaction
    $self instvar routertype
    $self instvar fixed
    $self instvar agentlist
    $self instvar routelist
    $self instvar sim
    $self instvar realtime
    $self instvar isvirt
    $self instvar virthost
    $self instvar issubnode
    $self instvar desirelist
    var_import ::TBCOMPAT::default_osids
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::GLOBALS::default_ip_routing_type
    var_import ::GLOBALS::enforce_user_restrictions
    
    # If we haven't specified a osid so far then we should fill it
    # with the id from the node_types table now.
    if {$osid == {}} {
	if {$virthost == 0} {
	    if {[info exists default_osids($type)]} {
		set osid $default_osids($type)
	    }
	}
    } else {
	# Do not allow user to set os for virt nodes at this time.
	if {$enforce_user_restrictions && $isvirt} {
	    perror "You may not specify an OS for virtual nodes ($self)!"
	    return
	}
	# Do not allow user to set os for host running virt nodes.
	if {$enforce_user_restrictions && $virthost} {
	    perror "You may not specify an OS for hosting virtnodes ($self)!"
	    return
	}
    }

    #
    # If a subnode, then it must be fixed to a pnode, or we have to
    # create one on the fly and set the type properly. 
    # 
    if {$issubnode && $fixed == ""} {
	$sim spitxml_data "virt_nodes" [list "vname" "type" "ips" "osname" "cmd_line" "rpms" "deltas" "startupcmd" "tarfiles" "fixed" ] [list "host-$self" "pc" "" "" "" "" "" "" "" $self ]
    }

    # We need to generate the IP column from our iplist.
    set ipraw {}
    set i 0
    foreach ip $iplist {
	lappend ipraw $i:$ip
	incr i
    }

    foreach agent $agentlist {
	$agent updatedb $DB
    }

    $self add_routes_to_DB $DB

    # Update the DB
    $sim spitxml_data "virt_nodes" [list "vname" "type" "ips" "osname" "cmd_line" "rpms" "deltas" "startupcmd" "tarfiles" "failureaction" "routertype" "fixed" ] [list $self $type $ipraw $osid $cmdline $rpms $deltas $startup $tarfiles $failureaction $default_ip_routing_type $fixed ]

    # Put in the desires, too
    foreach desire [lsort [array names desirelist]] {
	set weight $desirelist($desire)
	$sim spitxml_data "virt_node_desires" [list "vname" "desire" "weight"] [list $self $desire $weight]
    }
}

# add_lanlink lanlink
# This creates a new virtual port and connects the specified LanLink to it.
# The port number is returned.
Node instproc add_lanlink {lanlink} {
    $self instvar portlist
    $self instvar iplist
    $self instvar simulated

    # Check if we're making too many lanlinks to this node
    # XXX Could come from db from node_types if necessary
    # For now, no more than 4 links or interfaces per node
    # XXX Ignore if the lanlink is simulated i.e. one that
    # has all simulated nodes in it. 
#    set maxlanlinks 4
#    if { [$lanlink set simulated] != 1 && $maxlanlinks == [llength $portlist] } {
#	# adding this one would put us over
#	perror "Too many links/LANs to node $self! Maximum is $maxlanlinks."
#    }

    lappend portlist $lanlink
    lappend iplist ""
    return [expr [llength $portlist] - 1]
}

#
# Find the lan that both nodes are attached to. Very bad. If more than
# comman lan, returns the first.
#
Node instproc find_commonlan {node} {
    $self instvar portlist
    set match -1

    foreach ll $portlist {
	set match [$node find_port $ll]
	if {$match != -1} {
	    break
	}
    }
    return $match
}

# ip port
# ip port ip
# In the first form this returns the IP address associated with the port.
# In the second from this sets the IP address of a port.
Node instproc ip {port args} {
    $self instvar iplist
    $self instvar sim
    if {$args == {}} {
	return [lindex $iplist $port]
    } else {
	set ip [lindex $args 0]
	set iplist [lreplace $iplist $port $port $ip]
    }    
}

# find_port lanlink
# This takes a lanlink and returns the port it is connected to or 
# -1 if there is no connection.
Node instproc find_port {lanlink} {
    return [lsearch [$self set portlist] $lanlink]
}

# Attach an agent to a node. This mainly a bookkeeping device so
# that the we can update the DB at the end.
Node instproc attach-agent {agent} {
    $self instvar agentlist

    lappend agentlist $agent
    $agent set_node $self
}

#
# Return and bump next agent portnumber,
Node instproc next_portnumber {} {
    $self instvar next_portnumber_
    
    set next_port [incr next_portnumber_]
    return $next_port
}

#
# Add a route.
# The nexthop to <dst> from this node is <target>.
#
Node instproc add-route {dst nexthop} {
    $self instvar routelist

    if {[info exists routelist($dst)]} {
	perror "\[add-route] route from $self to $dst already exists!"
    }
    set routelist($dst) $nexthop
}

#
# Set the type/isremote/isvirt for a node. Called from tb_compat.
#
Node instproc set_hwtype {hwtype isrem isv issub} {
    $self instvar type
    $self instvar isremote
    $self instvar isvirt
    $self instvar issubnode

    set type $hwtype
    set isremote $isrem
    set isvirt $isv
    set issubnode $issub
}

#
# Fix a node. Watch for fixing a node to another node.
#
Node instproc set_fixed {pnode} {
    $self instvar fixed
    $self instvar issubnode

    if { [Node info instances $pnode] != {} } {
        # $pnode is an object instance of class Node
	if {$issubnode} {
	    $pnode set subnodehost 1
	    $pnode set subnodechild $self
	} else {
	    perror "\[set-fixed] Improper fix-node $self to $pnode!"
	    return
	}
    }    
    set fixed $pnode
}

#
# Update DB with routes
#
Node instproc add_routes_to_DB {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar routelist
    $self instvar sim

    foreach dst [lsort [array names routelist]] {
	set hop $routelist($dst)
	set port -1

	#
	# Convert hop to IP address. Need to find the link between the
	# the this node and the hop. This is easy if its a link. If its
	# a lan, then its ugly.
	#
	set hoplink [$sim find_link $self $hop]
	if {$hoplink == {}} {
	    set port [$self find_commonlan $hop]
	} else {
	    set port [$hop find_port $hoplink]
	}
	if {$port == -1} {
	    perror "\[add-route] Cannot find a link from $self to $hop!"
	    return
	}
	set hopip [$hop ip $port]
	
	#
	# Convert dst to IP address.
	#
	switch -- [$dst info class] {
	    "Node" {
		if {[llength [$dst set portlist]] != 1} {
		    perror "\[add-route] $dst must have only one link."
		}
		set link  [lindex [$dst set portlist] 0]
		set mask  [$link get_netmask]
		set dstip [$dst ip 0]
		set type  "host"
	    }
	    "SimplexLink" {
		set link  [$dst set mylink]
		set mask  [$link get_netmask]
		set src   [$link set src_node]
		set dstip [$src ip [$src find_port $link]]
		set type  "net"
	    }
	    "Link" {
		set dstip [$dst get_subnet]
		set mask  [$dst get_netmask]
		set type  "net"
	    }
	    "Lan" {
		set dstip [$dst get_subnet]
		set mask  [$dst get_netmask]
		set type  "net"
	    }
	    unknown {
		perror "\[add-route] Bad argument. Must be a node or a link."
		return
	    }
	}
	$sim spitxml_data "virt_routes" [list "vname" "dst" "nexthop" "dst_type" "dst_mask"] [list $self $dstip $hopip $type $mask]
    }
}

#
# Create a program object to run on the node when the experiment starts.
#
Node instproc start-command {command} {
    $self instvar sim

    set newprog [new Program $sim]
    $newprog set node $self
    $newprog set command $command

    # Starts at time 0
    $sim at 0  "$newprog start"

    return $newprog
}

#
# Add a desire to the node, with the given weight
# Fails if the desire already exists, but maybe it could just update the
# weight?
#
Node instproc add-desire {desire weight} {
    $self instvar desirelist
    if {[info exists desirelist($desire)]} {
	perror "\[add-desire] Desire $desire on $self already exists!"
    }
    set desirelist($desire) $weight
}
