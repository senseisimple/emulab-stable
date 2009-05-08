#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::ForkFramework;
use Data::Dumper;
use Test::Exception;
use Test::More tests => 2;

my $results = TestBed::ForkFramework::MaxWorkersScheduler::work(2, sub { my $d = `date`; chomp($d); $_[0] . " $d "  . $$; }, ['K1', 'K2', 'K3', 'K4'] );
ok($results->[0]== 0 && @{ $results->[1] } == 4, 'ForkFramework::MaxWorkersScheduler::work');
#say Dumper($results);
$results = TestBed::ForkFramework::RateScheduler::work(4, sub { my $d = `date`; chomp($d); $_[0] . " $d "  . $$; },
[[1, 1], [1, 0], [2, 2], [3, 3]],
['K1', 'K2', 'K3', 'K4'] );
ok($results->[0]== 0 && @{ $results->[1] } == 4, 'ForkFramework::RateScheduler::work');
#say Dumper($results);
