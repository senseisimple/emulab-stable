#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TBConfig;
use Tools::Network;
use Tools::TBSSH;
use Data::Dumper;
use Test::More tests => 2;

ok(Tools::Network::ping($TBConfig::OPS_SERVER), 'ping');
ok(Tools::Network::traceroute($TBConfig::OPS_SERVER, 'boss.emulab.net', 'public-router', 'boss'), 'traceroute ops to boss');
