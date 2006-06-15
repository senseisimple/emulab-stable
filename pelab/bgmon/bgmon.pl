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
        Notification must contain the following attributues:
dstnode = destination node of the test. Example, node10
testtype = type of test to run. Examples, latency or bw
testper  = period of time between successive tests.


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
  reporting here.)
=cut

use Getopt::Std;
use strict;
use DB_File;
use IO::Socket::INET;
use IO::Select;

$| = 1;

sub usage {
	warn "Usage: $0 [-s server] [-c cmdport] [-a ackport] [-p sendport] [-e pid/eid] -d <working_dir> [hostname]\n";
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
my $rcvBufferSize = 2048;  #max size for an incoming UDP message
my %outsockets = ();       #sockets keyed by dest node
#MARK_RELIABLE
# each result waiting to be acked has an id number and corresponding file
my $resultDBlimit = 100;
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
getopts("s:a:p:e:d:i:h",\%opt);

#if ($opt{h}) { exit &usage; }
#if (@ARGV > 1) { exit &usage; }

my ($server, $cmdport, $cmdackport, $sendport, $ackport,$expid,$workingdir,$iperfport);

if ($opt{s}) { $server = $opt{s}; } else { $server = "ops"; }
if ($opt{c}) { $cmdport = $opt{c}; } else { $cmdport = 5060; }
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

# Create a UDP socket to receive commands on
my $socket_cmd = IO::Socket::INET->new( LocalPort => $cmdport,
					Proto    => 'udp' )
    or die "Couldn't create socket on $cmdport\n";
# Create a UDP socket to receive acks on
my $socket_ack = IO::Socket::INET->new( LocalPort => $ackport,
					Proto    => 'udp' )
    or die "Couldn't create socket on $ackport\n";

print time()." creating socket\n";
my $socket_snd = IO::Socket::INET->new( PeerPort => $sendport,
					Proto    => 'udp',
					PeerAddr => $server );
print time()." end creating socket\n";

#create Select object.
my $sel = IO::Select->new();
$sel->add($socket_cmd);
$sel->add($socket_ack);


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


sub handleincomingmsgs()
{
    my $inmsg;

    #check for pending received events
    my @ready = $sel->can_read(0.1);  #wait max of 0.1 sec. Don't want to
                                      #have 0 here, or CPU usage goes high
    foreach my $handle (@ready){
	$handle->recv( $inmsg, $rcvBufferSize );
#	print "received msg: $inmsg\n";

#	my %udpin = %{ Storable::thaw $inmsg};
	my %udpin = %{ deserialize_hash($inmsg) };
	my $cmdtype = $udpin{cmdtype};
	if( !defined($cmdtype) ){
	    warn "bad message format\n";
	    return 0;  #bad message
	}
	if( $udpin{expid} ne $expid ){
	    return 0;  #not addressed to this experiment
	}
	if( $cmdtype eq "ACK" ){
	    my $index = $udpin{index};
	    print time()." Ack for index $index. Deleting backup file\n";
	    if (exists($reslist{$index})) {
		unlink($reslist{$index});
		delete($reslist{$index});
	    }
	}
	elsif( $cmdtype eq "EDIT" ){
	    my $linkdest = $udpin{dstnode};
	    my $testtype = $udpin{testtype};
	    my $testper = $udpin{testper};
	    $testevents{$linkdest}{$testtype}{"testper"} = $testper;
	    $testevents{$linkdest}{$testtype}{"flag_scheduled"} = 0;
	    $testevents{$linkdest}{$testtype}{"timeOfNextRun"} = time_all();
	    print( "EDIT:\n");
	    print( "linkdest=$linkdest\n".
		   "testype =$testtype\n".
		   "testper=$testper\n" );
	}
	elsif( $cmdtype eq "INIT" ){
	    print "INIT: ";
	    my $testtype = $udpin{testtype};
	    my @destnodes 
		= split(" ",$udpin{destnodes});
	    my $testper = $udpin{testper};
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
	elsif( $cmdtype eq "SINGLE" ){
	    my $linkdest = $udpin{dstnode};
	    my $testtype = $udpin{testtype};
	    $testevents{$linkdest}{$testtype}{"testper"} = 0;
	    $testevents{$linkdest}{$testtype}{"timeOfNextRun"} = time_all()-1;
	    $testevents{$linkdest}{$testtype}{"flag_scheduled"} = 1;
	    $testevents{$linkdest}{$testtype}{"pid"} = 0;

	    print( "SINGLE\n".
		   "linkdest=$linkdest\n".
		   "testype =$testtype\n");
	}
	elsif( $cmdtype eq "STOPALL" ){
	    print "STOPALL\n";
	    %testevents = ();
	    %waitq = ();
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

    handleincomingmsgs();

    #re-try wait Q every $subtimer_reset times through poll loop
    #try to run tests on queue
    if( $subtimer == 0 ){
	sendOldResults();

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
#			print "test $destaddr / $testtype finished\n";
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

sub sendOldResults()
{
    #
    # Check for results that could not be sent due to error. We want to wait
    # a little while though to avoid resending data that has yet to be
    # acked because the network is slow or down.
    #
    my $count    = 0;
    my $maxcount = 10;	# Wake up and send only this number at once.
    
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
#	    print "Delaying a bit before sending more old results!\n";
#	    sleep(2);
	    last;
	}
    }

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
    tie( %db, "DB_File", $filename ) or 
	eval {
	    warn time()." cannot create db file";
	    return -1;
	};
    for my $key (keys %$results ){
	$db{$key} = $$results{$key};
    }
    untie %db;

    $reslist{$index} = createDBfilename($index);
    return $index;
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
#    my $result_serial = Storable::freeze \%result ;
    my $result_serial = serialize_hash( \%result );

#    print time()." sending results\n";
    $socket_snd->send($result_serial);
#    print time()." end sending results\n";

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


#############################################################################

#
# Custom sub to turn a hash into a string. Hashes must not contain
# the substring of $separator anywhere!!!
#
sub serialize_hash($)
{
    my ($hashref) = @_;
    my %hash = %$hashref;
    my $separator = "::";
    my $out = "";

    for my $key (keys %hash){
	$out .= $separator if( $out ne "" );
	$out .= $key.$separator.$hash{$key};
    }
    return $out;
}

sub deserialize_hash($)
{
    my ($string) = @_;
    my $separator = "::";
    my %hashout;

    my @tokens = split( /$separator/, $string );

    for( my $i=0; $i<@tokens; $i+=2 ){
	$hashout{$tokens[$i]} = $tokens[$i+1];
	print "setting $tokens[$i] => $tokens[$i+1]\n";
    }
    return \%hashout;
}
