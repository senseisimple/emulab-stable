#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
package SyncTests;
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;

my $Sync= << 'END';
set ns [new Simulator]
source tb_compat.tcl

set node0 [$ns node]
set node1 [$ns node]
set node2 [$ns node]

tb-set-node-os $node0 @OS@
tb-set-node-os $node1 @OS@
tb-set-node-os $node2 @OS@

tb-set-sync-server $node0

$ns run
END

my $OS ="RHL90-STD";

sub sync_test {
  my $e = shift;
  my $eid = $e->eid;
  my $ct = 500;
  ok( $e->node('node0')->ssh->cmdsuccess("/usr/testbed/bin/emulab-sync -a -i $ct"),  "$eid setup $ct node barrier" );
  ok( $e->node('node1')->ssh->cmdsuccess("\'for x in `seq 1 $ct`; do :(){ /usr/testbed/bin/emulab-sync > /dev/null 2> /dev/null < /dev/null &};: ; done\'"), "$eid prun sh" );
}

rege(e('sync'), concretize($Sync, OS => $OS), \&sync_test, 2, 'ImageTest-sync test');

1;
