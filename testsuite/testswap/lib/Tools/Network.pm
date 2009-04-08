#!/usr/bin/perl
package Tools::Network;
use SemiModern::Perl;
use Net::Ping;
use Tools::TBSSH;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(ping test_traceroute);
use Data::Dumper;

sub ping {
  my ($host) = @_;
  my $p = Net::Ping->new('tcp', 2);
  $p->ping($host);
}

sub test_traceroute ($$@) {
  my ($src,$dest,@path) = @_;
  Tools::TBSSH::cmdcheckoutput($src, "traceroute $dest", 
  sub {
    local $_ = $_[0];
    local @_ = grep {!/^traceroute/} split /\n/;
    if (@_+0 != @path+0) {
      printf "*** traceroute $src->$dest: expected %d hops but got %d.\n",
      @path+0, @_+0;
      return 0;
    }
    for (my $i = 0; $i < @_; $i++) {
      local $_ = $_[$i];
      my ($n) = /^\s*\d+\s*(\S+)/;
      next if $n eq $path[$i];
      printf "*** traceroute $src->$dest: expected %s for hop %d but got %s\n",
      $path[0], $i+1, $n;
      return 0;
    }
    return 1;
  });
}

1;
