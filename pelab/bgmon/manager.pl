#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#

use lib '/usr/testbed/lib';
use libtbdb;
use event;
use Getopt::Std;
use strict;
use IO::Socket::INET;
use IO::Select;
#use lib '/q/proj/tbres/gebhardt/testbed/pelab/bgmon';
use libwanetmon;

#
# Turn off line buffering on output
#
$| = 1;

sub usage {
    print "Usage: $0 [-s server] [-p port] [-c commandport] [-d] [-i]\n";
    return 1;
}

my $debug    = 0;
my $impotent = 0;
my $nocap    = 0;
my $bwperiod = 5;

#
# $evexpt is the experiment ("pid/eid") in which the entire bgmon framework
# (probes, manager, clients) is running.  This is used for confining Emulab
# events (client/manager communication) to just that experiment.  The
# '-e pid/eid' option to the manager sets the context for that instance.
#
# $bgmonexpt is an incompletely implemented feature that allows multiple
# probe experiments to share the manager.  Here the client specifies the
# experiment of the probes (passed in the message) and one common manager
# muxes/demuxes messages to the different probe experiments.  There are
# still seperate opsrecv processes.  It isn't yet clear how useful this
# might be.
# 

my $evexpt   = "__none";
my $default_bgmonexpt = "tbres/pelabbgmon";
my $bgmonexpt;
my ($server,$port,$cmdport);
my %opt = ();
if (!getopts("s:p:c:dNihe:", \%opt)) {
    exit &usage;
}

#
# Caps enforced on users and globally. Totally made-up.
# Rates are in kilobytes / second, data transfer sizes are in kilobytes
# XXX: I think I have a constant wrong here somewhere, these limts should
# not have to be this high
#
my $USER_AVGRATE_CAP = 10 * 1024 / 8.0; # 10Mbps
my $USER_DAILY_CAP = 1000 * 1024 * 1024 / 8.0; # 100Gbit
my $USER_TOTAL_CAP = 2 * $USER_DAILY_CAP; # 200Gbit
my $GLOBAL_AVGRATE_CAP = $USER_AVGRATE_CAP * 5;
my $GLOBAL_DAILY_CAP = $USER_DAILY_CAP * 5;
my $GLOBAL_TOTAL_CAP = $USER_TOTAL_CAP * 5;

#
# Keeps track of outstanding requests
#
my @requests;

