#!/usr/bin/perl
package TestBed::TestSuite::Experiment::Macros;
use SemiModern::Perl;
use TestBed::XMLRPC::Client::Pretty;
use Data::Dumper;
require Exporter;
our @ISA = qw(Test::More);
our @EXPORT =qw(e echo list list_brief list_full
      plistexps);

use TestBed::TestSuite;
use TestBed::TestSuite::Experiment;
use Test::More;

sub echo          { e()->echo(@_); }
sub list          { e()->getlist; }
sub list_brief    { e()->getlist_brief; }
sub list_full     { e()->getlist_full; }
sub plistexps     { pretty_listexp(list_full); }

=head1 NAME

TestBed::TestSuite::Experiment::Macros

B<EXPERIMENTAL>

provides some common used class methods as global functions in the current package namespace

=cut

1;
