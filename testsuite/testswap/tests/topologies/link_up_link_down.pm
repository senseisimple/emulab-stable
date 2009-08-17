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

set link1 [$ns duplex-link $node1 $node2 100Mb 50ms DropTail]

$ns run
NSEND
  
my $test = sub { 
  my ($e) = @_; 
  my $eid = $e->eid;
  ok($e->linktest, "$eid linktest"); 

  ok($e->link("link1")->down, "link down");
  sleep(2);

  my $n1ssh = $e->node("node1")->ssh;
  ok($n1ssh->cmdfailuredump("ping -c 5 10.1.2.3"));

  ok($e->link("link1")->up, "link up");
  sleep(2);
  ok($n1ssh->cmdsuccessdump("ping -c 5 10.1.2.3"));
};

rege(e('tplinkupdown'), $ns, $test, 5, 'single_node_tests');
