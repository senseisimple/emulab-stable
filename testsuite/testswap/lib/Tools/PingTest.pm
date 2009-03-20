#!/usr/bin/perl
use SemiModern::Perl;
use Net::Ping;

package Tools::PingTest;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(ping);
use Data::Dumper;

sub ping {
  my ($host) = @_;
  my $p = Net::Ping->new('tcp', 2);
  $p->ping($host);
}

1;
