# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
#
# Sequence support.
#
######################################################################

Class EventSequence

namespace eval GLOBALS {
    set new_classes(EventSequence) {}
}

EventSequence instproc init {s seq args} {
    global ::GLOBALS::last_class

    ::GLOBALS::named-args $args { -errorseq 0 }

    $self set sim $s

    # event list is a list of {time vnode vname otype etype args atstring}
    $self set event_list {}
    $self set event_count 0
    $self set error_seq $(-errorseq)

    $self instvar event_list

    foreach line $seq {
	if {[llength $line] > 0} {
	    set rc [$s make_event "sequence" $line]
	    if {$rc != {}} {
		lappend event_list $rc
	    }
	}
    }

    set ::GLOBALS::last_class $self
}

EventSequence instproc append {event} {
    $self instvar sim
    $self instvar event_list
    
    set rc [$sim make_event "sequence" $event]
    if {$rc != {}} {
	lappend event_list $rc
    }
}

EventSequence instproc rename {old new} {
    $self instvar sim

    $sim rename_sequence $old $new
}

EventSequence instproc dump {file} {
    $self instvar event_list

    foreach event $event_list {
	puts $file "$event"
    }
}

EventSequence instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::TBCOMPAT::objtypes
    var_import ::TBCOMPAT::eventtypes
    $self instvar sim
    $self instvar event_list
    $self instvar error_seq
    
    foreach event $event_list {
	$sim spitxml_data "eventlist" [list "vnode" "vname" "objecttype" "eventtype" "arguments" "atstring" "parent"] [list [lindex $event 0] [lindex $event 1] $objtypes([lindex $event 2]) $eventtypes([lindex $event 3]) [lindex $event 4] [lindex $event 5] $self ]
    }

    $sim spitxml_data "virt_agents" [list "vnode" "vname" "objecttype" ] \
	    [list "*" $self $objtypes(SEQUENCE)]
}
