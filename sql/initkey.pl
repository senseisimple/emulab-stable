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
    DBQueryFatal("select pid,eid,gid,expt_head_uid from experiments ".
		 "where eventkey is null");

while (($pid,$eid,$gid,$creator) = $query_result->fetchrow_array()) {
    my $eventkey = TBGenSecretKey();

    DBQueryFatal("update experiments set eventkey='$eventkey' ".
		 "where pid='$pid' and eid='$eid'");

    my $keyfile = TBDB_EVENTKEY($pid, $eid);

    if (!open(KEY, ">$keyfile")) {
	warn("Could not create $keyfile: $!\n");
	next;
    }
    print KEY $eventkey;
    close(KEY);
    system("chown $creator $keyfile");
}
