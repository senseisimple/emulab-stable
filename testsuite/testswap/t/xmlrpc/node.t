#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use SemiModern::Perl;
use TestBed::XMLRPC::Client::Node;
use Data::Dumper;
use Test::More tests => 8;

my $node = TestBed::XMLRPC::Client::Node->new();
ok($node, 'Node new works');
isa_ok($node, 'TestBed::XMLRPC::Client::Node');

ok($node->available, 'nodes available');
ok($node->available('type' => 'pc3000'), 'pc3000 nodes available');
ok($node->getlist('type' => 'pc3000'), 'getlist returns hash');
ok((keys %{$node->typeinfo}) > 20, 'node typeinfo has at least 20 types');
ok($node->get_free('type' => 'pc3000'), 'get_free type => pc3000');
ok($node->get_free_names('type' => 'pc3000'), 'get_free_names type => pc3000');
