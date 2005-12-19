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
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$query_result =
    DBQueryFatal("select uid,unix_uid from users");

while (($uid,$unix_uid) = $query_result->fetchrow_array()) {
    DBQueryFatal("update user_stats set uid_idx='$unix_uid' ".
		 "where uid='$uid'");
}

