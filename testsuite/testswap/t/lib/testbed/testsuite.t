#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Data::Dumper;
use Test::More tests => 1;

my $a = { 
  'a' => [qw(a1 a2 a3)],
  'b' => [qw(b1 b2 b3)],
  'c' => [qw(c1 c2 c3)],
};

our $filter = sub {
  ($_->{'a'} eq 'a1');
};

our $gen = sub {
  if ($_->{'b'} eq 'b1') {
    +{ %{$_}, 'a' => "COOL" }
  }
  else {
    $_;
  }
};
for (CartProd($a, 'filter' => $filter, 'generator' => $gen)) {
  say Dumper($_);
}
ok(1);
