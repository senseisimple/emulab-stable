#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::ForkFramework;
use TestBed::ParallelRunner::Executor;
use RPC::XML;
use Data::Dumper;
use Test::Exception;
use Test::More tests => 3;

my $results = TestBed::ForkFramework::MaxWorkersScheduler::work(2, sub { my $d = `date`; chomp($d); $_[0] . " $d "  . $$; }, ['K1', 'K2', 'K3', 'K4'] );
ok($results->has_errors == 0 && @{ $results->successes } == 4, 'ForkFramework::MaxWorkersScheduler::work');
#say Dumper($results);

$results = TestBed::ForkFramework::RateScheduler::work(4, sub { my $d = `date`; chomp($d); $_[0] . " $d "  . $$; },
[[1, 1], [1, 0], [2, 2], [3, 3]],
['K1', 'K2', 'K3', 'K4'] );
ok($results->has_errors == 0 && @{ $results->successes } == 4, 'ForkFramework::RateScheduler::work');
#say Dumper($results);

use TestBed::ParallelRunner::ErrorStrategy;
$results = TestBed::ForkFramework::RateScheduler::work(4, 
  sub { die TestBed::ParallelRunner::Executor::SwapinError->new( 
    original => RPC::XML::struct->new( { value => { cause => 'temp' } })); },
  [[1, 0]],
  [ TestBed::ParallelRunner::Executor->new(test_count => 1,
    error_strategy => TestBed::ParallelRunner::ErrorRetryStrategy->new) ]
);

ok($results->has_errors == 1 && @{ $results->errors} == 1, 'ForkFramework::RateScheduler::work, retry');
#say Dumper($results);
