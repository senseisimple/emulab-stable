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

if {[file dirname [info script]] == "."} {
    set updir ".."
} else {
    set updir [file dirname [file dirname [info script]]]
}
if {[file exists $updir/../assign/assign]} {
    set assign "$updir/../assign/assign"
} else {
    set assign "$updir/../assign"
}

load /usr/testbed/lib/sql.so
set DB [sql connect]
sql selectdb $DB tbdb

set maxrun 5
set delaythresh .25

if {[llength $argv] != 2} {
    puts stderr "Syntax: assign <ir> <ptop>"
    exit 1
}

set testbed [lindex $argv 1]
set irfile [lindex $argv 0]

# Generate top file.
set fp [open $irfile "r"]

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

    set links([lindex $line 0]) [lrange $line 1 end]
}

# close the ir for now
close $fp

# now we need to generate a top file.
set tmpfile "/tmp/[pid].top"
puts "Writing topfile ($tmpfile)"

set topfp [open $tmpfile w]

# calculate delay stuff
set delayI 0
set delayinfo {}
foreach link [array names links] {
    set src [lindex $links($link) 0]
    set dst [lindex $links($link) 2]
    set bw [string trimright [lindex $links($link) 4] "MbmB"]
    set delay [string trimright [lindex $links($link) 6] "msMS"]
    set loss [lindex $links($link) 8]
    if {($bw != 100 && $bw != 10) ||
	($delay > $delaythresh) ||
	($loss > 0)} {
	# we need a delay node
	set nodes(delay$delayI) [list delay]
	set rlinks(dsrc_$link) [list $src -1 delay$delayI -1 100Mb 100Mb 1ms 1ms]
	set rlinks(ddst_$link) [list $dst -1 delay$delayI -1 100Mb 100Mb 1ms 1ms]
	lappend delayinfo "$link delay$delayI $bw $delay $loss"
	incr delayI
    } else {
	set rlinks($link) $links($link)
    }
}

# write nodes
foreach node [array names nodes] {
    puts $topfp "node $node [lindex $nodes($node) 0]"
}

# write links
foreach link [array names rlinks] {
    set src [lindex $rlinks($link) 0]
    set dst [lindex $rlinks($link) 2]
    set bw [string trimright [lindex $rlinks($link) 4] "Mb"]
    puts $topfp "link $link [lindex $src 0] [lindex $dst 0] $bw"
}

close $topfp

# run assign on the topfile and $testbed
puts "Running assign ($assign -b -t $testbed $tmpfile)"
puts "  Log in [file dirname $irfile]/assign.log"
set run 0
while {$run < $maxrun} {
    set assignfp [open "|$assign -b -t $testbed $tmpfile | tee -a [file dirname $irfile]/assign.log" r]
    set problems 0
    set score -1
    set seed 0
    while {$problems == 0 && [gets $assignfp line] >= 0} {
	if {[regexp {BEST SCORE: ([0-9]+)} $line match score] == 1} {
	    continue;
	}
	if {[regexp {With ([0-9]+) violations} $line match problems] == 1} {
	    break;
	}
	if {[regexp {seed = ([0-9]+)} $line match seed] == 1} {
	    continue
	}
    }
    if {$problems > 0} {
	incr run
	puts "Run $run resulted in $problems violations."
	continue
    }
    # we should now be ready to read the solution
    readto $assignfp "Nodes:"
    while {[gets $assignfp line] >= 0} {
	if {[regexp {unassigned:} $line] == 1} {
	    puts stderr "Assign error ($line). I'm confused! (not deleting $tmpfile)"
	    exit 1
	}
	if {$line == "End Nodes"} {break}
	lappend map([lindex $line 1]) [list [lindex $line 0] [lindex $line 2]]
	set nmap([lindex $line 0]) [lrange $line 1 end]
    }
    readto $assignfp "Edges:"
    while {[gets $assignfp line] >= 0} {
	if {$line == "End Edges"} {break}
	set plinks([lindex $line 0]) [lrange $line 1 end]
    }
    break
}
if {$run >= $maxrun} {
    puts "Could not find solution (not deleting $tmpfile)!"
    exit 1
}
close $assignfp

# append virtual section to ir
set fp [open $irfile a]

puts "Adding virtual section"
# XXX: we don't do links or lans yet
# XXX: We need to handle delay links
puts $fp "START virtual"
puts $fp "START nodes"
foreach switch [array names map] {
    foreach pair $map($switch) {
	set v2pmap([lindex $pair 0]) [lindex $pair 1]
	puts $fp "$pair"
    }
}
puts $fp "END nodes"

proc get_link_name {s} {
    set t [split $s _]
    set v [lindex $t 0]
    if {$v == "dsrc" || $v == "ddst"} {
	return [join [lrange $t 1 end] _]
    } else {
	return $s
    }
}

