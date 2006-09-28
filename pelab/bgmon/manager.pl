#!/usr/bin/perl

use lib '/usr/testbed/lib';
use libtbdb;
use libwanetmon;
use event;
use Getopt::Std;
use strict;
use IO::Socket::INET;
use IO::Select;


#
# Turn off line buffering on output
#
$| = 1;

sub usage {
    print "Usage: $0 [-s server] [-p port] [-c commandport] [-e pid/eid] [-d] [-i]\n";
    return 1;
}

my $debug    = 0;
my $impotent = 0;
my $evexpt   = "__none";
my $bgmonexpt = "tbres/pelabbgmon";
my ($server,$port,$cmdport);
my %opt = ();
if (!getopts("s:p:c:e:dih", \%opt)) {
    exit &usage;
}


if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }
if ($opt{c}) { $cmdport = $opt{c};} else { $cmdport = 5052; }
if ($opt{h}) { exit &usage; }
if ($opt{e}) { $evexpt = $opt{e}; }
if ($opt{d}) { $debug = 1; }
if ($opt{i}) { $impotent = 1; } 

if (@ARGV !=0) { exit &usage; }

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBNAME = "pelab";
my $DBUSER = "pelab";
my $DBErrorString = "";
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

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

# Register for normal "manager client" notifications
my $handle_mc = event_register($URL,0);
if (!$handle_mc) { die "Unable to register with event system\n"; }
my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }
%$tuple = ( host      => $event::ADDRESSTUPLE_ALL,
	    objtype   => "WANETMON"
	    , objname   => "managerclient"
	    , expt      => $evexpt
	    , scheduler => 0
	    );
if (!event_subscribe($handle_mc,\&callbackFunc,$tuple)) {
    die "Could not subscribe to event\n";
}



# Create a TCP socket to receive command requests on
my $socket_cmd = IO::Socket::INET->new( LocalPort => $cmdport,
					Proto    => 'tcp',
					Blocking => 0,
					Listen   => 1,
					ReuseAddr=> 1 )
    or die "Couldn't create socket on $cmdport\n";

#create Select object.
my $sel = IO::Select->new();
$sel->add($socket_cmd);

print "setting cmdport $cmdport and cmdexpt $bgmonexpt\n";
setcmdport($cmdport);
setexpid($bgmonexpt);


#main()

while (1) {

    #check for pending received events from MANAGERCLIENTS
    event_poll_blocking($handle_mc, 100);


    #check for commands sent by the AUTOMANAGERCLIENT
    my @ready = $sel->can_read(1); #TODO: make non-blocking time longer?
    foreach my $handle (@ready){
	my %sockIn;
	my $cmdHandle;
	if( $handle eq $socket_cmd ){
#	    print "accepting connection\n";
	    $cmdHandle = $handle->accept();
	    my $inmsg = <$cmdHandle>;
#	    print "gotmsg: $inmsg\n";
	    chomp $inmsg;
	    %sockIn = %{ deserialize_hash($inmsg) };
	    my $srcnode = $sockIn{srcnode};
	    my @res = sendcmd( $srcnode, \%sockIn ); #send hash on out
	    if( $res[0] == 1 ){
		print $cmdHandle "OK\n";
	    }else{
		print $cmdHandle "FAIL\n";
	    }
	}
    }
}


#
# callback for managerclient requests
#
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

	print "EVENT: $time $objtype $eventtype\n"
	    if ($debug);

	# TODO: Does this have to be listed in lib/libtb/tbdefs.h ??
	if( $eventtype eq "EDIT" ){
	    #
	    # Got a request to modify a path's test freq
	    #

	    my $srcnode = event_notification_get_string($handle,
							$notification,
							"srcnode");
	    my $dstnode = event_notification_get_string($handle,
							 $notification,
							 "dstnode");
	    my $testtype = event_notification_get_string($handle,
							 $notification,
							 "testtype");
	    my $period = event_notification_get_string($handle,
						       $notification,
						       "testper");
	    my $duration = event_notification_get_string($handle,
							 $notification,
							 "duration");
	    my $managerID = event_notification_get_string($handle,
							  $notification,
							  "managerID");

	    my %cmd = ( expid     => $bgmonexpt,
			cmdtype   => $eventtype,
			dstnode   => $dstnode,
			testtype  => $testtype,
			testper   => "$period",
			duration  => "$duration",
			managerID => $managerID
			);


	    print "got EDIT: $srcnode, $dstnode\n";


#	    my $socket;
#	    my $sel = IO::Select->new();

	    # only automanager can send "forever" edits (duration=0)
	    if( $duration > 0 || $managerID eq "automanagerclient" ){
		print "sending cmd from $srcnode\n";
		sendcmd( $srcnode, \%cmd );
	    }

	}
	elsif( $eventtype eq "INIT" ){
	    #Got a request to init a node
	    my $srcnode = event_notification_get_string($handle,
							$notification,
							"srcnode");
	    my $destnodes = event_notification_get_string($handle,
							 $notification,
							 "dstnodes");
	    my $testtype = event_notification_get_string($handle,
							 $notification,
							 "testtype");
	    my $testper = event_notification_get_string($handle,
						       $notification,
						       "testper");
	    my $duration = event_notification_get_string($handle,
							 $notification,
							 "duration");
	    my $managerID = event_notification_get_string($handle,
							  $notification,
							  "managerID");

	    my %cmd = ( expid     => $bgmonexpt,
			cmdtype   => $eventtype,
			destnodes  => $destnodes,
			testtype  => $testtype,
			testper   => "$testper",
			duration  => "$duration"
			,managerID => $managerID
			);

	    print "got $eventtype:$srcnode,$destnodes,$testtype,".
		"$testper,$duration,$managerID,$bgmonexpt\n";

	    # only automanager can send "forever" edits (duration=0)
	    if( $duration > 0 ){ #|| $managerID eq "automanagerclient" ){
		print "sending cmd from $srcnode\n";
		sendcmd( $srcnode, \%cmd );
	    }
	}
	elsif( $eventtype eq "STOPALL" ){
	    #got a request to stop all tests from a given node
	    my $srcnode = event_notification_get_string($handle,
							$notification,
							"srcnode");
	    my $managerID = event_notification_get_string($handle,
							  $notification,
							  "managerID");

	    print "got $eventtype:$srcnode,$managerID,$bgmonexpt\n";
	    stopnode($srcnode, $managerID);
	}

}


=pod
sub event_poll_amc($){
    my ($handle) = @_;
    my $rv =  c_event_poll($handle);
    if ($rv) {
	die "Trouble in event_poll - returned $rv\n";
    }

    my $data = dequeue_callback_data();
    if( defined $data ){
	&$event::callback($handle,$data->{callback_notification},
			  $event::callback_data);
	event_notification_free($handle,$data->{callback_notification});
	free_callback_data($data);
    }
    
}
=cut
