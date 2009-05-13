#!/usr/bin/perl
package Tools::WrappedSSH;
use SemiModern::Perl;
use Data::Dumper;
use TBConfig;
use IPC::Run3;
use Mouse;

has 'host' => ( isa => 'Str', is => 'rw');
has 'user' => ( isa => 'Str', is => 'rw');

sub cmd {
  my ($ssh, $cmd) = @_;
  my $out;
  my $err;
  my $host = $ssh->host;
  my $user = $ssh->user;
  my $sshcmd = "ssh -x -o BatchMode=yes -o StrictHostKeyChecking=no $user\@$host $cmd";
  run3($sshcmd, undef, \$out, \$err);
  my $rc = $? >> 8;
  ($out, $err, $rc);
}

=head1 NAME

Tools::TBSSH

=over 4

=item C< wrapped_ssh($host, $user, $cmd, $checker)>

B<LOWLEVEL SUB> execute $cmd on $host as $user and check result with $checker sub

=item C<cmdcheckoutput($host, $cmd, $checker = sub { my ($out, $err, $resultcode) = @_; ... }>

executes $cmd as $TBConfig::EMULAB_USER on $host and calls checker with ($out, $err, $resultcode)

=item C<cmdsuccess($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER

=item C<cmdsuccessdump($host, $cmd)>

returns the ssh result code of executing $cmd as $TBConfig::EMULAB_USER and dumps the ssh stdout, stderr, resultcode

=back

=cut

1;
