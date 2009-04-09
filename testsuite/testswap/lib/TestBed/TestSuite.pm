#!/usr/bin/perl
package TestBed::TestSuite;
use SemiModern::Perl;
use TestBed::TestSuite::Experiment;
use Data::Dumper;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(e ep dpe);

sub ep  { TestBed::TestSuite::Experiment->new }
sub e   { TestBed::TestSuite::Experiment->new('pid'=> shift, 'eid' => shift) }
sub dpe { TestBed::TestSuite::Experiment->new('pid'=> $TBConfig::DEFAULT_PID, 'eid' => shift) }

1;
