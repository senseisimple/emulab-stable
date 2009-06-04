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
  say $sshcmd if ($TBConfig::DEBUG_XML_CLIENT);
  run3($sshcmd, undef, \$out, \$err);
  my $rc = $? >> 8;
  ($out, $err, $rc);
}

sub scp_worker {
  my ($ssh, @files) = @_;
  my $out;
  my $err;
  my $host = $ssh->host;
  my $user = $ssh->user;
  my $sshcmd = "scp -o BatchMode=yes -o StrictHostKeyChecking=no @files";
  say $sshcmd if ($TBConfig::DEBUG_XML_CLIENT);
  run3($sshcmd, undef, \$out, \$err);
  my $rc = $? >> 8;
  ($out, $err, $rc);
}


=head1 NAME

Tools::TBSSH

=over 4

=item C<< $ssh->cmd($cmd) >>

B<LOWLEVEL SUB> execute $cmd on $host as $user by wrapping cmdline ssh

=item C<< $ssh->scp_worker(@files)  >>

B<LOWLEVEL SUB> execute $scp with $files as arguments

=back

=cut

1;
