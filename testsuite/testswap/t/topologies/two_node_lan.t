#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use TestBed::TestSuite::Experiment;
use Test::More tests => 1;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 100Mb 0ms]
$ns run
NSEND

ok(launchpingswapkill(e('tewkt'), $ns));
