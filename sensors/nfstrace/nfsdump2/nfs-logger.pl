#!/usr/local/bin/perl
#
# $Id: nfs-logger.pl,v 1.1 2005-11-28 15:44:00 stack Exp $
#
# Dan Ellard
#
# Manager script for the nfsdump program to monitor an NFS server. 
# This is intended to be started by a cron job, but it can also be run
# by hand (for shorter data collections).  The LifeTime period should
# slop over the cron period a little bit to allow a grace period in
# case the cron job is slow in starting.
#
# The first argument and only argument to this script is the name of
# the template that specifies all the parameters used by this script. 
# An example template is given in nfs-logger.tmplt.

die "Parameter file not specified."	unless (@ARGV != 2);

die "Cannot find param file $ARGV[0]"	unless (-f $ARGV[0]);

require $ARGV[0];

die "Missing ProgDir param"		unless defined $ProgDir;
die "Missing DataDir param"		unless defined $DataDir;
die "Missing LogName param"		unless defined $LogName;

die "Missing LifeTime param"		unless defined $LifeTime;
die "Missing LogInterval param"		unless defined $LogInterval;
die "Missing SnifferProgName param"	unless defined $SnifferProgName;
die "Missing SnifferFilter param"	unless defined $SnifferFilter;

$NumIntervals	= int ($LifeTime / $LogInterval);

print	"LogInterval  = $LogInterval\n";
print	"LifeTime     = $LifeTime (= $NumIntervals intervals)\n";
print	"LogName      = $LogName\n";
print	"filter       = $SnifferFilter\n";

if (! -d "$DataDir" ) {
	`mkdir -p $DataDir`;

	die "Cannot create $DataDir."	unless (-d "$DataDir");
}

die "Cannot chdir to $DataDir."		unless (chdir "$DataDir");
die "Cannot execute $SnifferProgName."	unless (-x "$SnifferProgName");

$cmd  = "$SnifferProgName $SnifferProgArgs ";
$cmd .= "-s 400 ";
$cmd .= "-N $NumIntervals ";
$cmd .= "-I $LogInterval ";
$cmd .= "-B $LogName ";
$cmd .= "-T TEMP-LOG.$$ ";
$cmd .= "$SnifferFilter ";

print "Running ($cmd)\n";

`$cmd`;

exit (0);

# end of nfs-logger.pl

