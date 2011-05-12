#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2011 University of Utah and the Flux Group.
# All rights reserved.
#

use English;
use Errno;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

DBQueryFatal("lock tables reserved write, nodes write");

my $query_result =
    DBQueryFatal("select nodes.node_id from nodes ".
		 "left join reserved on nodes.node_id=reserved.node_id ".
		 "where reserved.node_id is null and ".
		 "      (nodes.type='pcvm' or nodes.type='pcplab')");

# Need to do this when we want to seek around inside the results.
$query_result = $query_result->WrapForSeek();

while (my ($vnodeid) = $query_result->fetchrow_array()) {
    DBQueryWarn("delete from reserved where node_id='$vnodeid'");
    DBQueryWarn("delete from nodes where node_id='$vnodeid'");
}

DBQueryFatal("unlock tables");

$query_result->dataseek(0);

while (my ($vnodeid) = $query_result->fetchrow_array()) {
    DBQueryWarn("delete from node_hostkeys where node_id='$vnodeid'");
    DBQueryWarn("delete from node_status where node_id='$vnodeid'");
    DBQueryWarn("delete from node_rusage where node_id='$vnodeid'");
}
