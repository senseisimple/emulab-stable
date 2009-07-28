#!/usr/bin/perl
package TestBed::ParallelRunner;
use SemiModern::Perl;
use TestBed::ParallelRunner::Executor;
use TestBed::ForkFramework;
use TestBed::TestBuilderWrapper;
use Data::Dumper;
use Mouse;
use TBConfig;

has executors => ( is => 'rw', default => sub { [] } );

our $GlobalRunner = TestBed::ParallelRunner->new;

sub executor { 
  my ($s, $itemId) = @_;
  $s->executors->[$itemId];
} 

sub add_executor { 
  my ($s, $executor) = @_;
  push @{$s->executors}, $executor;
  $executor;
} 

sub build_executor { 
  my $s = shift;
  $s->add_executor(TestBed::ParallelRunner::Executor::build(@_));
} 

sub runtests {
  my ($s, $concurrent_pre_runs, $concurrent_node_count_usage) = @_;
  $concurrent_pre_runs         ||= $TBConfig::concurrent_prerun_jobs;
  $concurrent_node_count_usage ||= $TBConfig::concurrent_node_usage;

  if ( $TBConfig::single ) {
    $s->Executors = [ (grep { $_->e->eid eq $TBConfig::single } @{$s->executors}) ];
  }

  #prerun step
  my $result = TestBed::ForkFramework::ForEach::max_work($concurrent_pre_runs, sub { shift->prerun }, $s->executors);
  if ($result->has_errors) {
    sayd($result->errors);
    warn 'TestBed::ParallelRunner::runtests died during test prep';
  }

  my $workscheduler =  TestBed::ForkFramework::WeightedScheduler->new( 
    items => $s->executors,
    proc => \&tap_wrapper,
    maxnodes => $concurrent_node_count_usage,
  );

  #add taskss to scheduler step
  my $total_test_count = 0;
  for (@{$result->successes}) {
    my $executor = $s->executor($_->itemid);
    my $maximum_nodes = $_->result->{'maximum_nodes'};
    my $eid = $executor->e->eid;

    if ($maximum_nodes > $concurrent_node_count_usage) {
      warn "$eid requires upto $maximum_nodes nodes, only $concurrent_node_count_usage concurrent nodes permitted\n$eid will not be run";
    }
    else {
      $workscheduler->add_task($executor, $maximum_nodes);
      $total_test_count += $executor->test_count;
    }
  }

  USE_TESTBULDER_PREAMBLE: {
    TestBed::TestBuilderWrapper::reset_test_builder($total_test_count, no_numbers => 1);
  }

  #run tests
  $result = $workscheduler->run;

  USE_TESTBULDER_POSTAMBLE: {
    $total_test_count = 0;
    for (@{$result->successes}) {
      my $executor = $s->executor($_->itemid);
      $total_test_count += $executor->test_count;
    }
    TestBed::TestBuilderWrapper::set_test_builder_to_end_state($total_test_count);
  }

  if ($result->has_errors) {
    sayd($result->errors);
    die 'TestBed::ParallelRunner::runtests died during test execution';
  }
  return;
}

our $ENABLE_SUBTESTS_FEATURE = 0;

sub tap_wrapper {
  my ($te) = @_;
  
  if ($ENABLE_SUBTESTS_FEATURE) {
    TestBed::ForkFramework::fork_redir( sub {
      my ($in, $out, $err, $pid) = @_;
      #while(<$out>) { print "K2" . $_; }
      use TAP::Parser;  
      my $tapp = TAP::Parser->new({'stream' => TAP::Parser::Iterator::StdOutErr->new($out, $err, $pid)});
      while ( defined( my $result = $tapp->next ) ) {
        #sayd($result);
      }
      ok(1, $te->desc) if $ENABLE_SUBTESTS_FEATURE && $tapp;
    },
    sub {
      TestBed::TestBuilderWrapper::reset_test_builder($te->test_count) if $ENABLE_SUBTESTS_FEATURE;
      TestBed::TestBuilderWrapper::setup_test_builder_ouputs(*STDOUT, *STDERR);
      $te->execute;
    });
  }
  else {
    $te->execute;
  }
  return 0;
}

=head1 NAME

TestBed::ParallelRunner

=over 4

=item C<< $pr->executor($itemid) >>

return the $itemid th executor

=item C<< $pr->build_executor >>

helper function called by rege.
creates a TestBed::ParallelRunner::Executor job

=item C<< $pr->add_executor($executor) >>

pushes $executor onto $s->executors list

=item C<< $pr->runtests($concurrent_pre_runs, $concurrent_node_count_usage) >>

allows a maximum of $concurrent_pre_runs during parallel execution
allows a maximum of $concurrent_nodes during parallel execution

start the execution of parallel tests

=item C<< set_test_builder_to_end_state >>
=item C<< reset_test_builder >>
=item C<< setup_test_builder_ouputs >>

B<INTERNAL> functions to get Test::Builder to behave correctly with parallel tests

=item C<< tap_wrapper >>

wraps two different ways of executing parallel tests and wrapping their TAP output stream

=back

=cut

1;

