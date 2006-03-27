# -*- tcl -*-
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

source ../ns2ir/nstb_compat.tcl

variable tbnsobj

# Linktest-specific functions. Source these before running
# linktest-ns.

# holds a pair of simplex links.
Class Duplink

Duplink instproc init {} {
}

Duplink instproc unknown {m args} {
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

#LTLink instproc clone {} {
#    $self instvar lanOrLink_ src_ dst_ bw_ delay_ loss_

#    set newLink [new LTLink]
#    $newLink set_src $src_
#    $newLink set_dst $dst_
#    $newLink set_bw $bw_ 
#    $newLink set_delay $delay_ 
#    $newLink set_loss $loss_ 
#    return $newLink
#}


# for final printing, always resolve lans to actual lists of hosts.
LTLink instproc toString {} {
    $self instvar lanOrLink_ src_ dst_ bw_ delay_ loss_ qtype_
    global hosts lans links tbnsobj

    if {[info exists links($lanOrLink_)]} {
	set name $links($lanOrLink_)
    } elseif {[info exists lans($lanOrLink_)]} {
	set name $lans($lanOrLink_)
    } else {
	set name [$lanOrLink_ set tbaltname_]
    }

    set qt $qtype_
    if {$qtype_ == "DropTail"} {
	set qt "droptail"
    } elseif {$qtype_ == "RED"} {
	set qt "red"
    } elseif {$qtype_ == "GRED"} {
	set qt "gred"
    }
    
    if {[$tbnsobj link $src_ $dst_] != ""} {
	set lq [[$tbnsobj link $src_ $dst_] queue]
	if {[$lq info vars red_] != ""} {
	    if {[$lq set red_] == 1} {
		if {[$lq info vars gentle_] != "" && [$lq set gentle_] == 1} {
		    set qt "gred"
		} else {
		    set qt "red"
		}
	    } else {
		set qt "droptail"
	    }
	}
    }

    return [format "l $hosts($src_) $hosts($dst_) %.0f %.4f %.6f $name $qt" $bw_ $delay_ $loss_ ]
}


# linktest representation of links, containing LTLinks.
variable lt_links {}

# called by ns to add the ns link to the linktest representation
# nice accessors not necessarily available so in some cases I get
# the inst vars directly
Simulator instproc addLTLink { linkref {qtype DropTail} } {
    $self instvar Node_ link_
    global hosts lans links lt_links tbnsobj

    set tbnsobj $self

    set newLink [new LTLink]
    $newLink set_src [$link_($linkref) src ]
    $newLink set_dst [$link_($linkref) dst ]
    $newLink set qtype_ $qtype

    if {0 == [string compare [$link_($linkref) info class ] "Vlink"]} {

	$newLink set_bw [$link_($linkref) set bw_ ]
	$newLink set_delay [$link_($linkref) set delay_ ]
	
	# lan reference
	$newLink set_lanOrLink [$link_($linkref) set lan_ ]

	# netbed-specific implementation for lans: add 1/2 the delay
	$newLink set_delay [expr [$newLink delay] / 2.0]

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

# just print the representation to stdout
Simulator instproc run {args} {
    global simname

    if {${::GLOBALS::security_level} >= 1} {
	global fw

	set fw [new Firewall $self]
    }
    
    join_lans
    output
    real_puts "s $simname"
}

# store the rtproto
Simulator instproc rtproto {arg} {
    global rtproto

    if {$arg == "Session" || $arg == "ospf"} {
	set rtproto "ospf"
    } elseif {$arg == "Manual"} {
	set rtproto "manual"
    } elseif {$arg == "Static"} {
	set rtproto "static"
    } elseif {$arg == "Static-ddijk"} {
	set rtproto "static-ddijk"
    } elseif {$arg == "Static-old"} {
	set rtproto "static-old"
    }
}

# update lt_links such that lans become new links containing destination hosts
# delay: sum both delays
# loss:  product both losses
# bandwidth: min of both bandwidths (the bottleneck)
# 
proc join_lans {} {
    global lt_links lans
    set new_links {}
    set all_lans [array names lans]

    foreach srclink $lt_links {
	# dst a lan link?
	if { [lsearch $all_lans [$srclink dst]] > -1  } {
	    set lan [$srclink dst]

	    # find all of the "receivers" for this lan.
	    foreach dstlink $lt_links {
		if { $lan == [$dstlink src] 
		    && 
		     [$srclink src] != [$dstlink dst]
		 } {
		    set newLink [new LTLink]
		    $newLink set lanOrLink_ $lan
		    $newLink set qtype_ droptail
		    $newLink set_src [$srclink src]
		    $newLink set_dst [$dstlink dst]
		    $newLink set_bw [expr [$srclink bw] < [$dstlink bw] ? [$srclink bw] : [$dstlink bw]]
		    $newLink set_delay [expr [$srclink delay ] + [$dstlink delay]]
		    $newLink set_loss [expr 1.0 - (1.0 - [$srclink loss] ) * (1.0 - [$dstlink loss])]
#		    puts [$newLink toString]
		    lappend new_links $newLink
		    
		}
	    }


	    
	} elseif {[lsearch $all_lans [$srclink src]] > -1  } {
#	    puts "ignored"
	} else {
	    lappend new_links $srclink
	}
    }
    set lt_links $new_links
}


proc output {} {
    global hosts lans links lt_links rtproto simname
    global $simname

    real_set ns [set $simname]
    foreach name [$ns all-nodes-list] {
	if {[$name info class] == "Node"} {
	    if {[info exists hosts($name)]} {
		real_puts "h $hosts($name)"
	    } else {
		real_puts "h [$name set tbaltname_]"
	    }
	}
    }
    foreach link $lt_links {
	real_puts "[$link toString]"

    }
    if { [llength $rtproto]>0} {
	real_puts "r $rtproto"
    }
}
#
# from here on out, implement netbed-specific commands against lt_links
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

    # netbed-implentation detail: set loss to 1-sqrt(1-L)
    set a [expr 1.0 - $rate]
    set b [expr sqrt ($a)]
    set newloss [expr 1.0 - $b]

    foreach lt $lt_links {
	if { $lan == [$lt lanOrLink] } {
	    $lt set_loss $newloss
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

proc tb-set-lan-simplex-params {lan node todelay tobw toloss fromdelay frombw fromloss} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $lan == [$lt src] 
	    &&
	    $node == [$lt dst]
	} {
	    $lt set_delay $fromdelay
	    $lt set_bw $frombw
	    $lt set_loss $fromloss
	}
	if { 
	    $lan == [$lt dst] 
	    &&
	    $node == [$lt src]
	} {
	    $lt set_delay $todelay
	    $lt set_bw $tobw
	    $lt set_loss $toloss
	}

    }
}
proc tb-set-node-lan-delay {node lan delay} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $lan == [$lt src] 
	    &&
	    $node == [$lt dst]
	} {
	    $lt set_delay $delay
	}
	if { 
	    $lan == [$lt dst] 
	    &&
	    $node == [$lt src]
	} {
	    $lt set_delay $delay
	}
    }
}
proc tb-set-node-lan-bandwidth {node lan bandwidth} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $lan == [$lt src] 
	    &&
	    $node == [$lt dst]
	} {
	    $lt set_bw $bandwidth
	}
	if { 
	    $lan == [$lt dst] 
	    &&
	    $node == [$lt src]
	} {
	    $lt set_bw $bandwidth
	}
    }
}

proc tb-set-node-lan-params {node lan delay bw loss} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $lan == [$lt src] 
	    &&
	    $node == [$lt dst]
	} {
	    $lt set_loss $loss
	    $lt set_delay $delay
	    $lt set_bw $bw
	}
	if { 
	    $lan == [$lt dst] 
	    &&
	    $node == [$lt src]
	} {
	    $lt set_loss $loss
	    $lt set_delay $delay
	    $lt set_bw $bw
	}
    }

}

proc tb-set-node-lan-loss {node lan loss} {
    global lt_links
    foreach lt $lt_links {
	if { 
	    $lan == [$lt src] 
	    &&
	    $node == [$lt dst]
	} {
	    $lt set_loss $loss
	}
	if { 
	    $lan == [$lt dst] 
	    &&
	    $node == [$lt src]
	} {
	    $lt set_loss $loss
	}
    }
}

