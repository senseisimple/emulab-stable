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
Class Agent/Null -superclass Agent

Class Application -superclass NSObject
Class Application/Traffic/CBR -superclass Application

namespace eval GLOBALS {
    set new_classes(Agent/UDP) {}
    set new_classes(Agent/Null) {}
    set new_classes(Application/Traffic/CBR) {}
}

# Agent
Agent instproc init {} {
    $self set node {}
    $self set application {}
    $self set destination {}
}
Agent instproc set_node {node} {
    $self set node $node
}
Agent instproc set_application {application} { 
    $self set application $application
}
Agent instproc connect {dst} {
    $self instvar destination
    if {$destination != {}} {
	perror "\[connect] $self already has a destination: $destination."
	return
    }
    set destination $dst
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
    set gateport [lindex [$node set portlist] 0]
    set gate {}
    foreach nodeport [$gateport set nodelist] {
	set n [lindex $nodeport 0]
	if {$n != $node} {
	    set gate $n
	    break
	}
    }
    if {$gate == {}} {
	perror "\[connect] No gateway found for $node."
	set error 1
    }
    if {$error} {return}

    $node set osid "SEND"
    
    set interval [$application set interval_]
    set packetsize [$application set packetSize_]
    if {$interval != {}} {
	set pktrate [expr int(1.0/$interval)]
    } else {
	set rate [parse_bw [$application set rate_]]
	set pktrate [expr int(($rate*1024*1024) / (8*$packetsize))]
    }
    
    $node set cmdline "DST_NAME=$dest GATE_NAME=$gate PKT_SIZE=$packetsize PKT_RATE=$pktrate"
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
    set gateport [lindex [$node set portlist] 0]
    set gate {}
    foreach nodeport [$gateport set nodelist] {
	set n [lindex $nodeport 0]
	if {$n != $node} {
	    set gate $n
	    break
	}
    }
    if {$gate == {}} {
	perror "\[connect] No gateway found for $node."
	set error 1
    }
    if {$error} {return}

    $node set osid "CONSUME"
    $node set cmdline "DST_NAME=$dest GATE_NAME=$gate"
}

# Application
Application instproc init {} {
    $self set agent {}
}
Application instproc attach-agent {agent} {
    $self set agent $agent
    $agent set_application $self
}

# Application/Traffic/CBR
Application/Traffic/CBR instproc init {} {
    $self set packetSize_ 210
    $self set rate_ "100Mbps"
    $self set interval_ {}
    $self next
}
