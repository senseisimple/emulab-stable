#!/usr/bin/perl
use SemiModern::Perl;

package TestBed::TestSuite::Experiment;
use Mouse;
use TestBed::XMLRPC::Client::Experiment;
use TestBed::Wrap::tevc;
use TestBed::Wrap::linktest;
use Tools::TBSSH;
use Data::Dumper;
use TestBed::TestSuite::Node;

extends 'Exporter', 'TestBed::XMLRPC::Client::Experiment';
require Exporter;
our @EXPORT;
push @EXPORT, qw(e ep launchpingkill launchpingswapkill);

sub ep { TestBed::TestSuite::Experiment->new }
sub e  { TestBed::TestSuite::Experiment->new('pid'=> shift, 'eid' => shift) }

sub nodes {
  my ($e) = @_;
  my @node_instances = map { TestBed::TestSuite::Node->new('experiment' => $e, 'name'=>$_); } @{$e->nodeinfo()};
  \@node_instances;
}

sub ping_test {
  my ($e) = @_;
  for (@{$e->nodes}) {
    $_->ping();
  }
}

sub single_node_tests {
  my ($e) = @_;
  for (@{$e->nodes}) {
    $_->single_node_tests();
  }
}

sub linktest {
  my ($e) = @_;
  TestBed::Wrap::linktest::linktest($e->pid, $e->eid);
}

sub tevc {
  my ($e) = shift;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, @_);
}

sub trytest(&$) {
  eval {$_[0]->()};
  if ($@) {
    say $@;
    $_[1]->end;
    0; 
  }
  else {
    1;
  }
}

sub launchpingkill {
  my ($pid, $eid, $ns) = @_;
  my $e = e($pid, $eid);
  trytest {
    $e->batchexp_ns_wait($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

sub launchpingswapkill {
  my ($pid, $eid, $ns) = @_;
  my $e = e($pid, $eid);
  trytest {
    $e->batchexp_ns_wait($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->swapout_wait          && die "swap out $eid failed";
    $e->swapin_wait           && die "swap in $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}
1;
