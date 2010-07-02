#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2006 University of Utah and the Flux Group.
# All rights reserved.
#

use English;
use Errno;
#use POSIX;
#use Socket;
#use BSD::Resource;
#use URI::Escape;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

my $query_result =
    DBQueryFatal("select pid,eid,state ".
		 "from experiments where batchmode=0");

while (my ($pid,$eid,$state) =
       $query_result->fetchrow_array) {
    my $batchstate;

    if ($state eq EXPTSTATE_ACTIVATING) {
	$batchstate = BATCHSTATE_ACTIVATING;
    }
    elsif ($state eq EXPTSTATE_ACTIVE) {
	$batchstate = BATCHSTATE_RUNNING;
    }
    elsif ($state eq EXPTSTATE_SWAPPED) {
	$batchstate = BATCHSTATE_PAUSED;
    }
    elsif ($state eq EXPTSTATE_SWAPPING ||
	   $state eq EXPTSTATE_TERMINATING ||
	   $state eq EXPTSTATE_TERMINATED) {
	$batchstate = BATCHSTATE_TERMINATING;
    }
    elsif ($state eq EXPTSTATE_NEW ||
	   $state eq EXPTSTATE_PRERUN) {
	$batchstate = BATCHSTATE_PAUSED;
    }

    print "update experiments set batchstate='$batchstate' ".
	"where pid='$pid' and eid='$eid';\n";
}
		 
