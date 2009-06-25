#!/usr/bin/perl
package Tools::TBSSH;
use SemiModern::Perl;
use Data::Dumper;
use Mouse;

eval{
  #force use of 'Tools::WrappedSSH'
  require BOZO;
};
if ($@) {
  extends 'Tools::WrappedSSH';
}
else {
  die
  extends 'Tools::PerlSSH';
}

sub instance {
  my ($host, %options) = @_;
  $options{'user'} ||= $TBConfig::EMULAB_USER;
  Tools::TBSSH->new('host' => $host, %options);
}

sub wrapped_ssh {
  my ($invocant, $user, $cmd, $checker) = @_;
  my $ssh;
  if (ref $invocant) { $ssh = $invocant }
  else {
    $ssh = Tools::TBSSH->new('host' => $invocant, 'user' => $user);
  }
  my @results = $ssh->cmd($cmd);
  if ($TBConfig::DEBUG_XML_CLIENT) {
    $ssh->saydebug($cmd);
    sayd @results;
  }

  if (defined $checker) {
    &$checker(@results) || die "ssh checker of cmd $cmd failed";
  }
  ($results[2], @results);
}

sub wrapped_scp {
  my ($invocant, $user, $lfile, $rfile) = @_;
  my $ssh;
  if (ref $invocant) { $ssh = $invocant }
  else {
    $ssh = Tools::TBSSH->new('host' => $invocant, 'user' => $user);
  }
  my @results = $ssh->scp_worker($lfile, $rfile);

  ($results[2], @results);
}


sub cmdcheckoutput {
  my ($host, $cmd, $checker) = @_;
  return wrapped_ssh($host, $TBConfig::EMULAB_USER, $cmd, $checker);
}

sub cmdsuccess {
  my ($host, $cmd) = @_;
  return wrapped_ssh($host, $TBConfig::EMULAB_USER, $cmd, sub { $_[2] == 0; } );
}

sub cmdsuccessdump {
  my ($host, $cmd) = @_;
  return wrapped_ssh($host, $TBConfig::EMULAB_USER, $cmd, sub { print Dumper(\@_); $_[2] == 0; } );
}

sub cmdfailure {
  my ($host, $cmd) = @_;
  return wrapped_ssh($host, $TBConfig::EMULAB_USER, $cmd, sub { $_[2] != 0; } );
}

sub cmdfailuredump {
  my ($host, $cmd) = @_;
  return wrapped_ssh($host, $TBConfig::EMULAB_USER, $cmd, sub { print Dumper(\@_); $_[2] != 0; } );
}

sub scp {
  my ($host, @files) = @_;
  return wrapped_scp($host, $TBConfig::EMULAB_USER, @files);
}

=head1 NAME

Tools::TBSSH

=over 4

=item C< instance($host, %options) >

creates a new $ssh object with $host, $user = $TBConfig::EMULAB_USER, and %options

=item C<< $ssh->wrapped_ssh($user, $cmd, $checker) >>

=item C<< $host->wrapped_ssh($user, $cmd, $checker) >>

=item C<< $ssh->wrapped_scp($user, @files) >>

=item C<< $host->wrapped_scp($user, #files) >>

=item C<< $host->scp($host, #files) >>

=item C<cmdcheckoutput($host, $cmd, $checker = sub { my ($out, $err, $resultcode) = @_; ... }>

executes $cmd as $TBConfig::EMULAB_USER on $host and calls checker with ($out, $err, $resultcode)

=item C<cmdsuccess($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER

=item C<cmdsuccessdump($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER and dumps the ssh stdout, stderr, resultcode

=item C<cmdfailure($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER

=item C<cmdfailuredump($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER and dumps the ssh stdout, stderr, resultcode

=back

=cut

1;
