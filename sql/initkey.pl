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
    DBQueryFatal("select pid,eid from experiments");

while (($pid,$eid) = $query_result->fetchrow_array()) {
    my $secretkey = TBGenSecretKey();

    print "update experiments set eventkey='$secretkey' ".
	"where pid='$pid' and eid='$eid';\n";
}
