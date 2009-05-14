#!/usr/bin/perl
package RateLimitParallelExample;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;

my $test_body = sub {
  my $e = shift;
  ok(!($e->ping_test), 'Ping Test');
};

rege("k$_", $BasicNSs::TwoNodeLan, $test_body, 1, "k$_ desc" ) for (1..2);

1;
