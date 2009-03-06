#!/usr/bin/perl
use Modern::Perl;
use TestBed::TestSuite::Experiment tests => 3;
use Test::More;
use Data::Dumper;

my $teststr = "hello there";
like(echo($teststr), qr/$teststr/, "Experiment echo test");
ok(list_brief(), "Experiment getlist brief");
ok(list_full(), "Experiment getlist full");