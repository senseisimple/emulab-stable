# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# nse.sim.tcl
#
# Defines the Simulator class.  For our purpose a Simulator is a
# topology.  This contains a number nodes, lans, and links.  It
# provides methods for the creation of these objects as well as
# routines to locate the objects.  It also stores common state (such
# as IP subnet usage).  Finally it defines the import 'run' method
# which causes all remaining calculations to be done and updates the
# DB state.
#
# Note: Although NS distinguishs between LANs and Links, we do not.
# We merge both types of objects into a single one called LanLink.  
# See lanlink.tcl and README for more information.
######################################################################

Class Simulator

Simulator instproc init {args} {
    var_import GLOBALS::last_class
    var_import GLOBALS::last_cmd

    $self set id_counter 0

    $self set nseconfig {}
    $self set classname "Simulator"
    $self set objname $self

    $self instvar classname

    if { $last_cmd == {} } {
	if { $args == {} } {
	    $self set createcmd "\[new $classname]"
	} else {
	    $self set createcmd "\[new $classname $args]"
	}
    } else {
	$self set createcmd $last_cmd
    }

    real_set last_class $self

}

Simulator instproc inet-aton ipaddr {
    real_set c [regsub -all \\. $ipaddr " " addrlist]
    if { $c == 0} { 
        return [expr $ipaddr]
    }
    if { [llength $addrlist] != 4} {
        puts "Ill-formatted ip address $ipaddr, addrlistlth = [llength $addrlist]"
        return 0
    }
    if { $c == 3 } {
        real_set r 0
        foreach a $addrlist {
            if { $a < 0 || $a > 255 } {
                puts "Ill-formatted ip address $ipaddr, a = $a"
                return 0
            }
            real_set r [expr ($r << 8) + $a]     
        }
        return $r
    } else {
        puts "Ill-formatted ip address $ipaddr, c = $c"
        return 0
    }
}

# XXX netbed changes
Simulator instproc inet-ntoa ipaddr {
    real_set p1 [expr ($ipaddr >> 24) & 0xff]
    real_set p2 [expr ($ipaddr >> 16) & 0xff]
    real_set p3 [expr ($ipaddr >>  8) & 0xff]
    real_set p4 [expr ($ipaddr >>  0) & 0xff]
    return [format "%d\.%d\.%d\.%d" $p1 $p2 $p3 $p4]
}

# unknown 
Simulator instproc unknown {m args} {
    $self instvar nseconfig
    $self instvar objname

    append nseconfig "\$$objname $m $args\n"
}

# node
# This method adds a new node to the topology and returns the id
# of the node.
Simulator instproc node {args} {
    var_import ::GLOBALS::last_class
    var_import ::GLOBALS::last_cmd
    $self instvar id_counter
    $self instvar objname
    
    real_set curnode tbnode-n$id_counter
    if { $args == {} } {
	real_set last_cmd "\[\$$objname node]"
	Node $curnode
    } else {
	real_set last_cmd "\[\$$objname node $args]"
	Node $curnode $args
    }    
    incr id_counter

    return $curnode
}

# duplex-link <node1> <node2> <bandwidth> <delay> <type>
# This adds a new link to the topology.  <bandwidth> can be in any
# form accepted by parse_bw and <delay> in any form accepted by
# parse_delay.  Currently only the type 'DropTail' is supported.
Simulator instproc duplex-link {n0 n1 bw delay type args} {
    var_import ::GLOBALS::v2type
    var_import ::GLOBALS::nseconfiglanlinks
    var_import ::GLOBALS::nseconfigrlinks
    var_import ::GLOBALS::ips
    var_import ::GLOBALS::v2pmap
    var_import ::GLOBALS::v2vmap
    var_import ::GLOBALS::link_map
    var_import ::GLOBALS::lanlinks
    $self instvar objname

    real_set n0n1 [lsort [list [$n0 set objname] [$n1 set objname]]]
    real_set n0name [lindex $n0n1 0]
    real_set n1name [lindex $n0n1 1]
    real_set n0v2vname $v2vmap($n0name)
    real_set n1v2vname $v2vmap($n1name)

    # figure out what to do if n0 or n1 isn't a simulated node
    if { $v2type($n0v2vname) != "sim" || $v2type($n1v2vname) != "sim" } {
	# just ignore coz the info was collected during the 1st parse
	# At the end of this parse, we just use the DB information and construct
	# the information
	# We could possibly add nsenode_ipaddrlist or sthing like that
	return {}
    }

    # if n0 and n1 are in the same partition, we need to 
    # add to nseconfigs for that partition. Otherwise
    # we need to create a special kind of link in both
    # partitions
    real_set linkname $link_map($n0v2vname:$n1v2vname)
    real_set n0pnode $v2pmap($n0v2vname)
    real_set n1pnode $v2pmap($n1v2vname)
    real_set nodeport0 [lindex $lanlinks($linkname) 0]
    real_set nodeport1 [lindex $lanlinks($linkname) 1]
    scan $nodeport0 "%\[^:]:%s" node0 port0
    scan $nodeport1 "%\[^:]:%s" node1 port1

    if { $node0 == $n0v2vname } {
	real_set n0remoteip $ips("$node1:$port1")
	real_set n1remoteip $ips("$node0:$port0")
    } else {
	real_set n0remoteip $ips("$node0:$port0")
	real_set n1remoteip $ips("$node1:$port1")
    }
    real_set n0ip $n1remoteip
    real_set n1ip $n0remoteip

    if { $n0pnode == $n1pnode } {
	append nseconfiglanlinks($n0pnode) \
		"set $linkname \[\$$objname duplex-link \$$n0name \$$n1name $bw $delay $type $args]\n"
	append nseconfiglanlinks($n0pnode) \
		"\[\$$objname link \$$n0name \$$n1name\] set-ip $n0ip\n"
	append nseconfiglanlinks($n1pnode) \
		"\[\$$objname link \$$n1name \$$n0name\] set-ip $n1ip\n"
    } else {
	append nseconfigrlinks($n0pnode) \
		"set $linkname \[\$$objname rlink \$$n0name $n1ip $bw $delay $type $args]\n"
	append nseconfigrlinks($n0pnode) "\$\{$linkname\} set-ip $n0ip\n"
	append nseconfigrlinks($n1pnode) \
		"set $linkname \[\$$objname rlink \$$n1name $n0ip $bw $delay $type $args]\n"
	append nseconfigrlinks($n1pnode) "\$\{$linkname\} set-ip $n1ip\n"
    }

    return {}
}

