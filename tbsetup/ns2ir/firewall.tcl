# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
#
# Firewall support.
#
######################################################################

Class Firewall -superclass NSObject

namespace eval GLOBALS {
    set new_classes(Firewall) {}
}

Firewall instproc init {s} {
    global ::GLOBALS::last_class

    $self set sim $s

    $self set style "basic"
    $self set parent ""

    $self set next_rule 1
    $self instvar rules
    array set rules {}

    # Link simulator to this new object.
    if {[$s add_firewall $self] == 0} {
	set ::GLOBALS::last_class $self
    }
}

Firewall instproc rename {old new} {
    $self instvar sim

    $sim rename_firewall $old $new
}

#
# Set the style of the firewall
# 
Firewall instproc set-style {starg} {
    $self instvar style

    if {$starg == "open" || $starg == "closed" || $starg == "basic"} {
	set style $starg
    } else {
	punsup "firewall: unsupported style: $starg"
    }
}

#
# Add rules to the firewall.
# 
Firewall instproc add-rule {rule} {
    $self instvar next_rule
    $self instvar rules

    set rules($next_rule) $rule
    incr next_rule
}

#
# Add numbered rules to the firewall.
# 
Firewall instproc add-numbered-rule {num rule} {
    $self instvar rules

    if {$num >= 50000} {
	perror "\[add-numbered-rule] rule number must be < 50000!"
	return
    }
    if {[info exists rules($num)]} {
	perror "\[add-numbered-rule] rule $num already exists!"
	return
    }
    set rules($num) $rule
}

Firewall instproc child-of {pfw} {
    $self instvar parent

    if {[$pfw info class] != "Firewall"} {
	perror "\[child-of] $pfw is not a Firewall"
	return
    }
    if {$pfw == $self} {
	perror "\[child-of] cannot father yourself"
	return
    }
    if {$parent != {}} {
	perror "\[child-of] $self already a child of $parent"
	return
    }
    set parent $pfw
}

# updatedb DB
Firewall instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    $self instvar rules
    $self instvar sim
    $self instvar style

    # XXX add the firewall to the virt_nodes table to avoid assign hacking
    $sim spitxml_data "virt_nodes" [list "vname" "type" "ips" "osname" "cmd_line" "rpms" "startupcmd" "tarfiles" "fixed" ] [list "$self" "pc" "" "FW-IPFW" "" "" "" "" "" ]

    $sim spitxml_data "firewalls" [list "fwname" "type" "style"] [list $self "ipfw" $style]
    foreach rule [array names rules] {
	set names [list "fwname" "ruleno" "rule"]
	set vals  [list $self $rule $rules($rule)]
	$sim spitxml_data "firewall_rules" $names $vals
    }
}
