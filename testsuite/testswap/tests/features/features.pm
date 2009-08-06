#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use BasicNSs;

my $test = sub {
  my $e = shift;
  ok( $e->splat("JUNK", "junk.txt"), '$e->splat("JUNK", "junk.txt")');
  ok( $e->loghole_sync_allnodes, '$e->loghole_sync_allnodes');
#  ok( $e->parallel_tevc( sub {my $n = $_[0]; return "now $n"; }, [$e->hostnames]), '$e->parallel_tevc');
};

rege(e('features'), $BasicNSs::TwoNodeLan, $test, 2, 'tbts features');

