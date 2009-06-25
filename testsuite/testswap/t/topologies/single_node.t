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

ok(e('sn1')->startrunkill($ns, sub { shift->single_node_tests }), 'single_node_tests');
#ok(e('sn1')->single_node_tests, 'single_node_tests');
