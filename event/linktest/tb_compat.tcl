# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

# This is a nop tb_compact.tcl file that should be used when running scripts
# under ns.

proc tb-set-ip {node ip} {}
proc tb-set-ip-interface {src dst ip} {}
proc tb-set-ip-link {src link ip} {}
proc tb-set-ip-lan {src lan ip} {}
proc tb-set-hardware {node type args} {}
proc tb-set-node-os {node os} {}
#proc tb-set-link-loss {src args} {}
proc tb-set-lan-loss {lan rate} {}
proc tb-set-node-rpms {node args} {}
proc tb-set-node-startup {node cmd} {}
proc tb-set-node-cmdline {node cmd} {}
proc tb-set-node-deltas {node args} {}
proc tb-set-node-tarfiles {node args} {}
proc tb-set-node-lan-delay {node lan delay} {}
proc tb-set-node-lan-bandwidth {node lan bw} {}
proc tb-set-node-lan-loss {node lan loss} {}
proc tb-set-node-lan-params {node lan delay bw loss} {}
proc tb-set-node-failure-action {node type} {}
proc tb-set-ip-routing {type} {}
proc tb-fix-node {v p} {}
proc tb-make-weighted-vtype {name weight types} {}
proc tb-make-soft-vtype {name types} {}
proc tb-make-hard-vtype {name types} {}
proc tb-set-lan-simplex-params {lan node todelay tobw toloss fromdelay frombw fromloss} {}
proc tb-set-link-simplex-params {link src delay bw loss} {}
proc tb-set-uselatestwadata {onoff} {}
proc tb-set-usewatunnels {onoff} {}
proc tb-set-wasolver-weights {delay bw plr} {}
proc tb-set-uselinkdelays {onoff} {}
proc tb-set-forcelinkdelays {onoff} {}

Class Program

Program instproc init {args} {
}

Program instproc unknown {m args} {
}

Class NSENode -superclass Node

NSENode instproc make-simulated {args} {
    uplevel 1 eval $args
}

# We are just syntax checking the NS file
Simulator instproc run {args} {
}

Simulator instproc nsenode {args} {
    return [new NSENode]
}

Simulator instproc make-simulated {args} {
    uplevel 1 eval $args
}

# LINKTEST FUNCTIONS

# holds a pair of simplex links.
Class Duplink

# rename set in order to capture the variable names used in the ns file.
variable last_host {}
variable last_lan {}
variable last_link {}
#  arrays mapping tcl hostnames to variable names
variable hosts
variable lans
variable links

rename set real_set
proc set {args} {
    global last_host last_lan last_link
    global hosts lans links
    real_set var [lindex $args 0]
        
    # Here we change ARRAY(INDEX) to ARRAY-INDEX
    regsub -all {[\(]} $var {-} out
    regsub -all {[\)]} $out {} val

    if {[llength $last_host] > 0 } {
	array set hosts [list $last_host $val]
        real_set last_host {}
    }
    if {[llength $last_lan] > 0 } {
	array set lans [list $last_lan $val]
        real_set last_lan {}
    }
    if {[llength $last_link] > 0 } {
	array set links [list $last_link $val]
        real_set last_link {}
    }
    
    # in all cases do a real set.
    if {[llength $args] == 1} {
        return [uplevel real_set \{[lindex $args 0]\}]
    } else {
        return [uplevel real_set \{[lindex $args 0]\} \{[lindex $args 1]\}]
    }
}

# converts internal ns structures to linktest structure
# where it's easier for me to get to them.
Class LTLink

LTLink instproc init {} {
    $self instvar lanOrLink_ src_ dst_ bw_ delay_ loss_
    # note: lanOrLink is the "owner" entity.
    set loss_ 0
}

LTLink instproc lanOrLink {} {
    $self instvar lanOrLink_
    set lanOrLink_
}

LTLink instproc src {} {
    $self instvar src_
    set src_
}

