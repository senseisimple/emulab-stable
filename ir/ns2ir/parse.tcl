#!/usr/local/bin/otclsh

if {$argc != 2} {
   puts "usage: $argv0 ns_input_file ir_file"
   exit 1
}

set libdir [file dirname [info script]]
source $libdir/tcl-object.tcl
source $libdir/node.tcl
source $libdir/link.tcl
source $libdir/event.tcl

#nop is used for unimplemented $ns instprocs that are supposed to
#return things. the instproc returns a nop, which users call in their
#ns input file. 
proc nop {args} {}

#begin at 0. 1,2,3... i cheerfully ignore the possibility of wrapping...
set nodeID 0
set linkID 0
set eventID 0

set nodelist [list]

set linkslist [list]

set eventlist [list]

# sim.tcl handles the ns Simulator methods
source $libdir/sim.tcl

# stubs.tcl contains a lot of dummy things to allow execution of 
# ns files without going through the trouble of redoing ns or something.
# i fear that it will grow without bound (or at least until I give up and
# make this whole thing into an ns add-on and keep all of the ns behavior)

source $libdir/stubs.tcl
 
# argv[0] is the ns input file
source [lindex $argv 0]
