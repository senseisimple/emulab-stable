#!/usr/bin/perl
package TestBed::TestSuite::Link;
use SemiModern::Perl;
use Mouse;
#use TestBed::XMLRPC::Client::Link;
use Tools::Network;
use Tools::TBSSH;
use Data::Dumper;
use TestBed::Wrap::tevc;

has 'name' => ( isa => 'Str', is => 'rw');
has 'experiment' => ( isa => 'TestBed::TestSuite::Experiment', is => 'rw');

=head1 NAME
TestBed::TestSuite::Link

=over 4

=item C<< $l->up >>

uses tevc to bring up a link
=cut
sub up { shift->tevc("up"); }

=item C<< $l->down >>

uses tevc to bring down a link
=cut
sub down { shift->tevc("down"); }

=item C<< $l->tevc($cmd) >>

uses tevc to control link
=cut
sub tevc {
  my ($self, $cmd) = @_;
  my $name = $self->name;
  TestBed::Wrap::tevc::tevc($self->experiment, "now $name $cmd");
}

=back 

=cut

1;
