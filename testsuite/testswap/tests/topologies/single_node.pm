#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More;
use Data::Dumper;

my $ns = <<'NSEND';
source tb_compat.tcl
set ns [new Simulator]
set node1 [$ns node]
$ns run
NSEND

rege(e('tpsinglenode'), $ns, sub { ok(shift->single_node_tests, "tpsinglenode single node tests"); }, 1, 'single_node_tests');
