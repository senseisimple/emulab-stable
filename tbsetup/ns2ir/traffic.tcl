######################################################################
# traffic.tcl
#
#
# This defines the various agents and applications needed to support
# traffic generation.  Specifically it defines Agent/UDP, Agent/Null,
# and Application/Traffic/CBR.
#
# Added: TCP traffic generation using NSE. Defines
# Agent/TCP/FullTcp, Agent/TCP/FullTcp/Reno, Agent/TCP/FullTcp/Newreno,
# Agent/TCP/FullTcp/Tahoe, Agent/TCP/FullTcp/Sack,
# Application/FTP and Application/Telnet
######################################################################

Class Agent -superclass NSObject
Class Agent/UDP -superclass Agent
Class Agent/TCP -superclass Agent
Class Agent/Null -superclass Agent
Class Agent/TCPSINK -superclass Agent
Class Agent/TCP/FullTcp -superclass Agent
Class Agent/TCP/FullTcp/Reno -superclass Agent/TCP/FullTcp
Class Agent/TCP/FullTcp/Newreno -superclass Agent/TCP/FullTcp
Class Agent/TCP/FullTcp/Tahoe -superclass Agent/TCP/FullTcp
Class Agent/TCP/FullTcp/Sack -superclass Agent/TCP/FullTcp

Class Application -superclass NSObject
Class Application/Traffic/CBR -superclass Application
Class Application/FTP -superclass Application
Class Application/Telnet -superclass Application

namespace eval GLOBALS {
    set new_classes(Agent/UDP) {}
    set new_classes(Agent/Null) {}
    set new_classes(Agent/TCP) {}
    set new_classes(Agent/TCPSINK) {}
    set new_classes(Agent/TCP/FullTcp) {}
    set new_classes(Agent/TCP/FullTcp/Reno) {}
    set new_classes(Agent/TCP/FullTcp/Newreno) {}
    set new_classes(Agent/TCP/FullTcp/Tahoe) {}
    set new_classes(Agent/TCP/FullTcp/Sack) {}
    set new_classes(Application/Traffic/CBR) {}
    set new_classes(Application/FTP) {}
    set new_classes(Application/Telnet) {}
}

# Agent
Agent instproc init {} {
    $self set node {}
    $self set application {}
    $self set destination {}
    $self set proto {}
    $self set role {}
    $self set port {}
    $self set generator "TG"
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
    $self instvar generator
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
    sql exec $DB "insert into virt_trafgens (pid,eid,vnode,vname,role,proto,port,target_vnode,target_port,generator) values ('$pid','$eid','$node','$application','$role','$proto', $port,'$target_vnode',$target_port,'$generator')";

}

# get_nseconfig is only defined for subclasses that will be simulated by NSE
# Will be called from Node updatedb
Agent instproc get_nseconfig {} {
   return ""
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

# Agent/TCP/FullTcp
Agent/TCP/FullTcp instproc init args {

    eval $self next $args
    $self set simulated 1
    $self set tcptype ""
    $self set segsize_ 1460
    $self set role "source" 
    $self set generator "NSE"
}

# Agent/TCP/FullTcp/Newreno
Agent/TCP/FullTcp/Newreno instproc init args {
    eval $self next $args
    $self set tcptype "Newreno"
}

# Agent/TCP/FullTcp/Tahoe
Agent/TCP/FullTcp/Tahoe instproc init args {
    eval $self next $args
    $self set tcptype "Tahoe"
}

# Agent/TCP/FullTcp/Sack
Agent/TCP/FullTcp/Sack instproc init args {
    eval $self next $args
    $self set tcptype "Sack"
}

# Agent/TCP/FullTcp
Agent/TCP/FullTcp instproc listen {} {

   $self set role "sink"

}

# updatedb DB
# This adds rows to the virt_trafgens table corresponding to this agent.
# We do things differently from the base class version. For NSE, the
# vname in the table is the FullTcp agent object and not the application
# object connected to it. The important thing is using "$ns at", we can
# send events and control any of these objects
Agent/TCP/FullTcp instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar application
    $self instvar destination
    $self instvar node
    $self instvar proto
    $self instvar role
    $self instvar generator
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

    set vname $self

    # Update the DB
    sql exec $DB "insert into virt_trafgens (pid,eid,vnode,vname,role,proto,port,target_vnode,target_port,generator) values ('$pid','$eid','$node','$vname','$role','$proto', $port,'$target_vnode',$target_port,'$generator')";


}

