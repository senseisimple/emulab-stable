#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 3;
use Data::Dumper;
use BasicNSs;

my $e = e('ensureactive');
=pod
ok(!$e->startexp_ns_wait($BasicNSs::TwoNodeLan), 'first start');
ok($e->startexp_ns_wait($BasicNSs::TwoNodeLan), 'failed second start');
ok(!$e->ensure_active_ns($BasicNSs::TwoNodeLan), 'ensure active_start');
ok(!$e->end);
=cut

sleep(5);
system('./sc');

ok(!$e->ensure_active_ns($BasicNSs::TwoNodeLan), 'ensure active_start');
ok($e->startexp_ns_wait($BasicNSs::TwoNodeLan), 'failed second start');
ok(!$e->end);
