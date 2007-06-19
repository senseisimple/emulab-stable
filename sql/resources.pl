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

$| = 1;

my $impotent      = 0;
my %bytimestamp   = ();
my %resources     = ();
my %mapping       = ();
my %testbed_stats = ();
my $nextidx;
my $nullrowref    = {};

sub copy_hash($$) {
    my ($new, $old) = @_;
    
    foreach (keys(%$old)) {
	$new->{$_} = $old->{$_};
    }
}

sub InsertRecord($$$$$;$)
{
    my ($exptidx, $copyidx, $statidx, $unixtime, $uid_idx, $newrsrc) = @_;
    
    $unixtime++;

    my $query_result = DBQueryFatal("select FROM_UNIXTIME('$unixtime')");
    ($tstamp) = $query_result->fetchrow_array();
    my $rowref = {};
    $newrsrc = $nextidx++
	if (!defined($newrsrc));

    copy_hash($rowref, $resources{$copyidx});
    $rowref->{'idx'}             = $newrsrc;
    $rowref->{'tstamp'}          = $tstamp;
    $rowref->{'swapin_time'}     = 0;
    $rowref->{'uid_idx'}         = $uid_idx;
    $rowref->{'swapout_time'}    = 0;
    $rowref->{'swapmod_time'}    = 0;
    
    $rowref->{'save'}            = 1;
    $rowref->{'unixtime'}        = $unixtime;

    $resources{$newrsrc} = $rowref;
    $bytimestamp{$unixtime} = $rowref;
    
    # testbed_stats gets the new pointer.
    $testbed_stats{$statidx} = $newrsrc;

    $mapping{$copyidx} = $newrsrc;
    print "Mapping $copyidx to $newrsrc ($tstamp)\n";
    return $newrsrc;
}

sub UpdateResourceRecord($$$$;$$$)
{
    my ($exptidx, $rsrcidx, $action, $tstamp, $uididx,
	$byswapmod, $byswapin) = @_;

    print "Updating resource record $rsrcidx. action=$action\n";
    my $rowref = $resources{$rsrcidx};

    $rowref->{'byswapin'}  = $byswapin
	if (defined($byswapin));
    $rowref->{'byswapmod'} = $byswapmod
	if (defined($byswapmod));
	
    if ($action eq "swapin" || $action eq "start") {
	$rowref->{'swapin_time'}    = $tstamp;
	$rowref->{'uid_idx'}        = $uididx;
    }
    elsif ($action eq "swapout") {
	$rowref->{'swapout_time'}   = $tstamp;
    }
    elsif ($action eq "preload") {
	$rowref->{'uid_idx'}        = $uididx;
    }
    elsif ($action eq "swapmod") {
	$rowref->{'swapmod_time'}   = $tstamp;
	$rowref->{'uid_idx'}        = $uididx
	    if (defined($uididx));
    }
    elsif ($action eq "noaction") {
	$rowref->{'uid_idx'}        = $uididx
	    if (defined($uididx));
    }
    $rowref->{'save'} = 1;
}

sub ClearResourceRecord($)
{
    my ($rsrcidx) = @_;
    my $rowref = $resources{$rsrcidx};

    foreach my $slot ("vnodes",
		      "pnodes",
		      "wanodes",
		      "plabnodes",
		      "simnodes",
		      "jailnodes",
		      "delaynodes",
		      "linkdelays",
		      "walinks",
		      "links",
		      "lans",
		      "shapedlinks",
		      "shapedlans",
		      "wirelesslans",
		      "minlinks",
		      "maxlinks") {

	$rowref->{$slot} = 0;
    }
    $rowref->{'save'} = 1;
}

DBQueryFatal("create table if not exists experiment_resources_new ".
	     "like experiment_resources");
DBQueryFatal("delete from experiment_resources_new");
DBQueryFatal("insert into experiment_resources_new ".
	     "select * from experiment_resources");

