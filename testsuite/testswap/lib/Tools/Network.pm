#!/usr/bin/perl
package Tools::Network;
use SemiModern::Perl;
use Net::Ping;
use Tools::TBSSH;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(ping test_traceroute);
use Data::Dumper;

=head1 NAME

Tools::Network

=over 4

=item C<ping($hostname)>

pings $hostname returning 0 or 1

=cut

sub ping {
  my ($host) = @_;
  my $p = Net::Ping->new('tcp', 2);

  #Net::Ping returns 0 on success
  !$p->ping($host);
}

=item C<test_traceroute($src, $dest, ['hop1_host', 'hop2_host', ...])>

ssh to host $src and executes a traceroute to $dest ensuring it follows the path specified
returns 0 or 1
=cut
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

=back

=cut

1;
