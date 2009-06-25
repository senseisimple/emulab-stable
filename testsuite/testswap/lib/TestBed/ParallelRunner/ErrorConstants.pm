#!/usr/bin/perl
package TestBed::ParallelRunner::ErrorConstants;
use SemiModern::Perl;
require Exporter;

our @ISA = qw(Exporter);
our @EXPORT = qw(RETURN_AND_REPORT);

use constant RETURN_AND_REPORT => 4096;

1;
