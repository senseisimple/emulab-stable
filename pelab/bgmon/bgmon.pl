#!/usr/bin/perl

#TODO!! Result index numbers... window size 2x > buffer size???
# (1/30/06) No Ack's, just trust that sent data gets there.
# Q: how to determine if OPS receives it correctly?
#    (1/31) Just checking if "send notification" is successful
#
# (2/3/06): CLEANUP ALLOCATED MEMORY using .._free() functions
# (3/20/06): Look into usefullness of "nohang()"... did not get tested,
#            and may not be needed

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
  STOPALL: stop all tests, running or pending.

OPERATION NUANCES:
- If a ping test is scheduled to have a period shorter than the normal
  latency to the destination, pings will be run sequentially one after
  the other. There will be no simultaneous tests of the same type to the same
  destination.
- If a path latency is longer than 60 seconds, an error value is reported
  instead of a latency (milliseconds) value. See %ERRID
- If a testing process abnormally exits with a value not 0 (say, iperf dies),
  then the test is rescheduled to be run at a future time. This time is
  the normal period times the %TEST_FAIL_RETRY ratio. (TODO: add error
  reporting here.)
=cut

use lib '/usr/testbed/lib';
use event;
use Getopt::Std;
use strict;
use DB_File;
use Socket;

sub usage {
	warn "Usage: $0 [-s server] [-p port] [-e pid/eid] -d <working_dir> [hostname]\n";
	return 1;
}


#*****************************************
my %MAX_SIMU_TESTS = (latency => "10",
		      bw      => "1");
my $iperfduration = 10;
my $iperftimeout = 30;   #kill an iperf process lasting longer than this.
# percentage of testing period to wait after a test process abnormally exits
# note: 0.1 = 10%
my %TEST_FAIL_RETRY= (latency => 0.3,
		      bw      => 0.1);
#MARK_RELIABLE
# each result waiting to be acked has an id number and corresponding file
my $resultDBlimit = 1000;
my %reslist       = ();
my $magic         = "0xDeAdBeAf";

my %testevents = ();

#queue for each test type containing tests that need to
# be run, but couldn't, due to MAX_SIMU_TESTS.
# keys are test types, values are lists
# This separate Q, which has higher priority than the normal %testevents
# should solve "starvation" issues if an "early" test blocks for too long
# such as during a network timeout with iperf
my %waitq = ( latency => [],
	      bw      => [] );

my %ERRID;
$ERRID{timeout} = -1;
$ERRID{ttlexceed} = -2;
$ERRID{unknown} = -3;

#*****************************************

my %opt = ();
getopts("s:p:e:d:i:h",\%opt);

#if ($opt{h}) { exit &usage; }
#if (@ARGV > 1) { exit &usage; }

my ($server,$port,$evexpt,$workingdir,$iperfport);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }
if ($opt{e}) { $evexpt = $opt{e}; } else { $evexpt = "__none"; }
if ($opt{d}) { $workingdir = $opt{d}; `cd $workingdir`; }
if ($opt{i}) { $iperfport = $opt{i}; } else { $iperfport = 5002; }

my $thismonaddr;
if( defined  $ARGV[0] ){
    $thismonaddr = $ARGV[0];
}else{
    $_ = `cat /var/emulab/boot/nodeid`;
    /plabvm(\d+)-/;
    $thismonaddr = "plab$1";
}
print "thismonaddr = $thismonaddr\n";

print "server=$server\n";

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }
#TOTAL HACK - bug in event library: after unregister, segfault if assigning
#  a val to $handle
my @handles =();
push @handles, event_register($URL,0);
my $handle = $handles[$#handles];

if (!$handle) { die "Unable to register with event system\n"; }
print "registered with $server\n";
my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }

%$tuple = ( host      => $event::ADDRESSTUPLE_ALL,
	    objtype   => "BGMON",
	    objname   => $thismonaddr,
            expt      => $evexpt);

if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	die "Could not subscribe to event\n";
}

# This is for our ack from ops.
$tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }

%$tuple = ( objname   => "ops",
	    eventtype => "ACK",
	    expt      => "__none",
	    objtype   => "BGMON");

if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	die "Could not subscribe to ack event\n";
}

#this call will reconnect event system if it has failed
sendBlankNotification();

#############################################################################
# Note a difference from tbrecv.c - we don't yet have event_main() functional
# in perl, so we have to poll. (Nothing special about the select, it's just
# a wacky way to get usleep() )
#############################################################################
#main()

#
# At startup, look for any old results that did not get acked. Add them to
# the reslist so they get resent below.
#
for (my $i = 0; $i < $resultDBlimit; $i++) {
    if (-e createDBfilename($i)) {
	$reslist{$i} = createDBfilename($i);
    }
}

my $subtimer_reset = 100;  # subtimer reaches 0 this many times thru poll loop
my $subtimer = $subtimer_reset;  #decrement every poll-loop.


