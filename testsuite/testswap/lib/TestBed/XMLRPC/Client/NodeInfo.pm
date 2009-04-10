#!/usr/bin/perl

package TestBed::XMLRPC::Client::NodeInfo;
use SemiModern::Perl;
require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(parseNodeInfo);
use Data::Dumper;

=head1 NAME

TestBed::XMLRPC::Client::NodeInfo

parses nodeInfo text returned from xmlrpc
=cut

my $ni= <<'END';
Experiment: tbres/tewkt         
State: active                                                                     

Virtual Node Info:
ID              Type         OS              Qualified Name
--------------- ------------ --------------- --------------------
node1           pc                           node1.tewkt.tbres.emulab.net
node2           pc                           node2.tewkt.tbres.emulab.net
END

my $li= <<'END';
Experiment: tbres/kk
State: active

Virtual Lan/Link Info:
ID              Member/Proto    IP/Mask         Delay     BW (Kbs)  Loss Rate
--------------- --------------- --------------- --------- --------- ---------
lan1            node1:0         10.1.1.2        0.00      100000    0.00000000
                ethernet        255.255.255.0   0.00      100000    0.00000000
lan1            node2:0         10.1.1.3        0.00      100000    0.00000000
                ethernet        255.255.255.0   0.00      100000    0.00000000
link1           node1:1         10.1.2.2        25.00     100000    0.00000000
                ethernet        255.255.255.0   25.00     100000    0.00000000
link1           node2:1         10.1.2.3        25.00     100000    0.00000000
                ethernet        255.255.255.0   25.00     100000    0.00000000

Physical Lan/Link Mapping:
ID              Member          IP              MAC                  NodeID
--------------- --------------- --------------- -------------------- ---------
lan1            node1:0         10.1.1.2        00:d0:b7:13:f3:87    pc32
                                                1/1 <-> 8/35         cisco1
lan1            node2:0         10.1.1.3        00:03:47:94:ca:c1    pc49
                                                4/1 <-> 8/22         cisco1
link1           node1:1         10.1.2.2        00:d0:b7:13:fa:d7    pc32
                                                0/1 <-> 8/33         cisco1
link1           node2:1         10.1.2.3        00:02:b3:35:e6:29    pc49
                                                3/1 <-> 8/21         cisco1

Virtual Queue Info:
ID              Member          Q Limit    Type    weight/min_th/max_th/linterm
--------------- --------------- ---------- ------- ----------------------------
lan1            node1:0         50 slots   Tail    0/0/0/0
lan1            node2:0         50 slots   Tail    0/0/0/0
link1           node1:1         50 slots   Tail    0/0/0/0
link1           node2:1         50 slots   Tail    0/0/0/0

END

=over 4

=item splitlines
=cut
sub splitlines {
  my @lines = split(/\n/, $_[0]);
  \@lines;
}

=item beforeaftermatch

Return lines from $array before and after matching $pattern
=cut
sub beforeaftermatch { 
  my ($pattern, $array) = @_;
  my $i = 0;
  my $d = 0;
  for (@{$array}) {
    if ($_ =~ $pattern) {
      $d = $i;
      last;
    }
    $i++;
  };
  my @a = @{$array};
  my @before = @a[0 .. ($d-1)];
  my @after  = @a[($d+1) .. $#a];
  (\@before, \@after);
}

sub aftermatch { 
  [beforeaftermatch(@_)]->[1]
}

=item parse_node_names

parses out node1.tewkt.tbres.emulab.net from the output

node1           pc                           node1.tewkt.tbres.emulab.net
=cut
sub parse_node_names {
  my ($nodes) = @_;
  my @nodes;
  for (@$nodes) {
    if($_ =~ /(\S+)$/) {
      push @nodes, $1;
    }
  }
  \@nodes;
}

=item parseNodeInfo

parses nodeInfo text returned form xmlrpc server into a list of node names

=cut
sub parseNodeInfo {
  my ($text) = @_;
  parse_node_names(aftermatch(qr/---------------/, splitlines($text)));
}

=item chunkit

split into before and after chunks using delimiters

=cut
sub chunkit {
  my ($array, @patterns) = @_;

  my @chunks;
  my ($block, $rest);
  for (@patterns) {
    ($block, $rest) = beforeaftermatch($_, $array);
    push @chunks, $block;
    $array = $rest;
  }
  push @chunks, $rest;

  @chunks;
}

=item unique

returns unique items from an array

=cut
sub unique {
  my %hash;
  for (@_) {
    if( ! exists $hash{$_}) {
      $hash{$_} = 1;
    }  
  }
  my @result;
  while (my ($k, $v) = each %hash) {
    push @result, $k;
  }
  @result;
}

=item parseLinkInfo

parses linkInfo text returned form xmlrpc server into a list of link names

=cut
sub parseLinkInfo {
  my ($text) = @_;
  my @chunks = chunkit(splitlines($text), 'Virtual Lan/Link Info:',  'Physical Lan/Link Mapping:', 'Virtual Queue Info:');
  #say Dumper(\@chunks);
  my @a = @{$chunks[1]};
  #say Dumper(\@a);
  my @b = @a[2 .. $#a];
  #say Dumper(\@b);

  my @links;
  for (@b) {
    if($_ =~ /^(\S+)/) {
      push @links, $1;
    }
  }
  @links = unique(@links);
  #say Dumper(\@links);
  \@links;
}

=back

=cut
1;
