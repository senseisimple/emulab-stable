#!/usr/bin/perl
use SemiModern::Perl;

package TestBed::XMLRPC::Client::NodeInfo;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(parseNodeInfo);
use Data::Dumper;

our $ni= <<'END';
Experiment: tbres/tewkt         
State: active                                                                     

Virtual Node Info:
ID              Type         OS              Qualified Name
--------------- ------------ --------------- --------------------
node1           pc                           node1.tewkt.tbres.emulab.net
node2           pc                           node2.tewkt.tbres.emulab.net
END

sub splitlines {
  my @lines = split(/\n/, $_[0]);
  \@lines;
}

sub asplitmatch { 
  my ($pat, $array) = @_;
  my $i = 0;
  my $d = 0;
  for (@{$array}) {
    if ($_ =~ $pat) {
      $d = $i;
      last;
    }
    $i++;
  };
  my @a = @{$array};
  my @aa = @a[($d+1) .. $#a];
  \@aa;
}

sub project_nodes {
  my ($nodes) = @_;
  my @nodes;
  for (@$nodes) {
    if($_ =~ /(\S+)$/) {
      push @nodes, $1;
    }
  }
  \@nodes;
}

sub parseNodeInfo {
  my ($text) = @_;
  project_nodes(asplitmatch(qr/---------------/, splitlines($text)));
}

1;