if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }
if ($opt{c}) { $cmdport = $opt{c};} else { $cmdport = 5052; }
if ($opt{h}) { exit &usage; }
if ($opt{e}) { $evexpt = $opt{e}; setevexpid($evexpt); $bgmonexpt=$evexpt;}
if ($opt{d}) { $debug = 1; }
if ($opt{i}) { $impotent = 1; } 
if ($opt{N}) { $nocap = 1; print "*** No bandwidth cap enforced\n"; }

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
	    chomp $inmsg;
	    print "gotmsg from amc: $inmsg\n";
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
#XXX / TODO: stuff for tool generalization
#  hacked in here for now, but source should be from the 
# managerclient' message (?)
#
sub addToolSpecificFields
{
    my ($cmdRef) = @_;
    my $testtype = $cmdRef->{testtype};
    my $toolname;
    my ($req_params_actual, $opt_params_actual);

    if( $testtype eq "bw" ){
        $toolname = "iperf";
#        $toolwrapperpath = "/tmp/iperfwrapper";
#        $tooltype = "one-shot";
        $req_params_actual = "port 5002 duration 5";
    }elsif( $testtype eq "latency"){
        $toolname = "fping";
#        $toolwrapperpath = "/tmp/fpingwrapper";
#        $tooltype = "one-shot";
        $req_params_actual = "timeout 10000 retries 1";
    }

    my $sth = DBQuery("select * from tool_spec where toolname='$toolname'");
    my ( $toolname, $metric, $type, $toolwrapperpath, 
         $req_params_formal, $opt_params_formal)
        = ( $sth->fetchrow_array() );

    #XXX / TODO Check that all given actual parameters match the formal params
    #
    $cmdRef->{toolname} = $toolname;
    $cmdRef->{toolwrapperpath} = $toolwrapperpath;
    $cmdRef->{tooltype} = $type;
    $cmdRef->{req_params} = $req_params_actual;
    $cmdRef->{opt_params} = $opt_params_actual;

    print "CMD: \n";
    foreach my $key (keys %{$cmdRef}){
        my $value = ${$cmdRef}{$key};
        print "  $key=$value  \n";
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
        my $thisexpid = event_notification_get_string($handle,
                                                     $notification,
                                                     "expid");
        if( defined $thisexpid && $thisexpid ne ""  ){
            setexpid($thisexpid);
            $bgmonexpt = $thisexpid;
        }else{
            $bgmonexpt = $default_bgmonexpt;
        }

#	print "Event: $time $site $expt $group $host $objtype $objname " .
#		"$eventtype\n";

	print "EVENT: $time $objtype $eventtype\n"
	    if ($debug);

	# XXX just to make sure we get the event
	if( $eventtype eq "TEST" ){
	    my $managerID = event_notification_get_string($handle,
							  $notification,
							  "managerID");
	    print "got TEST event: managerID=$managerID\n";
	}

	# TODO: Does this have to be listed in lib/libtb/tbdefs.h ??
	elsif( $eventtype eq "EDIT" ){
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
	    my $newexpid = event_notification_get_string($handle,
							  $notification,
							  "expid");


	    if( !defined $newexpid || $newexpid eq "" ){
		$newexpid = $bgmonexpt;
	    }

	    my %cmd = ( expid     => $newexpid,
                    cmdtype   => $eventtype,
                    dstnode   => $dstnode,
                    testtype  => $testtype,
                    testper   => "$period",
                    duration  => "$duration",
                    managerID => $managerID
			);
        
        addToolSpecificFields(\%cmd);

	    print "got EDIT: $srcnode, $dstnode: $newexpid\n";

	    # only automanager can send "forever" edits (duration=0)
	    if( isCmdValid(\%cmd) ){
		print "sending cmd to $srcnode on behalf of $managerID\n";
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
	    my $newexpid = event_notification_get_string($handle,
							 $notification,
							 "expid");

	    if( !defined $newexpid || $newexpid eq "" ){
		$newexpid = $bgmonexpt;
	    }

	    my %cmd = ( expid     => $newexpid,
                    cmdtype   => $eventtype,
                    destnodes  => $destnodes,
                    srcnode   => $srcnode,
                    testtype  => $testtype,
                    testper   => "$testper",
                    duration  => "$duration"
                    ,managerID => $managerID
			);

        addToolSpecificFields(\%cmd);

	    print "got $eventtype:$srcnode,$destnodes,$testtype,".
		"$testper,$duration,$managerID,$newexpid\n";

	    if( isCmdValid(\%cmd) ){
            print "sending cmd to $srcnode on behalf of $managerID\n";
            sendcmd( $srcnode, \%cmd );
	    }else{
            print "rejecting $testtype cmd for $srcnode\n";
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
        elsif( $eventtype eq "DIE" ){
            my $node = event_notification_get_string($handle,
							$notification,
							"srcnode");
            my $managerID = event_notification_get_string($handle,
							  $notification,
							  "managerID");

	    print "got $eventtype:$node,$managerID,$bgmonexpt\n";
            killnode($node);
        }
}


sub isCmdValid($)
{
    my ($cmdref) = @_;
    my $valid = 1;

    #list of invalid conditions
    if( $cmdref->{managerID} ne "automanagerclient" ){
	#managerclient is not the AMC
	if( $cmdref->{duration} == 0 ){
	    #only AMC can send "forever" commands
	    return(0);
	}
	if( $cmdref->{testtype} eq "bw" ){
	    # Cannot specify period less than test length
	    if ( $cmdref->{testper} < $bwperiod ){
		return(0);
	    }

	    # No caps enforced
	    if ($nocap) {
		return(1);
	    }

	    my @destnodes;
	    if( $cmdref->{cmdtype} eq "INIT" ){
		@destnodes = split(" ",$cmdref->{destnodes});
	    }elsif( $cmdref->{cmdtype} eq "EDIT" ){
		push @destnodes, $cmdref->{dstnode};
	    }
	    my $numDest = scalar(@destnodes);
	    #bandwidth must conform to limit
	    #SET TO MAX RATE, but it becomes VALID

	    #for now, just limit duration if bw period is low
	    if( $cmdref->{testper} < 600 && 
		$cmdref->{duration} > 1200 )
	    {
		$cmdref->{duration} = "1200";
		$cmdref->{testper} = "600";
	    }
	    #limit duration, if testper is given with a duty-cycle
	    # higher than 20% for the given size set of destinations
	    elsif( $cmdref->{duration} > 120 &&
		    $cmdref->{testper} < ($numDest-1) * 5 * 1/.20 )
	    {
                #TODO: Commented out to prevent problems in our own tests.
		#$cmdref->{duration} = "120";
	    }

            #
            # Check to see if the request would exceed our caps on bandwidth
            # rates and total bandwidth
            # TODO: Only really works for INIT events now
            #
            if ($cmdref->{cmdtype} eq "INIT") {

                # Start by clearing out any old requests that have expired
                clear_expired_requests(\@requests);

                my $totalbw = 0.0;
                foreach my $dst (@destnodes) {
                    my $pairbw = getBandwidth($cmdref->{srcnode},$dst);
                    $totalbw += $pairbw;
                }
                # Duty cycle is the % of the time we'll be running a test
                # XXX: This hardcodes the length of bandwidth tests!
                my $dutycycle =  ($numDest * 5.0) / $cmdref->{testper};
                # This is the average sending rate of the test at any point in
                # time
                my $avgrate = $totalbw * $dutycycle;
                # This is how much traffic we can expect to send in a day
                my $dailytransfer = $avgrate * 60 * 60 * 24;
                # This is how much traffic we can expect to send over the
                # duration of the requested test
                my $totaltransfer = $avgrate * $cmdref->{duration};

                # Add these stats up for all clients, and for all requests from
                # this client
                # XXX: Does not take into account the fact that higher-rate
                # requests 'override', not add-to lower-rate ones
                my ($globalAvgrate, $globalDailytransfer, $globalTotaltransfer);
                my ($userAvgrate, $userDailytransfer, $userTotaltransfer);
                $globalAvgrate = $userAvgrate = $avgrate;
                $globalDailytransfer = $userDailytransfer = $dailytransfer;
                $globalTotaltransfer = $userTotaltransfer = $totaltransfer;
                foreach my $request (@requests) {
                    my $samerequestor = ($request->{cmd}{managerID} eq
                        $cmdref->{managerID});
                    $globalAvgrate += $request->{avgrate};
                    $globalDailytransfer += $request->{dailytransfer};
                    $globalTotaltransfer += $request->{totaltransfer};
                    if ($samerequestor) {
                        $userAvgrate += $request->{avgrate};
                        $userDailytransfer += $request->{dailytransfer};
                        $userTotaltransfer += $request->{totaltransfer};
                    }
                }

                #
                # Check against limits - we don't actually reject any commands
                # for now, just report that we would have
                #
                print "Command from $cmdref->{managerID} for ".
                        "$cmdref->{testtype}\n";
                print "  $numDest destination nodes, " .
                        "period $cmdref->{testper}, " .
                        "duration $cmdref->{duration}\n";
                print "  total bandwidth $totalbw, duty cycle $dutycycle\n";
                print "  avgrate = $avgrate dailytransfer = $dailytransfer " .
                         "totalbw = $totalbw\n";
                print "  userAvgrate = $userAvgrate " .
                         "userDailytransfer = $userDailytransfer " .
                         "userTotaltransfer = $userTotaltransfer\n";
                print "  globalAvgrate = $globalAvgrate " .
                         "globalDailytransfer = $globalDailytransfer " .
                         "globalTotaltransfer = $globalTotaltransfer\n";

                if ($userAvgrate > $USER_AVGRATE_CAP) {
                    print "  REJECTED: user average rate cap exceeded\n";
                }
                if ($userDailytransfer > $USER_DAILY_CAP) {
                    print "  REJECTED: user daily rate cap exceeded\n";
                }
                if ($userTotaltransfer > $USER_TOTAL_CAP) {
                    print "  REJECTED: user total rate cap exceeded\n";
                }
                if ($globalAvgrate > $GLOBAL_AVGRATE_CAP) {
                    print "  REJECTED: global average rate cap exceeded\n";
                }
                if ($globalDailytransfer > $GLOBAL_DAILY_CAP) {
                    print "  REJECTED: global daily rate cap exceeded\n";
                }
                if ($globalTotaltransfer > $GLOBAL_TOTAL_CAP) {
                    print "  REJECTED: global total rate cap exceeded\n";
                }

                if ($valid) {
                    # Remember this request, so that we can track state between
                    # requests
                    my %request = ( cmd => $cmdref,
                                    avgrate => $avgrate,
                                    dailytransfer => $dailytransfer,
                                    totaltransfer => $totaltransfer,
                                    received => time() );
                    push @requests, \%request;
                }
            }
	}
    }
    return $valid;
}

#
# Clear out all requests whose durations have expired
#
sub clear_expired_requests($) {
    #my ($requests) = (@_);

    my $now = time();

    my @keep;
    #foreach my $request (@$requests) {
    foreach my $request (@requests) {
        # A duration of '0' means to continue forever
        my $expiration = $request->{received} + $request->{cmd}->{duration};
        if ($request->{cmd}{duration} != 0 && $expiration < $now) {
            print "Removing command (recv = $request->{received}, dur = $request->{cmd}->{duration}, expiration = $expiration, now = $now)\n";
        } else {
            push @keep, $request;
        }
    }

    #$requests = \@keep;
    @requests = @keep;
}

#
# Get an estimate of the bandwidth between two nodes
#
sub getBandwidth() {
    my ($src, $dst) = @_;

    #
    # Find the site indexes
    #
    my $result = DBQueryWarn("select site_idx from site_mapping where " .
                             "node_id='$src'");

    if ($result->numrows() != 1) {
        warn "Error getting site_idx for node $src";
        return 0;
    }
    my $srcidx = ($result->fetchrow_array())[0];
    $result = DBQueryWarn("select site_idx from site_mapping where " .
                             "node_id='$dst'");

    if ($result->numrows() != 1) {
        warn "Error getting site_idx for node $dst";
        return 0;
    }
    my $dstidx = ($result->fetchrow_array())[0];

    # Get the average bandwidth in the ops database for the pair
    $result = DBQueryWarn("select AVG(bw) from pair_data " .
                          "where srcsite_idx='$srcidx' ".
                          "and dstsite_idx='$dstidx' ".
                          "and bw is not null and bw > 0");
    my $avgbw = ($result->fetchrow_array())[0];
    if (!defined($avgbw) || $avgbw <= 0) {
        warn "Unable to get measurements for pair $src,$dst " .
               "($srcidx,$dstidx)\n";
        return 0;
    } else {
        return $avgbw;
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
