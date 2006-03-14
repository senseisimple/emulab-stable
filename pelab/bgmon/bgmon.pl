#!/usr/bin/perl

#TODO!! Result index numbers... window size 2x > buffer size???
# (1/30/06) NOPE to above. No Ack's, just trust that sent data gets there.
# Q: how to determine if OPS receives it correctly?
#    (1/31) Just checking if "send notification" is successful
#
# (2/3/06): CLEANUP ALLOCATED MEMORY using .._free() functions

=pod
HOW TO USE:
Send a notification to this node to perform a desired action by:
objtype   => BGMON
objname   => plabxxx   where plabxxx is the this node address
eventtype => <COMMAND> where COMMAND is any of:
  EDIT: Modify a specific link test to specific destination node.
        Notification must contain the following attributues:
          "linkdest" = destination node of the test. Example, "node10"
          "testtype" = type of test to run. Examples, "latency" or "bw"
          "testper"  = period of time between successive tests.
  INIT: Initialize the link tests with default destination nodes and frequency.
        **todo** ^^needs updating 
  SINGLE: Perform a single link test to a specifc destination node.
          Notification must contain the following attributues:
           "linkdest" = destination node of the test. Example, "node10"
           "testtype" = type of test to run. Examples, "latency" or "bw"
=cut


use lib '/usr/testbed/lib';
use event;
use Getopt::Std;
use strict;
use DB_File;

sub usage {
	warn "Usage: $0 [hostname] -d [workingdir]\n";
	return 1;
}


#*****************************************
my %MAX_SIMU_TESTS = (latency => "10",
		      bw      => "1");
# a ratio of testing period to wait after a test failure
my %TEST_FAIL_RETRY= (latency => 0.5,
		      bw      => 0.1);
my $resultDBlimit = 100;
my $resIndex = 0;
my %testevents = ();
#queue for each test type containing tests that need to
# be run, but couldn't, due to MAX_SIMU_TESTS.
# keys are test types, values are lists
# This separate Q, which has higher priority than the normal %testevents
# should solve "starvation" issues if an "early" test blocks for too long
# such as during a network timeout with iperf
my %waitq = ( latency => [],
	      bw      => [] );
#*****************************************

my %opt = ();
#getopt(\%opt,"s:p:h");
getopt("s:p:h:d",\%opt,);

#if ($opt{h}) { exit &usage; }
#if (@ARGV > 1) { exit &usage; }

my $thismonaddr;
if( defined  $ARGV[0] ){
    $thismonaddr = $ARGV[0];
}else{
    $_ = `cat /var/emulab/boot/nodeid`;
    /plabvm(\d+)-/;
    $thismonaddr = "plab$1";
}
print "thismonaddr = $thismonaddr\n";


my ($server,$port,$workingdir);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }
if( $opt{d}) { $workingdir = $opt{d}; `cd $workingdir`; }

print "server=$server\n";

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }
my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }
print "registered with $server\n";
my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }

%$tuple = ( host      => $event::ADDRESSTUPLE_ALL,
	    objtype   => "BGMON",
	    objname   => $thismonaddr);

if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	die "Could not subscribe to event\n";
}

#this call will reconnect event system if it has failed
sendBlankNotification();

#############################################################################
# Note a difference from tbrecv.c - we don't yet have event_main() functional
# in perl, so we have to poll. (Nothing special about the select, it's just
# a wacky way to get usleep() )
#############################################################################
#main()

