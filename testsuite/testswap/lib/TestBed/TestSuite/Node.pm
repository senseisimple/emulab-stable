#!/usr/bin/perl
package TestBed::TestSuite::Node;
use SemiModern::Perl;
use Mouse;
#use TestBed::XMLRPC::Client::Node;
use Tools::Network;
use Tools::TBSSH;
use Data::Dumper;

has 'name' => ( isa => 'Str', is => 'rw');
has 'experiment' => ( is => 'rw');

=head1 NAME
TestBed::TestSuite::Node

=over 4

=item C<< $n->ping_test >>

=cut

sub ping_test {
  my ($self) = @_;
  ping($self->name);
}

=item C<< $n->single_node_tests >>

executes hostname, sudo ls, mount via ssh on the remote node
=cut
sub single_node_tests {
  my ($self) = @_;
  my $ssh = $self->ssh();
  $ssh->cmdsuccess("hostname");
  $ssh->cmdsuccess("sudo ls");
  $ssh->cmdsuccess("mount");
}

=item C<$n->ssh>

returns a $ssh connection to the node
=cut
sub ssh {
  my $self = shift;
  my $fqname = $self->name . "." . $self->experiment->eid . "." . $self->experiment->pid . ".emulab.net";
  my $ssh = Tools::TBSSH::instance($fqname);
}

=back 

=cut

1;
