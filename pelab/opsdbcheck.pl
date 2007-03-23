#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

# Script to test if data is being entered into the ops db properly.

use strict;
use English;
use Getopt::Std;
use lib '/usr/testbed/lib';
use libtbdb;
use libwanetmondb;

my $numMeas_ok = 100000;
my $numMeas_too_many = 5000000;
my $lastMeasTime_ok = time() - 60;  #last measurement must be within 60 seconds

my $query 
    = "select unixstamp from pair_data order by idx desc limit $numMeas_ok";

my @results = getRows($query);
if( scalar(@results) < $numMeas_ok ){
    print "fail: too few measurements in ops db\n";
    die -1;
}
if( scalar(@results) > $numMeas_too_many ){
    print "fail: too many measurements in ops db\n";
    die -1;
}
if( $results[0]->{unixstamp} < $lastMeasTime_ok ){
    print "fail: last measurement in ops db too old\n";
    die -2;
}
