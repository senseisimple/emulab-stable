######################################################################
# traffic.tcl
#
#
# This defines the various agents and applications needed to support
# traffic generation.  Specifically it defines Agent/UDP, Agent/Null,
# and Application/Traffic/CBR.
######################################################################

Class Agent -superclass NSObject
Class Agent/UDP -superclass Agent
Class Agent/TCP -superclass Agent
Class Agent/Null -superclass Agent
Class Agent/TCPSINK -superclass Agent

Class Application -superclass NSObject
Class Application/Traffic/CBR -superclass Application

namespace eval GLOBALS {
    set new_classes(Agent/UDP) {}
    set new_classes(Agent/Null) {}
    set new_classes(Agent/TCP) {}
    set new_classes(Agent/TCPSINK) {}
    set new_classes(Application/Traffic/CBR) {}
}

# Agent
Agent instproc init {} {
    $self set node {}
    $self set application {}
    $self set destination {}
    $self set proto {}
    $self set role {}
    $self set port {}
    global ::GLOBALS::last_class
    set ::GLOBALS::last_class $self
}
Agent instproc set_node {node} {
    $self set node $node
    $self set port [$node next_portnumber]
}
Agent instproc get_node {} {
    $self instvar node
    return $node
}
Agent instproc set_application {application} { 
    $self set application $application
    $self set role [$application set role]
    
}
Agent instproc connect {dst} {
    $self instvar destination
    if {$destination != {}} {
	perror "\[connect] $self already has a destination: $destination."
	return
    }
    set destination $dst
}
Agent instproc rename {old new} {
    $self instvar application
    # In normal conditions this will never occur.
    if {$application != {}} {
	$application set agent $new
    }
}

# updatedb DB
# This adds rows to the virt_trafgens table corresponding to this agent.
Agent instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar application
    $self instvar destination
    $self instvar node
    $self instvar proto
    $self instvar role
    $self instvar port

    if {$role == {}} {
	perror "\[updatedb] $self has no role."
	return
    }
    if {$destination == {}} {
	perror "\[updatedb] $self has no destination."
	return
    }
    set target_vnode [$destination set node]
    set target_port [$destination set port]

    if {$role == "sink"} {
	set application [$destination set application]
	set proto [$destination set proto]
    }
#   set src_link [lindex [$node set portlist] 0]
#   set dst_link [lindex [$target_vnode set portlist] 0]

    # Update the DB
    sql exec $DB "insert into virt_trafgens (pid,eid,vnode,vname,role,proto,port,target_vnode,target_port) values ('$pid','$eid','$node','$application','$role','$proto', $port,'$target_vnode',$target_port)";
}

# Agent/UDP 
Agent/UDP instproc connect {dst} {
    $self next $dst
    $self instvar node
    $self instvar application
    $self instvar destination

    set error 0
    if {$node == {}} {
	perror "\[connect] $self is not attached to a node."
	set error 1
    }
    if {$application == {}} {
	perror "\[connect] $self does not have an attached application."
	set error 1
    }
    set dest [$destination set node]
    if {$dest == {}} {
	perror "\[connect] $destination is not attached to a node."
	set error 1
    }
    if {[llength [$node set portlist]] != 1} {
	perror "\[connect] $node must have exactly one link to be a traffic generator."
	set error 1
    }
    if {$error} {return}

    $self set proto "udp"
    $node set osid "FBSD-STD"
}

# Agent/TCP
Agent/TCP instproc connect {dst} {
    $self next $dst
    $self instvar node
    $self instvar application
    $self instvar destination

    set error 0
    if {$node == {}} {
	perror "\[connect] $self is not attached to a node."
	set error 1
    }
    if {$application == {}} {
	perror "\[connect] $self does not have an attached application."
	set error 1
    }
    set dest [$destination set node]
    if {$dest == {}} {
	perror "\[connect] $destination is not attached to a node."
	set error 1
    }
    if {[llength [$node set portlist]] != 1} {
	perror "\[connect] $node must have exactly one link to be a traffic generator."
	set error 1
    }
    if {$error} {return}

    $self set proto "tcp"
    $node set osid "FBSD-STD"
}

# Agent/Null
Agent/Null instproc connect {dst} {
    $self next $dst
    $self instvar node
    $self instvar application
    $self instvar destination
    set error 0
    if {$node == {}} {
	perror "\[connect] $self is not attached to a node."
	set error 1
    }
    set dest [$destination set node]
    if {$dest == {}} {
	perror "\[connect] $destination is not attached to a node."
	set error 1
    }
    if {[llength [$node set portlist]] != 1} {
	perror "\[connect] $node must have exactly one link to be a traffic consumer."
	set error 1
    }
    if {$error} {return}

    $self set role "sink"
    $node set osid "FBSD-STD"
}

# Agent/Null
Agent/TCPSINK instproc connect {dst} {
    $self next $dst
    $self instvar node
    $self instvar application
    $self instvar destination
    set error 0
    if {$node == {}} {
	perror "\[connect] $self is not attached to a node."
	set error 1
    }
    set dest [$destination set node]
    if {$dest == {}} {
	perror "\[connect] $destination is not attached to a node."
	set error 1
    }
    if {[llength [$node set portlist]] != 1} {
	perror "\[connect] $node must have exactly one link to be a traffic consumer."
	set error 1
    }
    if {$error} {return}

    $self set role "sink"
    $self set proto "tcp"
    $node set osid "FBSD-STD"
}

# Application
Application instproc init {} {
    $self set agent {}
    $self set role {}
    global ::GLOBALS::last_class
    set ::GLOBALS::last_class $self
}
Application instproc attach-agent {agent} {
    $self set agent $agent
    $agent set_application $self
}
Application instproc get_node {} {
    $self instvar agent
    return [$agent get_node]
}
Application instproc rename {old new} {
    $self instvar agent
    # In normal condition this will never occur.
    if {$agent != {}} {
	$agent set_application $self
    }
}

# Application/Traffic/CBR
Application/Traffic/CBR instproc init {} {
    $self set packetSize_ 210
    $self set rate_ "100Mbps"
    $self set interval_ {}
    $self next
    
    $self set role "source"
}

Application/Traffic/CBR instproc get_params {} {
    $self instvar packetSize_
    $self instvar rate_
    $self instvar interval_

    if {$rate_ != {} && $rate_ != 0} {
	set rate [parse_bw $rate_]
    } else {
	set rate -1
    }
    set param "PACKETSIZE=$packetSize_ RATE=$rate"

    if {$interval_ != {} && $interval_ != 0} {
	set param "$param INTERVAL=$interval_"
    } else {
	set param "$param INTERVAL=-1"
    }
    return $param
}
