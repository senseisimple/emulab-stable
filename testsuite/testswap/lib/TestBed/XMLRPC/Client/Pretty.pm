#!/usr/bin/perl

package TestBed::XMLRPC::Client::Pretty;
use SemiModern::Perl;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(pretty_listexp);
use Data::Dumper;

sub pretty_listexp {
  my ($h) = @_;
  while(my ($pk, $v) = each %$h) {
    while(my ($gk, $v) = each %$v) {
      for my $e (@$v) {
        my $eid;
        if ( exists $e->{'name'} )  { $eid = sprintf("%s %s", $e->{'name'}, $e->{'state'});}
        else { $eid = $e; }
        say "$pk :: $gk :: $eid";
      }
    }
  }
}

1;