# make-lan <nodelist> <bw> <delay>
# This adds a new lan to the topology. <bandwidth> can be in any
# form accepted by parse_bw and <delay> in any form accepted by
# parse_delay.
Simulator instproc make-lan {nodelist bw delay args} {
    var_import ::GLOBALS::v2type
    var_import ::GLOBALS::nseconfiglanlinks
    $self instvar lan_map
    $self instvar objname

    # figure out what to do if one of the nodes is not a sim node 
    foreach node $nodelist {
	if { $v2type([$node set objname]) != "sim" } {
	    return {}
	}
	lappend nodenamelist [$node set objname]
    }

    real_set lanstring [join [lsort $nodenamelist] ":"]
    real_set lan $lan_map($lanstring)
}

# run is ignored here coz it shouldn't
# happen inside make-simulated. On the
# client side, nseinput.tcl automatically
# does call run
Simulator instproc run {} {
}

# attach-agent <node> <agent>
# This creates an attachment between <node> and <agent>.
Simulator instproc attach-agent {node agent} {
    var_import ::GLOBALS::vnodetoip
    var_import ::GLOBALS::v2vmap
    $self instvar objname
    $agent instvar nseconfig

    $agent set_node $node
    real_set nodename $v2vmap([$node set objname])
    $agent set ip [lindex $vnodetoip($nodename) 0]
    real_set port [$node nextportnumber]
    $agent set port $port

    append nseconfig "\$$objname attach-agent \$[$node set objname] \$[$agent set objname] $port\n"
}

# connect <src> <dst>
# Connects two agents together.
Simulator instproc connect {src dst} {
    $src connect $dst
    $dst connect $src
}


# at <time> <event>
# Known events:
#   <traffic> start
#   <traffic> stop
#   <link> up
#   <link> down
#   ...
Simulator instproc at { time eventstring } {

    var_import ::GLOBALS::objtypes
    var_import ::GLOBALS::eventtypes
    var_import ::GLOBALS::event_list
    $self instvar objname
    real_set eventlist [split $eventstring ";"]
    real_set otype $objtypes(NSE)
    real_set etype $eventtypes(NSEEVENT)

    # Check that time is float
    if {[regexp {(^[0-9]+(\.[0-9]+)?$)|(^\.[0-9]+$)} $time] == 0} {
	perror "Invalid time spec: $time"
	return
    }

    foreach event $eventlist {
	# Check otype/command
	real_set obj [lindex $event 0]
	real_set atstring "$event"
	if { [catch {real_set cls [$obj info class]}] == 1 } {
	    punsup "Invalid event specification. $obj is not an object\n"
	    continue
	}
	if { $cls == "NullClass" } {
	    punsup "Invalid event specification. Events for objects of type [$obj set type] unsupported\n"
	    continue
	}
	if { $cls == "Simulator" } {
	    punsup "Invalid event specification. Events for objects of type $cls unsupported\n"
	    continue
	}

	switch -- $cls {
	    "Node" -
	    "Agent" -
	    "Application" {
		set args "\$[$obj set objname] [lrange $event 1 end]"
		set vname $obj
	    }
	    "Link" -
	    "Lan" {
		# Links and Lans need to be treated differently
		# We need to have as many events as there are
		# partitions in which these links are present. For
		# example, a duplex link could be split across
		punsup "Invalid event specification. Currently unsupported\n"
		continue
	    }
	}

	lappend event_list($vname) [list $time $otype $etype $args $atstring]
    }
}

#
# Spit out XML
#
Simulator instproc spitxml_init {} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid

    puts "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
    puts "<virtual_experiment pid='$pid' eid='$eid'>"
}

Simulator instproc spitxml_finish {} {
    puts "</virtual_experiment>"
}

Simulator instproc spitxml_data {tag fields values} {
    ::spitxml_data $tag $fields $values
}

proc spitxml_data {tag fields values} {
    puts "  <$tag>"
    puts "    <row>"
    foreach field $fields {
	set value  [lindex $values 0]
	set values [lrange $values 1 end]
	set value_esc [xmlencode $value]

	puts "      <$field>$value_esc</$field>"
    }
    puts "    </row>"
    puts "  </$tag>"
}

proc xmlencode {args} {
    set retval [eval append retval $args]
    regsub -all "&" $retval "\\&amp;" retval
    regsub -all "<" $retval "\\&lt;" retval
    regsub -all ">" $retval "\\&gt;" retval
    regsub -all "\"" $retval "\\&\#34;" retval
    regsub -all "]" $retval "\\&\#93;" retval
    
    return $retval
}
