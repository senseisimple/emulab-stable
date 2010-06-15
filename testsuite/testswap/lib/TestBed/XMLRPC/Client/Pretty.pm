#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package TestBed::XMLRPC::Client::Pretty;
use SemiModern::Perl;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(pretty_listexp experiments_hash_to_list);
use Data::Dumper;

sub pretty_listexp {
  for my $ed (experiments_hash_to_list(@_)) {
    my ($pid, $gid, $eid,) = @{ $ed->[0] };
    my $status = $ed->[1]->{'state'};
    say "$pid :: $gid :: $eid $status";
  }
}

sub experiments_hash_to_list {
  my ($h) = @_;
  my @exper_list;
  while(my ($pk, $v) = each %$h) {
    while(my ($gk, $v) = each %$v) {
      for my $e (@$v) {
        my $eid;
        my $status = "";
        if ( ref $e && exists $e->{'name'} )  { 
          $eid = $e->{'name'};
          $status = $e->{'state'};
        }
        else { $eid = $e; }
        push @exper_list, [ [$pk, $gk, $eid], $e];
      }
    }
  }
  return wantarray ? @exper_list : \@exper_list;
}

=head1 NAME

TestBed::XMLRPC::Client::Pretty;

=over 4

=item C<pretty_listexp>

pretty prints the XMLRPC response from listexp

=item C<experiments_hash_to_list>

converts the nested explist hash to an array of [ [$pid, $gid, $pid] $e] 

=back

=cut

1;