while (1) {

    #check for pending received events
    event_poll_blocking($handle,100);

    #TODO: this loop shouldn't be run every poll loop.
    #try to run tests on queue
    foreach my $testtype (keys %waitq){
	my $arrlen = scalar(@{$waitq{$testtype}});
	#DONOT use "foreach" here, since the call to spawnTest may add to waitq
	for( my $i = 0; $i < $arrlen; $i++ ){
	    my $destnode = pop @{$waitq{$testtype}};
#	    print "WAIT Q: REMOVED $destnode \n";
	    spawnTest( $destnode, $testtype );
	}
    }

    #iterate through all event structures
    for my $destaddr ( keys %testevents ) {
	for my $testtype ( keys %{ $testevents{$destaddr} } ){
	    #mark flags of finished tests
	    #check if process is running
	    my $pid = $testevents{$destaddr}{$testtype}{"pid"};
	    if( $pid != 0 ){
		use POSIX ":sys_wait_h";
		my $cnt = waitpid( $pid, &WNOHANG );
		if( $cnt != 0 )
		{
		    if( $? == 0 ){
			#process finished, so mark it's "finished" flag
			$testevents{$destaddr}{$testtype}{"flag_finished"} = 1;
			$testevents{$destaddr}{$testtype}{"pid"} = 0;
		    }else{
			#process exited abnormally
			#reset pid
			$testevents{$destaddr}{$testtype}{"pid"} = 0;
			#schedule next test at a % of a normal period from now
			my $nextrun = time_all() + 
			    $testevents{$destaddr}{$testtype}{"testper"} *
				$TEST_FAIL_RETRY{$testtype};
			$testevents{$destaddr}{$testtype}{"timeOfNextRun"} = 
			    $nextrun;
			#delete tmp filename
			my $filename = createtmpfilename($destaddr, $testtype);
			unlink($filename) or die "can't delete temp file";
		    }
		}		
	    }

	    #timeOfNextRun for finished events
	    if( $testevents{$destaddr}{$testtype}{"flag_finished"} == 1 ){
		#read raw results from temp file
		my $filename = createtmpfilename($destaddr, $testtype);
		open FILE, "< $filename" 
		    or die "can't open file $filename";
	        my @raw_lines = <FILE>;
		my $raw;
		foreach my $line (@raw_lines){
		    $raw = $raw.$line;
		}
		close FILE;
		unlink($filename) or die "can't delete temp file";
		#parse raw data
		my $parsedData = parsedata($testtype,$raw);
		$testevents{$destaddr}{$testtype}{"results_parsed"} =
		    $parsedData;

		my %results = 
		    ("sourceaddr" => $thismonaddr,
		     "destaddr" => $destaddr,
		     "testtype" => $testtype,
		     "result" => $parsedData,
		     "tstamp" => $testevents{$destaddr}{$testtype}
		               {"tstamp"} );
		#save results to local DB
		saveTestToLocalDB(\%results);
		#send results to remote DB
		sendResults(\%results, $resIndex);
		$resIndex = ($resIndex+1) % $resultDBlimit;

		#reset flags
		$testevents{$destaddr}{$testtype}{"flag_finished"} = 0;
		$testevents{$destaddr}{$testtype}{"flag_scheduled"} = 0;
	    }

	    #schedule new tests
	    if( $testevents{$destaddr}{$testtype}{"flag_scheduled"} == 0 &&
		$testevents{$destaddr}{$testtype}{"testper"} > 0 )
	    {
		#schedule next test
#		$testevents{$destaddr}{$testtype}{"timeOfNextRun"} =
#		    $testevents{$destaddr}{$testtype}{"testper"} + time_all();

		if( time_all() < 
		    $testevents{$destaddr}{$testtype}{"timeOfNextRun"}
		    + $testevents{$destaddr}{$testtype}{"testper"} )
		{
		    #if time of next run is in the future, set it to that
		    $testevents{$destaddr}{$testtype}{"timeOfNextRun"} 
		      += $testevents{$destaddr}{$testtype}{"testper"};
		}else{
		    #if time of next run is in the past, set to current time
		    $testevents{$destaddr}{$testtype}{"timeOfNextRun"} 
		      = time_all();
		}

		$testevents{$destaddr}{$testtype}{"flag_scheduled"} = 1;
	    }

#	    print "nextrun="
#		.$testevents{$destaddr}{$testtype}{"timeOfNextRun"}."\n";
#	    print "scheduled?=".$testevents{$destaddr}{$testtype}{"flag_scheduled"}.
		"\n";
#	    print "pid=".$testevents{$destaddr}{$testtype}{"pid"}."\n";

	    #check for new tests ready to run
	    if( scalar(@{$waitq{bw}}) == 0 &&
		$testevents{$destaddr}{$testtype}{"timeOfNextRun"} 
		                                           <= time_all() &&
		$testevents{$destaddr}{$testtype}{"flag_scheduled"} == 1 &&
		$testevents{$destaddr}{$testtype}{"pid"} == 0 )
	    {
		#run test
		spawnTest( $destaddr, $testtype );
	    }
	}

	# may not be needed, but may help detect errors
	my $hangres = detectHang($destaddr);
	if( $hangres ne "nohang" ){
	    print "HANG: $hangres,  $destaddr\n";
	}
    }





=pod    
    #check for results that have not been sent due to error
    for( my $i=0; $i < $resultDBlimit; $i++ ){
	if( -e createDBfilename($i) ){
	    #resend
	    my %results;
	    my %db;
	    tie( %db, "DB_File", createDBfilename($i) ) 
		 or die "cannot open db file";
	    for my $key (keys %db ){
		$results{$key} = $db{$key};
	    }
	    untie %db;
	    sendResults(\%db,$i);
	}
    }
=cut

    #sleep for a time
#    my $tmp = select(undef, undef, undef, 0.25);
#    open NULL,"/dev/null";
#    printTimeEvents();
}