DBQueryFatal("create table if not exists experiment_stats_new ".
	     "like experiment_stats");
DBQueryFatal("delete from experiment_stats_new");
DBQueryFatal("insert into experiment_stats_new ".
	     "select * from experiment_stats");

DBQueryFatal("create table if not exists testbed_stats_new ".
	     "like testbed_stats");
DBQueryFatal("delete from testbed_stats_new");
DBQueryFatal("insert into testbed_stats_new ".
	     "select * from testbed_stats");

if (0) {
    DBQueryFatal("lock tables ".
		 "   experiments write, " .
		 "   testbed_stats write, " .
		 "   experiment_stats write, " .
		 "   experiment_resources write, " .
		 "   testbed_stats_new write, " .
		 "   experiment_stats_new write, " .
		 "   experiment_resources_new write");
}

my $query_result =
    DBQueryFatal("select MAX(idx) from experiment_resources_new");
($nextidx) = $query_result->fetchrow_array();
$nextidx += 100000;

$query_result =
    DBQueryFatal("select s.exptidx,e.state from experiment_stats as s ".
		 "left join experiments as e on e.idx=s.exptidx ".
#		 "where s.exptidx=8751 or s.exptidx=30605 or s.exptidx=13307 ".
#		 "  or s.exptidx=19412 ".
#		 "where s.exptidx=30605  ".
		 "order by idx");

