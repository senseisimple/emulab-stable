#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 7;
use Test::Exception;
use Data::Dumper;
use BasicNSs;

my $e = e('ensureactive');

ok(!$e->startexp_ns_wait($BasicNSs::TwoNodeLan), 'first start');
throws_ok {$e->startexp_ns_wait($BasicNSs::TwoNodeLan)} 'RPC::XML::struct', 'failed second start';
ok(!$e->ensure_active_ns($BasicNSs::TwoNodeLan), 'ensure active_start');
ok(!$e->end_wait, 'kill_wait succeded');

ok(!$e->ensure_active_ns($BasicNSs::TwoNodeLan), 'ensure active_start');
throws_ok {$e->startexp_ns_wait($BasicNSs::TwoNodeLan)} 'RPC::XML::struct', 'failed second start';
ok(!$e->end_wait, 'kill_wait succeded');
