#!/usr/bin/perl
package TestBed::TestSuite::Experiment;
use SemiModern::Perl;
use Mouse;
use TestBed::XMLRPC::Client::Experiment;
use TestBed::Wrap::tevc;
use TestBed::Wrap::linktest;
use TestBed::Wrap::loghole;
use Tools;
use Tools::TBSSH;
use Data::Dumper;
use TestBed::TestSuite;
use TestBed::TestSuite::Node;
use TestBed::TestSuite::Link;

extends 'TestBed::XMLRPC::Client::Experiment';

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

=item C<< $e->nodenames() >>

returns a list of node names representing each node in the experiment
=cut
sub nodenames {
  my ($e) = @_;
  my $nodenames = $e->nodeinfo();
  return wantarray ? @{$nodenames} : $nodenames;
}

=item C<< $e->hostnames() >>

returns a list of node hostnames representing each node in the experiment
=cut
sub hostnames {
  my ($e) = @_;
  my $nodenames = $e->nodeinfo();
  my @hostnames = map { $_ =~ /([^\.]*)/; $1 } @$nodenames;
  return wantarray ? @hostnames : \@hostnames;
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

=item C<< $e->tevc(@args) >>

runs tevc on ops for this experiment.
takes an argument string such as "now link1 down"
=cut
sub tevc {
  my ($e) = shift;
  TestBed::Wrap::tevc::tevc($e->pid, $e->eid, @_);
}

=item C<< $e->tevc_at_host($host, @args) >>

runs tevc on $host for this experiment.
takes an argument string such as "now link1 down"
=cut

sub tevc_at_host {
  my ($e) = shift;
  TestBed::Wrap::tevc::tevc_at_host($e->pid, $e->eid, @_);
}

=item C<< $e->parallel_tevc($proc, $items) >>

runs tevc on ops for each cmdline produced by calling $proc on each $item.
=cut
sub parallel_tevc {
  my ($e, $proc, $items) = @_;
  my $result = TestBed::ForkFramework::ForEach::work(sub {
    my @tevc_cmd = $proc->(@_);
    TestBed::Wrap::tevc::tevc($e->pid, $e->eid, @tevc_cmd);
  }, $items);
  if ($result->[0]) {
    sayd($result->[2]);
    die 'TestBed::ParallelRunner::runtests died during parallel_tevc';
  }
}

=item C<< $e->parallel_tevc_at_host($host, $proc, $items) >>

runs tevc on $host for each cmdline produced by calling $proc on each $item.
=cut
sub parallel_tevc_at_host {
  my ($e, $host, $proc, $items) = @_;
  my $result = TestBed::ForkFramework::ForEach::work(sub {
    my @tevc_cmd = $proc->(@_);
    TestBed::Wrap::tevc::tevc_at_host($e->pid, $e->eid, $host, @tevc_cmd);
  }, $items);
  if ($result->[0]) {
    sayd($result->[2]);
    die 'TestBed::ParallelRunner::runtests died during parallel_tevc';
  }
}

=item C<< $e->loghole($cmd) >>

runs loghole on ops
=cut
sub loghole {
  my ($e) = shift;
  TestBed::Wrap::loghole::loghole($e, @_);
}

=item C<< $e->loghole_sync_allnodes($cmd) >>

runs loghole sync all hostnames on ops
=cut
sub loghole_sync_allnodes {
  my ($e) = shift;
  my @hostnames = $e->hostnames;
  TestBed::Wrap::loghole::loghole($e, "sync @hostnames");
}

=item C<< $e->splat($data, $filename) >>

splats $data to $filename on each node
=cut
sub splat {
  my ($e, $data, $fn) = @_;
  my $temp = splat_to_temp($data);
  my $rc = 0;
  for (@{$e->nodes}) {
    my $user = $TBConfig::EMULAB_USER;
    my $host = $_->name;
    my $dest = "$user\@$host:$fn";
    my @results = $_->scp($temp, $dest);
    $rc ||= $results[0];
    die "splat to $dest failed" if $rc;
  }
  return !$rc;
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


sub pretty_list {
  use TestBed::XMLRPC::Client::Pretty;
  pretty_listexp(shift->getlist_full);
}

=item C<trytest { code ... } $e>

catches exceptions while a test is running and cleans up the experiment
=cut
use constant TRYTEST_SUCCES  => 1;
use constant TRYTEST_FAILURE => 0;
sub trytest(&$) {
  my ($sub, $e) = @_;
  eval {$sub->()};
  if ($@) {
    say $@;
    eval {$e->end};
    if ($@) { my $eid = $e->eid; warn "finally cleanup of $eid failed in trytest";}
    TRYTEST_FAILURE; 
  }
  else {
    TRYTEST_SUCCES;
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
    $e->ensure_active_ns($ns) && die "batchexp $eid failed";
    $worker->($e)             || die "worker function failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

=item C<< $e->startrun($ns_contents, $worker_sub) >>

starts an experiment given a $ns file and a $worker
call the $worker passing in the experiment $e
=cut
sub startrun {
  my ($e, $ns, $worker) = @_;
  my $eid = $e->eid;
  $e->ensure_active_ns($ns) && die "batchexp $eid failed";
  $worker->($e)             || die "worker function failed";
}

=item C<launchpingkill($e, $ns)>

method that starts an experiment, runs a ping_test, and ends the experiment
=cut
sub launchpingkill {
  my ($e, $ns) = @_;
  my $eid = $e->eid;
  trytest {
    $e->ensure_active_ns($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

=item C<launchpingswapkill($e, $ns)>

method that starts an experiment, runs a ping_test, 
swaps the experiment out and then back in, runs a ping test, and finally
ends the experiment
=cut
sub launchpingswapkill {
  my ($e, $ns) = @_;
  my $eid = $e->eid;
trytest {
    $e->ensure_active_ns($ns) && die "batchexp $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->swapout_wait          && die "swap out $eid failed";
    $e->swapin_wait           && die "swap in $eid failed";
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}

=item C<< $e->pingkill() >>

method that runs a ping_test, and ends the experiment
=cut
sub pingkill {
  my ($e, $ns) = @_;
  my $eid = $e->eid;
  trytest {
    $e->ping_test             && die "connectivity test $eid failed";
    $e->end                   && die "exp end $eid failed";
  } $e;
}


=item C<< $e->pingswapkill() >>

method that runs a ping_test, 
swaps the experiment out and then back in, runs a ping test, and finally
ends the experiment
=cut
sub pingswapkill {
  my ($e) = @_;
  my $eid = $e->eid;
trytest {
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
