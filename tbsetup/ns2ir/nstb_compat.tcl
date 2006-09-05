# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

# This is a nop tb_compact.tcl file that should be used when running scripts
# under ns.

namespace eval GLOBALS {
    variable security_level 0
    variable pid {}
    variable gid {}
    variable eid {}
}

proc tb-set-ip {node ip} {}
proc tb-set-ip-interface {src dst ip} {}
proc tb-set-ip-link {src link ip} {}
proc tb-set-ip-lan {src lan ip} {}
proc tb-set-netmask {lanlink netmask} {}
proc tb-set-hardware {node type args} {}
proc tb-set-node-os {node os} {}
proc tb-set-link-loss {src args} {}
proc tb-set-lan-loss {lan rate} {}
proc tb-set-node-rpms {node args} {}
proc tb-set-node-startup {node cmd} {}
proc tb-set-node-cmdline {node cmd} {}
proc tb-set-node-tarfiles {node args} {}
proc tb-set-node-lan-delay {node lan delay} {}
proc tb-set-node-lan-bandwidth {node lan bw} {}
proc tb-set-node-lan-loss {node lan loss} {}
proc tb-set-node-lan-params {node lan delay bw loss} {}
proc tb-set-node-failure-action {node type} {}
proc tb-set-ip-routing {type} {}
proc tb-fix-node {v p} {}
proc tb-make-weighted-vtype {name weight types} {}
proc tb-make-soft-vtype {name types} {}
proc tb-make-hard-vtype {name types} {}
proc tb-set-lan-simplex-params {lan node todelay tobw toloss fromdelay frombw fromloss} {}
proc tb-set-link-simplex-params {link src delay bw loss} {}
proc tb-set-uselatestwadata {onoff} {}
proc tb-set-usewatunnels {onoff} {}
proc tb-set-wasolver-weights {delay bw plr} {}
proc tb-use-endnodeshaping {onoff} {}
proc tb-force-endnodeshaping {onoff} {}
proc tb-set-multiplexed {link onoff} {}
proc tb-set-endnodeshaping {link onoff} {}
proc tb-set-noshaping {link onoff} {}
proc tb-set-useveth {link onoff} {}
proc tb-set-link-encap {link style} {}
proc tb-set-allowcolocate {lanlink onoff} {}
proc tb-set-colocate-factor {factor} {}
proc tb-set-sync-server {node} {}
proc tb-set-mem-usage {usage} {}
proc tb-set-cpu-usage {usage} {}
proc tb-bind-parent {sub phys} {}
proc tb-fix-current-resources {onoff} {}
proc tb-set-encapsulate {onoff} {}
proc tb-set-vlink-emulation {style} {}
proc tb-set-sim-os {os} {}
proc tb-set-jail-os {os} {}
proc tb-set-delay-os {os} {}
proc tb-set-delay-capacity {cap} {}
proc tb-use-ipassign {onoff} {}
proc tb-set-ipassign-args {args} {}
proc tb-set-lan-protocol {lanlink protocol} {}
proc tb-set-lan-accesspoint {lanlink node} {}
proc tb-set-lan-setting {lanlink capkey capval} {}
proc tb-set-node-lan-setting {lanlink node capkey capval} {}
proc tb-use-physnaming {onoff} {}
proc tb-feedback-vnode {vnode hardware args} {}
proc tb-feedback-vlan {vnode lan args} {}
proc tb-feedback-vlink {link args} {}
proc tb-elab-in-elab {onoff} {}
proc tb-elab-in-elab-topology {topo} {}
proc tb-set-inner-elab-eid {eid} {}
proc tb-set-elabinelab-cvstag {cvstag} {}
proc tb-set-node-inner-elab-role {node role} {}
proc tb-set-node-id {vnode myid} {}
proc tb-set-link-est-bandwidth {srclink args} {}
proc tb-set-lan-est-bandwidth {lan bw} {}
proc tb-set-node-lan-est-bandwidth {node lan bw} {}
proc tb-set-node-plab-role {node role} {}

proc tb-set-security-level {level} {

    switch -- $level {
	"Green" {
	    set level 0
	}
	"Blue" {
	    set level 1
	}
	"Yellow" {
	    set level 2
	}
	"Orange" {
	    set level 3
	}
	"Red" {
	    perror "\[tb-set-security-level] Red security not implemented yet"
	    return
	}
	unknown {
	    perror "\[tb-set-security-level] $level is not a valid level"
	    return
	}
    }
    set ::GLOBALS::security_level $level
}

#
# Set the startup command for a node. Replaces the tb-set-node-startup
# command above, but we have to keep that one around for a while. This
# new version dispatched to the node object, which uses a program object.
# 
proc tb-set-node-startcmd {node command} {
    if {[$node info class] != "Node"} {
	perror "\[tb-set-node-startcmd] $node is not a node."
	return
    }
    set command "($command ; /usr/local/etc/emulab/startcmddone \$?)"
    set newprog [$node start-command $command]

    return $newprog
}

#
# Create a program object to run on the node when the experiment starts.
#
Node instproc start-command {command} {
    global hosts

    $self instvar sim
    set newname "$hosts($self)_startcmd"

    set newprog [uplevel 2 "set $newname [new Program]"]
    $newprog set node $self
    $newprog set command $command

    return $newprog
}

Class Program

Program instproc init {args} {
}

Program instproc unknown {m args} {
}

Class Firewall

Firewall instproc init {sim args} {
    global last_fw
    global last_fw_node
    real_set tmp [$sim node]
    real_set last_fw $self
    real_set last_fw_node $tmp
}

Firewall instproc unknown {m args} {
}

Class EventSequence

EventSequence instproc init {args} {
}

EventSequence instproc unknown {m args} {
}

Class EventTimeline

EventTimeline instproc init {args} {
}

EventTimeline instproc unknown {m args} {
}

Class EventGroup

EventGroup instproc init {args} {
}

EventGroup instproc unknown {m args} {
}

Class Console -superclass Agent

Console instproc init {args} {
}

Console instproc unknown {m args} {
}

Topography instproc load_area {args} {
}

Topography instproc checkdest {args} {
    return 1
}

Class NSENode -superclass Node

NSENode instproc make-simulated {args} {
    uplevel 1 eval $args
}

# We are just syntax checking the NS file
Simulator instproc run {args} {
}

Simulator instproc nsenode {args} {
    return [new NSENode]
}

Simulator instproc make-simulated {args} {
    uplevel 1 eval $args
}

Simulator instproc event-sequence {args} {
    $self instvar id_counter

    incr id_counter
    return [new EventSequence]
}

Simulator instproc event-timeline {args} {
    $self instvar id_counter

    incr id_counter
    return [new EventTimeline]
}

Simulator instproc event-group {args} {
    return [new EventGroup]
}

Simulator instproc make-cloud {nodes bw delay args} {
    return [$self make-lan $nodes $bw $delay]
}

Node instproc program-agent {args} {
}

Node instproc topography {args} {
}

Node instproc console {} {
    return [new Console]
}

Node instproc unknown {m args} {
}

Simulator instproc connect {src dst} {
}

Simulator instproc define-template-parameter {name args} {
    # install the name/value in the outer environment.
    set value [lindex $args 0]    
    uplevel 1 set \{$name\} \{$value\}
}

LanNode instproc trace {args} {
}

LanNode instproc trace_endnode {args} {
}

LanNode instproc unknown {m args} {
}
