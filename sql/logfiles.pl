#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
use English;

use lib "/usr/testbed/lib";
use libdb;
use libtestbed;
use Experiment;
use Logfile;

#
# Untaint the path
# 
$ENV{'PATH'} = '/bin:/usr/bin:/usr/sbin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

my $query_result =
    DBQueryFatal("select idx,logfile,gid_idx from experiments ".
		 "where logfile!='' and logfile is not null");

while (my ($idx, $logname, $gid_idx) = $query_result->fetchrow_array()) {
    next
	if (! ($logname =~ /^\//));
    
    my $experiment = Experiment->Lookup($idx);
    if (!defined($experiment)) {
	print "Could not lookup experiment object for $idx\n";
	next;
    }
    my $logfile = Logfile->Create($gid_idx, $logname);
    $experiment->SetLogFile($logfile);
}

