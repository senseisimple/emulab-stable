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
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$query_result =
    DBQueryFatal("select distinct ex.pid,ex.eid,vname from eventlist as ex ".
		 "left join event_eventtypes as et on ex.eventtype=et.idx ".
		 "left join event_objecttypes as ot on ex.objecttype=ot.idx ".
		 "where ot.type='LINK'");

while (($pid,$eid,$lan) = $query_result->fetchrow_array()) {
    DBQueryFatal("update virt_lans set mustdelay=1 ".
		 "where pid='$pid' and eid='$eid' and vname='$lan'");
}
