#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 1;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]

$ns run
NSEND

ok(e('sn1')->launchpingswapkill($ns));
