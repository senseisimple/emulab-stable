#!/usr/bin/perl
package RateLimitParallelExample;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;

my $test_body = sub {
  my $e = shift;
  my $eid = $e->eid;
  sleep(5);
  ok(!($e->ping_test), "$eid Ping Test");
  sleep(5);
};

rege(e("ksks$_"), $BasicNSs::SingleNode, $test_body, 1, "k$_ desc" ) for (1..5);

1;
