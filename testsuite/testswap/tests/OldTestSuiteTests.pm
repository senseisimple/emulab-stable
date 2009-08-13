#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use Data::Dumper;
use OldTestSuite;

our @requires_db      = qw( db1 );
our @too_big          = qw( spinglass );
our @should_pass      = qw( fixed basic cbr complete5 delaylan1 delaylink multilink ixp lan1 nodes singlenode trafgen simplelink simplex setip red ping vtypes mininodes );
our @should_fail      = qw( negprerun toomanylinks toofast dnardshelf );
our @broken           = qw( );
our @unknown          = qw( sharkshelf spinglass2 basicrsrv wideareatypes minisetip db1 buddycache trivial minitbcmd wideareamapped minimultilink tbcmd );

=pod
vtypes (may want to parameterize the vtypes)
fixed (you will have to change the ns file depending on which nodes are available)
=cut

for (@unknown) {
  my $eid = $_;
  my $ns = $OldTestSuite::tests->{$_}->{'nsfile'};
  rege(e($_), $ns, sub { ok(!shift->ping_test, $eid); }, 1, $_)
}

=pod
for (@should_fail) {
  my $eid = $_;
  my $ns = $OldTestSuite::tests->{$_}->{'nsfile'};
  rege(e($_), $ns, sub { ok(!shift->ping_test, $eid); }, 1, $_, )
}
=cut
1;
