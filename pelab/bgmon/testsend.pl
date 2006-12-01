#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Configure variables
#
use lib '/usr/testbed/lib';

use event;
use Getopt::Std;
use strict;
use libwanetmon;

sub usage {
    warn "Usage: $0 [-i managerID] [-e pid/eid] [-p period] [-d duration]".
	"<srcnode> <dstnode> <testtype> <COMMAND>\n";
    return 1;
}

my %opt = ();
getopt("i:e:p:d:h",\%opt);
if ($opt{h}) { exit &usage; }
my ($managerid, $period, $duration, $type, $srcnode, $dstnode, $cmd);
my ($server,$bgmonexpt);
$server = "localhost";
if($opt{i}){ $managerid=$opt{i}; } else{ $managerid = "testsend"; }
if($opt{e}){ $bgmonexpt=$opt{e}; } else{ $bgmonexpt = "tbres/pelabbgmon"; }
if($opt{p}){ $period = $opt{p}; } else{ $period = 0; }
if($opt{d}){ $duration = $opt{d}; } else{ $duration = 0; }
my $URL = "elvin://$server";
#if ($port) { $URL .= ":$port"; }

if (@ARGV != 4) { exit &usage; }
($srcnode, $dstnode, $type, $cmd) = @ARGV;

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }

my %cmd = ( managerID => $managerid,
	    srcnode => $srcnode,
	    dstnode => $dstnode,
	    testtype => $type,
	    testper => "$period",
	    duration => "$duration",
	    expid    => "$bgmonexpt"
	    );

sendcmd_evsys($cmd, \%cmd, $handle);


if (event_unregister($handle) == 0) {
    die("could not unregister with event system");
}

exit(0);
