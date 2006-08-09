#!/usr/bin/perl

#TODO!! Result index numbers for ACKs?... window size 2x > buffer size???

=pod

Send a UDP message to this node, containing a Storable::freeze hash
with the following key/value pairs:

cmdtype => EDIT or INIT or SINGLE or STOPALL
and the following for each of the above cmdtypes:
EDIT:
dstnode  => plab2
testtype => latency or bw
testper  => 30
INIT:
  ** todo **
destnodes => "plab1 plab2"
SINGLE:
dstnode => plabxxx
testtype => latency or bw

where,
EDIT: Modify a specific link test to specific destination node.
INIT: Initialize the link tests with default destination nodes and frequency.
SINGLE: Perform a single link test to a specifc destination node.


TODO: ACK documentation?


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
  reporting here.) CHANGED: 8/3/06  This behavior has changed to the following:
    - regardless of the return value, parsing the results continues as normal,
      with the parsing function dealing with setting the error codes in the
      result.
=cut

use Getopt::Std;
use strict;
use DB_File;
use IO::Socket::INET;
use IO::Select;
use libwanetmon;


$| = 1;

sub usage {
	warn "Usage: $0 [-s server] [-c cmdport] [-a ackport] [-p sendport] [-e pid/eid] -d <working_dir> [hostname]\n";
	return 1;
}


#*****************************************
my $pollPer = 0.1;  #number of seconds to sleep between poll loops
my %MAX_SIMU_TESTS = (latency => "10",
		      bw      => "1");
my $cacheSendRate = 5;  #number of cached results per second
my %cacheLastSentAt;    #{indx} -> tstamp of last sent cache result
my $cacheIndxWaitPer = 5; #wait x sec between cache send attempts of same indx
my $iperfduration = 5;  #length of each iperf test in seconds
my $iperftimeout = 30;   #kill an iperf process lasting longer than this.
# percentage of testing period to wait after a test process abnormally exits
# note: 0.1 = 10%
=pod
my %TEST_FAIL_RETRY= (latency => 0.3,
		      bw      => 0.5);
=cut  
my $rcvBufferSize = 2048;  #max size for an incoming UDP message
my %outsockets = ();       #sockets keyed by dest node
#MARK_RELIABLE
# each result waiting to be acked has an id number and corresponding file
my $resultDBlimit = 100;
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

my %runningtestPIDs = ();  # maps from PID's -> array of destnode, bw

#*****************************************

my %opt = ();
getopts("s:a:p:e:d:i:h",\%opt);

#if ($opt{h}) { exit &usage; }
#if (@ARGV > 1) { exit &usage; }

my ($server, $cmdport, $cmdackport, $sendport, $ackport,$expid,$workingdir,$iperfport);

if ($opt{s}) { $server = $opt{s}; } else { $server = "ops"; }
if ($opt{c}) { $cmdport = $opt{c}; } else { $cmdport = 5052; }
if ($opt{a}) { $ackport = $opt{a}; } else { $ackport = 5050; }
if ($opt{p}) { $sendport = $opt{p}; } else { $sendport = 5051; }
if ($opt{e}) { $expid = $opt{e}; } else { $expid = "none"; }
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

# Create a TCP socket to receive commands on
my $socket_cmd = IO::Socket::INET->new( LocalPort => $cmdport,
					LocalHost => $thismonaddr,
					Proto    => 'tcp',
					Blocking => 0,
					Listen   => 1,
					ReuseAddr=> 1 )
    or die "Couldn't create socket on $cmdport\n";
# Create a UDP socket to receive acks on
my $socket_ack = IO::Socket::INET->new( LocalPort => $ackport,
					Proto    => 'udp' )
    or die "Couldn't create socket on $ackport\n";

my $socket_snd = IO::Socket::INET->new( PeerPort => $sendport,
					Proto    => 'udp',
					PeerAddr => $server );

#create Select object.
my $sel = IO::Select->new();
$sel->add($socket_cmd);
$sel->add($socket_ack);

my $subtimer_reset = 50;  # subtimer reaches 0 this many times thru poll loop
my $subtimer = $subtimer_reset;  #decrement every poll-loop.


