#!/usr/bin/perl
package TestBed::ParallelRunner::ErrorStrategy;
use SemiModern::Perl;
use Mouse;
use TestBed::ParallelRunner::ErrorConstants;

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
    if    ( $error->isa ( 'TestBed::ParallelRunner::Executor::SwapinError'))  { return $s->swapin_error  ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::RunError'))     { return $s->run_error     ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::SwapoutError')) { return $s->swapout_error ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::KillError'))    { return $s->end_error     ( @_); }
    elsif ( $error->isa ( 'TestBed::ParallelRunner::Executor::Exception'))    { return $s->run_error     ( @_); }
    else { return RETURN_AND_REPORT; }
  }
  else { return RETURN_AND_REPORT; }
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

sub swapin_error  { return RETURN_AND_REPORT; }
sub run_error     { return RETURN_AND_REPORT; }
sub swapout_error { return RETURN_AND_REPORT; }
sub end_error     { return RETURN_AND_REPORT; }

package TestBed::ParallelRunner::ErrorRetryStrategy;
use SemiModern::Perl;
use Mouse;
use TestBed::ParallelRunner::ErrorConstants;

extends 'TestBed::ParallelRunner::ErrorStrategy';

sub swapin_error {
  my ($s, $executor, $scheduler, $result) = @_;
  if ($s->is_retry_cause) { return $scheduler->retry($result); }
  else { return RETURN_AND_REPORT; }
}

sub is_retry_cause {
  my $s = shift;
  my $cause = $s->xmlrpc_error_cause;
  my @retry_causes = qw(temp internal software hardware canceled unknown);
  if ( grep { /$$cause/ } @retry_causes) { return 1; }
  return 0;
}

=head1 NAME

TestBed::ParallelRunner::ErrorStrategy

handle parallel run errors;

=over 4

=item C<< $es->handleResult( $executor, $scheduler, $result ) >>

  dispatch experiment errors to the right handler

=item C<< swapin_error($error) >>
=item C<< run_error($error) >>
=item C<< swapout_error($error) >>
=item C<< end_error($error) >>

=back

=cut

1;
