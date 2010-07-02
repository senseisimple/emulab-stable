#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;

my $ns = <<'NSEND';
source tb_compat.tcl

set ns [new Simulator]

set node1 [$ns node]
set node2 [$ns node]

set lan1 [$ns make-lan "$node1 $node2" 100Mb 0ms]
$ns run
NSEND

rege(e('twonodelan'), $ns, sub { ok(shift->single_node_tests, "twonodelan single node tests"); }, 1, 'two_node_lan single_node_tests');