sub handleincomingmsgs()
{
    my $inmsg;
    my $cmdHandle;

    #check for pending received events
    my @ready = $sel->can_read($pollPer);  #wait max of 0.1 sec. Don't want to
                                      #have 0 here, or CPU usage goes high
    foreach my $handle (@ready){

	my %sockIn;
	if( $handle eq $socket_ack ){
	    $handle->recv( $inmsg, $rcvBufferSize );
	    %sockIn = %{ deserialize_hash($inmsg) };
	}elsif( $handle eq $socket_cmd ){
	    my $cmdHandle = $handle->accept();
	    $inmsg = <$cmdHandle>;
	    chomp $inmsg;
	    %sockIn = %{ deserialize_hash($inmsg) };
	    if( !isMsgValid(\%sockIn) ){  return 0; }
	    print $cmdHandle "ACK\n";
	    close $cmdHandle;
	}
#	print "received msg: $inmsg\n";

	if( !isMsgValid(\%sockIn) ){ return 0; }

	my $cmdtype = $sockIn{cmdtype};	
	if( $cmdtype eq "ACK" ){
	    my $index = $sockIn{index};
	    my $tstamp =$sockIn{tstamp};

	    # delete cache entry if ACK tstamp  matches the actual stored data
	    my %db;
	    my $filename = createDBfilename();
	    tie( %db, "DB_File", $filename) or 
		eval {
		    warn time()." cannot open db file";
		    return -1;
		};
	    my %results = %{ deserialize_hash($db{$index}) };
	    print time()." Ack for index $index. Deleting cache entry\n";
	    delLocalDBEntry($index)
		if( $tstamp eq $results{tstamp} );
	}
	elsif( $cmdtype eq "EDIT" ){
	    my $linkdest = $sockIn{dstnode};
	    my $testtype = $sockIn{testtype};
	    my $testper = $sockIn{testper};
	    my $testev = \%{ $testevents{$linkdest}{$testtype} };

#	    $testev->{"limitTime"} = time_all()+10;
	    # TODO: Implement a limit on time of change
	    #  need to save "baseline" testing period
	    $testev->{"limitTime"} = $sockIn{limitTime};
	    $testev->{"testper"} = $testper;
	    $testev->{"flag_scheduled"} = 0;
	    $testev->{"timeOfNextRun"} = time_all();
	    
	    print time()." EDIT:\n";
	    print( "linkdest=$linkdest\n".
		   "testype =$testtype\n".
		   "testper=$testper\n" );
	}
	elsif( $cmdtype eq "INIT" ){
	    print time()." INIT: ";
	    my $testtype = $sockIn{testtype};
	    my @destnodes 
		= split(" ",$sockIn{destnodes});
	    my $testper = $sockIn{testper};
	    #distribute start times from offset 0 to testper/2
	    my $offsetinc = 0;
	    if( (scalar @destnodes) != 0 ){
		$offsetinc = $testper/(scalar @destnodes)/2.0;
	    }
	    my $offset = 0;
            foreach my $linkdest (@destnodes){
		my $testev = \%{ $testevents{$linkdest}{$testtype} };
		#be smart about adding tests
		# don't want to change already running tests
		# only change those tests which have been updated
		if( defined($testev->{"testper"}) &&
		    $testper == $testev->{"testper"} )
		{
		    # do nothing... keep test as it is
		}else{
		    # update test
		    $testev->{"testper"} =$testper;
		    $testev->{"flag_scheduled"} =0;
		    # TODO? be smart about when the first test should run?
		    $testev->{"timeOfNextRun"} = time_all() + $offset;
		    $offset += $offsetinc;
		}
	    }
	    print " $testtype  $testper\n";
	}
	elsif( $cmdtype eq "SINGLE" ){
	    my $linkdest = $sockIn{dstnode};
	    my $testtype = $sockIn{testtype};
	    my $testev = \%{ $testevents{$linkdest}{$testtype} };
	    $testev->{"testper"} = 0;
	    $testev->{"timeOfNextRun"} = time_all()-1;
	    $testev->{"flag_scheduled"} = 1;
	    $testev->{"pid"} = 0;

	    print( time()." SINGLE\n".
		   "linkdest=$linkdest\n".
		   "testype =$testtype\n");
	}
	elsif( $cmdtype eq "STOPALL" ){
	    print time()." STOPALL\n";
	    %testevents = ();
	    %waitq = ();
	}
	elsif( $cmdtype eq "DIE" ){
	    die "Received termination command. Exiting.\n";
	}
    }
}

