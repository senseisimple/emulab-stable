#!/usr/bin/perl

########################
# TODO:
# 1) provide a coherency protocol to the site/node id cache
# 2) buffer updates to 1 per 5 seconds (or so)
#######################


#
# Configure variables
#

use lib '/usr/testbed/lib';
use libtbdb;
#require Mysql;
#use libpelabdb;
use event;
use Getopt::Std;
use strict;

# node and site id caches
my %nodeids;
my %siteids;

sub usage {
	warn "Usage: $0 ";
	return 1;
}

my %opt = ();
getopt(\%opt,"s:p:h");

#if ($opt{h}) { exit &usage; }
if (@ARGV !=0) { exit &usage; }

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
##TODO: CHANGE TO "pelab" and "pelab"
my $DBNAME = "pelab";
my $DBUSER = "pelab";
my $DBErrorString = "";
#TODO: change this back to file
#my $DBPWD = "";
my $DBPWD = `cat $PWDFILE`;
if( $DBPWD =~ /^([\w]*)\s([\w]*)$/ ) {
    $DBPWD = $1;
}else{
    fatal("Bad chars in password!");
}
#print "PWD: $DBPWD\n";

#connect to database
my ($DB_data, $DB_sitemap);
TBDBConnect($DBNAME,$DBUSER,$DBPWD);
#DBConnect(\$DB_data, $DBNAME,$DBUSER,$DBPWD);
#DBConnect(\$DB_sitemap, $DBNAME,$DBUSER,$DBPWD);

my ($server,$port);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }


#watch for notifications to ops
%$tuple = ( host      => $event::ADDRESSTUPLE_ALL,
	    objtype   => "BGMON",
	    objname   => "ops"
	    , expt      => "__none"
	    , scheduler => 1
	    );

if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	die "Could not subscribe to event\n";
}


#############################################################################
# Note a difference from tbrecv.c - we don't yet have event_main() functional
# in perl, so we have to poll. (Nothing special about the select, it's just
# a wacky way to get usleep() )
#############################################################################
#main()

while (1) {

    #check for pending received events
#    event_poll($handle);
    event_poll_blocking($handle, 100);


    #sleep for a time
#    select(undef, undef, undef, 0.05);
}

#############################################################################

if (event_unregister($handle) == 0) {
    die "Unable to unregister with event system\n";
}

exit(0);


sub callbackFunc($$$) {
	my ($handle,$notification,$data) = @_;

	my $time      = time();
	my $site      = event_notification_get_site($handle, $notification);
	my $expt      = event_notification_get_expt($handle, $notification);
	my $group     = event_notification_get_group($handle, $notification);
	my $host      = event_notification_get_host($handle, $notification);
	my $objtype   = event_notification_get_objtype($handle, $notification);
	my $objname   = event_notification_get_objname($handle, $notification);
	my $eventtype = event_notification_get_eventtype($handle,
							 $notification);
#	print "Event: $time $site $expt $group $host $objtype $objname " .
#		"$eventtype\n";

	print "EVENT: $time $objtype $eventtype\n";

        my $linksrc = event_notification_get_string($handle,
						    $notification,
						    "linksrc");
        my $linkdest = event_notification_get_string($handle,
						     $notification,
						     "linkdest");
	my $testtype = event_notification_get_string($handle,
						     $notification,
						     "testtype");
	my $result = event_notification_get_string($handle,
						     $notification,
						     "result");
	my $tstamp = event_notification_get_string($handle,
						   $notification,
						   "tstamp");
#	my $scheduler = event_notification_get_string($handle,
#						      $notification,
#						      "SCHEDULER");

	#change values and/or initialize
	if( $eventtype eq "RESULT" ){
	    print "***GOT RESULT***\n";
	}

				      
	print(
	      "linksrc=$linksrc\n".
	      "linkdest=$linkdest\n".
	      "testtype =$testtype\n".
	      "result=$result\n".
	      "tstamp=$tstamp\n"
#	      ."SCHEDULER=$scheduler\n"
	      );


	saveTestToDB(linksrc   => $linksrc,
		     linkdest  => $linkdest,
		     testtype  => $testtype,
		     result    => $result,
		     tstamp    => $tstamp );


#	if (event_unregister($handle) == 0) {
#	    die "Unable to unregister with event system\n";
#	}
#	exit(0);
}

#############################################################################

sub saveTestToDB()
{
    my %results = @_;

    my @tmp;
    my ($srcsite, $srcnode, $dstsite, $dstnode);

    if( !exists $siteids{$results{linksrc}} ){
	@tmp = DBQuery("SELECT site_idx FROM site_mapping ".
		       "WHERE node_id='$results{linksrc}'") ->fetch;
	$srcsite = $tmp[0][0];
	$siteids{$results{linksrc}} = $srcsite;
    }else{
	$srcsite = $siteids{$results{linksrc}};
	print "++++++++++++++++++++++++++++hash hit: site=$srcsite\n";
    }
    if( !exists $nodeids{$results{linksrc}} ){       
	@tmp = DBQuery("SELECT node_idx FROM site_mapping ".
		       "WHERE node_id='$results{linksrc}'") ->fetch;
	$srcnode = $tmp[0][0];
	$nodeids{$results{linksrc}} = $srcnode;
    }else{
	$srcnode = $nodeids{$results{linksrc}};
    }
    if( !exists $siteids{$results{linkdest}} ){
	@tmp = DBQuery("SELECT site_idx FROM site_mapping ".
		       "WHERE node_id='$results{linkdest}'") ->fetch;
	$dstsite = $tmp[0][0];
	$siteids{$results{linkdest}} = $dstsite;
    }else{
	$dstsite = $siteids{$results{linkdest}};
    }
    if( !exists $nodeids{$results{linkdest}} ){
	@tmp = DBQuery("SELECT node_idx FROM site_mapping ".
		       "WHERE node_id='$results{linkdest}'") ->fetch;
	$dstnode = $tmp[0][0];
	$nodeids{$results{linkdest}} = $dstnode;
    }else{
	$dstnode = $nodeids{$results{linkdest}};
    }

    if( $srcsite eq "" || $srcnode eq "" || $dstsite eq "" || $dstnode eq "" ){
	warn "No results matching node id's $results{linksrc} and/or ".
	    "$results{linkdest}. Results:\n";
	warn "srcsite=$srcsite\n";
	warn "srcnode=$srcnode\n";
	warn "dstsite=$dstsite\n";
	warn "dstnode=$dstnode\n";
	#DBWarn("DBWARN:");
    }

    my $query = 
	"INSERT INTO pair_data (srcsite_idx, srcnode_idx, ".
	"dstsite_idx, dstnode_idx, unixstamp, $results{testtype}) ".
	"VALUES($srcsite, $srcnode, $dstsite, $dstnode, ".
        "$results{tstamp}, $results{result})";

    my $query_res = DBQuery($query);
#    my $query_res = DBQuery_data($query);
    print "QUERY: $query\n";
#    my $query_res = DBQueryFatal("describe pair_data");
    
}