LTLink instproc dst {} {
    $self instvar dst_
    set dst_
}
LTLink instproc bw {} {
    $self instvar bw_
    set bw_
}
LTLink instproc delay {} {
    $self instvar delay_
    set delay_
}
LTLink instproc loss {} {
    $self instvar loss_
    set loss_
}
LTLink instproc set_lanOrLink { lanOrLink } {
    $self instvar lanOrLink_
    set lanOrLink_ $lanOrLink
}
LTLink instproc set_src { src } {
    $self instvar src_
    set src_ $src
}
LTLink instproc set_dst { dst } {
    $self instvar dst_
    set dst_ $dst
}
# use some parsing procs provided by ns.
LTLink instproc set_bw { bw } {
    $self instvar bw_
    set bw_ [bw_parse $bw]
}
LTLink instproc set_delay { delay } {
    $self instvar delay_
    set delay_ [time_parse $delay]
}
LTLink instproc set_loss { loss } {
    $self instvar loss_
    set loss_ $loss
}


LTLink instproc toString {} {
    $self instvar lanOrLink_ src_ dst_ bw_ delay_ loss_
    global hosts lans links
    if { 0 == [llength [array get hosts $dst_] ] } {
	return "link $lanOrLink_ $hosts($src_) $lans($dst_) $bw_ $delay_ $loss_"
    } elseif {
	0 == [llength [array get hosts $src_] ]
    } {
	return "link $lanOrLink_ $lans($src_) $hosts($dst_) $bw_ $delay_ $loss_"
    } else {
	return "link $lanOrLink_ $hosts($src_) $hosts($dst_) $bw_ $delay_ $loss_"
    }

}


# linktest representation of links, containing LTLinks.
variable lt_links {}

# called by ns to add the ns link to the linktest representation
# nice accessors not necessarily available so in some cases I get
# the inst vars directly
Simulator instproc addLTLink { linkref } {
    $self instvar Node_ link_
    global hosts lans links lt_links

    set newLink [new LTLink]
    $newLink set_src [$link_($linkref) src ]
    $newLink set_dst [$link_($linkref) dst ]
    
    if {0 == [string compare [$link_($linkref) info class ] "Vlink"]} {

	$newLink set_bw [$link_($linkref) set bw_ ]
	$newLink set_delay [$link_($linkref) set delay_ ]
	
	# lan reference
	$newLink set_lanOrLink [$link_($linkref) set lan_ ]
	
    } elseif {0 == [string compare [$link_($linkref) info class ] "SimpleLink"]} {
	$newLink set_bw [$link_($linkref) bw ]
	$newLink set_delay [$link_($linkref) delay ]
	
	# duplink reference
	$newLink set_lanOrLink [$link_($linkref) set linkRef_ ]
	
    } else {
	error "unknown link type!"
    }
    lappend lt_links $newLink
}

proc output {} {
    global hosts lans links lt_links

    # node names
    foreach name [array names hosts] {
	puts "node $hosts($name)"
    }
    foreach name [array names lans] {
	puts "lan $name $lans($name)"
    }
    foreach name [array names links] {
	puts "link $name $links($name)"
    }

    foreach lt $lt_links {
	puts "[$lt toString]"

    }
}
#
# from here on out, ns is not changed, just lt_links.
#
proc tb-set-link-loss {args} {
    global lt_links
    if { 2 == [llength $args] } {
	set link [lindex $args 0]
	set loss [lindex $args 1]

	foreach lt $lt_links {
	    if { $link == [$lt lanOrLink] } {
		$lt set_loss $loss
	    }
	}
    } else {

	set nodes [lrange $args 0 1]
	set loss [lindex $args 2]

	foreach lt $lt_links {
	    if { 
		[lsearch $nodes [$lt src]] > -1
		&&
		[lsearch $nodes [$lt dst]] > -1
	    } {
		$lt set_loss $loss
	    }
	}
    }
}

proc tb-set-lan-loss {lan rate} {
    global lt_links
    foreach lt $lt_links {
	if { $lan == [$lt lanOrLink] } {
	    $lt set_loss $rate
	}
    }
}

proc tb-set-link-simplex-params {link src delay bw loss} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $link == [$lt lanOrLink] 
	    &&
	    $src == [$lt src]
	} {
	    $lt set_delay $delay
	    $lt set_bw $bw
	    $lt set_loss $loss
	    return
	}
    }
}


