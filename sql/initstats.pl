#!/usr/bin/perl -w
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
    DBQueryFatal("select uid from users");

while (($uid) = $query_result->fetchrow_array()) {
    print "insert into user_stats (uid) values ('$uid');\n";
}

$query_result =
    DBQueryFatal("select eid,pid,expt_head_uid,gid,expt_created,batchmode,idx ".
		 "from experiments order by expt_swapped");

while (($eid,$pid,$creator,$gid,$created,$batchmode,$exptidx) =
       $query_result->fetchrow_array()) {

    print "insert into experiment_resources (idx, tstamp, exptidx) ".
	"values (0, '$created', $exptidx);\n";
    
    print "insert into experiment_stats ".
	"(eid, pid, creator, gid, created, batch, exptidx, rsrcidx) ".
        "select '$eid', '$pid', '$creator', '$gid', '$created', $batchmode,".
	"$exptidx,r.idx from experiments as e ".
	"left join experiment_resources as r on e.idx=r.exptidx ".
	"where pid='$pid' and eid='$eid';\n";
}
