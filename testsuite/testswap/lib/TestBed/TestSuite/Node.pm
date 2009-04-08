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

sub ping_test {
  my ($self) = @_;
  ping($self->name);
}

sub ssh_hostname_test { shift->cmdsuccess("hostname"); }
sub sudo_test         { shift->cmdsuccess("sudo ls"); }
sub mounted_test      { shift->cmdsuccess("mount"); }

sub single_node_tests {
  my ($self) = @_;
  my $ssh = $self->ssh();
  $ssh->cmdsuccess("hostname");
  $ssh->cmdsuccess("sudo ls");
  $ssh->cmdsuccess("mount");
}

sub ssh {
  my $self = shift;
  my $ssh = Tools::TBSSH::ssh($self->name, $TBConfig::EMULAB_USER);
}

sub sshcmddump {
  my $self = shift;
  my $ssh = $self->ssh;
  $ssh->cmddump(@_);
}

1;