#
#  MAIN POLL LOOP
#    check for newly sent commands, checks for finished tests,
#    starts new tests, etc..
#
while (1) {

    $subtimer--;

    sendOldResults();

    handleincomingmsgs();

    #re-try wait Q every $subtimer_reset times through poll loop
    #try to run tests on queue
    if( $subtimer == 0 ){	

	foreach my $testtype (keys %waitq){

	    if( scalar(@{$waitq{$testtype}}) != 0 ){
		print time_all()." $testtype QUEUE: ";
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

    #iterate through all outstanding running tests
    #mark flags of finished tests
    foreach my $pid (keys %runningtestPIDs){
	use POSIX ":sys_wait_h";

	#get nodeid and testtype
	my $destaddr = $runningtestPIDs{$pid}[0];
	my $testtype = $runningtestPIDs{$pid}[1];
	my $testev = \%{ $testevents{$destaddr}{$testtype} };

	my $kid = waitpid( $pid, &WNOHANG );
	if( $kid == $pid )
	{
	    # process is dead

	    $testev->{"flag_finished"} = 1;
	    $testev->{"pid"} = 0;

=pod        #NOT USED ANYMORE
	    if( $testtype eq "bw" && $? != 0 ){
		# IPERF SPECIFIC... exited with error
		#process exited abnormally
		#reset pid
		$testev->{"pid"} = 0;
		#schedule next test at a % of a normal period from now
		my $nextrun = time_all() + 
		    $testev->{"testper"} *
			$TEST_FAIL_RETRY{$testtype};
		$testev->{"timeOfNextRun"} = $nextrun;
		#delete tmp filename
		my $filename = createtmpfilename($destaddr, $testtype);
		unlink($filename) or warn "can't delete temp file";
		
		# TODO - send errors to find down paths
		my %results = 
		    ("sourceaddr" => $thismonaddr,
		     "destaddr" => $destaddr,
		     "testtype" => $testtype,
		     "result" => $ERRID{unknown},
		     "tstamp" => $testev->{"tstamp"},
		     "magic"  => "$magic",
		     );
		
		#save result to local DB
		my $index = saveTestToLocalDB(\%results);
		#send result to remote DB
		sendResults(\%results, $index);
		
		print time_all().
		    " $testev->{testtype} proc $pid exited abnormally\n";
	    }
	    else{
		#process finished, so mark it's "finished" flag
		$testev->{"flag_finished"} = 1;
		$testev->{"pid"} = 0;
#			print "test $destaddr / $testtype finished\n";
	    }
=cut

	    delete $runningtestPIDs{$pid};
	}

	#If this process is bandwidth, kill it if it has
	#      been running too long (iperf has a looong timeout)
	elsif( $testtype eq "bw" &&
	       time_all() >
	       $testev->{"tstamp"} + 
	       $iperftimeout )
	{
	    kill 'TERM', $pid;
	    delete $runningtestPIDs{$pid};
	    print time_all()." bw timeout: killed $destaddr, ".
		"pid=$pid\n";

	    $testev->{"pid"} = 0;
	    $testev->{"flag_scheduled"} = 0;
	    #delete tmp filename
	    my $filename = createtmpfilename($destaddr, $testtype);
	    unlink($filename) or warn "can't delete temp file";
	    my %results = 
		("sourceaddr" => $thismonaddr,
		 "destaddr" => $destaddr,
		 "testtype" => $testtype,
		 "result" => $ERRID{timeout},
		 "tstamp" => $testev->{"tstamp"},
		 "magic"  => "$magic",
		 );
	    #save result to local DB
	    my $index = saveTestToLocalDB(\%results);
	    #send result to remote DB
	    sendResults(\%results, $index);
	}

    }

    #iterate through all event structures
    for my $destaddr ( keys %testevents ) {
	for my $testtype ( keys %{ $testevents{$destaddr} } ){

	    my $testev = \%{ $testevents{$destaddr}{$testtype} };

	    #check for finished events
	    if( $testev->{"flag_finished"} == 1 ){
		#read raw results from temp file
		my $filename = createtmpfilename($destaddr, $testtype);
		open FILE, "< $filename"
		    or warn "can't open file $filename";
	        my @raw_lines = <FILE>;
		my $raw;
		foreach my $line (@raw_lines){
		    $raw = $raw.$line;
		}
		close FILE;
		unlink($filename) or die "can't delete temp file";
		#parse raw data
		my $parsedData = parsedata($testtype,$raw);
		$testev->{"results_parsed"} = $parsedData;

		my %results = 
		    ("sourceaddr" => $thismonaddr,
		     "destaddr" => $destaddr,
		     "testtype" => $testtype,
		     "result" => $parsedData,
		     "tstamp" => $testev->{"tstamp"},
		     "magic"  => "$magic",
		     "ts_finished" => time()
		     );

		#MARK_RELIABLE
		#save result to local DB
		my $index = saveTestToLocalDB(\%results);

		#send result to remote DB
		sendResults(\%results, $index);

		#reset flags
		$testev->{"flag_finished"} = 0;
		$testev->{"flag_scheduled"} = 0;
	    }

	    #schedule new tests
	    if( $testev->{"flag_scheduled"} == 0 && $testev->{"testper"} > 0 )
	    {
		if( time_all() < 
		    $testev->{"timeOfNextRun"} + $testev->{"testper"} )
		{
		    #if time of next run is in the future, set it to that
		    $testev->{"timeOfNextRun"} += $testev->{"testper"};
		}else{
		    #if time of next run is in the past, set to current time
		    $testev->{"timeOfNextRun"} 
		      = time_all();
		}

		$testev->{"flag_scheduled"} = 1;
	    }

#	    print "nextrun="
#		.$testevents{$destaddr}{$testtype}{"timeOfNextRun"}."\n";
#	    print "scheduled?=".$testevents{$destaddr}{$testtype}{"flag_scheduled"}.
		"\n";
#	    print "pid=".$testevents{$destaddr}{$testtype}{"pid"}."\n";

	    #check for new tests ready to run
	    if( $testev->{"timeOfNextRun"} <= time_all() &&
		$testev->{"flag_scheduled"} == 1 && 
		$testev->{"pid"} == 0 )
	    {
		#run test
		spawnTest( $destaddr, $testtype );
	    }

	}#end loop

	# may not be needed, but may help detect errors
	my $hangres = detectHang($destaddr);
	if( $hangres eq "bw" ){
	    print "HANG: $hangres,  $destaddr\n";
	    # reset time of next run
	    $testevents{$destaddr}{bw}{"timeOfNextRun"} = time_all();
	}
    }

    if( $subtimer == 0 ){
	$subtimer = $subtimer_reset;
    }
}


my $iterSinceLastRun = 0;  #when cacheSendRate < 1/pollPer, only send old
                           # results a fraction of the number of calls to
                           # sendOldResults
sub sendOldResults()
{
    #
    # Check for results that could not be sent due to error. We want to wait
    # a little while though to avoid resending data that has yet to be
    # acked because the network is slow or down.
    #
    my $count    = 0;
    	# Wake up and send only this number at once.
    my $maxcount = int( $pollPer*$cacheSendRate );

    my %db;
    my $filename = createDBfilename();
    tie( %db, "DB_File", $filename) or 
	eval {
	    warn time()." cannot open db file";
	    return -1;
	};

    my @ids = keys %db;
   
    $iterSinceLastRun++;
    while( ($count < $maxcount && $count < scalar(@ids)) 
	|| ($iterSinceLastRun > (1/($pollPer*$cacheSendRate))) )
    {
	if( scalar(@ids) == 0 ){
	    last;
	}
	my $index = $ids[int( rand scalar(@ids) )];
	my %results = %{ deserialize_hash($db{$index}) };
	next if( (defined $cacheLastSentAt{$index})  &&
		 time() - $cacheLastSentAt{$index} < 
		 $cacheIndxWaitPer );

	if (!exists($results{"magic"}) || $results{"magic"} ne $magic) {
	    # Hmm, something went wrong!
	    print "Old results for index $index are scrogged; deleting!\n";
	    delLocalDBEntry($index);
	    @ids = keys %db;
#	    next;
	}

	$count++;
	$iterSinceLastRun = 0;

	#don't send recently completed tests
	next if( time() - $results{ts_finished} < 10 );
	print "sending old result:  $index\n";
	sendResults(\%results, $index);
	# TODO: set last tstamp here in cacheLastSentAt
	$cacheLastSentAt{$index} = time();
    }


    untie %db;

    #TODO?: combine re-sends into a large message?

}

#############################################################################


exit(0);



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
	  $runningtestPIDs{$pid} = [$linkdest, $testtype];
	  
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
	      #one ping, using fping (2 total attempts, 10 sec timeout between)
	      exec "sudo $workingdir".
		  "fping -t10000 -s -r1 $linkdest >& $filename"
#	      exec "ping -c 1 -w 60 $linkdest >$filename"
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
	# for fping results parsing, using -s option.
	# Note, these must be in this order!


	# TODO: get these regex's to work as OR logic...
=pod
	if(
	    /ICMP Network Unreachable/ ||
	    /ICMP Host Unreachable from/ ||
	    /ICMP Protocol Unreachable/ ||
	    /ICMP Port Unreachable/ ||
	    /ICMP Unreachable/
=cut
	if( /^ICMP / )
	{
	    $parsed = $ERRID{ICMPunreachable};
	}elsif( /address not found/ ){
	    $parsed = $ERRID{unknownhost};
	}elsif( /2 timeouts/ ){
	    $parsed = $ERRID{timeout};
	}elsif( /[\s]+([\S]*) ms \(avg round trip time\)/ ){
	    $parsed = "$1" if( $1 ne "0.00" );
	}else{
	    $parsed = $ERRID{unknown};
	}
=pod
        #this section of parsing is for linux ping hosts.
	if(     /100\% packet loss/ ){
	    $parsed = $ERRID{timeout};
	}elsif( /Time to live exceeded/ ){ # ?
	    $parsed = $ERRID{ttlexceed};
	}elsif( /unknown host/ ){
	    $parsed = $ERRID{unknownhost};
	}elsif( /time=(.*)\s?ms/ ){
	    $parsed = "$1";
	}else{
	    $parsed = $ERRID{unknown};
	}
        # This section of parsing is a bit wrong.. for freebsd hosts??
	if( /0 packets received/ ){
	    $parsed = $ERRID{timeout};
	}elsif( /Time to live exceeded/ ){
	    $parsed = $ERRID{ttlexceed};
	}elsif( /time=(.*)\s?ms/ ){
	    $parsed = "$1";
	}else{
	    $parsed = $ERRID{unknown};
	}
=cut
    }elsif( $type eq "bw" ){
	if(    /connect failed: Connection timed out/ ){
	    # this one shouldn't happen, if the timeout check done by
            # bgmon is set low enough.
	    $parsed = $ERRID{timeout};
	}elsif( /write1 failed:/ ){
	    $parsed = $ERRID{iperfHostUnreachable};
	}elsif( /error: Name or service not known/ ){
	    $parsed = $ERRID{unknownhost};
	}elsif( /\s+(\S*)\s+([MK])bits\/sec/ ){
	    $parsed = $1;
	    if( $2 eq "M" ){
		$parsed *= 1000;
	    }
	}else{
	    $parsed = $ERRID{unknown};
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
    my ($results) = @_;
    
    my %db;
    my $filename = createDBfilename();
    tie( %db, "DB_File", $filename ) or 
	eval {
	    warn time()." cannot create db file";
	    return -1;
	};

    #
    # Find an unused index. Leave zero unused to indicate we ran out.
    #
    my $index;
    for ($index = 1; $index < $resultDBlimit; $index++) {
	last
	    if( !defined($db{$index}) );
    }
    return 0
	if ($index == $resultDBlimit);

    
    $db{$index} = serialize_hash($results);
    untie %db;

    return $index;
}

sub delLocalDBEntry($)
{
    my ($index) = @_;
    my %db;
    my $filename = createDBfilename();
    tie( %db, "DB_File", $filename) or 
	eval {
	    warn time()." cannot open db file";
	    return -1;
	};    

    if( defined $db{$index} ){
	delete $db{$index};
    }

    untie %db;
}

#############################################################################
sub sendResults($$){
    my ($results, $index) = @_;

    my %result = ( expid    => $expid,
		   linksrc  => $results->{sourceaddr},
		   linkdest => $results->{destaddr},
		   testtype => $results->{testtype},
		   result   => $results->{result},
		   tstamp   => $results->{tstamp},
		   index    => $index );

    my $result_serial = serialize_hash( \%result );

    $socket_snd->send($result_serial);
}


#############################################################################

sub createtmpfilename($$)
{
    return "$workingdir$_[0]-$_[1].tmp";
}

sub createDBfilename()
{
#    return "$workingdir$_[0].bgmonbdb";
    return "$workingdir"."cachedResults.bdb";
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


sub isMsgValid(\%)
{
    my ($hashref) = @_;
    my %msgHash = %$hashref;
    if( !defined($msgHash{cmdtype}) ){
	warn "bad message format\n";
	return 0;  #bad message
    }
    if( $msgHash{expid} ne $expid ){
	return 0;  #not addressed to this experiment
    }
    return 1;
}

