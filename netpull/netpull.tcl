#!/usr/local/bin/tclsh

######################################################################
# netpull.tcl
#
# This program takes as an argument a file containing a list of IP
# addresses.  It then uses traceroute to construct a picture of the
# network involving this IP address and output a machine parseable report
# on the state of the network including topology and mean and variance
# of RTT.
#
# OUTPUT:
#  NODE <ip> <state>
#  LINK <src> <dst> <state> <rtt mean> <rtt variance> <rtt n samples> <final destinations>
######################################################################

######################################################################
# Initialization
######################################################################

# load lpop
source calfeldlib.tcl
namespace import CalfeldLib::lpop

# Set some static settings
set traceroute /usr/sbin/traceroute

######################################################################
# Globals
######################################################################

# Bogus is a counter for assinging IP address to unknown hosts
variable Bogus 1

# The number of runs to make.  Can be configured on command line.
variable NumRuns 4

# The number of trace routes to have running at once.  Can be configured
# on command line.
variable NumSpawns 4

# The number of trace routes currently running
variable NumSpawned 0

# A flag that is waited on.  Writing to this variable indicates that
# we are done.
variable Done 0

# This is an array of what links exist.  It is used to gain a list
# of all links.
# LinksL

# This contains the last line processed
variable CurLine {}

# This indicates whether to display verbose output
variable Verbose 0

# A list of IP address to run on.
variable ToRun {}

# PREV is indexed by fp and is the previous node visited for a
# running traceroute on that run.

# State variables
# NODES(<ip>) => state 
# LINKS(<src>,<dst>) => state
#       :mean)
#       :var)
#       :n)
#       :final) => list of final destinations

######################################################################
# Procedures
######################################################################

##################################################
# verbose <msg>
#  Sends <msg> to stderr if Verbose is 1
##################################################
proc verbose {s} {
    variable Verbose

    if {$Verbose} {
	puts stderr "$s"
    }
}

##################################################
# spawntr <ip>
#  This spawns a traceroute, updating all state variables.
# It also sets up handleinput to be run on each line.
##################################################
proc spawntr {ip} {
    variable traceroute
    variable PREV
    variable NumSpawned

    verbose "Spawning trace on $ip"
    set fp [open "|$traceroute -n $ip 2> /dev/null" r]
    set PREV($fp) $ip
    fconfigure $fp -blocking 0 -buffering line
    incr NumSpawned
    fileevent $fp readable "handleinput $fp $ip"
}

##################################################
# nexttr
#  This spawns more traceroutes, until the number run is
# equal to NumSpawns.
##################################################
proc nexttr {} { 
    variable ToRun
    variable NumSpawned
    variable NumSpawns
    set numtorun [expr $NumSpawns - $NumSpawned]
    if {$ToRun == {}} {
	alldone
    }
    for {set i 0} {$i < $numtorun} {incr i} {
	if {$ToRun == {}} {
	    break
	}
	spawntr [lpop ToRun]
    }
}

##################################################
# alldone
#  This is called when all traceroutes are finished.  It displays
# the output and flips the Done flag.
##################################################
proc alldone {} {	    
    variable Done
    variable NODES
    variable LinksL
    variable LINKS
    foreach node [array names NODES] {
	puts "NODE $node $NODES($node)"
    }
    foreach link [array names LinksL] {
	set t [split $link ,]
	puts "LINK [lindex $t 0] [lindex $t 1] $LINKS($link) $LINKS($link:mean) $LINKS($link:var) $LINKS($link:n) $LINKS($link:final)"
    }
    set Done 1
}

##################################################
# handleinput <fp> <ip>
#  This hanlde the input from a traceroute.  fp is a file pointer
# to the input stream and IP is the ultimate IP address being traced
# to.
# This also detects termination of traceroutes and calls
# nexttr.
##################################################
proc handleinput {fp ip} {
    variable PREV
    variable NumSpawned
    if {[eof $fp]} {
	close $fp
	incr NumSpawned -1
	verbose "$ip finished"
	nexttr
    } else {
	gets $fp line
	processline PREV($fp) $ip $line
    }
}

##################################################
# bgerror <msg>
#  Handly any errors that run in event spawned code.
##################################################
proc bgerror {msg} {
    variable CurLine
    puts stderr "bgerror: $msg"
    puts stderr "Info: $CurLine"
}

