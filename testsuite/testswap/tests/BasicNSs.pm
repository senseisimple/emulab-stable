#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
package BasicNSs;

our $TwoNodeLan = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 100Mb 0ms]
$ns run
END

our $TwoNodeLan5Mb = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 5Mb 20ms]
$ns run
END

our $SingleNode = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]

$ns run
END

our $TwoNodeLanWithLink = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 5Mb 20ms]
set link1 [$ns duplex-link $node1 $node2 100Mb 50ms DropTail]
$ns run
END

our $TooManyLans = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 100Mb 0ms]
set lan2 [$ns make-lan "$node1 $node2" 100Mb 0ms]
set lan3 [$ns make-lan "$node1 $node2" 100Mb 0ms]
set lan4 [$ns make-lan "$node1 $node2" 100Mb 0ms]
set lan5 [$ns make-lan "$node1 $node2" 100Mb 0ms]
$ns run
END

1;
