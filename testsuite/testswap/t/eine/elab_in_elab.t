#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TBConfig;
use TestBed::TestSuite;
use TestBed::TestSuite::Experiment;
use Test::More tests => 2;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl
set ns [new Simulator]

tb-elab-in-elab 1
tb-elabinelab-singlenet

namespace eval TBCOMPAT {
    set elabinelab_maxpcs 3
    set elabinelab_hardware("boss") pc3000
    set elabinelab_hardware("ops") pc3000
    set elabinelab_nodeos("boss") FBSD62-STD
    set elabinelab_nodeos("ops") FBSD62-STD
}

$ns run 
NSEND

my $eid='eine';
my $e = e($eid);

#ok($e->startrun($ns, \&run_inside_exper), 'e-in-e started');

sub run_inside_exper {
  my $boss_name = $e->node('myboss.eine.tbres.emulab.net')->name;
  my $boss_url = "https://$boss_name:3069/usr/testbed";
  say $boss_url;
  my $cmd = "./tbts -d -x '$boss_url' t/xmlrpc/experiment.t";
  say $cmd;
  ok(!system($cmd), 'eine single node experiment');
}
run_inside_exper;
