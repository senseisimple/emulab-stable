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

my $ADDPROJADMINLIST = "/usr/testbed/sbin/addprojadminlist";

#
# Turn off line buffering on output
#
$| = 1;

my $gentopofile = "/usr/testbed/libexec/gentopofile";

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$query_result =
    DBQueryFatal("select pid from projects");

while (($pid) = $query_result->fetchrow_array()) {
    print "Creating $pid-admin mailing list\n";
    system("$ADDPROJADMINLIST $pid") == 0 or
	    die("$ADDPROJADMINLIST $pid failed!");
}
