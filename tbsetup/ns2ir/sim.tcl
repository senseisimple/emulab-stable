# this was part of ns as tcl/lib/ns-lib.tcl.
# It contains many of the Simulator methods.
# for the testbed, we are replacing ns behavior with, well, testbed behavior

#
# Copyright (c) 1996 Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the MASH Research
# 	Group at the University of California Berkeley.
# 4. Neither the name of the University nor of the Research Group may be
#    used to endorse or promote products derived from this software without
#    specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

# @(#) $Header: /home/cvs_mirrors/cvs-public.flux.utah.edu/CVS/testbed/tbsetup/ns2ir/Attic/sim.tcl,v 1.3 2000-12-26 20:10:09 calfeld Exp $

#

#
# Word of warning to developers:
# this code (and all it sources) is compiled into the
# ns executable.  You need to rebuild ns or explicitly
# source this code to see changes take effect.
#
# Create the core OTcl class called "Simulator".
# This is the principal interface to the simulation engine.
#
Class Simulator

#ROB added the following
Simulator instproc set-address-format {opt args} {
}
#---------

Simulator instproc init args {
}

Simulator instproc nullagent {} {
    return nop
}

Simulator instproc use-scheduler type {
}

#
# A simple method to wrap any object around
# a trace object that dumps to stdout
#
Simulator instproc dumper obj {
}

# delaynode. has bandwidth and delay.

Simulator instproc delaynode {b d} {
    set currnode [$self node 0]
    $currnode set type delay
    $currnode set bw $b
    $currnode set delay $d
    return $currnode
}

# NODE

Simulator instproc node args {
    global nodeID
    global nodelist

    set currnode n$nodeID
    node $currnode

    #set them all to pc now. other code may change it, however
    $currnode set type pc
    $currnode set id $nodeID
    $currnode set nodelinks [list]
    incr nodeID

    lappend nodelist $currnode
    return $currnode
}


Simulator instproc hier-node haddr {
}

Simulator instproc now {} {
    return nop
}

# AT

Simulator instproc at args {
    #args should look like: time op node node
    #op is: 'up' or 'down'. ns lets you give arbitrary commands for op, but
    #we ignore those...
    global eventID
    global eventlist
    set currEvent ev$eventID
    event $currEvent
    $currEvent set time [lindex $args 0]

    #don't tackle arbitrary commands yet...
    switch [lindex $args 1] {
	up {$currEvent set op link_up}
	down {$currEvent set op link_down}
	default {return}
    }

    #we can't even guarantee that a link is found, since a pair of
    #nodes is used to specify the link. perhaps nobody had the sense
    #to actually link those nodes...
    $currEvent set link No_link_set.

    # try to find link
    foreach currlink [[lindex $args 2] set nodelinks] {
	if {[$currlink set src] == [lindex $args 3] ||  \
		[$currlink set dst] == [lindex $args 3] } {
	    $currEvent set link $currlink
	    break
	}
    }

    lappend eventlist $currEvent
    return nop
}

Simulator instproc at-now args {
    return nop
}

Simulator instproc cancel args {
    return nop
}

Simulator instproc after {ival args} {
}

#
# check if total num of nodes exceed 2 to the power n 
# where <n=node field size in address>
#
Simulator instproc check-node-num {} {
}

#
# Check if number of items at each hier level (num of nodes, or clusters or domains)
# exceed size of that hier level field size (in bits). should be modified to support 
# n-level of hierarchies
#
Simulator instproc chk-hier-field-lengths {} {
}

# RUN

Simulator instproc run {} {
    global nodelist
    global linkslist
    global eventlist
    global lanlist
    global irfile

    set IRfile [open $irfile w]

    puts $IRfile "START topology\nSTART nodes"
    foreach node $nodelist {
	$node print $IRfile
    }
    puts $IRfile "END nodes\nSTART links"
    foreach link $linkslist {
	$link print $IRfile
    }
    puts $IRfile "END links\nEND topology\nSTART events"
    foreach event $eventlist {
	$event print $IRfile
    }
    puts $IRfile "END  events"
    puts $IRfile "START lans"
    foreach lan $lanlist {
	$lan print $IRfile
    }
    puts $IRfile "END lans"

    close $IRfile
}

