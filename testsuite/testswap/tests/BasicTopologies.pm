#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;
use Data::Dumper;

my $linkupdowntest = sub {
  my ($e) = @_; 
  my $eid = $e->eid;
  ok($e->linktest, "$eid linktest"); 

  ok($e->link("link1")->down, "$eid link down");
  sleep(2);

  my $n1ssh = $e->node("node1")->ssh;
  ok($n1ssh->cmdfailure("ping -c 5 10.1.2.3"), "$eid expected ping failure");

  ok($e->link("link1")->up, "$eid link up");
  sleep(2);
  ok($n1ssh->cmdsuccess("ping -c 5 10.1.2.3"), "$eid expected ping success");
};

rege(e('linkupdown'), $BasicNSs::TwoNodeLanWithLink, $linkupdowntest, 5, 'link up and down with ping on link');

my $twonodelan5Mbtest = sub {
  my ($e) = @_; 
  my $eid = $e->eid;
  ok($e->linktest, "$eid linktest"); 
};

rege(e('2nodelan5Mb'), $BasicNSs::TwoNodeLan5Mb, $twonodelan5Mbtest, 1, '2nodelan5Mb linktest');
rege(e('1singlenode'), $BasicNSs::SingleNode, sub { ok(shift->ping_test, "1singlenode ping test"); }, 1, 'single node pingswapkill');
rege(e('2nodelan'), $BasicNSs::TwoNodeLan, sub { ok(shift->ping_test, "2nodelan ping test"); }, 1, 'two node lan pingswapkill');

1;
