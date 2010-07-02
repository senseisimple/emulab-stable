#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

# We want to change the group_membership table later, so save the ids.
my %uids = ();
my %gids = ();

# Nice place to start.
my $next_gid = 10000;

# Locals
my $TBOPSPID = TBOPSPID();

#
# Generate new unique indexes for all existing users. 
#
my $query_result =
    DBQueryFatal("select uid,unix_uid from users ".
		 "order by usr_created");

while (my ($uid,$unix_uid) = $query_result->fetchrow_array()) {
    #print "$uid -> $unix_uid\n";
    
    DBQueryFatal("update users set uid_idx=$unix_uid ".
		 "where uid='$uid'");
    
    # These were left out in initial testbed setup.
    if ($uid eq "elabckup" || $uid eq "elabman" || $uid eq "operator") {
	DBQueryFatal("replace into user_stats set ".
		     " uid='$uid',uid_idx=$unix_uid");
    }

    $uids{$uid} = $unix_uid;
}
DBQuery("alter table users drop primary key");
DBQueryFatal("alter table users add PRIMARY KEY (uid_idx)");

DBQuery("alter table projects drop PRIMARY KEY");
DBQuery("alter table project_stats drop PRIMARY KEY");
DBQuery("alter table groups drop PRIMARY KEY");
DBQuery("alter table group_stats drop PRIMARY KEY");
DBQuery("alter table group_membership drop PRIMARY KEY");

# These were left out in initial testbed setup.
DBQueryFatal("replace into group_stats set ".
	     " pid='$TBOPSPID',gid='$TBOPSPID',gid_idx=0");
DBQueryFatal("replace into project_stats set ".
	     " pid='$TBOPSPID',pid_idx=0");

#
# Get the current list of pid/gid pairs. Generate new indexes for
# all pids and gids.
#
$query_result =
    DBQueryFatal("select pid from projects order by pid");

while (my ($pid) = $query_result->fetchrow_array()) {
    DBQueryFatal("update projects set pid_idx=$next_gid ".
		 "where pid='$pid'");
    DBQueryFatal("update project_stats set pid_idx=$next_gid ".
		 "where pid='$pid'");
    
    $gids{"$pid:$pid"} = $next_gid;
    $next_gid++;
}

$query_result =
    DBQueryFatal("select pid,gid from groups order by pid,gid");

while (my ($pid,$gid) = $query_result->fetchrow_array()) {
    my $pid_idx = $gids{"$pid:$pid"};
    my $gid_idx;

    if ($pid eq $gid) {
	$gid_idx = $pid_idx;
    }
    else {
	$gids{"$pid:$gid"} = $gid_idx = $next_gid;
	$next_gid++;
    }
    
    DBQueryFatal("update groups set gid_idx=$gid_idx,pid_idx=$pid_idx ".
		 "where pid='$pid' and gid='$gid'");
    DBQueryFatal("update group_stats set gid_idx=$gid_idx ".
		 "where pid='$pid' and gid='$gid'");
}
DBQueryFatal("replace into emulab_indicies set ".
	     " name='next_gid', idx='$next_gid'");

#
# Now patch up the group_membership table.
#
$query_result =
    DBQueryFatal("select uid,pid,gid from group_membership");

while (my ($uid, $pid, $gid) = $query_result->fetchrow_array()) {
    my $uid_idx = $uids{$uid};
    my $pid_idx = $gids{"$pid:$pid"};
    my $gid_idx = $gids{"$pid:$gid"};

    if (!defined($uid_idx)) {
	print "*** WARNING: No idx for $uid\n";
	next;
    }
    if (!defined($pid_idx)) {
	die("No idx for $pid:$pid\n");
    }
    if (!defined($gid_idx)) {
	die("No idx for $pid:$gid\n");
    }

    DBQueryFatal("update group_membership set ".
		 "  uid_idx=$uid_idx, pid_idx=$pid_idx, gid_idx=$gid_idx ".
		 "where uid='$uid' and pid='$pid' and gid='$gid'");
}

DBQueryFatal("alter table projects add PRIMARY KEY (pid_idx)");
DBQueryFatal("alter table project_stats add PRIMARY KEY (pid_idx)");
DBQueryFatal("alter table groups add PRIMARY KEY (gid_idx)");
DBQueryFatal("alter table group_stats add PRIMARY KEY (gid_idx)");
DBQueryFatal("alter table group_membership add PRIMARY KEY (uid_idx,gid_idx)");
