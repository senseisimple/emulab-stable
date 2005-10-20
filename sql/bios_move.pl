#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;

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
    DBQueryFatal("select node_id,bios_version from nodes ".
                 "order by node_id");

while (my @row = $query_result->fetchrow_array()) {

    if ($row[1]) {
        print "REPLACE INTO node_attributes VALUES ". 
            "('$row[0]', 'bios_version', '$row[1]');\n";
    }
}
