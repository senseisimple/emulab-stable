# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
# console.tcl
#
# This defines the console agent.
#
######################################################################

Class Console -superclass NSObject

namespace eval GLOBALS {
    set new_classes(Console) {}
}

Console instproc init {s n} {
    $self set sim $s
    $self set node $n
    $self set connected 0
}

Console instproc rename {old new} {
    $self instvar sim

    $sim rename_console $old $new
}

# updatedb DB
# This adds rows to the virt_trafgens table corresponding to this agent.
Console instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::TBCOMPAT::objtypes
    $self instvar node
    $self instvar sim

    if {$node == {}} {
	perror "\[updatedb] $self has no node."
	return
    }

    # Update the DB
    $sim spitxml_data "virt_agents" [list "vnode" "vname" "objecttype"] [list $node $self $objtypes(CONSOLE)]
}
