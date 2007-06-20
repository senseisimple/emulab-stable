#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;
use Experiment;

$| = 1;

my %nodeids	= ();


# Need this for computing downtime.
my $downexp = Experiment->Lookup(NODEDEAD_PID(), NODEDEAD_EID());
if (!defined($downexp)) {
    die("Could not lookup hwdown experiment object\n");
}

#
# Cleanup some stuff ...
#
$query_result =
    DBQueryFatal("select e.pid,e.eid,s.rsrcidx,e.idx,e.creator_idx,s.created ".
		 " from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "left join experiment_resources as r on r.idx=s.rsrcidx ".
		 "where r.exptidx is null");

while (my ($pid,$eid,$rsrcidx,$exptidx,$creator_idx,$created) =
       $query_result->fetchrow_array()) {
    DBQueryFatal("insert into experiment_resources ".
		"(idx, tstamp, exptidx, uid_idx) ".
		"values ($rsrcidx, '$created', $exptidx, $creator_idx)");
}

#
# Get the list of all current physical nodes.
#
$query_result =
    DBQueryFatal("select node_id from nodes as n ".
		 "left join node_types as t on t.type=n.type ".
		 "where t.class='pc' or t.class='pcplabphys'");

while (my ($node_id) = $query_result->fetchrow_array()) {
    $nodeids{$node_id} = {};
}

#
# Need to form an inception date. Use the first entry we find in the
# node_history table, or the current time. 
#
foreach my $nodeid (keys(%nodeids)) {
    $query_result =
	DBQueryFatal("select MIN(stamp) from node_history ".
		     "where node_id='$nodeid'");
    if (!$query_result->numrows) {
	$nodeids{$nodeid}->{'inception'} = time();
    }
    else {
	my ($stamp) = $query_result->fetchrow_array();
	$stamp = time()
	    if (!defined($stamp));

	$nodeids{$nodeid}->{'inception'} = $stamp;
    }
    $nodeids{$nodeid}->{'allocated'} = 0;
    $nodeids{$nodeid}->{'down'}      = 0;
}

#
# Now compute utilization.
#
foreach my $nodeid (keys(%nodeids)) {
    $query_result =
	DBQueryFatal("select op,exptidx,stamp from node_history ".
		     "where node_id='$nodeid' ".
		     "order by stamp");
    next
	if (!$query_result->numrows);

    my $alloctime = 0;
    my $downtime  = 0;
    my $alloced   = 0;
    my $lastexpt  = 0;

    while (my ($op,$exptidx,$stamp) = $query_result->fetchrow_array()) {
	#print "$op, $exptidx, $stamp\n";
	
	if ($op eq "alloc") {
	    $alloced  = $stamp;
	    $lastexpt = $exptidx;
	}
	elsif ($op eq "move") {
	    if (!$alloced) {
		$alloced  = $stamp;
		$lastexpt = $exptidx;
		next;
	    }
	    my $diff = $stamp - $alloced;

	    if ($lastexpt == $downexp->idx()) {
		$downtime += $diff;
	    }
	    else {
		$alloctime += $diff;
	    }
	    $alloced    = $stamp;
	    $lastexpt   = $exptidx;
	}
	else {
	    # Missing records ...
	    next
		if (!$alloced);
	    
	    my $diff = $stamp - $alloced;

	    if ($exptidx == $downexp->idx()) {
		$downtime += $diff;
	    }
	    else {
		$alloctime += $diff;
	    }
	    $alloced  = 0;
	    $lastexpt = 0;
	}
    }
    $nodeids{$nodeid}->{'allocated'} = $alloctime;
    $nodeids{$nodeid}->{'down'}      = $downtime;
}

foreach my $nodeid (keys(%nodeids)) {
    $inception = $nodeids{$nodeid}->{'inception'};
    $alloctime = $nodeids{$nodeid}->{'allocated'};
    $downtime  = $nodeids{$nodeid}->{'down'};

    print "$nodeid, $inception, $alloctime, $downtime\n";

    DBQueryFatal("update nodes set inception=FROM_UNIXTIME($inception) ".
		 "where node_id='$nodeid'");
    DBQueryFatal("replace into node_utilization set ".
		 "  node_id='$nodeid', allocated=$alloctime, down=$downtime");
}