while (my ($exptidx,$expstate) = $query_result->fetchrow_array()) {
    my $state = "nostate";
    my $skip  = 0;
    my $munge  = 0;
    my $lastrsrc = 0;
    my $swapins = 0;
    my @stats = ();
    my $laststamp = 0;
    %resources = ();
    %testbed_stats = ();
    %mapping   = ();
    %bytimestamp = ();

    # See where the current rsrcidx and lastidx are pointing in case
    # they need to be updated.
    my $estats_result =
	DBQueryFatal("select rsrcidx,lastrsrc from experiment_stats_new ".
		     "where exptidx='$exptidx'");
    my ($expt_rsrcidx,$expt_lastidx) = $estats_result->fetchrow_array();

    print "\n";
    print "--------------------------------------------------\n";
    print "$exptidx $expt_rsrcidx " .
	(defined($expt_lastidx) ? $expt_lastidx : "") . "\n";

    #
    # Generate a list of the resource records that we can munge
    # here, and then write back later.
    #
    my $resources_result =
	DBQueryFatal("select *,UNIX_TIMESTAMP(tstamp) as unixtime ".
		     "   from experiment_resources_new ".
		     "where exptidx='$exptidx'");

    while (my $rowref = $resources_result->fetchrow_hashref()) {
	my $idx = $rowref->{'idx'};
	my $unixtime = $rowref->{'unixtime'};

	$resources{$idx} = $rowref;
	$bytimestamp{$unixtime} = $rowref;
    }

    my $stats_result =
	DBQueryFatal("select *,UNIX_TIMESTAMP(start_time) as start_time, ".
		     "       UNIX_TIMESTAMP(end_time) as end_time ".
		     "  from testbed_stats_new ".
		     "where exptidx='$exptidx' ".
		     "order by UNIX_TIMESTAMP(end_time)");

    while (my $rowref = $stats_result->fetchrow_hashref()) {
	push(@stats, $rowref);

	my $rsrcidx = $rowref->{'rsrcidx'};
	$resources{$rsrcidx}->{'save'} = 1;
    }
    
    foreach my $rowref (@stats) {
	my $idx     = $rowref->{'idx'};
	my $rsrcidx = $rowref->{'rsrcidx'};
	my $lastidx = $resources{$rsrcidx}->{'lastidx'};
	my $action  = $rowref->{'action'};
	my $ecode   = $rowref->{'exitcode'};
	my $tstamp  = $rowref->{'start_time'};
	my $uid_idx = $rowref->{'uid_idx'};
	my $lastecode = 0;
	my $lastaction = "";
	$tstamp = $rowref->{'end_time'}
	    if (!$tstamp);

	$lastrsrc = $rsrcidx
	    if (!$lastrsrc);
	$lastidx = 0
	    if (!defined($lastidx));

	print "$idx $rsrcidx $lastidx $tstamp $action $ecode\n";

	# Not sure how this happens, but want to kill this record.
	if ($action eq "swapin" && $state eq "active" &&
	    $rsrcidx != $lastrsrc) {
	    next;
	}

	# Skip errors unless its a swapout. Odds are it really did swapout
	# and we can just ignore a subsequent swapout if there is one.
	if ($ecode) {
	    goto skip
		if ($action ne "swapout");
	}
	
	#
	# Figure out experiment state as we process the actions. If we
	# get into trouble, we bail.
	#
	if ($state eq "nostate") {
	    # Must start with a preload or a start, although some early
	    # records start with a swapin.
	    if ($action eq "preload") {
		$state = "swapped";
	    }
	    elsif ($action eq "start" || $action eq "swapin") {
		$state = "active";
		$swapins++;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "swapped") {
	    # Must be a swapmod or a swapin or a destroy
	    if ($action eq "swapin") {
		$state = "active";
		if ($rsrcidx == $lastrsrc) {
		    print "swapped/active: need to fix records for $exptidx\n";
		    $munge = 1;
		}
		$swapins++;
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		if ($rsrcidx == $lastrsrc && $lastidx) {
		    print "swapped/swapmod: ".
			"Need to fix records for $exptidx\n";
		    $munge = 1;
		}
	    }
	    elsif ($action eq "destroy") {
		$state = "done";
	    }
	    elsif ($action eq "swapout") {
		# No change; assume a swapout error with no ecode.
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "active") {
	    # Must be a swapmod or a swapout
	    if ($action eq "swapout") {
		if ($rsrcidx != $lastrsrc) {
		    print "swapout/active: Need to fix records for $exptidx\n";
		    $munge = 1;
		}
		$state = "swapped";
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		if ($rsrcidx == $lastrsrc) {
		    print "swapmod/active: Need to fix records for $exptidx\n";
		    $munge = 1;
		}
	    }
	    elsif ($action eq "swapin") {
		# This seems to happen a lot; ignore the record. 
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "done") {
	    print "Bad action ($action) in state $state for $exptidx\n";
	    $skip = 1;
	    last;
	}
      skip:
	$lastrsrc   = $rsrcidx;
	$lastecode  = $ecode;
	$lastaction = $action;
    }
    #
    # If we get here and the state is still active, something went wrong
    # in the testbed_stats gathering, and we need to fix things up by hand.
    #
    if ($state eq "active") {
	print "Experiment still active at end of stats: $exptidx\n";
	$munge = 1;
    }
    next
	if (0);

    next
	if ($skip);

    print "-----\n";

    $state = "nostate";
    $lastrsrc = 0;
    $swapins = 0;

    foreach my $rowref (@stats) {
	my $idx     = $rowref->{'idx'};
	my $copyidx = $rowref->{'rsrcidx'};
	my $rsrcidx = $copyidx;
	my $action  = $rowref->{'action'};
	my $ecode   = $rowref->{'exitcode'};
	my $uid_idx = $rowref->{'uid_idx'};
	my $tstamp  = $rowref->{'start_time'};
	$tstamp = $rowref->{'end_time'}
    	    if (!$tstamp);

	if (exists($mapping{$rsrcidx})) {
	    my $newrsrc = $mapping{$rsrcidx};
	    # testbed_stats gets the new pointer.
	    $testbed_stats{$idx} = $newrsrc;
	    $rowref->{'rsrcidx'} = $newrsrc;
	    $rsrcidx = $newrsrc;
	}
	my $lastidx = $resources{$rsrcidx}->{'lastidx'};
	$lastidx = 0
	    if (!defined($lastidx));
	
	$lastrsrc = $rsrcidx
	    if (!$lastrsrc);

	print "$idx $rsrcidx $lastidx $tstamp $action $ecode\n";

	# Not sure how this happens, but want to kill this record.
	if ($action eq "swapin" && $state eq "active" &&
	    $rsrcidx != $lastrsrc) {
	    next;
	}
	
	# Skip errors unless its a swapout. Odds are it really did swapout
	# and we can just ignore a subsequent swapout if there is one.
	goto skip
	    if ($ecode && $action ne "swapout");

	#
	# Figure out experiment state as we process the actions. If we
	# get into trouble, we bail.
	#
	if ($state eq "nostate") {
	    # Must start with a preload or a start, although some early
	    # records start with a swapin.
	    if ($action eq "preload") {
		$state = "swapped";
	    }
	    elsif ($action eq "start" || $action eq "swapin") {
		$state = "active";
		$swapins++;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "swapped") {
	    # Must be a swapmod or a swapin or a destroy
	    if ($action eq "swapin") {
		$state = "active";
		if ($rsrcidx == $lastrsrc) {
		    print "swapped/active: fixing $rsrcidx for $exptidx\n";
		    $rsrcidx = InsertRecord($exptidx, $copyidx,
					    $idx, $tstamp, $uid_idx);
		    $rowref->{'rsrcidx'} = $rsrcidx;
		}
		$swapins++;
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		if ($rsrcidx == $lastrsrc && $lastidx) {
		    print "swapped/swapmod: Fixing record for $exptidx\n";
		    $rsrcidx = InsertRecord($exptidx, $copyidx,
					    $idx, $tstamp, $uid_idx);
		    $rowref->{'rsrcidx'} = $rsrcidx;
		}
	    }
	    elsif ($action eq "destroy") {
		$state = "done";
	    }
	    elsif ($action eq "swapout") {
		# No change; assume a swapout error with no ecode.
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "active") {
	    # Must be a swapmod or a swapout
	    if ($action eq "swapout") {
		if ($rsrcidx != $lastrsrc) {
		    print "swapout/active: Backing up one for $exptidx\n";
		    next;
		}
		$state = "swapped";
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		if ($rsrcidx == $lastrsrc) {
		    print "swapmod/active: Need to fix records for $exptidx\n";
		    $rsrcidx = InsertRecord($exptidx, $copyidx,
					    $idx, $tstamp, $uid_idx);
		    $rowref->{'rsrcidx'} = $rsrcidx;
		}
	    }
	    elsif ($action eq "swapin") {
		# This seems to happen a lot; ignore the record. 
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
		$skip = 1;
		last;
	    }
	}
	elsif ($state eq "done") {
	    print "Bad action ($action) in state $state for $exptidx\n";
	    $skip = 1;
	    last;
	}
      skip:
	$lastrsrc = $rsrcidx;
    }

    # Kill off ones we did not use.
    foreach my $idx (keys(%resources)) {
	my $rowref   = $resources{$idx};
	my $save     = $rowref->{'save'};
	my $unixtime = $rowref->{'unixtime'};

	if (!$save) {
	    print "D: $idx\n";
	    DBQueryFatal("delete from experiment_resources_new ".
			 "where idx=$idx");
	    delete($bytimestamp{$unixtime});
	    delete($resources{$idx});
	}
    }

    # Reorder the records.
    my @order = sort {$a <=> $b} keys(%bytimestamp);
    my $this  = pop(@order);
    while (@order) {
	my $next = pop(@order);
	my $thisidx = $bytimestamp{$this}->{'idx'};
	my $nextidx = $bytimestamp{$next}->{'idx'};
	my $lastidx = $bytimestamp{$this}->{'lastidx'};
	print "$this $next $thisidx $nextidx\n";
	if (!defined($lastidx) || $lastidx != $bytimestamp{$next}->{'idx'}) {
	    $bytimestamp{$this}->{'lastidx'} = $bytimestamp{$next}->{'idx'};
	}
	$this = $next;
    }

    print "-----\n";

    $state = "nostate";
    $lastrsrc = 0;
    $swapins = 0;

    foreach my $rowref (@stats) {
	my $idx     = $rowref->{'idx'};
	my $rsrcidx = $rowref->{'rsrcidx'};
	my $action  = $rowref->{'action'};
	my $ecode   = $rowref->{'exitcode'};
	my $uid_idx = $rowref->{'uid_idx'};
	my $lastidx = $resources{$rsrcidx}->{'lastidx'};
	my $tstamp  = $rowref->{'start_time'};
	
	$tstamp = $rowref->{'end_time'}
    	    if (!$tstamp);
	$lastidx = 0
	    if (!defined($lastidx));
	
	$lastrsrc = $rsrcidx
	    if (!$lastrsrc);

	print "$idx $rsrcidx $lastidx $action $ecode\n";

	# Not sure how this happens, but want to kill this record.
	if ($action eq "swapin" && $state eq "active" &&
	    $rsrcidx != $lastrsrc) {
	    next;
	}

	# Skip errors unless its a swapout. Odds are it really did swapout
	# and we can just ignore a subsequent swapout if there is one.
	goto skip
	    if ($ecode && $action ne "swapout");

	#
	# Figure out experiment state as we process the actions. If we
	# get into trouble, we bail.
	#
	if ($state eq "nostate") {
	    # Must start with a preload or a start, although some early
	    # records start with a swapin.
	    if ($action eq "preload") {
		UpdateResourceRecord($exptidx, $rsrcidx, $action,
				     $tstamp, $uid_idx);
		$state = "swapped";
	    }
	    elsif ($action eq "start" || $action eq "swapin") {
		$state = "active";
		UpdateResourceRecord($exptidx, $rsrcidx, $action,
				     $tstamp, $uid_idx, 0,
				     ($action eq "swapin" ? 1 : 0));
		$swapins++;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
	    }
	}
	elsif ($state eq "swapped") {
	    # Must be a swapmod or a swapin or a destroy
	    if ($action eq "swapin") {
		$state = "active";
		UpdateResourceRecord($exptidx, $rsrcidx, $action,
				     $tstamp, $uid_idx, 0, 1);
		$swapins++;
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		UpdateResourceRecord($exptidx, $lastrsrc, $action,
				     $tstamp, $uid_idx);
		UpdateResourceRecord($exptidx, $rsrcidx,
				     "noaction", $tstamp, $uid_idx, 1, 0);
		ClearResourceRecord($rsrcidx);
	    }
	    elsif ($action eq "destroy") {
		$state = "done";
	    }
	    elsif ($action eq "swapout") {
		# No change; assume a swapout error with no ecode.
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
	    }
	}
	elsif ($state eq "active") {
	    # Must be a swapmod or a swapout
	    if ($action eq "swapout") {
		my $change = ($rsrcidx != $lastrsrc ? $lastrsrc : $rsrcidx);
		
		UpdateResourceRecord($exptidx, $change, $action,
				     $tstamp, $uid_idx);
		$state = "swapped";
	    }
	    elsif ($action eq "swapmod") {
		# No change to state.
		# Previous record gets swapmod timestamp
		UpdateResourceRecord($exptidx, $lastidx, $action, $tstamp);
		# Current record gets swapin timestamp
		UpdateResourceRecord($exptidx, $rsrcidx, "swapin",
				     $tstamp, $uid_idx, 1, 0);
	    }
	    elsif ($action eq "swapin") {
		# This seems to happen a lot; ignore the record. 
		;
	    }
	    else {
		print "Bad action ($action) in state $state for $exptidx\n";
	    }
	}
	elsif ($state eq "done") {
	    print "Bad action ($action) in state $state for $exptidx\n";
	}
      skip:
	$lastrsrc = $rsrcidx;
	$laststamp = $tstamp;
    }
    if ($state eq "active" &&
	(!defined($expstate) || $expstate eq "swapped" ||
	 ($expstate eq "swapping" && (time() - $laststamp) > (3600 * 24)))) {
	print "Fixing still active at end of stats: $exptidx\n";

	UpdateResourceRecord($exptidx, $lastrsrc, "swapout", $laststamp + 1);
    }
    
    foreach my $idx (keys(%resources)) {
	my $rowref = $resources{$idx};
	my $lastidx = $rowref->{'lastidx'};
	my $swapin_time = $rowref->{'swapin_time'};
	my $swapout_time = $rowref->{'swapout_time'};
	my $tstamp = $rowref->{'tstamp'};
	$lastidx = 0
	    if (! defined($lastidx));

	next
	    if (!defined($tstamp));

	print "$idx, $lastidx, $tstamp, $swapin_time, $swapout_time\n";

	# We added this above.
	delete($rowref->{'unixtime'});
	delete($rowref->{'save'});

	my $query = "replace into experiment_resources_new set ";
	my @keys  = keys(%$rowref);

	while (@keys) {
	    my $key = pop(@keys);
	    my $val = $rowref->{$key};
		
	    if ($key eq "thumbnail") {
		$query .= "thumbnail=" . DBQuoteSpecial($val);
	    }
	    elsif (!defined($val)) {
		$query .= "$key=NULL";
	    }
	    else {
		$query .= "$key='$val'";
	    }
	    $query .= ","
		if (@keys);
	}
	DBQueryFatal($query)
	    if (!$impotent);
    }

    foreach my $idx (keys(%testbed_stats)) {
	my $rsrcidx = $testbed_stats{$idx};

	print "Update testbed_stats:$idx to $rsrcidx\n";

	if (! $impotent) {
	    DBQueryFatal("update testbed_stats_new set rsrcidx=$rsrcidx ".
			 "where idx=$idx");
	}
    }
    if (exists($mapping{$expt_rsrcidx})) {
	my $rsrcidx = $mapping{$expt_rsrcidx};
	
	print "Update experiment_stats:$expt_rsrcidx to $rsrcidx\n";

	if (! $impotent) {
	    DBQueryFatal("update experiment_stats_new set rsrcidx=$rsrcidx ".
			 "where exptidx=$exptidx");
	}
    }
    if (defined($expt_lastidx) && exists($mapping{$expt_lastidx})) {
	my $rsrcidx = $mapping{$expt_lastidx};
	
	print "Update experiment_stats:$expt_lastidx to $rsrcidx\n";
	
	if (! $impotent) {
	    DBQueryFatal("update experiment_stats_new set lastrsrc=$rsrcidx ".
			 "where exptidx=$exptidx");
	}
    }
}

# Last bit of cleanup for records that are too hosed to worry about.
$query_result =
    DBQueryFatal("select e.idx,r.idx from experiment_resources_new as r ".
		 " left join experiments as e on e.idx=r.exptidx ".
		 "where e.state='swapped' and swapin_time!=0 and ".
		 "      swapout_time=0 and swapmod_time=0 and pnodes>0");
while (my ($exptidx,$rsrcidx) = $query_result->fetchrow_array()) {
    print "Cleaning up $exptidx,$rsrcidx\n";
    DBQueryFatal("update experiment_resources_new set ".
		 "  swapout_time=swapout_time+1 ".
		 "where idx=$rsrcidx");
}

if (1) {
#   DBQueryFatal("unlock tables");
    DBQueryFatal("rename table ".
		 "testbed_stats to testbed_stats_backup, ".
		 "experiment_stats to experiment_stats_backup, ".
		 "experiment_resources to experiment_resources_backup, ".
		 "testbed_stats_new to testbed_stats, ".
		 "experiment_stats_new to experiment_stats, ".
		 "experiment_resources_new to experiment_resources");
}
