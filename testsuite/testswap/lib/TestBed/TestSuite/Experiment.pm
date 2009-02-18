#!/usr/bin/perl
use Modern::Perl;

package TestBed::TestSuite::Experiment;
use Tools::PingTest;

use TestBed::XMLRPC::Client::Experiment;
use Test::More;
our @ISA = qw(Test::More);
our @EXPORT = qw(e ep echo newexp batchexp list list_brief list_full launchpingswapkill);

sub ep { TestBed::XMLRPC::Client::Experiment->new() }
sub e  { TestBed::XMLRPC::Client::Experiment->new('pid'=> shift, 'eid' => shift) }

sub echo          { ep()->echo(@_); }
sub _newexp       { my $e = e(shift, shift); $e->batchexp_ns(shift, @_); $e }
sub _newexp_wait  { _newexp(@_, 'wait' => 1); }
sub newexp        { _newexp(@_); }
sub newexp_wait   { _newexp_wait(@_); }
sub batchexp      { _newexp(@_); }
sub batchexp_wait { _newexp_wait(@_); }
sub list          { ep()->getlist; }
sub list_brief { ep()->getlist_brief; }
sub list_full  { ep()->getlist_full; }

sub connectivity_test {
  ping("node1.tewkt.tbres.emulab.net");
  ping("node2.tewkt.tbres.emulab.net");
}

sub launchpingswapkill {
  my ($pid, $eid, $ns) = @_;
  my $e = e($pid, $eid);
  $e->batchexp_ns($ns) && die "batchexp $eid failed";
  $e->waitforactive();
  connectivity_test($e);
  $e->nodeinfo();
  $e->swapoutw();
  $e->waitforswapped();
  $e->swapinw();
  $e->waitforactive();
  connectivity_test($e);
  $e->end();
}
1;
