#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2006 University of Utah and the Flux Group.
# All rights reserved.
#

use English;
use Errno;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;

my $allexp_result =
    DBQueryFatal("select pid,eid from experiments");

while (my ($pid,$eid) = $allexp_result->fetchrow_array) {
    my %ips = ();
    
    my $query_result =
	DBQueryFatal("select vname,ips from virt_nodes ".
		     "where pid='$pid' and eid='$eid'");
		     
    while (my ($vnode,$ips) = $query_result->fetchrow_array) {
	# Take apart the IP list.
	foreach $ipinfo (split(" ", $ips)) {
	    my ($port,$ip) = split(":",$ipinfo);

	    $ips{"$vnode:$port"} = $ip;
	}
    }

    $query_result =
	DBQueryFatal("select vname,member from virt_lans ".
		     "where pid='$pid' and eid='$eid'");
		     
    while (my ($vname,$member) = $query_result->fetchrow_array) {
	my ($vnode,$port) = split(":", $member);
	my $ip = $ips{$member};

	DBQueryFatal("update virt_lans set ".
		     "vnode='$vnode',ip='$ip',vport='$port' ".
		     "where pid='$pid' and eid='$eid' and ".
		     "vname='$vname' and member='$member'");
    }
}



