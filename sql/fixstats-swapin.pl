#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
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

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

my $query_result =
    DBQueryFatal("select t.*,s.* from testbed_stats as t ".
		 "left join experiment_stats as s on s.exptidx=t.exptidx ".
		 "where start_time=end_time and action='swapin' and ".
		 "      exitcode=0 ");

while (my $row = $query_result->fetchrow_hashref()) {
    my $uid     = $row->{'uid'};
    my $idx     = $row->{'idx'};
    my $pid     = $row->{'pid'};
    my $eid     = $row->{'eid'};
    my $gid     = $row->{'gid'};
    my $exptidx = $row->{'exptidx'};
    my $rsrcidx = $row->{'rsrcidx'};

#    print "$uid $pid $eid $gid $idx $exptidx\n";

    print("update project_stats set exptswapin_count=exptswapin_count-1 ".
	  "where pid='$pid';\n");
    print("update group_stats set exptswapin_count=exptswapin_count-1 ".
	  "where pid='$pid' and gid='$gid';\n");
    print("update user_stats set exptswapin_count=exptswapin_count-1 ".
	  "where uid='$uid';\n");
    print("update projects set expt_count=expt_count-1 ".
	  "where pid='$pid';\n");
    print("update groups set expt_count=expt_count-1 ".
	  "where pid='$pid' and gid='$gid';\n");
    print("update experiment_stats set swapin_count=swapin_count-1 ".
	  "where pid='$pid' and eid='$eid' and exptidx=$exptidx;\n");
    print("delete from testbed_stats where idx='$idx';\n");
}

