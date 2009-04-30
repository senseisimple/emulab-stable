#!/usr/bin/perl
package T2;
use TestBed::TestExperiment;
use BasicNSs;
use Test::More tests => 4;

my $sub = sub {
  ok(1, 'I\'m alive');
};

teste("k$_", "k$_ desc", $BasicNSs::TwoNodeLan, $sub) for (1..4);
1;
