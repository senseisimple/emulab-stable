#!/usr/bin/perl
package TestBed::TestSuite::Experiment::Macros;
use SemiModern::Perl;
use TestBed::XMLRPC::Client::Pretty;
use Data::Dumper;
require Exporter;
our @ISA = qw(Test::More);
our @EXPORT =qw(e ep echo newexp batchexp list list_brief list_full
      plistexps);

use TestBed::TestSuite;
use TestBed::TestSuite::Experiment;
use Test::More;

sub echo          { ee()->echo(@_); }
sub batchexp      { my $e = e(shift, shift); $e->batchexp_ns(@_); $e }
sub batchexp_wait { my $e = e(shift, shift); $e->batchexp_ns_wait(@_); $e }
sub newexp        { batchexp(@_, batch => 0); }
sub newexp_wait   { batchexp_wait(@_, batch => 0); }
sub list          { ee()->getlist; }
sub list_brief    { ee()->getlist_brief; }
sub list_full     { ee()->getlist_full; }
sub plistexps     { pretty_listexp(list_full); }

=head1 NAME

TestBed::TestSuite::Experiment::Macros

B<EXPERIMENTAL>

provides some common used class methods as global functions in the current package namespace

=cut

1;
