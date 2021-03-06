#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TBConfig;
use TestBed::TestSuite;
use Data::Dumper;
use Test::Exception;
use Test::More tests => 36;

my $a = { 
  'a' => [qw(a1 a2 a3)],
  'b' => [qw(b1 b2 b3)],
  'c' => [qw(c1 c2 c3)],
};

my $b = { 
  'a' => [qw(a1 a2)],
  'b' => [qw(b1 b2)],
};

our $filter = sub {
  return undef if ($_->{'a'} eq 'a1');
  $_
};

our $filter2 = sub {
  !($_->{'a'} eq 'a1');
};

our $gen = sub {
  if ($_->{'b'} eq 'b1') {
    +{ %{$_}, 'a' => "COOL" }
  }
  else {
    $_;
  }
};

my $expected1 = [ { 'a' => 'COOL', 'b' => 'b1' }, { 'a' => 'a2', 'b' => 'b2' } ];
my $expected2 = [ { 'a' => 'a2', 'b' => 'b1' }, { 'a' => 'a2', 'b' => 'b2' } ];
my @result1 = CartProd($b, 'filter' => $filter, 'generator' => $gen);
is_deeply($expected1, \@result1, 'CartProd($config, filter => $f, generator => $g)');
#say Dumper($_) for (@result);
my @result2 = CartProd($b, 'filter' => $filter2);
is_deeply($expected2, \@result2, 'CartProd($config, filter => $f_and_gen)');
@result2 = CartProd($b, $filter2);
is_deeply($expected2, \@result2, 'CartProd($config, $filter_and_gen)');
#say Dumper($_) for (@result2);

use VNodeTest;

my $config = {
  'OS'       => [qw( AOS BOS COS )],
  'HARDWARE' => [qw( AHW BHW CHW )],
  'LINKTYPE' => [qw( ALT BLT CLT )],
};

CartProdRunner(\&VNodeTest::VNodeTest, $config);


is_deeply(( defaults({ 'a' => 'B' }, 'a' => 'A', b => 'B'), { 'a' => 'B', 'b' => 'B' } ), 'defaults1');
is_deeply(( override({ 'a' => 'B' }, 'a' => 'A', b => 'B'), { 'a' => 'A', 'b' => 'B' } ), 'override1');

dies_ok( sub { TestBed::TestSuite::_build_e_from_positionals(1, 2, 3, 4) }, 'e(1,2,3,4) dies');
is(e()->eid, "RANDEID1", 'random eid');


is_deeply(concretize('@OS@', OS=>'FOOBAR'), "FOOBAR", 'OS=>FOOBAR');
$TBConfig::cmdline_defines = { OS=>'GOODBYE' };
is_deeply(concretize('@OS@', OS=>'FOOBAR'), "GOODBYE", 'OS=>FOOBAR -D OS=GOODBY');