Simulator instproc halt {} {
}

Simulator instproc dumpq {} {
}

Simulator instproc is-started {} {
    return nop
}

Simulator instproc clearMemTrace {} {
}

Simulator instproc simplex-link { n1 n2 bw delay qtype args } {

    $self duplex-link $n1 $n2 $bw $delay $qtype $args
}

#
# This is used by Link::orient to register/update the order in which links 
# should created in nam. This is important because different creation order
# may result in different layout.
#
# A poor hack. :( Any better ideas?
#
Simulator instproc register-nam-linkconfig link {
}

#
# GT-ITM may occasionally generate duplicate links, so we need this check
# to ensure duplicated links do not appear in nam trace files.
#
Simulator instproc remove-nam-linkconfig {i1 i2} {
}

Simulator instproc duplex-link { n1 n2 bw delay type args } {
    global linkID
    global linkslist
    global nodeID

    # if there are delay or bandwidth restrictions, add a delay node
    # and link to it
    #    if {$delay!="" && $delay!=0  || $bw!="" && $bw!=0} {
    #	#delaynode is not a 'real' Sim method. created for testbed.
    #	set dnode [$self delaynode [$self bw_parse $bw] [$self delay_parse $delay]]
    #
    #	$self duplex-link $n1 $dnode 0 0 $type $args
    #	$self duplex-link $n2 $dnode 0 0 $type $args
    #    }

    set currLink l$linkID
    link $currLink
    $currLink set src $n1
    $currLink set srcport -1
    $currLink set dst $n2
    $currLink set dstport -1
    $currLink set delay $delay
    $currLink set bw $bw

    $currLink set id $linkID
    $n1 addlink $currLink
    $n2 addlink $currLink

    incr linkID

    lappend linkslist $currLink
}

# ROB added
Simulator instproc duplex-link-of-interfaces { n1 n2 bw delay type args } {
    $self duplex-link $n1 $n2 $bw $delay $type $args
}
#----------

Simulator instproc duplex-intserv-link { n1 n2 bw pd sched signal adc args } {
}

Simulator instproc simplex-link-op { n1 n2 op args } {
}

Simulator instproc duplex-link-op { n1 n2 op args } {
}

Simulator instproc flush-trace {} {
}

Simulator instproc namtrace-all file {
}

Simulator instproc namtrace-some file {
}

Simulator instproc trace-all file {
}

Simulator instproc get-nam-traceall {} {
    return nop
}

Simulator instproc get-ns-traceall {} {
    return nop
}

# If exists a traceAllFile_, print $str to $traceAllFile_
Simulator instproc puts-ns-traceall { str } {
}

# If exists a traceAllFile_, print $str to $traceAllFile_
Simulator instproc puts-nam-traceall { str } {
}

# namConfigFile is used for writing color/link/node/queue/annotations. 
# XXX It cannot co-exist with namtraceAll.
Simulator instproc namtrace-config { f } {
}

Simulator instproc get-nam-config {} {
    return nop
}

# Used only for writing nam configurations to trace file(s). This is different
# from puts-nam-traceall because we may want to separate configuration 
# informations and actual tracing information
Simulator instproc puts-nam-config { str } {
}

Simulator instproc color { id name } {
}

Simulator instproc get-color { id } {
    return nop
}

# you can pass in {} as a null file
Simulator instproc create-trace { type file src dst {op ""} } {
}

Simulator instproc namtrace-queue { n1 n2 {file ""} } {
}

Simulator instproc trace-queue { n1 n2 {file ""} } {
}

#
# arrange for queue length of link between nodes n1 and n2
# to be tracked and return object that can be queried
# to learn average q size etc.  XXX this API still rough
#
Simulator instproc monitor-queue { n1 n2 qtrace { sampleInterval 0.1 } } {
    return nop
}

