#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package TestBed::ParallelRunner::ErrorStrategy;
use SemiModern::Perl;
use Mouse;

has 'executor' => (is => 'rw');
has 'scheduler' => (is => 'rw');
has 'result' => (is => 'rw');

sub handleResult {
  my $s = shift;
  my ($executor, $scheduler, $result) = @_;
  $s->executor($executor);
  $s->scheduler($scheduler);
  $s->result($result);

  if ($result->is_error) {
    my $error = $result->error;
    if    ( $error->isa ( 'TestBed::ParallelRunner::Executor::PrerunError'))  { return $s->prerun_error  ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::SwapinError'))  { return $s->swapin_error  ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::RunError'))     { return $s->run_error     ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::SwapoutError')) { return $s->swapout_error ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::KillError'))    { return $s->end_error     ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::Exception'))    { return $s->run_error     ( @_); }
    else { $s->ensure_end_and_report; }
  }
  else { $s->report_to_scheduler; }
}

sub xmlrpc_error_cause {
  my ($s) = @_;
  my $result = $s->result;
  if ( $result->error->original ) {
    my $xmlrpc_error = $result->error->original;
    if ($xmlrpc_error->isa ( 'RPC::XML::struct' ) ) {
      return $xmlrpc_error->{value}->{'cause'};
    }
  }
  return;
}

sub is_retry_cause {
  my $s = shift;
  my $cause = $s->xmlrpc_error_cause;
  my @retry_causes = qw(temp internal software hardware canceled unknown);
  if ( grep { /$$cause/ } @retry_causes) { return 1; }
  return 0;
}

sub ensure_end_and_report {
  my $s = shift;
  #say("ensure_end_and_report " . $s->executor->e->eid);
  eval { $s->executor->ensure_end; };
  $s->report_to_scheduler
}

sub report_to_scheduler {
  my $s = shift;
  $s->scheduler->return_and_report($s->result);
}

sub prerun_error  { shift->ensure_end_and_report; }
sub swapin_error  { shift->ensure_end_and_report; }
sub run_error     { shift->ensure_end_and_report; }
sub swapout_error { shift->ensure_end_and_report; }
sub end_error     { shift->ensure_end_and_report; }

package TestBed::ParallelRunner::PrerunExpectFail;
use SemiModern::Perl;
use Mouse;

extends 'TestBed::ParallelRunner::ErrorStrategy';

sub prerun_error {
  my ($s, $executor, $scheduler, $result) = @_;
  $s->ensure_end_and_report;
}

package TestBed::ParallelRunner::ErrorRetryStrategy;
use SemiModern::Perl;
use Mouse;

extends 'TestBed::ParallelRunner::ErrorStrategy';

sub swapin_error {
  my ($s, $executor, $scheduler, $result) = @_;
  if ($s->is_retry_cause) {
    warn "Retrying";# . $executor->e->eid;
    return $s->scheduler->retry($result); 
  }
  else { $s->ensure_end_and_report; }
}

package TestBed::ParallelRunner::BackoffStrategy;
use SemiModern::Perl;
use Mouse;

has 'starttime'  => (is => 'rw', default => sub { time; } );
has 'retries'    => (is => 'rw', default => 0 );
has 'maxtime'    => (is => 'rw', default => 5 * 60 * 60 );
has 'maxretries' => (is => 'rw', default => 3 );
has 'waittime'   => (is => 'rw', default => 10 * 60 );
has 'exponent'   => (is => 'rw', default => 0.0 );

extends 'TestBed::ParallelRunner::ErrorStrategy';

sub build {
  shift;
  my $s = TestBed::ParallelRunner::BackoffStrategy->new; 
  $s->parse(shift);
  $s;
} 

sub parse {
  my ($s, $args) = @_;
  my @args = split(/:/, $args);
  my $val;
  $val = shift @args; $s->waittime($val) if $val;
  $val = shift @args; $s->maxretries($val) if $val;
  $val = shift @args; $s->exponent($val) if $val;
  $val = shift @args; $s->maxtime($val) if $val;
}

sub incr_retries { shift->{retries}++; }

sub backoff {
  my ($s, $result) = @_;
  
  my $maxretries = $s->maxretries;
  if ($s->retries >= $maxretries) {
    warn "Max retries $maxretries reached, terminating experiment";
    return $s->ensure_end_and_report;
  }
  my $timeout = $s->starttime + $s->maxtime;
  if (time >= $timeout) {
    warn "Max timeout $timeout reached, terminating experiment";
    return $s->ensure_end_and_report;
  }
  
  $s->incr_retries;
  $s->waittime($s->waittime * (2 ** $s->exponent));
  my $next_time = time + $s->waittime;
  warn "Backing off " . ($next_time - time) . " seconds.";
  return $s->scheduler->schedule_at($result, $next_time);
}

sub swapin_error {
  my ($s, $executor, $scheduler, $result) = @_;
  if ($s->is_retry_cause) {
    return $s->backoff($result); 
  }
  else { return $s->ensure_end_and_report; }
}

=head1 NAME

TestBed::ParallelRunner::ErrorStrategy

handle parallel run errors;

=over 4

=item C<< $es->handleResult( $executor, $scheduler, $result ) >>

  dispatch experiment errors to the right handler

=item C<< $es->prerun_error($error) >>
=item C<< $es->swapin_error($error) >>
=item C<< $es->run_error($error) >>
=item C<< $es->swapout_error($error) >>
=item C<< $es->end_error($error) >>

=item C<< $es->xmlrpc_error_cause($error) >>

parses the xmlrpc error cause out of the original embedded xmlrpc error

=item C<< es->is_retry_cause >>

return true if the error is retriable 

=item C<< es->ensure_end_and_report >>

ensures the executor experiment end method has been called and
reports result to scheduler

=item C<< es->report_to_scheduler >>

reports a result back to the scheduler

=back

=head1 NAME

TestBed::ParallelRunner::BackoffStrategy

implement parallel backoff

=over 4

=item C<< $bs->build( "waittime:maxretires:exponent:maxtime" ) >>

builds backoff strategy from arg string

=item C<< $bs->parse( "waittime:maxretires:exponent:maxtime" ) >>

parses arg string and sets object members

=item C<< $bs->incr_retries >>

incrementes attempted retries

=item C<< $bs->incr_retries >>

schedules task for exectution after backoff time

=back

=cut

1;
