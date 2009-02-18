#!/usr/bin/perl
use Modern::Perl;
use TestBed::TestSuite::Experiment tests => 1;
use Test::More;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 100Mb 0ms]
$ns run
NSEND

ok(launchpingswapkill('tbres', 'tewkt', $ns));
