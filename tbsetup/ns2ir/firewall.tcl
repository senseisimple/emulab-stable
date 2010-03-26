# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005, 2010 University of Utah and the Flux Group.
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
    var_import ::GLOBALS::explicit_firewall
    var_import ::GLOBALS::security_level

    $self set sim $s

    $self set type    "ipfw2-vlan"
    $self set style   "basic"
    $self set osid    "FW-IPFW2"
    $self set cmdline ""
    $self set parent  ""

    $self set next_rule 100
    $self instvar rules
    array set rules {}

    # Link simulator to this new object.
    if {[$s add_firewall $self] == 0} {
	set ::GLOBALS::last_class $self
    }

    if {$security_level} {
	perror "\[add_firewall] cannot combine firewall with security-level"
    }

    # avoid conflicts with security_level
    set explicit_firewall 1
}

Firewall instproc rename {old new} {
    $self instvar sim

    $sim rename_firewall $old $new
}

#
# Set the type of the firewall
# 
Firewall instproc set-type {targ} {
    $self instvar type
    $self instvar osid
    $self instvar cmdline

    switch -- $targ {
	"ipfw" {
	    set type $targ
	    set osid "FW-IPFW"
	}
	"ipfw2-vlan" {
	    set type $targ
	    set osid "FW-IPFW2"
	    set cmdline "/kernel.fw"
	}
	"ipfw2" -
	"ipchains" {
	    perror "\[set-type] firewall type $targ not yet implemented"
	}
	default {
	    perror "\[set-type] unknown firewall type: $targ"
	}
    }
}

#
# Set the style of the firewall
# 
Firewall instproc set-style {starg} {
    $self instvar style

    switch -- $starg {
	"closed" -
	"basic" -
	"emulab" -
	"open" {
	    set style $starg
	}
	default {
	    perror "\[set-style] unsupported firewall style: $starg"
	}
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

    if {$num < 100 || $num >= 50000} {
	perror "\[add-numbered-rule] 100 <= rule_number < 50000!"
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
    $self instvar rules
    $self instvar sim
    $self instvar type
    $self instvar style
    $self instvar osid
    $self instvar cmdline

    # XXX add the firewall to the virt_nodes table to avoid assign hacking
    $sim spitxml_data "virt_nodes" [list "vname" "type" "ips" "osname" "cmd_line" "rpms" "startupcmd" "tarfiles" "fixed" ] [list "$self" "pc" "" $osid $cmdline "" "" "" "" ]

    $sim spitxml_data "virt_firewalls" [list "fwname" "type" "style"] [list $self $type $style]
    foreach rule [array names rules] {
	set names [list "fwname" "ruleno" "rule"]
	set vals  [list $self $rule $rules($rule)]
	$sim spitxml_data "firewall_rules" $names $vals
    }
}
