#!/usr/bin/perl
use Modern::Perl;

package TestBed::TestSuite::Experiment::Macros;
use Data::Dumper;
require Exporter;
our @ISA = qw(Test::More);
our @EXPORT =qw(e ep echo newexp batchexp list list_brief list_full);

use TestBed::TestSuite::Experiment;
use Test::More;

sub echo          { ep()->echo(@_); }
sub _newexp       { my $e = e(shift, shift); $e->batchexp_ns(shift, @_); $e }
sub _newexp_wait  { my $e = e(shift, shift); $e->batchexp_ns_wait(shift, @_); $e }
sub newexp        { _newexp(@_); }
sub newexp_wait   { _newexp_wait(@_); }
sub batchexp      { _newexp(@_); }
sub batchexp_wait { _newexp_wait(@_); }
sub list          { ep()->getlist; }
sub list_brief    { ep()->getlist_brief; }
sub list_full     { ep()->getlist_full; }

1;
