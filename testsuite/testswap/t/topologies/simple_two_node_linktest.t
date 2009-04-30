#!/usr/bin/perl
use SemiModern::Perl;
use TBConfig;
use TestBed::TestSuite;
use Test::More tests => 1;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 5Mb 20ms]
$ns run
NSEND

my $eid='simple';
my $e = e($eid);
$e->startrunkill($ns,  
  sub { 
    my ($e) = @_; 
    ok($e->linktest, "$eid linktest"); 
  } 
);
