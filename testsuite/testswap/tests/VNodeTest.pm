#!/usr/bin/perl
package VNodeTest;
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More 'no_plan';

my $nsfile = <<'END';
set ns [new Simulator]
source tb_compat.tcl

set node0 [$ns node]
tb-set-node-os $node0 @OS@
tb-set-hardware $node @HARDWARE@

set node1 [$ns node]
tb-set-node-os $node1 @OS@
tb-set-hardware $node @HARDWARE@

set node2 [$ns node]
tb-set-node-os $node2 @OS@
tb-set-hardware $node @HARDWARE@

#@LINKTYPE@
set lan0 [$ns make-lan "$node0 $node1 $node2 " 100Mb 0ms]

$ns rtproto Static
$ns run
END

sub VNodeTest {
  my ($params) = @_;
  my $options = defaults($params, 'OS' => 'RedHat9', 'HARDWARE' => 'pc3000', 'LINKTYPE' => 'REALLYFAST');


  my $ns = concretize($nsfile, %$options);
  say $ns;
  ok(1);
}

sub VNodeTest2 {
  my ($config) = @_;

  my @cases = CartProd($config);

  for (@cases) {
    my $ns = concretize($nsfile, %$_);
    say $ns;
    ok(1);
  }
}

VNodeTest unless caller;
1;
