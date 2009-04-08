#!/usr/bin/perl

package TestBed::Wrap::tevc;
use SemiModern::Perl;
use TBConfig;
use Data::Dumper;
use Tools;
use Tools::TBSSH;

=pod
tevc -e proj/expt time objname event [args ...]

where the time parameter is one of:

* now
* +seconds (floating point or integer)
* [[[[yy]mm]dd]HH]MMss 

For example, you could issue this sequence of events.

tevc -e testbed/myexp now cbr0 set interval_=0.2
tevc -e testbed/myexp +10 cbr0 start
tevc -e testbed/myexp +15 link0 down
tevc -e testbed/myexp +17 link0 up
tevc -e testbed/myexp +20 cbr0 stop
=cut

sub tevc {
  my ($pid, $eid, @args) = @_;
  my $cmd = 'PATH=/usr/testbed/bin:$PATH tevc ' . "-e $pid/$eid " . join(" ", @args);
  say $cmd;
  Tools::TBSSH::cmdsuccessdump($TBConfig::OPS_SERVER, $cmd);
}

1;
