######################################################################
# npdiff.tcl
# 
# This is a quick program to compare two reports from netpull.  It
# reports:
#  NodeAppear <ip>
#  NodeVanish <ip>
#  NodeChange <ip> <old state> <new state>
#  LinkAppear <src> <dst>
#  LinkVanish <src> <dst>
#  LinkChange <src> <dst> <old state> <new state>
#  LinkRtt    <src> <dst> <rtt diff A - B>
#
# It takes as argument two files and optionally a -v # which is the multiple
# to multiply variances by for LinkChange reports.
######################################################################

source calfeldlib.tcl
namespace import CalfeldLib::ldiff
namespace import CalfeldLib::lpop

######################################################################
# Procs
######################################################################

# shows help and exits
proc showhelp {} {
    puts "npdiff.tcl \[-v <mult>\] <fileA> <fileB>"
    exit 1
}

# parses a file into the named variable
# variables are <name>Links <name>Nodes <name>LinksL
proc parsefile {filename varprefix} {
    upvar ${varprefix}Links LINKS
    upvar ${varprefix}Nodes NODES
    upvar ${varprefix}LinksL LinksL

    set fp [open $filename r]
    while {[gets $fp line] >= 0} {
	switch -- [lindex $line 0] {
	    "LINK" {
		set name [join [lrange $line 1 2] ,]
		set LinksL($name) 1
		set LINKS($name) [lindex $line 3]
		set LINKS($name:mean) [lindex $line 4]
		set LINKS($name:var) [lindex $line 5]
		set LINKS($name:n) [lindex $line 6]
	    }
	    "NODE" {
		set NODES([lindex $line 1]) [lindex $line 2]
	    }
	    default {
		puts stderr "Bad line: $line"
	    }
	}
    }
    close $fp
}


# parse arguments
set fileA {}
set fileB {}
set mult 4
while {$argv != {}} {
    set arg [lpop argv]
    if {$arg == "-v"} {
	if {$argv == {}} {showhelp}
	set mult [lpop argv]
    } else {
	if {$fileA == {}} {
	    set fileA $arg
	} else {
	    if {$fileB != {}} {showhelp}
	    set fileB $arg
	}
    }
}
if {$fileA == {} || $fileB == {}} {showhelp}

# parse the two files
parsefile $fileA A
parsefile $fileB B

# construct lists
set alinks [array names ALinksL]
set blinks [array names BLinksL]
set anodes [array names ANodes]
set bnodes [array names BNodes]

# find node differences
set t [ldiff $anodes $bnodes]
set vanish [lindex $t 0]
set appear [lindex $t 2]
set same [lindex $t 1]
foreach node $vanish {
    puts "NodeVanish $node"
}
foreach node $appear {
    puts "NodeAppear $node"
}
foreach node $same {
    if {$ANodes($node) != $BNodes($node)} {
	puts "NodeChange $node $ANodes($node) $BNodes($node)"
    }
}

# compare links
set t [ldiff $alinks $blinks]
set vanish [lindex $t 0]
set appear [lindex $t 2]
set same [lindex $t 1]
foreach link $vanish {
    set t [split $link ,]
    puts "LinkVanish $t"
}
foreach link $appear {
    set t [split $link ,]
    puts "LinkAppear $t"
}
foreach link $same {
    set t [split $link ,]
    if {$ALinks($link) != $BLinks($link)} {
	puts "LinkChange $t $ALinks($link) $BLinks($link)"
    } else {
	# RTT stuff
	set diff [expr $ALinks($link:mean) - $BLinks($link:mean)]
	if {[expr abs($diff) > $mult * $ALinks($link:var)] ||
	    [expr abs($diff) > $mult * $BLinks($link:var)]} {
	    puts "LinkRtt $t $diff"
	}
    }
}
