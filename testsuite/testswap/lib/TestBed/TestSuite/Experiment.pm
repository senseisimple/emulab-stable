#!/usr/bin/perl
package TestBed::TestSuite::Experiment;
use SemiModern::Perl;
use Mouse;
use TestBed::XMLRPC::Client::Experiment;
use TestBed::Wrap::tevc;
use TestBed::Wrap::linktest;
use Tools::TBSSH;
use Data::Dumper;
use TestBed::TestSuite::Node;
use TestBed::TestSuite::Link;

extends 'Exporter', 'TestBed::XMLRPC::Client::Experiment';
require Exporter;
our @EXPORT;
push @EXPORT, qw(launchpingkill launchpingswapkill);

=head1 NAME

TestBed::TestSuite::Experiment

framwork class for starting and testing experiments

=over 4

=item C<< $e->node($nodename) >>

returns a node object representing node $nodename in the experiment
=cut
sub node {
  my ($e, $nodename) = @_;
  TestBed::TestSuite::Node->new('experiment' => $e, 'name' => $nodename);
}

=item C<< $e->link($linkname) >>

returns a link object representing link $linkname in the experiment
=cut
sub link {
  my ($e, $linkname) = @_;
  TestBed::TestSuite::Link->new('experiment' => $e, 'name' => $linkname);
}

=item C<< $e->nodes() >>

returns a list of node objects representing each node in the experiment
=cut
sub nodes {
  my ($e) = @_;
  my @node_instances = map { TestBed::TestSuite::Node->new('experiment' => $e, 'name'=>$_); } @{$e->nodeinfo()};
  \@node_instances;
}

=item C<< $e->ping_test() >>

runs a ping test across all nodes
=cut
sub ping_test {
  my ($e) = @_;
  for (@{$e->nodes}) {
    die $_->name . "failed ping" unless $_->ping();
  }
}

=item C<< $e->single_node_tests() >>

runs a single_node_tests test across all nodes
=cut
sub single_node_tests {
  my ($e) = @_;
  for (@{$e->nodes}) {
    die $_->name . "failed single_node_tests" unless $_->single_node_tests();
  }
}

=item C<< $e->linktest >>

runs a linktest on the experiment
=cut
sub linktest {
  my ($e) = @_;
  TestBed::Wrap::linktest::linktest($e->pid, $e->eid);
}

=item C<< $e->tevc($arg) >>

runs tevc on ops for this experiment.
takes an argument string such as "now link1 down"
=cut
sub tevc {
  my ($e) = shift;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, @_);
}

=item C<< $e->linkup($linkname) >>

uses tevc to bring down a link
=cut
sub linkup {
  my ($e, $link) = @_;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, "now $link up");
}

=item C<< $e->linkdown($linkname) >>

uses tevc to bring up a link
=cut
sub linkdown {
  my ($e, $link) = @_;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, "now $link down");
}

=item C<trytest { code ... } $e>

catches exceptions while a test is running and cleans up the experiment
=cut
sub trytest(&$) {
  my ($sub, $e) = @_;
  eval {$sub->()};
  if ($@) {
    say $@;
    $e->end;
    0; 
  }
  else {
    1;
  }
}

=item C<< $e->startrunkill($ns_contents, $worker_sub) >>

starts an experiment given a $ns file and a $worker
call the $worker passing in the experiment $e
ends the experiemnt
=cut
sub startrunkill {
  my ($e, $ns, $worker) = @_;
  my $eid = $e->eid;
  trytest {
    $e->startexp_ns_wait($ns) && die "batchexp $eid failed";
    $worker->($e)             || die "worker function failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

sub startrun {
  my ($e, $ns, $worker) = @_;
  my $eid = $e->eid;
  $e->startexp_ns_wait($ns) && die "batchexp $eid failed";
  $worker->($e)             || die "worker function failed";
}

=item C<launchpingkill($pid, $eid, $ns)>

class method that starts an experiment, runs a ping_test, and ends the experiment
=cut
sub launchpingkill {
  my ($pid, $eid, $ns) = @_;
  my $e = e($pid, $eid);
  trytest {
    $e->startexp_ns_wait($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

=item C<launchpingkill($pid, $eid, $ns)>

class method that starts an experiment, runs a ping_test, 
swaps the experiment out and then back in, runs a ping test, and finally
ends the experiment
=cut
sub launchpingswapkill {
  my ($pid, $eid, $ns) = @_;
  my $e = e($pid, $eid);
trytest {
    $e->startexp_ns_wait($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->swapout_wait          && die "swap out $eid failed";
    $e->swapin_wait           && die "swap in $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

=back

=cut

1;
