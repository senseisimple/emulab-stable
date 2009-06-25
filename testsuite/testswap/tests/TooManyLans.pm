#!/usr/bin/perl
package TooManyLans;
use SemiModern::Perl;
use TestBed::TestSuite;
use BasicNSs;
use Test::More;
use TestBed::ParallelRunner;

my $test_body = sub {
  my $e = shift;
  ok(!($e->ping_test), 'Ping Test');
};

sub handleResult {
  my ($executor, $scheduler, $result) = @_;
  if ($result->error) {
    my $newns = $BasicNSs::TooManyLans;
    $newns =~ s/^set lan5.*$//m;
    say "In TooManyLans::handleResult";
    $executor->e->modify_ns_wait($newns);
    say "Done with TooManyLans->modify_ns_wait";
    $executor->e->swapin_wait;
    exit;
  }
}

pr_e(e('toomanylans'), $BasicNSs::TooManyLans, $test_body, 1, "too many lans", retry => 1, pre_result_handler => \&handleResult);
1;
