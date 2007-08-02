#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2007 University of Utah and the Flux Group.
# All rights reserved.
#

use lib '/usr/testbed/lib';

use libdb;
use Project;
use English;
use Getopt::Long;
use strict;

# Getopts stuff goes here...

sub usage() {
    print << "END";
Usage:  $0 -h 
        $0 -a [-s]
        $0 -p <pid>[,<eid>] [-s] [-m <metric>[,<metric>]]
        $0 [-s] <vnode1> [... <vnodeN>]

-h   Prints this message
-a   Print information on all reserved nodes in the testbed.
-p   Restrict output to project 'pid' and optionally experiment 'eid'
-s   Summary display.  Per-node stats are not shown.
-m   Restrict metrics shown:
       last_tty - last tty activity.
       loadavg - 1, 5, and 15min load averages
       ifaces - interface input and output counters
[vnode ...] optional list of nodes to examine.  

The -a option, -p option, and [vnode ...] arguments are mutually exclusive.
END
    return 1;
}

#
# Global defined vars
#
my $debug = 1;
my %mets = ( last_tty => 0, loadavg => 0, ifaces => 0 );

#
# Global undefined/null vars
#
my %opts = ();
my @nodes = ();
my $pid;
my $eid;


#
# Parse options, and check permissions.
#

GetOptions(\%opts,'h','a','p=s','s','m=s');

if ($opts{"h"}) 
{
    exit &usage;
}

if (($opts{"p"} && (@ARGV > 0 || $opts{"a"})) 
    || ($opts{"a"} && (@ARGV > 0 || $opts{"p"}))) {
    print "Can't specify nodes and/or -a and/or -p options simultaneously.\n";
    exit &usage;
}


if ($opts{"p"}) {
    ($pid,$eid) = split(/,/, $opts{"p"});
    my $project = Project->Lookup($pid);
    if (!defined($project)) {
	die "Error:  Unknown project specified: '$pid'";
    }
    if (defined($eid)) {
        die "Error:  no such experiment '$eid' in project '$pid'.\n"
            unless ExpState($pid, $eid);
        die "Error:  you don't have access to this experiment.\n"
            unless TBExptAccessCheck($UID, $pid, $eid, TB_EXPT_READINFO);
    }
    else {
        die "Error:  You don't have appropriate access to this project.\n"
            unless TBProjAccessCheck($UID, $pid, $pid, TB_PROJECT_READINFO);
    }
    if ($debug) {
        print "valid pid: $pid\n";
        print "valid eid: $eid\n" if defined($eid);
    }
}


if ($opts{"m"}) {
    my $j;
    foreach $j (split(/,/, $opts{"m"})) {
        if (defined($mets{$j})) {
            $mets{$j} = 1;
            if ($debug) {
                print "metric specified: $j\n";
            }
        }
        else {
            print "\n\tError: unknown metric: $j\n\n";
            exit &usage;
        }
    }
}

if (@ARGV > 0) {
    my $j;
    @nodes = @ARGV;
    die "Permission to some or all nodes specified denied.\n" 
        unless TBNodeAccessCheck($UID, TB_NODEACCESS_READINFO, @nodes);
    foreach $j (@nodes) {
        die "Node $j is not a valid node.\n"
            unless TBValidNodeName($j);
    }
    if ($debug) {
        print "nodes specified: @nodes\n";
    }
}

exit 0;
