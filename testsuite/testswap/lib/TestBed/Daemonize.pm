package TestBed::Daemonize;
use SemiModern::Perl;

sub ForkOrDie {
  my $pid;
  return $pid if (defined($pid = fork));
  die "Fork failed: $!";
}

sub daemonize {
  exit if ForkOrDie;
  die "Cannot detach from controlling Terminal" unless POSIX::setsid;
  exit if ForkOrDie;
  open(STDIN,  "+>/dev/null");
  open(STDOUT, "+>", "stdout.$$");
  open(STDERR, "+>", "stderr.$$");
}

sub email {
  my $s = eval "use Email::Stuff; Email::Stuff->new;";
  if ($@) { 
    die "Email::Stuff not installed";
  }
  return $s;
}

sub attach_text {
  use File::Basename;
  my ($s, $fn) = @_;
  my $bn = File::Basename::basename($fn) or undef;
  return $s->attach(Tools::slurp($fn), name => $bn, filename => $bn, content_type => 'text/plain');
} 

sub email_daemonize_logs {
  my ($to) = @_;
  my $s = email;
  $s->from     ('TestSwap__dont_reply@emulab.net' )
    ->to       ($to )
    ->subject  ("TestSwap run $$")
    ->text_body("TestSwap run $$");
  attach_text($s,"stdout.$$");
  attach_text($s,"stderr.$$");
  $s->send;
}
=pod

=head1 TestBed::Daemonize

=over 4

=item C<ForkOrDie>

dies if fork fails

=item C<daemonize>

daemonizes the process redirecting stdout and stderr to files

=item C<email>

generates a EMail::Stuff object

=item C<attach_text>

attaches a text file to a Email::Stuff object

=item C<email_daemonize_logs($to)>

send logs of daemon activity to $to

=back

=cut

1;
