#!/usr/local/bin/tclsh

#XXX - Add DB checking

#XXX - Move this to the database
set DEFAULT_OS RHL62-STD

if {[llength $argv] != 2} {
    puts stderr "Syntax: $argv0 ir_file ns_file"
    exit 1
}
set scriptdir [file dirname [info script]]
set updir [file dirname $scriptdir]

source "$scriptdir/libir.tcl"
namespace import TB_LIBIR::ir


set nsfile [lindex $argv 1]
set irfile [lindex $argv 0]

ir read $irfile

if {! [ir exists /virtual/nodes]} {
    puts stderr "Missing /virtual/nodes.  Run assign first."
    exit 1
}

set nmap [ir get /virtual/nodes]
foreach pair $nmap {
    set node_map([lindex $pair 0]) [lindex $pair 1]
    set os([lindex $pair 1]) $DEFAULT_OS
}

set fp [open $nsfile r]

while {[gets $fp line] >= 0} {
    if {[lindex $line 0] != "#TB"} {continue}
    switch [lindex $line 1] {
	"create-os" {
	    set label [lindex $line 2]
	    set path [lindex $line 3]
	    set partition [lindex $line 4]
	    set images($label) [list $path $partition]
	}
	"set-dnard-os" {
	    set shelf [lindex $line 2]
	    set n [lindex $line 3]
	    set label [lindex $line 4]

	    # XXX Check valid OS.  Should either be in images
	    # or DB
  	    set prefix $node_map($shelf)
	    if {[regexp {([0-9])-([0-9])} $n match s e] == 1} {
		for {set i $s} {$i <= $e} {incr i} {
		    set os($prefix-$i) $label
		}
	    }
	}
	"set-node-os" {
	    set node [lindex $line 2]
	    set label [lindex $line 3]
	    
	    # XXX Check valid - See above
	    foreach n [array names node_map] {
		if {[string match $node $n] == 1} {
		    set os($node_map($n)) $label
		}
	    }
	}
	"set-dnard-deltas" {
	    # Place holder
	}
	"set-node-deltas" {
	    # Place holder
	}
	default {
	    # NOP
	}
    }
}

# output
if {! [ir exists /os]} {
    set fp [open $irfile a]
    puts $fp "START os"
    puts $fp "START nodes"
    foreach n [array names os] {
	puts $fp "$n $os($n)"
    }
    puts $fp "END nodes"
    puts $fp "START images"
    foreach i [array names images] {
	puts $fp "$i $images($i)"
    }
    puts $fp "END images"
    puts $fp "END os"
    close $fp
} else {
    set nodes_raw {}
    foreach n [array names os] {
	lappend nodes_raw "$n $os($n)"
    }    
    set images_raw {}
    foreach i [array names images] {
	lappend images_raw "$i $images($i)"
    }
    ir set /os/nodes $nodes_raw
    ir set /os/images $images_raw
    ir write $irfile
}