while (1) {

    $subtimer--;

    #check for pending received events
    eval{
	event_poll_blocking($handle,100);
	die "forced dieing..\n";
    };
    if( $@ ){
	#event_poll died... reconnect
	print "event_poll had a fatal error:$@\n";
#	if( $subtimer == 0 ){
	    reconnect_eventsys();
#	}
    }

    #re-try wait Q every $subtimer_reset times through poll loop
    #try to run tests on queue
    if( $subtimer == 0 ){
	foreach my $testtype (keys %waitq){

	    if( scalar(@{$waitq{$testtype}}) != 0 ){
		print time_all()." $testtype ";
		foreach my $node (@{$waitq{$testtype}}){
		    print "$node ";
		}
		print "\n";
	    }
	    #
	    # Run next scheduled test
	    spawnTest( undef, $testtype );
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
		my $kid = waitpid( $pid, &WNOHANG );
		if( $kid != 0 )
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
			unlink($filename) or warn "can't delete temp file";
		    }
		}

		#TODO: if this process is bandwidth, kill it if it has
                #      been running too long (iperf has a looong timeout)
		elsif( $testtype eq "bw" &&
			 time_all() >
			 $testevents{$destaddr}{$testtype}{"tstamp"} + 
			 $iperftimeout )
		{
		    kill 'TERM', $pid;
		    print time_all()." killed $destaddr, pid=$pid\n";
		}


	    }

	    #check for finished events
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
		     "tstamp" => $testevents{$destaddr}{$testtype}{"tstamp"},
		     "magic"  => "$magic",
		     );
		#MARK_RELIABLE
		#save result to local DB
		my $index = saveTestToLocalDB(\%results);
		#send result to remote DB
		sendResults(\%results, $index);

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
#WRONG?	    if( scalar(@{$waitq{bw}}) == 0 &&
	    if(
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
	if( $hangres eq "bw" ){
	    print "HANG: $hangres,  $destaddr\n";
	    # reset time of next run
	    $testevents{$destaddr}{bw}{"timeOfNextRun"} = time_all();
	}
    }

    #
    # Check for results that could not be sent due to error. We want to wait
    # a little while though to avoid resending data that has yet to be
    # acked because the network is slow or down.
    #
    my $count    = 0;
    my $maxcount = 5;	# Wake up and send only this number at once.
    
    for (my $index = 0; $index < $resultDBlimit; $index++) {
	next
	    if (!exists($reslist{$index}));
	
	my $filename = $reslist{$index};

	if (! -e $filename) {
	    # Hmm, something went wrong!
	    delete($reslist{$index});
	    next;
	}

	# Stat file to get create time.
	my (undef,undef,undef,undef,undef,undef,undef,undef,
	    undef,undef,$ctime) = stat($filename);

	next
	    if ((time() - $ctime) < 10);

	#resend
	my %results;
	my %db;
	tie(%db, "DB_File", $filename) 
	    or die "cannot open db file";
	for my $key (keys %db ){
	    $results{$key} = $db{$key};
	}
	untie(%db);

	# Verify results in case the file was scrogged.
	if (!exists($results{"magic"}) || $results{"magic"} ne $magic) {
	    # Hmm, something went wrong!
	    print "Old results for index $index are scrogged; deleting!\n";
	    delete($reslist{$index});
	    unlink($filename);
	    next;
	}
	sendResults(\%results, $index);
	sleep(1);
	$count++;
	if ($count > $maxcount) {
	    print "Delaying a bit before sending more old results!\n";
	    sleep(2);
	    last;
	}
    }

    if( $subtimer == 0 ){
	$subtimer = $subtimer_reset;
    }
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

	# Ack from ops.
	if ($eventtype eq "ACK") {
	    my $index = event_notification_get_string($handle,
						      $notification,
						      "index");
	    
	    print "Ack for index $index. Deleting backup file\n";
	    if (exists($reslist{$index})) {
		unlink($reslist{$index});
		delete($reslist{$index});
	    }
	    return;
	}

	#change values and/or initialize
	if( $eventtype eq "EDIT" ){

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

	    print( "EDIT:\n");
	    print( "linkdest=$linkdest\n".
		   "testype =$testtype\n".
		   "testper=$testper\n" );
	}
	elsif( $eventtype eq "INIT" ){
	    print "INIT: ";
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
		#be smart about adding tests
		# don't want to change already running tests
		# only change those tests which have been updated
		if( defined($testevents{$linkdest}{$testtype}{"testper"}) &&
		    $testper == $testevents{$linkdest}{$testtype}{"testper"} )
		{
		    # do nothing... keep test as it is
		}else{
		    # update test
		    $testevents{$linkdest}{$testtype}{"testper"} =$testper;
		    $testevents{$linkdest}{$testtype}{"flag_scheduled"} =0;
		    # TODO? be smart about when the first test should run?
		    $testevents{$linkdest}{$testtype}{"timeOfNextRun"} =
			time_all();
		}
	    }
	    print " $testtype  $testper\n";
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

	# Should this be freed here? It segfaults if it is...
#	if( event_notification_free( $handle, $notification ) == 0 ){
#	    die "Unable to free notification";
#	}
}