#############################################################################

if (event_unregister($handle) == 0) {
    die "Unable to unregister with event system\n";
}

exit(0);


sub callbackFunc($$$) {
	my ($handle,$notification,$data) = @_;

	my $time      = time_all();
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

#	print "EVENT: $time $objtype $eventtype\n";



	#change values and/or initialize
	if( $eventtype eq "EDIT" ){
	    print "EDIT\n";
	    my $linkdest = event_notification_get_string($handle,
							 $notification,
							 "linkdest");
	    my $testtype = event_notification_get_string($handle,
							 $notification,
							 "testtype");
	    my $testper = event_notification_get_string($handle,
							$notification,
							"testper");
	    $testevents{$linkdest}{$testtype}{"testper"} = $testper;
	    $testevents{$linkdest}{$testtype}{"flag_scheduled"} = 0;
	    $testevents{$linkdest}{$testtype}{"timeOfNextRun"} = time_all();

	    print( "linkdest=$linkdest\n".
		   "testype =$testtype\n".
		   "testper=$testper\n" );
	}
	elsif( $eventtype eq "INIT" ){
	    print "INIT\n";
	    my $testtype = event_notification_get_string($handle,
							 $notification,
							 "testtype");
	    my @destnodes 
		= split(" ", event_notification_get_string($handle,
							   $notification,
							   "destnodes"));
            my $testper = event_notification_get_string($handle,
							$notification,
							"testper");
#	    print "$testtype*$@destnodes*$testper\n";

	    #TOOD: Add a start time offset, so as to schedule the initial test
            #      from the manager/controller
	    
            foreach my $linkdest (@destnodes){
		$testevents{$linkdest}{$testtype}{"testper"} = $testper;
		$testevents{$linkdest}{$testtype}{"flag_scheduled"} = 0;
		$testevents{$linkdest}{$testtype}{"timeOfNextRun"} 
		    = time_all();
	    }
	}
	elsif( $eventtype eq "SINGLE" ){
	    print "SINGLE\n";
	    #schedule a single test run NOW (this time, minus 1 second)
	    my $linkdest = event_notification_get_string($handle,
							 $notification,
							 "linkdest");
	    my $testtype = event_notification_get_string($handle,
							 $notification,
							 "testtype");
	    my $testper = event_notification_get_string($handle,
							$notification,
							"testper");
	    $testevents{$linkdest}{$testtype}{"testper"} = 0;
	    $testevents{$linkdest}{$testtype}{"timeOfNextRun"} = time_all()-1;
	    $testevents{$linkdest}{$testtype}{"flag_scheduled"} = 1;
	    $testevents{$linkdest}{$testtype}{"pid"} = 0;

	    print( "linkdest=$linkdest\n".
		   "testype =$testtype\n".
		   "testper=$testper\n" );
	    
	}
	elsif( $eventtype eq "STOPALL" ){
	    print "STOPALL\n";
	    %testevents = ();
	}

#	if (event_unregister($handle) == 0) {
#	    die "Unable to unregister with event system\n";
#	}
#	exit(0);

	#TODO: Why does this segfault?
#	if( event_notification_free( $handle, $notification ) == 0 ){
#	    die "Unable to free notification";
#	}
}

