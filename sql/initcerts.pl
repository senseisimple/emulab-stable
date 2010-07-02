#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin:/usr/testbed/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$query_result =
    DBQueryFatal("select u.uid from users as u ".
		 "left join user_stats as s on s.uid_idx=u.uid_idx ".
		 "where u.status='active' and u.webonly=0 ".
		 "order by s.weblogin_last desc");

# Avoid blizzard of audit email.
$ENV{'TBAUDITON'} = 1;

while (($uid) = $query_result->fetchrow_array()) {
    print "Generating new emulab cert for $uid\n";
    system("withadminprivs mkusercert $uid >/dev/null") == 0
	or warn("Failed to create SSL cert for user $uid\n");
}
