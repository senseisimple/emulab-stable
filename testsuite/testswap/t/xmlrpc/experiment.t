#!/usr/bin/perl
use SemiModern::Perl;
use TestBed::TestSuite;
use Test::More tests => 3;;
use Data::Dumper;

my $teststr = "hello there";
like(e->echo($teststr), qr/$teststr/, "Experiment echo test");
ok(e->getlist_brief(), "Experiment getlist brief");
ok(e->getlist_full(), "Experiment getlist full");
