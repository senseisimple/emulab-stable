#!/usr/bin/perl
package TestBed::TestExperiment;
use SemiModern::Perl;
use TestBed::TestExperiment::Test;

my $ExperimentTests = [];

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(teste runtests);

sub teste {
  push @$ExperimentTests, TestBed::TestExperiment::Test::tn('', '', @_);
}

sub runtests {
  for (@$ExperimentTests) {
    $_->prep();
  }
  
  for (@$ExperimentTests) {
    $_->run();
  }
}

1;
