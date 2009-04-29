#!/usr/bin/perl
package TestBed::XMLRPC::Client::Pretty;
use SemiModern::Perl;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(pretty_listexp);
use Data::Dumper;

=head1 NAME

TestBed::XMLRPC::Client::Pretty;

=over 4

=item C<pretty_listexp>

pretty prints the XMLRPC response from listexp
=cut
sub pretty_listexp {
  my ($h) = @_;
  while(my ($pk, $v) = each %$h) {
    while(my ($gk, $v) = each %$v) {
      for my $e (@$v) {
        my $eid;
        if ( ref $e && exists $e->{'name'} )  { $eid = sprintf("%s %s", $e->{'name'}, $e->{'state'});}
        else { $eid = $e; }
        say "$pk :: $gk :: $eid";
      }
    }
  }
}

=back

=cut

1;
