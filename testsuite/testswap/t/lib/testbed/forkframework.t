#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::ForkFramework;
use TestBed::ParallelRunner::Executor;
use RPC::XML;
use Data::Dumper;
use Test::Exception;
use Test::More tests => 4;

my $date_id_sub = sub { my $d = `date`; chomp($d); $_[0] . " $d "  . $$; };

my $results = TestBed::ForkFramework::ForEach::max_work(2, $date_id_sub, ['K1', 'K2', 'K3', 'K4'] );
ok($results->has_errors == 0 && @{ $results->successes } == 4, 'ForkFramework::ForEach::max_work');
#say Dumper($results);

$results = TestBed::ForkFramework::WeightedScheduler::work(4, $date_id_sub, [['K1', 1], ['K2', 1], ['K3', 2], ['K4', 3] ] );
ok($results->has_errors == 0 && @{ $results->successes } == 4, 'ForkFramework::WeightedScheduler::work');
#say Dumper($results);

$results = TestBed::ForkFramework::WeightedScheduler::work(1, 
  sub { die TestBed::ParallelRunner::Executor::SwapinError->new( 
    original => RPC::XML::struct->new( { value => { cause => 'temp' } })); },
  [[ TestBed::ParallelRunner::Executor->buildt(test_count => 1, retry => 1), 1]]
);

ok($results->has_errors == 1 && @{ $results->errors } == 1, 'ForkFramework::WeightedScheduler::work, retry');
#say Dumper($results);

my $launchtime = time;
$results = TestBed::ForkFramework::WeightedScheduler::work(4, 
  sub { 
    die TestBed::ParallelRunner::Executor::SwapinError->new( 
    original => RPC::XML::struct->new( { value => { cause => 'temp' } })) if (time - $launchtime < 6); 1;},
  [[ TestBed::ParallelRunner::Executor->buildt(test_count => 1, backoff => "2:10:0"), 1]]
);

ok($results->has_errors == 0 && @{ $results->successes } == 1, 'ForkFramework::WeightedScheduler::work, backoff');
#say Dumper($results);
