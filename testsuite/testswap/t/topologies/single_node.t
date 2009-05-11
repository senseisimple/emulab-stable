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

$ns run
NSEND

ok(launchpingswapkill(e('tewkt'), $ns));