# Agent/TCP/FullTcp
Agent/TCP/FullTcp instproc get_nseconfig {} {

    $self instvar tcptype
    $self instvar role
    $self instvar simulated
    $self instvar application
    
    set nseconfig ""

    # This agent could possibly be a real one in which case
    # we don't do NSE traffic generation
    if { $simulated != 1 } {
         return $nseconfig
    }

    if { ($tcptype == "") || ($tcptype == "Reno") } {
          set nseconfig "set $self \[new Agent/TCP/FullTcp]\n"
    } else {
          set nseconfig "set $self \[new Agent/TCP/FullTcp/$tcptype]\n"
    }
    
    if { $role == "sink" } {
         append nseconfig "\$$self listen\n\n"
    }

    # We end up including variables present only in TB parser. However
    # that does not matter coz all NS variables end with _ character
    # Pruning can be done later
    foreach var [$self info vars] {
           if { [$self set $var] != {} } {
             append nseconfig "\$$self set $var [$self set $var]\n"
           }
    }

    if { $application != {} } {
         append nseconfig [$application get_nseconfig]
    } 

    return $nseconfig
}


# Agent/TCP/FullTcp
Agent/TCP/FullTcp instproc connect {dst} {
    $self next $dst
    $self instvar node
    $self instvar application
    $self instvar destination
    $self instvar role

    set error 0
    if {$node == {}} {
        perror "\[connect] $self is not attached to a node."
        set error 1
    }
    if { ($role == "source") && ($application == {}) } {
        perror "\[connect] $self does not have an attached application."
        set error 1
    }
    set dest [$destination set node]
    if {$dest == {}} {
        perror "\[connect] $destination is not attached to a node."
        set error 1
    }

# This was an artifact of SEND-CONSUME CBR model. Now that we run freebsd on the
# traffic generator nodes, all we need to do is either enable OSPF routing by
# default or setup static routes
    if {[llength [$node set portlist]] != 1} {
        perror "\[connect] $node must have exactly one link to be a traffic generator."
        set error 1
    }
    if {$error} {return}

    $self set proto "tcp"
    $dst set proto "tcp"
    $node set osid "FBSD-STD"
    $dest set osid "FBSD-STD"
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

# Application/FTP
Application/FTP instproc init {} {
    $self next
   
    $self set role "source"
}

# Application/FTP
Application/FTP instproc get_nseconfig {} {

    $self instvar agent
    set nseconfig "set $self \[new Application/FTP]\n"

    if { $agent != {} } {
         append nseconfig "\$$self attach-agent \$$agent\n\n"
    }

    # XXX temporary untill event system changes get in
    append nseconfig "\[Simulator instance] at 30.0 \"\$$self start\"\n\n"

   return $nseconfig
}

# Application/Telnet
Application/Telnet instproc init {} {
    $self next
    $self instvar role
    $self instvar interval_

    $self set role "source"

# NS's default is 1.0 and therefore would result in inter-packet times
# to be chosen from the exponential distribution. If it is 0, it would
# be chosen based on the tcplib distribution. I differ from NS in my
# default because of the realism of tcplib distribution
    $self set interval_ 0.0
}

# Application/Telnet
Application/Telnet instproc get_nseconfig {} {
   
    $self instvar agent
    $self instvar interval_

    set nseconfig "set $self \[new Application/Telnet]\n"
               
    append nseconfig "\$$self set interval_ $interval_\n"
    if { $agent != {} } {
         append nseconfig "\$$self attach-agent \$$agent\n\n"
    }
        
    # XXX temporary untill event system changes get in
    append nseconfig "\[Simulator instance] at 30.0 \"\$$self start\"\n\n"

    return $nseconfig
}
