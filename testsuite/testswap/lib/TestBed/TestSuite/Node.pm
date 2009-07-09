#!/usr/bin/perl
package TestBed::TestSuite::Node;
use SemiModern::Perl;
use Mouse;
use Tools;
use Tools::Network;
use Tools::TBSSH;
use Data::Dumper;

has 'name' => ( isa => 'Str', is => 'rw');
has 'experiment' => ( isa => 'TestBed::TestSuite::Experiment', is => 'rw');

=head1 NAME
TestBed::TestSuite::Node

=over 4

=item C<< $n->ping >>

=cut

sub ping {
  my ($self) = @_;
  Tools::Network::ping($self->name);
}

=item C<< $n->single_node_tests >>

executes hostname, sudo ls, mount via ssh on the remote node
=cut
sub single_node_tests {
  my ($s) = @_;
  my $ssh = $s->ssh();
  my $eid = $s->experiment->eid;
  my $name = $s->name;
  $ssh->cmdmatch("hostname", qr/$name/, "$eid $name hostname died");
  $ssh->cmdmatch("sudo id", qr/uid=0\(root\)/, "$eid $name sudo died");
  $ssh->cmdmatch("mount", qr{/proj/}, "$eid $name mountdied");
}

=item C<< $n->ssh >>

returns a $ssh connection to the node
=cut
sub ssh {
  my $self = shift;
  return Tools::TBSSH::instance($self->name);
}

=item C<< $n->scp($lfile, $rfile) >>

returns a $ssh connection to the node
=cut
sub scp {
  my $self = shift;
  return Tools::TBSSH::scp($self->name, @_);
}

=item C<< $n->build_remote_name($filename) >>

builds "$user\@$host:$fn"
=cut
sub build_remote_name {
  my ($s, $fn) = @_;
  my $user = $TBConfig::EMULAB_USER;
  my $host = $s->name;
  return "$user\@$host:$fn";
}

=item C<< $n->splat($data, $filename) >>

cats $data to $filename on the node
=cut
sub splat {
  my ($s, $data, $fn) = @_;
  my $temp = Tools::splat_to_temp($data);
  my $dest = $s->build_remote_name($fn);
  my @results = $s->scp($temp, $dest);
  die "splat to $dest failed" if $results[0];
  return 1;
}

=item C<< $n->splatex($data, $filename) >>

cats $data to $filename on the node and make it executable
=cut
sub splatex {
  my ($s, $data, $fn) = @_;
  $s->splat($data, $fn);
  $s->ssh->cmdsuccess("chmod +x $fn");
}

=item C<< $n->slurp($filename) >>

pulls $filename content from node
=cut
sub slurp {
  my ($s, $fn) = @_;
  use File::Temp;
  my $temp = File::Temp->new;
  my $src = $s->build_remote_name($fn);
  my @results = $s->scp($src, $temp);
  die "spurp from $src failed" if $results[0];
  return Tools::slurp($temp);
  return 1;
}

=back 

=cut

1;
