#!/usr/bin/perl
use SemiModern::Perl;
package BasicNSs;

our $TwoNodeLan = << 'END';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 5Mb 20ms]
$ns run
END

1;
