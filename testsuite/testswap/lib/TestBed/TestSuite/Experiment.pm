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

extends 'Exporter', 'TestBed::XMLRPC::Client::Experiment';
require Exporter;
our @EXPORT;
push @EXPORT, qw(launchpingkill launchpingswapkill);

=head1 NAME

TestBed::TestSuite::Experiment

framwork class for starting and testing experiments

=over 4

=item C<nodes()>

returns a list of node objects representing each node in the experiment
=cut
sub nodes {
  my ($e) = @_;
  my @node_instances = map { TestBed::TestSuite::Node->new('experiment' => $e, 'name'=>$_); } @{$e->nodeinfo()};
  \@node_instances;
}

=item C<ping_test()>

runs a ping test across all nodes
=cut
sub ping_test {
  my ($e) = @_;
  for (@{$e->nodes}) {
    die $_->name . "failed ping" unless $_->ping();
  }
}

=item C<single_node_tests()>

runs a single_node_tests test across all nodes
=cut
sub single_node_tests {
  my ($e) = @_;
  for (@{$e->nodes}) {
    die $_->name . "failed single_node_tests" unless $_->single_node_tests();
  }
}

=item C<linktest>

runs a linktest on the experiment
=cut
sub linktest {
  my ($e) = @_;
  TestBed::Wrap::linktest::linktest($e->pid, $e->eid);
}

=item C<tevc>

runs tevc on ops for this experiment.
takes an argument string such as "now link1 down"
=cut
sub tevc {
  my ($e) = shift;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, @_);
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
