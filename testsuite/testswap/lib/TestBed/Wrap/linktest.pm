#!/usr/bin/perl

package TestBed::Wrap::linktest;
use SemiModern::Perl;
use TBConfig;
use Data::Dumper;
use Tools;
use Tools::TBSSH;

=pod
  sleep 10;
  test_cmd 'linktest1', [], "run_linktest.pl -v -L 1 -l 1 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest2', [], "run_linktest.pl -v -L 2 -l 2 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest3', [], "run_linktest.pl -v -L 3 -l 3 -e $pid/$eid";
  sleep 2;
  test_cmd 'linktest4', [], "run_linktest.pl -v -L 4 -l 4 -e $pid/$eid";
=cut

sub linktest {
  my ($pid, $eid) = @_;
  my $ssh = Tools::TBSSH::ssh($TBConfig::OPS_SERVER, $TBConfig::EMULAB_USER);
  sleep 8;
  for my $i (1..4) {
    sleep 2;
    $ssh->cmdsuccessdump("run_linktest.pl -v -L $i -l $i -e $pid/$eid");
  }
}

1;
