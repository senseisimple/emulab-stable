#!/usr/bin/perl

#
# Configure variables
#
use lib '/usr/testbed/lib';

use event;
use Getopt::Std;
use strict;

sub usage {
    warn "Usage: $0 [-s server] [-p port] <monitoraddr> <link dest> ".
	"<testtype> <testper(seconds)> <COMMAND>\n";
    return 1;
}

my %opt = ();
getopt("s:p:h",\%opt);

if ($opt{h}) { exit &usage; }
if (@ARGV != 5) { exit &usage; }

my ($server,$port);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }


%$tuple = ( objtype   => "BGMON",
	    objname   => $ARGV[0],
	    eventtype => $ARGV[4],
	    expt      => "__none");


my $notification = event_notification_alloc($handle,$tuple);
if (!$notification) { die "Could not allocate notification\n"; }
print "Sent at time " . time() . "\n";



#add attributes to event
if( 0 == event_notification_put_string( $handle,
					$notification,
					"linkdest",
					$ARGV[1] ) )
{ die "Could not add attribute to notification\n"; }
if( 0 == event_notification_put_string( $handle,
					$notification,
					"testtype",
					$ARGV[2] ) )
{ die "Could not add attribute to notification\n"; }
if( 0 == event_notification_put_string( $handle,
					$notification,
					"testper",
					$ARGV[3] ) )
{ die "Could not add attribute to notification\n"; }
			

#send notification
if (!event_notify($handle, $notification)) {
    die("could not send test event notification");
}

event_notification_free($handle, $notification);

if (event_unregister($handle) == 0) {
    die("could not unregister with event system");
}

exit(0);
