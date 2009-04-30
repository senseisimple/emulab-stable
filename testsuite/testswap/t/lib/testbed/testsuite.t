#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Data::Dumper;
use Test::Exception;
use Test::More tests => 10;

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

sub hash_equals {
  my ($a1, $a2) = @_;
  while (my ($k, $v) = each %$a1) {
    if ($a2->{$k} ne $v) {
      say "$k $v " . $a2->{$k};
      return 0;
    }
  }
  return 1;
}

sub array_of_hash_equals {
  my ($a1, $a2) = @_;
  return 0 if ((scalar @$a1) != (scalar @$a2));
  for (0 .. (@$a2 - 1)) {
    return 0 unless hash_equals($a1->[$_], $a2->[$_]);
  }
  return 1;
}

my $expected1 = [ { 'a' => 'COOL', 'b' => 'b1' }, { 'a' => 'a2', 'b' => 'b2' } ];
my $expected2 = [ { 'a' => 'a2', 'b' => 'b1' }, { 'a' => 'a2', 'b' => 'b2' } ];
my @result1 = CartProd($b, 'filter' => $filter, 'generator' => $gen);
ok(array_of_hash_equals( $expected1, \@result1), 'CartProd($config, filter => $f, generator => $g)');
#say Dumper($_) for (@result);
my @result2 = CartProd($b, 'filter' => $filter2);
ok(array_of_hash_equals( $expected2, \@result2), 'CartProd($config, filter => $f_and_gen)');
@result2 = CartProd($b, $filter2);
ok(array_of_hash_equals( $expected2, \@result2), 'CartProd($config, $filter_and_gen)');
#say Dumper($_) for (@result2);

ok(hash_equals( defaults({ 'a' => 'B' }, 'a' => 'A', b => 'B'), { 'a' => 'B', 'b' => 'B' } ), 'defaults1');
ok(hash_equals( override({ 'a' => 'B' }, 'a' => 'A', b => 'B'), { 'a' => 'A', 'b' => 'B' } ), 'override1');

is_deeply(TestBed::TestSuite::_build_e_from_positionals(), {}, 'e()');
is_deeply(TestBed::TestSuite::_build_e_from_positionals('e1'), { 'eid' => 'e1' }, 'e($eid)');
is_deeply(TestBed::TestSuite::_build_e_from_positionals('p1', 'e1'), { 'pid' => 'p1', 'eid' => 'e1' }, 'e($pid, $eid)');
is_deeply(TestBed::TestSuite::_build_e_from_positionals('p1', 'g1', 'e1'), {  'pid' => 'p1', 'gid' => 'g1', 'eid' => 'e1' }, 'e($pid, $gid, $eid)');
dies_ok( sub { TestBed::TestSuite::_build_e_from_positionals(1, 2, 3, 4) }, 'e(1,2,3,4) dies');