#############################################################################

sub spawnTest($$)
{
    my ($linkdest, $testtype) = @_;
    
    use Errno qw(EAGAIN);


    #exit and don't fork if the max number of tests is already being run
    if( getRunningTestsCnt($testtype) >= $MAX_SIMU_TESTS{$testtype} ){
#	print "Testcnt = ".getRunningTestsCnt($testtype);
#	print "Too many running tests of type $testtype\n";
	
	#add this to queue if it doesn't exist already
	#this seach is inefficient... use a hash? sparse array? sorted list?
	my $flag_duplicate = 0;	
	foreach my $element ( @{$waitq{$testtype}} ){
	    if( $element eq $linkdest ){
		$flag_duplicate = 1;
	    }
	}
	if( $flag_duplicate == 0 ){
	    unshift @{$waitq{$testtype}}, $linkdest;
#	    print "WAIT Q: ADDED $linkdest \n";
	}

	return 0;
    }

  FORK:{
      if( my $pid = fork ){
	  #parent
	  #save child pid in test event
	  $testevents{$linkdest}{$testtype}{"pid"} = $pid;
	  $testevents{$linkdest}{$testtype}{"tstamp"} = time_all();

      }elsif( defined $pid ){
	  #child
	  #exec 'ls','-l' or die "can't exec: $!\n";
	  
	  my $filename = createtmpfilename($linkdest,$testtype);

	  #############################
	  ###ADD MORE TEST TYPES HERE###
	  #############################
	  if( $testtype eq "latency" ){
	      #command line for "LATENCY TEST"
#	      print "##########latTest\n";
	      exec "ping -c 1 $linkdest >$filename"
		  or die "can't exec: $!\n";
	  }elsif( $testtype eq "bw" ){
	      #command line for "BANDWIDTH TEST"
#	      print "###########bwtest\n";
#	      exec "eval $workingdir".
#		   "iperf -c $linkdest -t 10 -p 5002 >$filename"
	      exec "$workingdir".
		  "iperf -c $linkdest -t 10 -p 5002 >$filename"
		      or die "can't exec: $!";
	  }else{
	      warn "bad testtype: $testtype";
	  }

      }elsif( $! == EAGAIN ){
	  #recoverable fork error, redo;
	  sleep 1;
	  redo FORK;
      }else{ die "can't fork: $!\n";}
  }
}

sub getRunningTestsCnt($)
{
    my ($type) = @_;
    my $testcount = 0;

    #count currently running tests
    for my $destaddr ( keys %testevents ) {
	if( $testevents{$destaddr}{$type}{"pid"} > 0 ){
	    #we have a running process, so inc it's counter
	    $testcount++;
	}
    }
#    print "testcount = $testcount\n";
    return $testcount;
}
#############################################################################
sub parsedata($$)
{
    my $type = $_[0];
    my $raw = $_[1];
    my $parsed;
    $_ = $raw;

#    print "Raw=$_";

    #############################
    ###ADD MORE TEST TYPES HERE###
    #############################
    #latency test
    if( $type eq "latency" ){
	/time=(.*) ms/;
	$parsed = "$1";
    }elsif( $type eq "bw" ){
	/\s+(\S*)\s+([MK])bits\/sec/;
	$parsed = $1;
	if( $2 eq "M" ){
	    $parsed *= 1000;
	}
#	print "parsed=$parsed\n";
    }
	   
    return $parsed;
}


#############################################################################
sub printTimeEvents {
    for my $destaddr ( keys %testevents ) {
	for my $testtype ( keys %{ $testevents{$destaddr} } ){
	    print 
		"finished? ".
		$testevents{$destaddr}{$testtype}{"flag_finished"}."\n".
		"scheduled?= ".
		$testevents{$destaddr}{$testtype}{"flag_scheduled"}."\n".
		"testper?= ".
                $testevents{$destaddr}{$testtype}{"testper"}."\n".
		"timeOfNextRun= ".
	        $testevents{$destaddr}{$testtype}{"timeOfNextRun"}."\n" .
		"results_parsed= ".
      	        $testevents{$destaddr}{$testtype}{"results_parsed"}."\n";
	}
    }
}



