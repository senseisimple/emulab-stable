#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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

$query_result =
    DBQueryFatal("select pid,gid from groups");

while (($pid,$gid) = $query_result->fetchrow_array()) {
    if ($pid eq $gid) {
	print "insert into project_stats (pid) values ('$pid');\n";
    }
    print "insert into group_stats (pid,gid) values ('$pid', '$gid');\n";
}

$query_result =
    DBQueryFatal("select u.uid,time from users as u ".
		 "left join lastlogin as l on u.uid=l.uid");

while (($uid,$stamp) = $query_result->fetchrow_array()) {
    my $count;
    if (defined($stamp)) {
	$stamp = "'$stamp'";
	$count = 1;
    }
    else {
	$stamp = "NULL";
	$count = 0;
    }

    print "insert into user_stats (uid, weblogin_last, weblogin_count) ".
	"values ('$uid', $stamp, $count);\n";
}

$query_result =
    DBQueryFatal("select state,eid,pid,expt_head_uid,idx,gid,expt_created, ".
		 "       expt_swapped ".
		 "from experiments order by expt_swapped");

while (($state,$eid,$pid,$creator,$idx,$gid,$created,$swapped) =
       $query_result->fetchrow_array()) {
    print "insert into experiment_stats ".
	"(eid, pid, creator, idx, gid, created) ".
        "VALUES ('$eid', '$pid', '$creator', $idx, '$gid', '$created');\n";

    if ($state eq EXPTSTATE_NEW ||
	($state eq EXPTSTATE_SWAPPED && !defined($swapped))) {
	print("update group_stats set ".
	      "  exptpreload_count=exptpreload_count+1, ".
	      "  exptpreload_last='$created' ".
	      "where pid='$pid' and gid='$gid';\n");
	print("update project_stats set ".
	      "  exptpreload_count=exptpreload_count+1, ".
	      "  exptpreload_last='$created' ".
	      "where pid='$pid';\n");
	print("update user_stats set ".
	      "  exptpreload_count=exptpreload_count+1, ".
	      "  exptpreload_last='$created' ".
	      "where uid='$creator';\n");
    }
    if ($state eq EXPTSTATE_ACTIVE && defined($swapped)) {
	my $pnodes = 0;
	my $pnodes_result =
	    DBQueryWarn("select r.node_id from reserved as r ".
			"left join nodes as n on r.node_id=n.node_id ".
			"where r.pid='$pid' and r.eid='$eid' and ".
			"      n.role='testnode'");
	$pnodes = $pnodes_result->numrows
	    if ($pnodes_result);
	
	print("update experiment_stats set ".
	      "  swapin_count=swapin_count+1, ".
	      "  swapin_last='$swapped', ".
	      "  pnodes=$pnodes ".
	      "where pid='$pid' and eid='$eid' and idx=$idx;\n");
	print("update group_stats set ".
	      "  exptswapin_count=exptswapin_count+1, ".
	      "  exptswapin_last='$swapped' ".
	      "where pid='$pid' and gid='$gid';\n");
	print("update project_stats set ".
	      "  exptswapin_count=exptswapin_count+1, ".
	      "  exptswapin_last='$swapped' ".
	      "where pid='$pid';\n");
	print("update user_stats set ".
	      "  exptswapin_count=exptswapin_count+1, ".
	      "  exptswapin_last='$swapped' ".
	      "where uid='$creator';\n");
    }
    if ($state eq EXPTSTATE_SWAPPED && defined($swapped)) {
	print("update experiment_stats set ".
	      "  swapout_count=swapout_count+1, ".
	      "  swapout_last='$swapped' ".
	      "where pid='$pid' and eid='$eid' and idx=$idx;\n");
	print("update group_stats set ".
	      "  exptswapout_count=exptswapout_count+1, ".
	      "  exptswapout_last='$swapped' ".
	      "where pid='$pid' and gid='$gid';\n");
	print("update project_stats set ".
	      "  exptswapout_count=exptswapout_count+1, ".
	      "  exptswapout_last='$swapped' ".
	      "where pid='$pid';\n");
	print("update user_stats set ".
	      "  exptswapout_count=exptswapout_count+1, ".
	      "  exptswapout_last='$swapped' ".
	      "where uid='$creator';\n");
    }
}