puts $fp "START links"
foreach link [array names plinks] {
    set pls {}
    foreach element [lrange $plinks($link) 1 end] {
	if {[string index $element 0] == "("} {continue}
	lappend pls $element
    }
    set name [get_link_name $link]
    if {! [info exists linktmp($name)]} {
	set linktmp($name) {}
    }
    foreach l $pls {
	lappend linktmp($name) $l
    }
}
proc node_name {s} {
    return [lindex [split $s -] 0]
}
foreach link [array names linktmp] {
    # Any "duplicates" should go together.
    # RULES: Can only swap a and b or c and d (i.e. within a link)
    # This will only happen in the case of a delay node which will
    # have four elements (two links of src/dst), we just check every
    # case.
    if {[llength $linktmp($link)] == 4} {
	set a [lindex $linktmp($link) 0]
	set b [lindex $linktmp($link) 1]
	set c [lindex $linktmp($link) 2]
	set d [lindex $linktmp($link) 3]
	if {[node_name $b] == [node_name $d]} {
	    set tmp $d
	    set d $c
	    set c $tmp
	} elseif {[node_name $a] == [node_name $c]} {
	    set tmp $b
	    set b $a
	    set a $tmp
	} elseif {[node_name $a] == [node_name $d]} {
	    set tmp $b
	    set b $a
	    set a $tmp
	    set tmp $d
	    set d $c
	    set c $tmp
	}
	set linktmp($link) [list $a $b $c $d]
    }
    puts $fp "$link $linktmp($link)"
}

puts $fp "END links"
puts $fp "END virtual"

## now we apprend the vlan section
# first we need to read in macs
puts "Adding VLAN section"
puts $fp "START vlan"

proc getmac {s} {
    set t [split [string trim $s "()"] ,]
    if {[lindex $t 0] == "(null)"} {
	return [lindex $t 1]
    } else {
	return [lindex $t 0]
    }
}

foreach link [array names plinks] {
    set t $plinks($link)
    set type [lindex $t 0]
    set name [get_link_name $link]
    switch $type {
	"intraswitch" {
	    # 2 links, but each goes to a switch
	    set srcmac [getmac [lindex $t 2]]
	    set dstmac [getmac [lindex $t 4]]
	    lappend linktmpb($name) [list $srcmac $dstmac]
	}
	"interswitch" {
	    # 3 or four links
	    # XXX - in later versions this will become an arbitrary number
	    # of links.
	    set ll {}
	    if {[llength $t] == 8} {set num 4} else {set num 3}
	    for {set i 0} {$i < $num} {incr i} {
		set l [lindex $t [expr $i*2+1]]
		set mac [getmac [lindex $t [expr $i*2+2]]]
		if {$mac != "(null)"} {
		    lappend ll $mac
		}
	    }
	    lappend linktmpb($name) $ll
	}
	"direct" {
	    # 1 link
	    set macs [split [string trim [lindex $t 2] "()"] ","]
	    lappend linktmpb($name) $macs
	}
    }
}
foreach link [array names linktmpb] {
    set llen [llength $linktmpb($link)]
    set i 0
    foreach l $linktmpb($link) {
	set vlaninfo($link-$i) $l
	puts $fp "$link-$i $l"
	incr i
    }
}

puts $fp "END vlan"

# add delay seciton
puts $fp "START delay"
foreach line $delayinfo {
    puts $fp [lrange $line 0 3]
}
puts $fp "END delay"

close $fp

file delete $tmpfile

# Add delay info to database
# Note on interface numbers:  A delayed link has two vlans
# <link>-0 and <link>-1 the second mac of -0 and the first mac
# of -1 are the two macs on the delay node and correspond to the two
# intercaces.
foreach line $delayinfo {
    set link [lindex $line 0]
    set node [lindex $line 1]
    set bw [lindex $line 2]
    set delay [lindex $line 3]
    set loss [lindex $line 4]
    
    # extra macs of the two interfaces
    set mac0 [lindex $vlaninfo($link-0) 1]
    set mac1 [lindex $vlaninfo($link-1) 0]

    # use database to get interface numbers
    sql query $DB "select card from interfaces where mac = \"$mac0\""
    set int0 [sql fetchrow $DB]
    sql endquery $DB
    sql query $DB "select card from interfaces where mac = \"$mac1\""
    set int1 [sql fetchrow $DB]
    sql endquery $DB
    
    # find the physical node for this dely
    set pnode $v2pmap($node)

    # remove any current entry in the delays table
    sql exec $DB "delete from delays where node_id = \"$pnode\""
    
    # add this entry
    sql exec $DB "insert into delays (node_id,card0,card1,delay,bandwidth,lossrate) values (\"$pnode\",\"$int0\",\"$int1\",\"$delay\",\"$bw\",\"$loss\")"
}

puts "Done"
