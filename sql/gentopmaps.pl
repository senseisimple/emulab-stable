#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

#
# Turn off line buffering on output
#
$| = 1;

my $gentopofile = "/usr/testbed/libexec/gentopofile";

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$query_result =
    DBQueryFatal("select pid,eid,expt_head_uid from experiments ");

while (($pid,$eid,$creator) = $query_result->fetchrow_array()) {
    my $workdir = TBExptWorkDir($pid, $eid);
    my $userdir = TBExptUserDir($pid, $eid);

    if (! -d $workdir) {
	print "Skipping $pid/$eid cause workdir does not exist!\n";
	next;
    }
    
    if (! -e "$workdir/topomap") {
	print "Generating topomap for $pid/$eid\n";
	
	if (system("cd $workdir; ".
		   "sudo -u $creator $gentopofile $pid $eid")) {
	    print "Failed to generate topomap for $pid/$eid!\n";
	    next;
	}
    }
}