#############################################################################
sub saveTestToLocalDB($)
{
    #save result to DB's in files.
    my $results = $_[0];
    my %db;
    my $filename = createDBfilename($resIndex);
    tie( %db, "DB_File", $filename ) or die "cannot create db file";
    for my $key (keys %$results ){
	$db{$key} = $$results{$key};
    }
    untie %db;
}

#############################################################################
sub sendResults($$){
    my $results = $_[0];
    my $index = $_[1];

    my $f_success = 1; #flag to indicate error during send

    my $tuple_res = address_tuple_alloc();
    if (!$tuple_res) { warn "Could not allocate an address tuple\n"; }
    
    %$tuple_res = ( objtype   => "BGMON",
		    objname   => "ops",
		    eventtype => "RESULT"
		    , expt      => "__none"
		    , scheduler => 1
		    );

    my $notification_res = event_notification_alloc($handle,$tuple_res);
    if (!$notification_res) { warn "Could not allocate notification\n"; }


    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "linksrc",
					    $results->{sourceaddr} ) )
    { warn "Could not add attribute to notification\n"; }


    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "linkdest",
					    $results->{destaddr} ) )
    { warn "Could not add attribute to notification\n"; }
    
    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "testtype",
					    $results->{testtype} ) )
    { warn "Could not add attribute to notification\n"; }

    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "result",
					    $results->{result} ) )
    { warn "Could not add attribute to notification\n"; }

    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "tstamp",
					    $results->{tstamp} ) )
    { warn "Could not add attribute to notification\n"; }
    
    #send notification
    if (!event_notify($handle, $notification_res)) {
	warn("could not send test event notification");
	$f_success = 0;
    }


#    if (!event_notify(undef, $notification_res)) {
#	warn("could not send test event notification");
#	$f_success = 0;
#    }

    #check for successful send
    if( $f_success == 1 ){
	#delete file of event result
	print "successful send: ";
	print "$results->{testtype}=$results->{result}";
	unlink( createDBfilename($index) );
    }

    if( event_notification_free( $handle, $notification_res ) == 0 ){
	die "unable to free notification_res";
    }
}

#############################################################################
sub time_all()
{
    package main;
    require 'sys/syscall.ph';
    my $tv = pack("LL",());
    syscall( &SYS_gettimeofday, $tv, undef ) >=0
	or warn "gettimeofday: $!";
    my ($sec, $usec) = unpack ("LL",$tv);
    return $sec + ($usec / 1_000_000);
#    return time();
}

#############################################################################

sub createtmpfilename($$)
{
    return "$workingdir$_[0]-$_[1].tmp";
}

sub createDBfilename($)
{
    return "$workingdir$_[0].bgmonbdb";
}


#############################################################################

#send a dummy event to reconnect the event system if the node has lost it
sub sendBlankNotification
{
    my $tuple_res = address_tuple_alloc();
    if (!$tuple_res) { warn "Could not allocate an address tuple\n"; }
    
    %$tuple_res = ( objtype   => "NOTHING",
		    objname   => "no_name",
		    eventtype => "no_event"
		    , expt      => "__none"
		    , scheduler => 1
		    );

    my $notification_res = event_notification_alloc($handle,$tuple_res);

    #send notification
    if (!event_notify($handle, $notification_res)) {
	warn("could not send test event notification");
    }

    if( event_notification_free( $handle, $notification_res ) == 0 ){
	die "unable to free notification_res";
    }
}

#############################################################################

sub detectHang($)
{
    my ($nodeid) = @_;
    my $TIMEOUT_NUM_PER = 5;

    if( 
	$testevents{$nodeid}{bw}{flag_scheduled} == 1 &&
	time_all() > $testevents{$nodeid}{bw}{timeOfNextRun} +
	$testevents{$nodeid}{bw}{testper} * $TIMEOUT_NUM_PER )
    {
	return "bw";
    }
    
    return "nohang";
}