Simulator instproc queue-limit { n1 n2 limit } {
}

Simulator instproc drop-trace { n1 n2 trace } {
}

Simulator instproc cost {n1 n2 c} {
}

Simulator instproc attach-agent { node agent } {
}

Simulator instproc attach-tbf-agent { node agent tbf } {
}


Simulator instproc detach-agent { node agent } {
}

#
#   Helper proc for setting delay on an existing link
#
Simulator instproc delay { n1 n2 delay } {
}

#XXX need to check that agents are attached to nodes already
Simulator instproc connect {src dst} {
    return nop
}

Simulator instproc simplex-connect { src dst } {
    return nop
}

#
# Here are a bunch of helper methods.
#


Simulator proc instance {} {
    return nop
}

Simulator instproc get-node-by-id id {
}

Simulator instproc all-nodes-list {} {
}

Simulator instproc link { n1 n2 } {
    return nop
}

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns the source agent.
Simulator instproc create-connection {s_type source d_type dest pktClass} {
    return nop
}

# Creates connection. First creates a source agent of type s_type and binds
# it to source.  Next creates a destination agent of type d_type and binds
# it to dest.  Finally creates bindings for the source and destination agents,
# connects them, and  returns a list of source agent and destination agent.
Simulator instproc create-connection-list {s_type source d_type dest pktClass} {
    return nop
}   

# This seems to be an obsolete procedure.
Simulator instproc create-tcp-connection {s_type source d_type dest pktClass} {
    return nop
}

Simulator instproc makeflowmon { cltype { clslots 29 } } {
    return nop
}

# attach a flow monitor to a link
# 3rd argument dictates whether early drop support is to be used

Simulator instproc attach-fmon {lnk fm { edrop 0 } } {
}

# Imported from session.tcl. It is deleted there.
### to insert loss module to regular links in detailed Simulator
Simulator instproc lossmodel {lossobj from to} {
}

# This function generates losses that can be visualized by nam.
Simulator instproc link-lossmodel {lossobj from to} {
}

Simulator instproc bw_parse { bspec } {
    if { [scan $bspec "%f%s" b unit] == 1 } {
	set unit b
    }
    # xxx: all units should support X"ps" --johnh
    switch $unit {
        b  { return [expr int($b/1000000)] }
        bps  { return [expr int($b/1000000] }
	    kb { return [expr int($b/1000)] }
	    Mb { return [expr int($b)] }
	    Gb { return [expr int($b*1000)] }
	    default { 
		puts "error: bw_parse: unknown unit `$unit'" 
		exit 1
	    }
        }
    }
    
    Simulator instproc delay_parse { dspec } {
	if { [scan $dspec "%f%s" b unit] == 1 } {
	    set unit s
        }
        switch $unit {
	    s  { return [expr int($b*1000)] }
	    ms { return [expr int($b)] }
	    ns { return [expr int($b/1000)] }
	    default { 
		puts "error: bw_parse: unknown unit `$unit'" 
		exit 1
	    }
        }
    }

######################################################################
# calfeld
# Lan code
######################################################################
    Simulator instproc make-lan {nodelist bw delay 
	{llType LL} 
	{ifqType Queue/DropTail} 
	{macType Mac} 
	{chanType Channel} 
	{phyType Phy/WiredPhy}} {
	    global lanlist
	    global lanID		       

	    foreach node $nodelist {
		if {[$node getLan] != {}} {
		    throw "$node already in a LAN!"
		}
	    }
	    
	    set currlan lan$lanID
	    lan $currlan
	    $currlan set nodes $nodelist
	    $currlan set bw $bw
	    $currlan set delay $delay
	    $currlan set id $lanID
	    
	    foreach node $nodelist {
		$node setLan $currlan
	    }
	    
	    lappend lanlist $currlan
	    
	    incr lanID

	    return $currlan
	}

				       

					 