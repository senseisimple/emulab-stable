#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package TooManyLans;
use SemiModern::Perl;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;
use TestBed::ParallelRunner;

my $test_body = sub {
  my $e = shift;
  my $eid = $e->eid;
  ok($e->ping_test, "$eid Ping Test");
};

sub handleResult {
  my ($executor, $scheduler, $result) = @_;
  if ($result->error) {
    my $newns = $BasicNSs::TooManyLans;
    $newns =~ s/^set lan5.*$//m;
    say "In TooManyLans::handleResult";
    $executor->e->modify_ns_wait($newns);
    say "Done with TooManyLans->modify_ns_wait";
  }
}

rege(e('toomanylans'), $BasicNSs::TooManyLans, $test_body, 1, "too many lans", retry => 1, pre_result_handler => \&handleResult);
1;
