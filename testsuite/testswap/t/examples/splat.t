#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 1;
use Data::Dumper;
use BasicNSs;
ok(e()->startrunkill($BasicNSs::TwoNodeLan, sub {
  my $e = shift;
  $e->splat("JUNK", "junk.txt");
}));
