#!/usr/bin/perl
package TestBed::ParallelRunner::Exception;
use Mouse;
  has original => ( is => 'rw');
no Mouse;
package TestBed::ParallelRunner::RetryAtEnd;
use Mouse;
  extends('TestBed::ParallelRunner::Exception');
no Mouse;

package TestBed::ParallelRunner::SwapOutFailed;
use Mouse;
  extends('TestBed::ParallelRunner::Exception');
no Mouse;

package TestBed::ParallelRunner::ErrorStrategy;
use SemiModern::Perl;
use Mouse;

sub swapin_error { }
sub run_error { }
sub swapout_error {
  my ($xmlrpc_error) = @_;
  die TestBed::ParallelRunner::SwapOutFailed->new($xmlrpc_error); 
}
sub end_error { }

package TestBed::ParallelRunner::ErrorRetryStrategy;
use SemiModern::Perl;
use Mouse;

extends 'TestBed::ParallelRunner::ErrorStrategy';


sub swapin_error {
  my @retry_causes = qw( temp internal software hardware canceled unknown);
  my ($xmlrpc_error) = @_;
  if ($xmlrpc_error =~ /RPC::XML::Struct/) {
    my $cause = $xmlrpc_error->value->value->{'cause'};
    if ( grep { /$cause/ } @retry_causes) {
      die TestBed::ParallelRunner::RetryAtEnd->new($xmlrpc_error); 
    }
  }
}

=head1 NAME

TestBed::ParallelRunner::ErrorStrategy

handle parallel run errors;

=over 4

=item C<< swapin_error($error) >>
=item C<< run_error($error) >>
=item C<< swapout_error($error) >>
=item C<< end_error($error) >>

=back

=cut


1;
