#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;
use Experiment;

my $tmpfile = "/tmp/nsfile.$$";

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

my $query_result =
    DBQueryFatal("select * from nsfiles");

while (my ($pid, $eid, $exptidx, $nsfile) = $query_result->fetchrow_array()) {
    my $experiment = Experiment->Lookup($exptidx);
    if (!defined($experiment)) {
	die("Could not lookup experiment object for $exptidx\n");
    }
    open(NSFILE, ">$tmpfile")
	or die("Could not open $tmpfile for writing!\n");
    print NSFILE $nsfile;
    close(NSFILE);

    $experiment->SetNSFile($tmpfile) == 0
	or die("Could not add $tmpfile to $experiment!\n");
}

