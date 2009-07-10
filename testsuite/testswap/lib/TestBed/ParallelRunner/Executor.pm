#!/usr/bin/perl
package TestBed::ParallelRunner::Executor::Exception;
use Mouse;
  has original => ( is => 'rw');
no Mouse;

package TestBed::ParallelRunner::Executor::PrepError;
use Mouse;
  extends('TestBed::ParallelRunner::Executor::Exception');
no Mouse;

package TestBed::ParallelRunner::Executor::SwapinError;
use Mouse;
  extends('TestBed::ParallelRunner::Executor::Exception');
no Mouse;

package TestBed::ParallelRunner::Executor::RunError;
use Mouse;
  extends('TestBed::ParallelRunner::Executor::Exception');
no Mouse;

package TestBed::ParallelRunner::Executor::SwapoutError;
use Mouse;
  extends('TestBed::ParallelRunner::Executor::Exception');
no Mouse;

package TestBed::ParallelRunner::Executor::KillError;
use Mouse;
  extends('TestBed::ParallelRunner::Executor::Exception');
no Mouse;

package TestBed::ParallelRunner::Executor;
use TestBed::ParallelRunner::ErrorStrategy;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use Mouse;
use Data::Dumper;

has 'e'    => ( isa => 'TestBed::TestSuite::Experiment', is => 'rw');
has 'desc' => ( isa => 'Str', is => 'rw');
has 'ns'   => ( is => 'rw');
has 'proc' => ( isa => 'CodeRef', is => 'rw');
has 'test_count' => ( isa => 'Any', is => 'rw');
has 'error_strategy' => ( is => 'rw', lazy => 1, default => sub { TestBed::ParallelRunner::ErrorStrategy->new; } );
has 'pre_result_handler' => ( isa => 'CodeRef', is => 'rw');

sub ns_text {
  my $s = shift;
  my $ns = $s->ns;
  if (ref($ns) eq 'CODE') {
    return $ns->();
  }
  return $ns;
}

sub parse_options {
  my %options = @_;

  if (defined (delete $options{retry})) {
    $options{error_strategy} = TestBed::ParallelRunner::ErrorRetryStrategy->new;
  }

  if (defined (my $params = delete $options{backoff})) {
    $options{error_strategy} = TestBed::ParallelRunner::BackoffStrategy->build($params);
    
  }
  
  if (defined (my $strategy = delete $options{strategy})) {
    $options{error_strategy} = $strategy;
  }
  
  %options;
}

sub buildt { shift; TestBed::ParallelRunner::Executor->new( parse_options(@_)); }

sub build {
  my ($e, $ns, $sub, $test_count, $desc) = (shift, shift, shift, shift, shift);
  return TestBed::ParallelRunner::Executor->new(
    'e'          => $e,
    'ns'         => $ns,
    'proc'       => $sub,
    'test_count' => $test_count,
    'desc'       => $desc,
    parse_options(@_)
  );
}

sub handleResult { 
  my ($s) = @_;
  my $prh = $s->pre_result_handler;
  $prh->(@_) if $prh;
  $s->error_strategy->handleResult( @_); 
}

sub prep {
  my $s = shift;
  if (checkno('create')) {
    return +{'maximum_nodes' => 0};
  }
  my $r = eval { $s->e->create_and_get_metadata($s->ns_text); };
  die TestBed::ParallelRunner::Executor::PrepError->new( original => $@ ) if $@;
  return $r;
}

sub checkno {
  my $stage = shift;
  return grep { $_ eq $stage } @{ $TBConfig::exclude_steps };
}

sub execute {
  my $s = shift;
  my $e = $s->e;

  my $run_exception;
  my $swapout_exception;

  eval { $e->swapin_wait; } unless checkno('swapin');
  my $swapin_exception = $@;

  unless ($swapin_exception) {
    eval { $s->proc->($e); } unless checkno('run');
    $run_exception = $@;

    eval { $e->swapout_wait; } unless checkno('swapout');
    $swapout_exception = $@;
  }

  eval { $e->end_wait; } unless checkno('end');
  my $end_exception = $@;

  die TestBed::ParallelRunner::Executor::SwapinError->new( original => $@ ) if $swapin_exception;
  die TestBed::ParallelRunner::Executor::RunError->new( original => $run_exception ) if $run_exception;
  die TestBed::ParallelRunner::Executor::SwapoutError->new( original => $swapout_exception ) if $swapout_exception;
  die TestBed::ParallelRunner::Executor::KillError->new( original => $end_exception ) if $end_exception;
  
  return 1;
}

=head1 NAME

TestBed::ParallelRunner::Executor

Represents a ParallelRunner Job

=over 4

=item C<< build($e, $ns, $sub, $test_count, $desc) >>

constructs a TestBed::ParallelRunner::Test job

=item C<< $prt->prep >>

executes the pre_running phase of experiment and determines min and max node counts.

=item C<< $prt->handleResult >>

handles the result using a error strategy

=item C<< $prt->execute >>

swaps in the experiment and runs the specified test
it kills the experiment unconditionaly after the test returns

=item C<< $prt->parse_options >>

parses retry =>1, backoff => "\d+:\d+:\d+:\d+", strategy => '....' options
and build the appropriate error_strategy object

=item C<< $prt->buildt >>

builds a naked TestBed::ParallelRunner::Executor for testing purposes

=back

=cut

1;
