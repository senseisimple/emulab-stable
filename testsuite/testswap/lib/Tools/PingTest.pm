#!/usr/bin/perl
use Modern::Perl;

package Tools::PingTest;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(ping);
use Data::Dumper;

sub ping {
  my ($host) = @_;
  system("ping -c 3 -W 1 -w 5 $host");
  $?;
}

1;
