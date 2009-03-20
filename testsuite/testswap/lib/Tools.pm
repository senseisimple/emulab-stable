package Tools;
use SemiModern::Perl;
use Log::Log4perl qw(get_logger :levels);
#use Log::Log4perl::Appender::Screen
#use Log::Log4perl::Appender::ScreenColoredLevels
#use Log::Log4perl::Appender::File

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(prettytimestamp timestamp sayts sayperl slurp splat toperl
                 init_tbts_logger);

sub slurp {
  my ($fn) = @_;
  open(my $fh, "<", $fn) or die "$fn not found or couldn't be opened";
  local( $/ );
  my $data = <$fh>;
  close($fh);
  return $data;
}

sub splat {
  my ($fn, $data) = @_;
  open(my $fh, ">", $fn);
  print $fh $data;
  close($fh);
}

sub prettytimestamp {
  my $t = shift || time;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($t);
  sprintf "%4d-%02d-%02dT%02d:%02d:%02d", $year+1900,$mon+1,$mday,$hour,$min,$sec;
}

sub timestamp {
  my $t = shift || time;
  my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($t);
  sprintf "%4d%02d%02d%02d%02d%02d", $year+1900,$mon+1,$mday,$hour,$min,$sec;
}

sub sayts {
  print prettytimestamp() . " ";
  say @_;
}

sub perlit {
  map {_perlit($_)} @_; 
}

sub toperl {
  join(", ", perlit(@_));
}

sub sayperl {
  say toperl(@_);
}

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

sub init_tbts_logger {
  my ($name, $file, $level, $app_type) = @_;
  $file ||= $name;
  $level ||= $INFO;
  $app_type ||= 'SCREEN';
  my $logger = get_logger($name);
  $logger->level($INFO);
  $logger->level($DEBUG) if $level =~ /DEBUG/;
  my $layout = Log::Log4perl::Layout::PatternLayout->new( "%d %p> %F{1}:%L %M - %m%n");

  if ($app_type =~ /FILE/) {
    my $appender = Log::Log4perl::Appender->new(
        "Log::Log4perl::Appender::File",
        filename => timestamp() . $file,
        mode     => "append",
        );
    $appender->layout($layout);
    $logger->add_appender($appender);
  }
  elsif ($app_type =~ /SCREEN/) {
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

1;
