#!/usr/bin/perl
use SemiModern::Perl;
use TBConfig;
use Tools::Network;
use Tools::TBSSH;
use Data::Dumper;
use Test::More tests => 1;

ok(Tools::Network::test_traceroute($TBConfig::OPS_SERVER, 'boss.emulab.net', 'public-router', 'boss'));
