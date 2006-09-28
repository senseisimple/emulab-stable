#!/usr/bin/perl

#
# Configure variables
#
use lib '/usr/testbed/lib';

use event;
use Getopt::Std;
use strict;
use libwanetmon;

sub usage {
    warn "Usage: $0 [-i managerID] [-p period] [-d duration]".
	"<srcnode> <dstnode> <testtype> <COMMAND>\n";
    return 1;
}

my %opt = ();
getopt("i:p:d:h",\%opt);
if ($opt{h}) { exit &usage; }
my ($managerid, $period, $duration, $type, $srcnode, $dstnode, $cmd);
my ($server);
$server = "localhost";
if($opt{i}){ $managerid=$opt{i}; } else{ $managerid = "testsend"; }
if($opt{p}){ $period = $opt{p}; } else{ $period = 0; }
if($opt{d}){ $duration = $opt{d}; } else{ $duration = 0; }
my $URL = "elvin://$server";
#if ($port) { $URL .= ":$port"; }

if (@ARGV != 4) { exit &usage; }
($srcnode, $dstnode, $type, $cmd) = @ARGV;

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }


%$tuple = ( objtype   => "BGMON",
	    objname   => "managerclient",
	    eventtype => $cmd,
	    expt      => "__none"
	    , scheduler => 0
);


my %cmd = ( managerID => $managerid,
	    srcnode => $srcnode,
	    dstnode => $dstnode,
	    testtype => $type,
	    testper => "$period",
	    duration => "$duration"
	    );

sendcmd_evsys($cmd, \%cmd, $handle);

=pod
my $notification = event_notification_alloc($handle,$tuple);
if (!$notification) { die "Could not allocate notification\n"; }
print "Sent at time " . time() . "\n";



#add attributes to event
if( 0 == event_notification_put_string( $handle,
					$notification,
					"srcnode",
					$srcnode ) )
{ die "Could not add attribute to notification\n"; }
if( 0 == event_notification_put_string( $handle,
					$notification,
					"dstnode",
					$dstnode ) )
{ die "Could not add attribute to notification\n"; }
if( 0 == event_notification_put_string( $handle,
					$notification,
					"testtype",
					$ARGV[2] ) )
{ die "Could not add attribute to notification\n"; }
if( 0 == event_notification_put_string( $handle,
					$notification,
					"testper",
					$period ) )
{ die "Could not add attribute to notification\n"; }
			

#send notification
if (!event_notify($handle, $notification)) {
    die("could not send test event notification");
}

event_notification_free($handle, $notification);

=cut

if (event_unregister($handle) == 0) {
    die("could not unregister with event system");
}

exit(0);
