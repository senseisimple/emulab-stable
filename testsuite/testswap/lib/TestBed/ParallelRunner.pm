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
use TestBed::ParallelRunner::Test;
use TestBed::ForkFramework;
use Data::Dumper;
use Mouse;

our $ExperimentTests = [];

my $teste_desc = <<'END';
Not enough arguments to teste
  teste(eid, $ns, $sub, $test_count, $desc);
  teste($pid, $eid, $ns, $sub, $test_count, $desc);
  teste($pid, $gid, $eid, $ns, $sub, $test_count, $desc);
END
      
sub add_experiment { push @$ExperimentTests, TestBed::ParallelRunner::Test::tn(@_); }

sub runtests {
  my ($concurrent_pre_runs, $concurrent_node_count_usage ) = @_;
  $concurrent_pre_runs ||= 4;
  $concurrent_node_count_usage ||= 20;

  #prerun step
  my $result = TestBed::ForkFramework::MaxWorkersScheduler::work($concurrent_pre_runs, sub { $_[0]->prep }, $ExperimentTests);
  if ($result->[0]) {
    sayd($result->[2]);
    die 'TestBed::ParallelRunner::runtests died during test prep';
  }

  #create schedule step
  my @weighted_experiements;
  for (@{$result->[1]}) {
    my ($hash, $item_id) = @$_;
    my $maximum_nodes = $hash->{'maximum_nodes'};
    my $eid = $ExperimentTests->[$item_id]->e->eid;
    #say "$eid $item_id $maximum_nodes";

    push @weighted_experiements, [ $maximum_nodes, $item_id ];
  }
  @weighted_experiements = sort { $a->[0] <=> $b->[0] } @weighted_experiements;

  #count tests step
  my $test_count = 0;
  map { $test_count += $_->test_count } @$ExperimentTests;

  #run tests
  reset_test_builder($test_count, no_numbers => 1);
  $result = TestBed::ForkFramework::RateScheduler::work($concurrent_node_count_usage, \&tap_wrapper, \@weighted_experiements, $ExperimentTests);
  set_test_builder_to_end_state($test_count);
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
    TestBed::ForkFramework::Scheduler->redir_std_fork( sub {
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
      $te->run_ensure_kill;
    });
  }
  else {
    $te->run_ensure_kill;
  }
  return 0;
}

sub build_TAP_stream {
  use TestBed::TestSuite;
  my ($in, $out, $err, $pid) = TestBed::ForkFramework::redir_fork(sub { runtests; });
  return TAP::Parser::Iterator::StdOutErr->new($out, $err, $pid);
}

1;
