#!/usr/bin/perl
package RateLimitParallelExample;
use TestBed::TestExperiment;
use BasicNSs;
use Test::More;

my $test_body = sub {
  my $e = shift;
  ok(!($e->ping_test), 'Ping Test');
};

#$eid, $ns, $test_desc, $ns, $desc)
teste("k$_", $BasicNSs::TwoNodeLan, $test_body, 1, "k$_ desc" ) for (1..2);
1;