##################################################
# processline <prev var> <final dst> <line>
#  This is the main processor.  It takes the name of the 
# variable containing the previous IP address, the IP of
# the final destionation, and the line to process.
##################################################
proc processline {prevV finaldst line} {
    variable NODES
    variable LINKS
    variable LinksL
    variable Bogus
    variable CurLine

    upvar $prevV prev
    set CurLine "$prev: $line"

    # FORMAT:
    # first word is link number and can be ignored
    # next is IP address or 3 *s
    # XXX we assume for now that we will either get a normal
    #  reply or a reply of all stars.  This is not accurate but
    #  is usually the case and simplifies things for now.
    # if 3 *s then we assign it a bogus IP address (0.0.0.#) and put
    #  the qualities as UP and unknown time.
    # if non three stars then the second word is the IP address
    #  and the following words are times and ! notes
    # XXX we also assume that all RTTs are in ms.  This may or may
    #  not be accurate.
    if {[regexp {traceroute:} $line]} {
	# error at this point... we'll jump out and let it be an unknown
	# XXX probably want to do something more intelligent at some point
	return
    }
    set l [lrange $line 1 end]
    set dst {}
    set num 0
    while {$dst == {} && $l != {}} {
	set e [lpop l]
	if {$e == "*"} {
	    incr num
	    if {$num == 3} {break}
	} else {
	    set dst $e
	}
    }
    if {$dst == {}} {
	set dst "0.0.0.$Bogus"
	incr Bogus
    }
    # load/init data
    if {[info exists LINKS($prev,$dst)]} {
	set state $LINKS($prev,$dst)
	set mean $LINKS($prev,$dst:mean)
	set n $LINKS($prev,$dst:n)
	set var $LINKS($prev,$dst:var)
    } else {
	set state UNKNOWN
	set mean 0
	set n 0
	set var 0
    }
    while {$l != {}} {
	set e [lpop l]
	if {$e == "ms"} {continue
	} elseif {$e == "!" || $e == "*"} {set state "OK?"
	} elseif {$e == "!H" || $e == "!N" || $e == "!P"} {
	    set state "DOWN"
	} elseif {$e == "!S" || $e == "!F"} {
	    set state "BAD"
	} elseif {$e == "!X"} {
	    set state "DENIED"
	} elseif {[string index $e 0] == "!"} {
	    set state "ERROR-[string range $e 1 end]"
	} else {
	    if {$state == "UNKNOWN"} {
		set state "UP"
	    }
	    set rtt $e
	    # update mean and variance
	    incr n
	    set oldmean $mean
	    set mean [expr 1.0*$mean+($rtt-$mean)/$n]
	    set k [expr 1.0*$var+$oldmean*$oldmean]
	    set var [expr 1.0*$k+($rtt*$rtt-$k)/$n-$mean*$mean]
	}
    }
    # update link state
    set LinksL($prev,$dst) 1
    set LINKS($prev,$dst) $state
    set LINKS($prev,$dst:mean) $mean
    set LINKS($prev,$dst:n) $n
    set LINKS($prev,$dst:var) $var
    if {! [info exists LINKS($prev,$dst:final)] ||
	[lsearch $LINKS($prev,$dst:final) $finaldst] == -1} {
	lappend LINKS($prev,$dst:final) $finaldst
    }
    # update node state 
    set NODES($prev) "UP"
    if {$state == "OK"} {
	set NODES($dst) "UP"
    } elseif {$state == "OK?"} {
	set NODES($dst) "UP?"
    }
    # update vars
    set prev $dst
}

##################################################
# showhelp
#  Displays help info to stdout and exits.
##################################################
proc showhelp {} {
    puts "netpull \[<options>\] <file>"
    puts "Options:"
    puts " -n #     - Number of simulatenous trace routes."
    puts " -r #     - Number of times to trace each ip."
    puts " -v       - Display extra output to stderr."
    exit 1
}

######################################################################
# Main
######################################################################

# Check arguments
set file {}
while {$argv != {}} {
    set arg [lpop argv]
    switch -- $arg {
	"-n" {
	    if {$argv == {}} {showhelp}
	    set NumSpawns [lpop argv]
	}
	"-r" {
	    if {$argv == {}} {showhelp}
	    set NumRuns [lpop argv]
	}
	"-v" {
	    set Verbose 1
	}
	default {
	    if {$file != {}} {showhelp}
	    set file $arg
	}
    }
}

# Open file
if {[catch "open $file r" fp]} {
    puts stderr "Error opening $file."
    exit 1
}

# Read the IP addresses
set ips [read $fp]
close $fp

# Create a list of IPs to run and initalize node state of edge nodes
foreach ip $ips {
    set NODES($ip) UNKNOWN
}
for {set i 0} {$i < $NumRuns} {incr i} {
    set ToRun [concat $ToRun $ips]
}

# spawn some trace routes
nexttr

# and wait for us to finish
vwait Done
