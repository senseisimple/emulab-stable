#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 1;
use Data::Dumper;
use BasicNSs;
ok(e()->startrunkill($BasicNSs::SingleNode, sub {
  my $e = shift;
  $e->parallel_tevc( sub {my $n = $_[0]; return "now $n"; }, [$e->hostnames]));
}));
