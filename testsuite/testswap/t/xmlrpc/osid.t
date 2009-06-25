#!/usr/bin/perl
use SemiModern::Perl;

use TestBed::XMLRPC::Client::OSID;
use Test::More tests => 7;
use Data::Dumper;
use Tools;

sub okcontains {
  my ($hash, @keys) = @_;
  ok(exists $hash->{$_}, "OSID list has $_") for (@keys);
}

my $osid = TestBed::XMLRPC::Client::OSID->new();
ok($osid, 'osid new works');
isa_ok($osid, 'TestBed::XMLRPC::Client::OSID');

my $resp = $osid->getlist;
sayd(keys %$resp);
ok($resp, 'getlist response');
okcontains($resp, 'RHL-STD', 'RHL90-STD', 'FBSD63-STD');

$resp = $osid->info('RHL-STD');
ok($osid, 'info response');
