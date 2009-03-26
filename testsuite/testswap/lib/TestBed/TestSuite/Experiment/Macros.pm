#!/usr/bin/perl
use SemiModern::Perl;

package TestBed::TestSuite::Experiment::Macros;
use TestBed::XMLRPC::Client::Pretty;
use Data::Dumper;
require Exporter;
our @ISA = qw(Test::More);
our @EXPORT =qw(e ep echo newexp batchexp list list_brief list_full
      plistexps);

use TestBed::TestSuite::Experiment;
use Test::More;

sub echo          { ep()->echo(@_); }
sub batchexp      { my $e = e(shift, shift); $e->batchexp_ns(@_); $e }
sub batchexp_wait { my $e = e(shift, shift); $e->batchexp_ns_wait(@_); $e }
sub newexp        { batchexp(@_, batch => 0); }
sub newexp_wait   { batchexp_wait(@_, batch => 0); }
sub list          { ep()->getlist; }
sub list_brief    { ep()->getlist_brief; }
sub list_full     { ep()->getlist_full; }
sub plistexps     { pretty_listexp(list_full); }

1;
