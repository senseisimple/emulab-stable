# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#

######################################################################
#
# Timeline support.
#
######################################################################

Class EventTimeline

namespace eval GLOBALS {
    set new_classes(EventTimeline) {}
}

EventTimeline instproc init {s} {
    global ::GLOBALS::last_class

    $self set sim $s

    # event list is a list of {time vnode vname otype etype args atstring}
    $self set event_list {}
    $self set event_count 0

    set ::GLOBALS::last_class $self
}

EventTimeline instproc rename {old new} {
    $self instvar sim

    $sim rename_timeline $old $new
}

EventTimeline instproc dump {file} {
    $self instvar event_list

    foreach event $event_list {
	puts $file "$event"
    }
}

EventTimeline instproc at {time eventstring} {
    $self instvar sim
    $self instvar event_list
    $self instvar event_count
    
    set ptime [::GLOBALS::reltime-to-secs $time]
    if {$ptime == -1} {
	perror "Invalid time spec: $time"
	return
    }
    set time $ptime

    if {$event_count > 4000} {
	perror "Too many events in your NS file!"
	exit 1
    }
    set eventlist [split $eventstring ";"]
    
    foreach event $eventlist {
	set rc [$sim make_event "timeline" $event]

	if {$rc != {}} {
	    set event_count [expr $event_count + 1]
	    lappend event_list [linsert $rc 0 $time]
	}
    }
}

EventTimeline instproc updatedb {DB} {
    var_import ::GLOBALS::pid
    var_import ::GLOBALS::eid
    var_import ::TBCOMPAT::objtypes
    var_import ::TBCOMPAT::eventtypes
    $self instvar sim
    $self instvar event_list
    
    foreach event $event_list {
	$sim spitxml_data "eventlist" [list "time" "vnode" "vname" "objecttype" "eventtype" "arguments" "atstring" "parent"] [list [lindex $event 0] [lindex $event 1] [lindex $event 2] $objtypes([lindex $event 3]) $eventtypes([lindex $event 4]) [lindex $event 5] [lindex $event 6] $self ]
    }

    $sim spitxml_data "virt_agents" [list "vnode" "vname" "objecttype" ] \
	    [list "*" $self $objtypes(TIMELINE)]
}
