#!/usr/local/bin/otclsh

if {$argc != 3} {
   puts "usage: $argv0 project ns_input_file ir_file"
   exit 1
}
set project [lindex $argv 0]
set nsfile [lindex $argv 1]
set irfile [lindex $argv 2]

set libdir [file dirname [info script]]
source $libdir/tcl-object.tcl
source $libdir/node.tcl
source $libdir/link.tcl
source $libdir/event.tcl

###
# calfeld@cs.utah.edu
# This some ugly/interesting tcl hacking to figure out what variables the user
# stored the node ids in.
###
rename set real_set
real_set skipset 0
proc set {args} {
    global skipset
    global nodeid_map
    if {! $skipset} {
	real_set skipset 1
	real_set var [lindex $args 0]
	if {$var != "currnode"} {
	    if {[llength $args] > 1} {
		real_set val [lindex $args 1]
		if {[regexp {^n[0-9]+$} $val] != -1} {
		    if {![info exists nodeid_map($val)]} {
			real_set nodeid_map($val) $var
		    }
		}
	    }
	}
	real_set skipset 0
    }
    if {[llength $args] == 1} {
	return [uplevel real_set [lindex $args 0]]
    } else {
	return [uplevel real_set [lindex $args 0] \{[lindex $args 1]\}]
    }
}
###

#nop is used for unimplemented $ns instprocs that are supposed to
#return things. the instproc returns a nop, which users call in their
#ns input file. 
proc nop {args} {}

#begin at 0. 1,2,3... i cheerfully ignore the possibility of wrapping...
set nodeID 0
set linkID 0
set eventID 0

set nodelist {}

set linkslist {}

set eventlist {}

# sim.tcl handles the ns Simulator methods
source $libdir/sim.tcl

# stubs.tcl contains a lot of dummy things to allow execution of 
# ns files without going through the trouble of redoing ns or something.
# i fear that it will grow without bound (or at least until I give up and
# make this whole thing into an ns add-on and keep all of the ns behavior)

source $libdir/stubs.tcl
 
set prefix $project[lindex [split [lindex [split $nsfile /] end] .] 0]
source $nsfile

