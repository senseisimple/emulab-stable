#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;

use TestBed::XMLRPC::Client::Emulab;
use Test::More tests => 3;
use Time::Local;
use Data::Dumper;
use Tools;
use RPC::XML qw(time2iso8601);


my $emuclient = TestBed::XMLRPC::Client::Emulab->new();
ok($emuclient, 'Emulab new works');
isa_ok($emuclient, 'TestBed::XMLRPC::Client::Emulab');

my $time = timegm(0,0,0,1,0,2008);
my $resp = $emuclient->news('starting' => time2iso8601($time));
ok($resp, 'Valid Emulab::new response');
