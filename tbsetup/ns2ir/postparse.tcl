#!/usr/local/bin/tclsh

### Bootstrapping code.  The whole purpose of this is to find the
# directory containing the script.
set file [info script]
while {![catch "file readlink $file" newfile]} {
    set file $newfile
}
set scriptdir [file dirname $file]
if {$scriptdir == "."} {set scriptdir [pwd]}
###

source $scriptdir/../ir/libir.tcl
namespace import TB_LIBIR::ir

if {$argc != 2} {
    puts "usage: $argv0 ns_file ir_file"
    exit 1
}

set nsfile [lindex $argv 0]
set irfile [lindex $argv 1]

ir read $irfile
set nodes [ir get /topology/nodes]

if {[catch "open $nsfile r" nsFP]} {
    puts stderr "ERROR opening $nsfile ($nsFP)."
    exit 1
}

while {[gets $nsFP line] >= 0} {
    if {[regexp {^#TB} $line] == 1} {
	set cmd [lindex $line 1]
	if {$cmd == "set-hardware"} {
	    set node [lindex $line 2]
	    set type [lindex $line 3]
	    # XXX currently only shark-shelf supported
	    set hwtype($node) $type
	} elseif {$cmd == "set-link-loss"} {
	    set src [lindex $line 2]
	    set dst [lindex $line 3]
	    set loss [lindex $line 4]
	    set linkloss($src:$dst) $loss
	}
    }
}

close $nsFP

set newnodes {}
foreach node $nodes {
    set name [lindex $node 0]
    set type [lindex $node 1]
    set links [lrange $node 2 end]
    if {[info exists hwtype($name)]} {
	set type $hwtype($name)
    }
    lappend newnodes [concat $name $type $links]
}

ir set /topology/nodes $newnodes

set links [ir get /topology/links]
set newlinks {}
foreach link $links {
    set src [lindex $link 1]
    set dst [lindex $link 3]
    if {[info exists linkloss($src:$dst)]} {
	lappend newlinks [concat $link $linkloss($src:$dst)]
    } else {
	lappend newlinks [concat $link 0.0]
    }
}
ir set /topology/links $newlinks

ir write $irfile

