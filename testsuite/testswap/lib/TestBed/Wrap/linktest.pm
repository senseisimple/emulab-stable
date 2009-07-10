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

=head1 NAME

TestBed::Wrap::linktest

=over 4

=item C<linktest($pid, $eid)>

executes linktest on $pid and $eid by sshing to ops

=back

=cut

sub linktest {
  my ($pid, $eid) = @_;
  my $results = 0;
  my $ssh = Tools::TBSSH::instance($TBConfig::OPS_SERVER);
  sleep 8;
  for my $i (1..4) {
    sleep 2;
    my $cmd = 'PATH=/usr/testbed/bin:$PATH '. "run_linktest.pl -v -L $i -l $i -e $pid/$eid";
    #say $cmd;
    $results && $ssh->cmdsuccess($cmd);
  }
  !$results;
}

1;
