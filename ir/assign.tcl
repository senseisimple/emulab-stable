#!/usr/local/bin/tclsh

# This program needs to:
#
# 1) Extract the topology section from the specificed IR file and translate
# into a top file.
#
# 2) Run assign on the top file and testbed.ptop.
#
# 3) Translate the results into the virtual section and apprend to the IR.
#
# 4) Cross reference with macs.txt and generate the VLAN section.

# XXX - this is rather hackish right now.  This should be rewritten using
# irlib when, if ever, that is written.

set testbed "testbed.ptop"
set macs "./macs.txt"
set assign "../assign/assign"
set maxrun 5
set switchports 32

if {[llength $argv] != 1} {
    puts stderr "Syntax: assign <ir>"
    exit 1
}

# Generate top file.
set fp [open $argv "r"]

proc readto {fp s} {
    while {[gets $fp line] >= 0} {
	# ignore comments
	if {[regexp "^\#" $line] == 1} {continue}	
	if {$line == {}} {continue}
	if {$line == $s} {return}
    }
    error "$s not found"
}

puts "Reading virtual topology"
readto $fp "START topology"
# XXX: we ignore lans for now.
readto $fp "START nodes"
# nodes is an array of nodes with contents of link list.
while {[gets $fp line] >= 0} {
    if {[regexp "^\#" $line] == 1} {continue}	
    if {$line == {}} {continue}
    if {$line == "END nodes"} {break}
    
    set nodes([lindex $line 0]) [lrange $line 1 end]
}

readto $fp "START links"
# XXX: we ignore ports for now.
while {[gets $fp line] >= 0} {
    if {[regexp "^\#" $line] == 1} {continue}	
    if {$line == {}} {continue}
    if {$line == "END links"} {break}

    set links([lindex $line 0]) [list [lrange $line 1 2] \
				     [lrange $line 3 4]]
}

# close the ir for now
close $fp

# now we need to generate a top file.
set tmpfile "/tmp/[pid].top"
puts "Writing topfile ($tmpfile)"

set topfp [open $tmpfile w]

# write nodes
foreach node [array names nodes] {
    puts $topfp "node $node [lindex $nodes($node) 0]"
}

# write links
foreach link [array names links] {
    set src [lindex $links($link) 0]
    set dst [lindex $links($link) 1]
    puts $topfp "link [lindex $src 0] [lindex $dst 0]"
}

close $topfp

# run assign on the topfile and $testbed
puts "Running assign ($assign -b -t $testbed $tmpfile)"
puts "  Log in [file dirname $argv]/assign.log"
set run 0
while {$run < $maxrun} {
    set assignfp [open "|$assign -b -t $testbed $tmpfile | tee -a [file dirname $argv]/assign.log" r]
    set problem 0
    while {$problem == 0 && [gets $assignfp line] >= 0} {
	if {[regexp {violated: (.+)} $line match reason] == 1} {
	    # XXX: add better parsing of reason
	    puts "$match"
	    if {[regexp {bandwidth} $reason] != -1} {
		set problem -1
	    } else {
		set problem 1
	    }
	} elseif {[regexp {Best solution: ([0-9]+)} $line match score] == 1} {
	    break
	}
    }
    if {$problem == 1} {
	incr run
	close $assignfp
	puts "Run $run resulted in violation ($reason)."
	continue
    }
    # we should now be ready to read the solution
    while {[gets $assignfp line] >= 0} {
	if {[regexp {unassigned:} $line] == 1} {
	    puts stderr "Assign error ($line). I'm confused! (not deleting $tmpfile)"
	    exit 1
	}
	if {$line == "End solution"} {break}
	lappend map([lindex $line 1]) [list [lindex $line 0] [lindex $line 2]]
	set nmap([lindex $line 0]) [lrange $line 1 end]
    }
    break
}
if {$run > $maxrun} {
    if {$problem == -1} {
	puts "WARNING: switch bandwidth exceeded."
    } else {
	puts "Could not find solution (not deleting $tmpfile)!"
	exit 1
    }
}
close $assignfp

# append virtual section to ir
set fp [open $argv a]

puts "Adding virtual section"
# XXX: we don't do links or lans yet
puts $fp "START virtual"
puts $fp "START nodes"
foreach switch [array names map] {
    foreach pair $map($switch) {
	puts $fp "$pair"
    }
}
puts $fp "END nodes"
puts $fp "END virtual"

# read macs file
puts "Reading macs ($macs)"
set macfp [open $macs r]
while {[gets $macfp line] >= 0} {
    set mac([lindex $line 0]) [lrange $line 1 end]
}
close $macfp

# do port computations
puts "Calculating port numbers"

# pass 1 - here we record all used ports
foreach node [array names nodes] {
    foreach link [lrange $nodes($node) 1 end] {
	if {$node == [lindex [lindex $links($link) 0] 0]} {
	    set port [lindex [lindex $links($link) 0] 1]
	    set index 0
	} else {
	    set port [lindex [lindex $links($link) 1] 1]
	    set index 1
	}
	if {$port == "-1"} {
	    # save for pass 2
	    lappend unresolved [list $link $index $node]
	} else {
	    set assigned($node:$port) 1
	}
    }
}
# pass 2 - here we fill in all -1's from unused numbers
foreach un $unresolved {
    set link [lindex $un 0]
    set index [lindex $un 1]
    set node [lindex $un 2]
    # find a free number
    set nnode [lindex $nmap($node) 0]
    if {[info exists mac($nnode)] == 0} {
	# switch
	set numports $switchports
    } else {
	set numports [llength $mac([lindex $nmap($node) 0])]
    }
    for {set i 0} {$i < $numports} {incr i} {
	if {! [info exists assigned($node:$i)]} {
	    # found one
	    puts "Port $i on node $node assigned to link $link"
	    set assigned($node:$i) 1
	    set links($link) [lreplace $links($link) $index $index \
				  [list $node $i]]
	    break
	}
    }
}

# now we apprend the vlan section
# first we need to read in macs
puts "Adding VLAN section"
puts $fp "START vlan"

# ok, this is harder.  We need to go through every link in links
# and those that are in the current switch output the correct vlan.
# We need to be especially careful to get the interlinks right.
#foreach switch [array names map] {
#    puts $fp "switch $switch"
    foreach link [array names links] {
	set srcp [lindex $links($link) 0]
	set dstp [lindex $links($link) 1]
	set src [lindex $srcp 0]
	set dst [lindex $dstp 0]
	set srcport [lindex $srcp 1]
	set dstport [lindex $dstp 1]
	set srcs [lindex $nmap($src) 0]
	set dsts [lindex $nmap($dst) 0]
	if {$srcs == $switch || $dsts == $switch} {
	    # output a vlan for this
	    set srcp [lindex $nmap($src) 1]
	    set dstp [lindex $nmap($dst) 1]
	    puts $fp "$link [lindex $mac($srcp) $srcport] [lindex $mac($dstp) $dstport]"
	}
    }
#    puts $fp "end"
#}

puts $fp "END vlan"

close $fp

file delete $tmpfile

puts "Done"
