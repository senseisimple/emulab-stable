#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
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
    DBQueryFatal("select u.uid,s.encrypted from users as u ".
		 "left join user_sslcerts as s on u.uid=s.uid ".
		 "where u.status='active' and u.webonly=0 and ".
		 "s.encrypted is null");

# Avoid blizzard of audit email.
$ENV{'TBAUDITON'} = 1;

while (($uid) = $query_result->fetchrow_array()) {
    system("/usr/local/bin/sudo -u $uid /usr/testbed/sbin/mkusercert $uid") == 0
	or die("Failed to create SSL cert for user $uid\n");
}
