package Tools;
use SemiModern::Perl;
use Log::Log4perl qw(get_logger :levels);
use POSIX qw(setsid);
#use Log::Log4perl::Appender::Screen
#use Log::Log4perl::Appender::ScreenColoredLevels
#use Log::Log4perl::Appender::File

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(prettytimestamp timestamp sayts sayperl slurp toperl
                 init_tbts_logger concretize yn_prompt splat_to_temp);

=head1 NAME

Tools

=over 4

=item C<concretize($text, NAME => VALUE, ...)>

fills in template $text via search and replace

=cut

sub concretize {
  my $text = shift;
  my %repl = @_;
  while (my ($k, $v) = each %repl) {
    $text =~ s/\@$k\@/$v/g;
  }
  $text;
}

=item C<slurp($filename)>

returns the entire contents of $filename
=cut
sub slurp {
  my ($fn) = @_;
  open(my $fh, "<", $fn) or die "$fn not found or couldn't be opened";
  local( $/ );
  my $data = <$fh>;
  close($fh);
  return $data;
}

=item C<splat($filename, $file_data)>

writes $file_data out to $filename
=cut
sub splat {
  my ($fn, $data) = @_;
  open(my $fh, ">", $fn);
  print $fh $data;
  close($fh);
}

=item C<prettytimestamp()>

returns a "2009-01-30T10:10:20" timestamp string
=cut
sub prettytimestamp {
  my $t = shift || time;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($t);
  sprintf "%4d-%02d-%02dT%02d:%02d:%02d", $year+1900,$mon+1,$mday,$hour,$min,$sec;
}

=item C<timestamp()>

returns a "20090130101020" timestamp string
=cut
sub timestamp {
  my $t = shift || time;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($t);
  sprintf "%4d%02d%02d%02d%02d%02d", $year+1900,$mon+1,$mday,$hour,$min,$sec;
}

=item C<sayts($msg)>

prints "2009-01-30T10:10:20 $msg\n"
=cut
sub sayts {
  print prettytimestamp() . " ";
  say @_;
}

=item C<perlit(@array)>

stringifies an array in perl syntax
=cut
sub perlit {
  map {_perlit($_)} @_; 
}

=item c<toperl(@array)

stringifies an array in perl syntax, joined with commas
=cut
sub toperl {
  join(", ", perlit(@_));
}

=item c<sayperl(@array)

stringifies and says an array in perl syntax, joined with commas
=cut
sub sayperl {
  say toperl(@_);
}

=item c<_perlit($item)

stringifies an item in perl syntax
=cut
sub _perlit {
  my ($x) = @_;
  $x ||= '';
  my $ref = ref($x);
  if (!defined $ref) {
  }
  elsif ($ref eq 'ARRAY') {
    "[" . join(", ", perlit(@$x)) . "]";
  }
  elsif ($ref eq 'HASH') {
    my $o = "[";
    my @els;
    while (my ($k, $v) = each (%$x)) {
      my $o =_perlit($k) . " => " . _perlit($v);
      push @els, $o;
    }
    $o .= join(", ", @els);
    $o .= "]";
    $o;
  }
  else {
    return "'" . $x . "'";
  }
}

=item  C<init_tbts_logger($name, $file, $level, $appender_type)>

shortcut subroutine to create a logger of name $name that appends to $file or SCREEN with level $level

=cut

sub init_tbts_logger {
  my ($name, $file, $level, $appender_type) = @_;
  $file ||= $name;
  $level ||= $INFO;
  $appender_type ||= 'SCREEN';
  my $logger = get_logger($name);
  $logger->level($INFO);
  $logger->level($DEBUG) if $level =~ /DEBUG/;
  my $layout = Log::Log4perl::Layout::PatternLayout->new( "%d %p> %F{1}:%L %M - %m%n");

  if ($appender_type =~ /FILE/) {
    my $appender = Log::Log4perl::Appender->new(
        "Log::Log4perl::Appender::File",
        filename => timestamp() . $file,
        mode     => "append",
        );
    $appender->layout($layout);
    $logger->add_appender($appender);
  }
  elsif ($appender_type =~ /SCREEN/) {
    my $appender = Log::Log4perl::Appender->new(
        "Log::Log4perl::Appender::Screen",
        filename => timestamp() . $file,
        mode     => "append",
        );
    $appender->layout($layout);
    $logger->add_appender($appender);
  }

  $logger;
}

=item C<getyn>

returns 1 if user types Y or y 0 otherwise

=cut

sub getyn {
  use Term::ReadKey;
  open(TTY, "</dev/tty");
  ReadMode "raw";
  my $key = ReadKey 0, *TTY;
  ReadMode "normal";
  print $key;
  lc($key) eq 'y';
}

=item C<yn_prompt($prompt)>

prints $prompt
returns 1 if user types Y or y 0 otherwise

=cut

sub yn_prompt {
  print $_[0] . " ";
  my $r = getyn;
  print "\n";
  return $r;
}

=item C<splat_to_temp($data)>

writes $data to tempfile and returns a File::Temp object

=cut

sub splat_to_temp {
 my $data = shift;
  use File::Temp;
  my $tmp = File::Temp->new;
  print $tmp $data;
  close $tmp;
  return $tmp;
}

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

=back

=cut

1;
