use Modern::Perl;

package Tools;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(prettytimestamp timestamp sayts sayperl);

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

sub sayperl {
  say join(", ", perlit(@_));
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

1;
