#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use BasicNSs;

my $test = sub {
  my $e = shift;
  ok( $e->ping_test, 'ping test');
  ok( $e->splat("JUNK", "junk.txt"), '$e->splat("JUNK", "junk.txt")');
  ok( $e->loghole_sync_allnodes, '$e->loghole_sync_allnodes');
#  ok( $e->parallel_tevc( sub {my $n = $_[0]; return "now $n"; }, [$e->hostnames]), '$e->parallel_tevc');
  ok($e->cartesian_ping, 'cartesian_ping');
  ok($e->cartesian_connectivity, 'cartesian_connectivity');
};

rege(e('features'), $BasicNSs::TwoNodeLan, $test, 5, 'tbts features');

