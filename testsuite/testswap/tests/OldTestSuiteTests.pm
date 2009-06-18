#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use Data::Dumper;
use OldTestSuite;

our @should_pass      = qw( basic cbr complete5 delaylan1 delaylink );
our @who_knows_passed = qw( lan1 multilink );
our @who_knows        = qw( ixp lan1 nodes singlenode trafgen simplelink simplex setip red ping );
our @should_fail      = qw( negprerun toomanylinks toofast );

#cbr basic complete5 delaylan1 delaylink 
#lan1 multilink 
#ixp nodes singlenode trafgen simplelink simplex red ping 
#negprerun toomanylinks toofast

#unclassified
#frontend dnardshelf mini_nodes spinglass sharkshelf full vtypes spinglass2 fixed set-ip basic_rsrv widearea_types delaycheck mini_set-ip db1 10mbit buddycache trivial mini_tbcmd widearea_mapped mini_multilink tbcmd 

=pod
vtypes (may want to parameterize the vtypes)
fixed (you will have to change the ns file depending on which nodes are available)
=cut

for (@who_knows) {
  my $eid = $_;
  my $ns = $OldTestSuite::tests->{$_}->{'nsfile'};
  rege($_, $ns, sub { ok(!shift->ping_test, $eid); }, 1, $_)
}

1;
