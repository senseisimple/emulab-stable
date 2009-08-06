#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use Data::Dumper;
use OldTestSuite;

our @should_pass      = qw( basic cbr complete5 delaylan1 delaylink multilink ixp lan1 nodes singlenode trafgen simplelink simplex setip red ping );
our @should_fail      = qw( negprerun toomanylinks toofast );

#unclassified
#frontend dnardshelf mini_nodes spinglass sharkshelf full vtypes spinglass2 fixed set-ip basic_rsrv widearea_types delaycheck mini_set-ip db1 10mbit buddycache trivial mini_tbcmd widearea_mapped mini_multilink tbcmd 

=pod
vtypes (may want to parameterize the vtypes)
fixed (you will have to change the ns file depending on which nodes are available)
=cut

=pod
for (@should_pass) {
  my $eid = $_;
  my $ns = $OldTestSuite::tests->{$_}->{'nsfile'};
  rege(e($_), $ns, sub { ok(!shift->ping_test, $eid); }, 1, $_)
}
=cut

for (@should_fail) {
  my $eid = $_;
  my $ns = $OldTestSuite::tests->{$_}->{'nsfile'};
  rege(e($_), $ns, sub { ok(!shift->ping_test, $eid); }, 1, $_, )
}

1;
