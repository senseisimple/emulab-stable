# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# nsenode.tcl
#
# This defines the NSENode class.  Instances of this class are created by
# the 'nsenode' method of Simulator.  
# This node supports a method make-simulated and when invoked with
# code in-between {} as an argument, treats the code as simulated
# and runs NSE on the physical NSENode with the provided script
######################################################################

Class NSENode -superclass Node

# need to have a single namespace for simulated node names
# lets have simulated node -> nsenode map

# need to support simulated to real node links, simulated to simulated
# across nse nodes => we need a duplex-link specialization even in
# child interpreters

# routing must be
# manually enabled so that a RAW IP packet written should automatically
# make it out

NSENode instproc init {s} {

    set new_classes(NSENode) {}
    $self next $s
    $self set nseconfig ""
    $self set simcode_present 0
}

NSENode instproc make-simulated {args} {

    var_import ::GLOBALS::simulated
    var_import ::GLOBALS::curnsenode
    $self instvar nseconfig
    $self instvar simcode_present

    set simulated 1
    set curnsenode $self
    global script
    set script [string trim $args "\{\}"]

    if { $script == {} } {
	set simulated 0
	set curnsenode ""
	return
    }

    set simcode_present 1

    # we ignore any type of errors coz they have
    # been caught when we ran the script through NSE
    catch {uplevel 1 eval $script}

    append nseconfig $script

    set simulated 0
    set curnsenode ""
}

NSENode instproc updatedb {DB} {
    $self instvar nseconfig
    $self instvar simcode_present
    var_import ::GLOBALS::sim_osname

    # for all the simulated nodes, we need to find out if
    # they are connected to a real node and if so, find the
    # ip address of the port on nsenode

    foreach node [Node info instances] {
	if { [$node set simulated] == 1 && [$node set nsenode] == $self } {

	    append nseconfig "\n\n\$$node set simulated 1\n"
	    append nseconfig "\$$node set nsenode $self\n"
	    
	    set nsenode_vportlist [$node set nsenode_vportlist]
	    if { $nsenode_vportlist != {} } {
		append nseconfig "\$$node set nsenode_vportlist \[list $nsenode_vportlist]\n"

		set ipaddrlist {}
		foreach v $nsenode_vportlist {
		    lappend ipaddrlist [$self ip $v]
		}
		append nseconfig "\$$node set nsenode_ipaddrlist \[list $ipaddrlist]\n\n"
	    }
	}
    }

    if { $simcode_present == 1 } {
	append nseconfig "set simcode_present $simcode_present\n\n"
    }

    # We prefer to get a pc850 if available to take advantage of more CPU horsepower
    tb-make-soft-vtype pcnsenode {pc850}
    tb-set-hardware $self pcnsenode

    # NSE code runs only FreeBSD as of now. 
    $self set osid $sim_osname

    # The Node class's updatedb method updates the DB
    $self next $DB
}