#############################################################################

#
# Run a test with the oldest destination on the Q.
#  destination given in parameters is added to Q.
#  if undef is given for destination, just run dest at head of Q.
#
sub spawnTest($$)
{
    my ($linkdest, $testtype) = @_;
    
    use Errno qw(EAGAIN);

    #
    #Add to queue if it doesn't exist already
    #  this seach is inefficient... use a hash? sparse array? sorted list?
    my $flag_duplicate = 0;
    if( $linkdest ne undef ){
	foreach my $element ( @{$waitq{$testtype}} ){
	    if( $element eq $linkdest ){
		$flag_duplicate = 1;
	    }
	}
	if( $flag_duplicate == 0 ){
	    unshift @{$waitq{$testtype}}, $linkdest;
#	    print time_all()." added $linkdest to Q $testtype\n";
	}
    }

    #exit and don't fork if the max number of tests is already being run
    if( getRunningTestsCnt($testtype) >= $MAX_SIMU_TESTS{$testtype} ){
	return -1;
    }

    #set the destination to be head of Q.
    if( scalar @{$waitq{$testtype}} == 0 ){
	return 0;
    }
    $linkdest = pop @{$waitq{$testtype}};
    print time_all()." running $linkdest / $testtype\n";

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
	      # one ping, with timeout of 60 sec
	      exec "ping -c 1 -t 60 $linkdest >$filename"
		  or die "can't exec: $!\n";
	  }elsif( $testtype eq "bw" ){
	      #command line for "BANDWIDTH TEST"
#	      print "###########bwtest\n";
	      exec "$workingdir".
		  "iperf -c $linkdest -t $iperfduration -p $iperfport >$filename"
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
    return 0;
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
	if( /time=(.*) ms/ ){
	    $parsed = "$1";
	}elsif( /0 packets received/ ){
	    $parsed = $ERRID{timeout};
	}elsif( /Time to live exceeded/ ){
	    $parsed = $ERRID{ttlexceed};
	}else{
	    $parsed = $ERRID{unknown};
	}
	
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
#MARK_RELIABLE
sub saveTestToLocalDB($)
{
    #
    # Find an unused index. Leave zero unused to indicate we ran out.
    #
    my $index;
    
    for ($index = 1; $index < $resultDBlimit; $index++) {
	last
	    if (!exists($reslist{$index}));
    }
    return 0
	if ($index == $resultDBlimit);

    #save result to DB's in files.
    my $results = $_[0];
    my %db;
    my $filename = createDBfilename($index);
    tie( %db, "DB_File", $filename ) or die "cannot create db file";
    for my $key (keys %$results ){
	$db{$key} = $$results{$key};
    }
    untie %db;

    $reslist{$index} = createDBfilename($index);
    return $index;
}

#############################################################################
sub sendResults($$){
    my $results = $_[0];
    my $index = $_[1];

    my $tuple_res = address_tuple_alloc();
    if (!$tuple_res) { warn "Could not allocate an address tuple\n"; }
    
    %$tuple_res = ( objtype   => "BGMON",
		    objname   => "ops",
		    eventtype => "RESULT"
		    , expt      => $evexpt
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

    if( 0 == event_notification_put_string( $handle,
					    $notification_res,
					    "index",
					    "$index" ) )
    { warn "Could not add attribute to notification\n"; }

    print "Sending results to ops. Index: $index\n";

    if (!event_notify($handle, $notification_res)) {
	warn("could not send test event notification");
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
		    , expt      => $evexpt
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


sub reconnect_eventsys()
{
    print time_all()." Reconnecting to event system\n";
    print "handle = $handle\n";
    print "unregistering\n";
    eval{
	if (event_unregister(pop @handles) == 0) {
	    die "Unable to unregister with event system\n";
	};
    };
    if( $@ ){
	print "unsubscribe had a fatal error:$@\n";
    }

    eval{

	print "registering\n";
	my $tmp = event_register($URL,0);
#	push @handles, event_register($URL,0);
	print "registered\n";
	$handle = $handles[$#handles];
	if (!$handle) { die "Unable to register with event system\n"; }

	print "allocating tuple\n";
	$tuple = address_tuple_alloc();
	if (!$tuple) { die "Could not allocate an address tuple\n"; }
	%$tuple = ( host      => $event::ADDRESSTUPLE_ALL,
		    objtype   => "BGMON",
		    objname   => $thismonaddr,
		    expt      => $evexpt);
	print "subscribing to control events\n";
	if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	    die "Could not subscribe to event\n";
	}

	$tuple = address_tuple_alloc();
	%$tuple = ( objname   => "ops",
		    eventtype => "ACK",
		    expt      => "__none",
		    objtype   => "BGMON");
	print "subscribing to ACK events\n";
	if (!event_subscribe($handle,\&callbackFunc,$tuple)) {
	    die "Could not subscribe to event\n";
	}
    };
    if( $@ ){
	print "re-register had a fatal error:$@\n";
    }

    print "exiting reconnect\n";
}
