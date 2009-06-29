#!/usr/bin/perl

package TAP::Parser::Iterator::StdOutErr;
use strict;
use warnings;
use vars qw($VERSION @ISA);

use TAP::Parser::Iterator::Process ();
use Config;
use IO::Select;

@ISA = 'TAP::Parser::Iterator::Process';

sub _initialize {
    
    my ( $self, $args ) = @_;
    shift;
    $self->{out}        = shift || die "Need out";
    $self->{err}        = shift || die "Need err";
    $self->{sel}        = IO::Select->new( $self->{out}, $self->{err} );
    $self->{pid}        = shift || die "Need pid";
    $self->{exit}       = undef;
    $self->{chunk_size} = 65536;

    return $self;
}

package TestBed::ParallelRunner;
use SemiModern::Perl;
use TestBed::ParallelRunner::Executor;
use TestBed::ForkFramework;
use Data::Dumper;
use Mouse;
use TBConfig;

our $Executors = [];

my $teste_desc = <<'END';
Not enough arguments to teste
  rege(eid, $ns, $sub, $test_count, $desc);
  rege($pid, $eid, $ns, $sub, $test_count, $desc);
  rege($pid, $gid, $eid, $ns, $sub, $test_count, $desc);
END
      
sub add_executor { 
  my $executor = shift;
  push @$Executors, $executor;
  $executor;
} 

sub build_executor { 
  my $executor = TestBed::ParallelRunner::Executor::build(@_);
  add_executor($executor);
} 

sub runtests {
  my ($concurrent_pre_runs, $concurrent_node_count_usage ) = @_;
  $concurrent_pre_runs         ||= $TBConfig::concurrent_prerun_jobs;
  $concurrent_node_count_usage ||= $TBConfig::concurrent_node_usage;

  #prerun step
  my $result = TestBed::ForkFramework::ForEach::max_work($concurrent_pre_runs, sub { shift->prep }, $Executors);
  if ($result->has_errors) {
    sayd($result->errors);
    warn 'TestBed::ParallelRunner::runtests died during test prep';
  }


  my $workscheduler =  TestBed::ForkFramework::WeightedScheduler->new( 
    items => $Executors,
    proc => &tap_wrapper,
    maxnodes => $concurrent_node_count_usage,
  );

  #add taskss to scheduler step
  my $total_test_count = 0;
  for (@{$result->successes}) {
    my $itemId = $_->itemid;
    my $executor = $Executors->[$itemId];
    my $maximum_nodes = $_->result->{'maximum_nodes'};
    my $eid = $executor->e->eid;

    if ($maximum_nodes > $concurrent_node_count_usage) {
      warn "$eid requires upto $maximum_nodes nodes, only $concurrent_node_count_usage concurrent nodes permitted\n$eid will not be run";
    }
    else {
      $workscheduler->add_task($itemId, $maximum_nodes);
      $total_test_count += $executor->test_count;
    }
  }

  USE_TESTBULDER_PREAMBLE: {
    reset_test_builder($total_test_count, no_numbers => 1);
  }

  #run tests
  $result = $workscheduler->run;

  USE_TESTBULDER_POSTAMBLE: {
    $total_test_count = 0;
    for (@{$result->successes}) {
      my $item_id = $_->itemid;
      my $executor = $Executors->[$item_id];
      $total_test_count += $executor->test_count;
    }
    set_test_builder_to_end_state($total_test_count);
  }

  if ($result->has_errors) {
    sayd($result->errors);
    die 'TestBed::ParallelRunner::runtests died during test execution';
  }
  return;
}

sub set_test_builder_to_end_state {
  my ($test_count, %options) = @_;
  use Test::Builder;
  my $b = Test::Builder->new;
  $b->current_test($test_count); 
}

sub reset_test_builder {
  my ($test_count, %options) = @_;
  use Test::Builder;
  my $b = Test::Builder->new;
  $b->reset; 
  $b->use_numbers(0) if $options{no_numbers};
  if ($test_count) { $b->expected_tests($test_count); }
  else { $b->no_plan; }
}

sub setup_test_builder_ouputs {
  my ($out, $err) = @_;
  use Test::Builder;
  my $b = Test::Builder->new;
  $b->output($out);
  $b->fail_output($out);
  $b->todo_output($out);
}

#use Carp;
#$SIG{ __DIE__ } = sub { Carp::confess( @_ ) };

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
      reset_test_builder($te->test_count) if $ENABLE_SUBTESTS_FEATURE;
      setup_test_builder_ouputs(*STDOUT, *STDERR);
      $te->execute;
    });
  }
  else {
    $te->execute;
  }
  return 0;
}

sub build_TAP_stream {
  use TestBed::TestSuite;
  my ($in, $out, $err, $pid) = TestBed::ForkFramework::fork_child_redir(sub { runtests; });
  return TAP::Parser::Iterator::StdOutErr->new($out, $err, $pid);
}

=head1 NAME

TestBed::ParallelRunner

=over 4

=item C<< build_executor >>

helper function called by rege.
creates a TestBed::ParallelRunner::Executor job

=item C<< add_executor($executor) >>

pushes $executor onto @$Executors

=item C<< runtests >>

kicks off execution of parallel tests.

=item C<< set_test_builder_to_end_state >>
=item C<< reset_test_builder >>
=item C<< setup_test_builder_ouputs >>

B<INTERNAL> functions to get Test::Builder to behave correctly with parallel tests

=item C<< tap_wrapper >>

wraps two different ways of executing parallel tests and wrapping their TAP output stream

=item C<< build_TAP_stream >>

given a TestBed::ParallelRunner returns a TAP stream

=back

=cut

1;

