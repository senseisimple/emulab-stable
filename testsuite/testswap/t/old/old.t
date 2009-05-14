#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use TestBed::TestSuite::Experiment
use Test::More qw(no_plan);
use Data::Dumper;
require 't/old/oldtestsuite.pm';
our @pass = qw(basic cbr complete5 delaylan1 delaylink);
our @who_knows_passed = qw( lan1 multilink );
our @who_knows = qw( ixp lan1 nodes singlenode trafgen simplelink simplex setip red ping );
our @should_fail = qw(negprerun toomanylinks toofast);

=pod
vtypes (may want to parameterize the vtypes)
S   fixed (you will have to change the ns file depending on which nodes are
           available)
=cut

#for (@pass) {
for (@who_knows) {
  my $ns = $Testbed::OldTestSuite::data->{$_}->{'nsfile'};
  say "Running " . $_;
  say $ns;
  ok(e($_)->launchpingkill($ns), $_);
}
