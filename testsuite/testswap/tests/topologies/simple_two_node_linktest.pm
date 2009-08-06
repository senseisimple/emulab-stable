#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 5Mb 20ms]
$ns run
NSEND

rege(e('twonodelinktest'), $ns, 
  sub { 
    my ($e) = @_; 
    my $e = $e->eid;
    ok($e->linktest, "$eid linktest"); 
  }, 1, 'single_node_tests');
