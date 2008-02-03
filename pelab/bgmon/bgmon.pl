#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#

=pod

Send a UDP message to this node, containing a 
libwanetmon::serialize_hash() hash
with the following key/value pairs:

cmdtype => one of the following types, and associated key/values
           (examples given)
EDIT:
dstnode  => plab2
testtype => latency or bw
testper  => 30
INIT:
  ** todo **
destnodes => "plab1 plab2"
SINGLE: ** DEPRECIATED **
dstnode => plabxxx
testtype => latency or bw
STOPALL:
STATUS: *TODO*

where,
EDIT: Modify a specific link test to specific destination node.
INIT: Initialize the link tests with default destination nodes and frequency.
    Measurements to nodes not in init set but already scheduled 
    will not be erased or changed.

SINGLE ** DEPRECIATED **: Perform a single link test to a specifc destination node.
STOPALL: Erase all scheduled measurements.
STATUS: get status of bgmon as a string description. In the reply,
        the message contains "ACK\n" followed by the status string terminated
        by  "\n"

*TODO* ACK documentation?


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
- If a node/sliver is rebooted, our startup script should run and re-start
  bgmon. However, the current state of running tests is lost (on purpose!).
  The management application can query the status using the STATUS command
=cut

use Getopt::Std;
use strict;
use DB_File;
use IO::Socket::INET;
use IO::Select;
use libwanetmon;
use Cmdqueue;

$| = 1;

sub usage {
        warn "Usage: $0 [-s server] [-c cmdport] [-a ackport] [-p sendport] [-e pid/eid] <-d working_dir> [hostname]\n";
        return 1;
}

#
# Continuous ping hack
#
my $CONT_THRESHOLD = 2;		# if period < this value (sec), go continuous

#*****************************************
my $pollPer = 0.1;  #number of seconds to sleep between poll loops
my %MAX_SIMU_TESTS = (latency => "10",
                      bw      => "1",
                      outage  => "10");
my $cacheSendRate = 5;  #number of cached results per second
my %cacheLastSentAt;    #{indx} -> tstamp of last sent cache result
my $cacheIndxWaitPer = 5; #wait x sec between cache send attempts of same indx
my $iperfduration = 5;  #length of each iperf test in seconds
my $iperftimeout = 30;   #kill an iperf process lasting longer than this.
my $fpingTimeoutDef = 10000; #milliseconds for fping to timeout
my $outManID = "outageMan"; #name to use for ManagerID when an outage is detctd
my $outDet_maxPastSuc = 12; #consider a path for outage detection if a valid
                            #result appeared within this many hours in the past
my %outTestFreq = ( high  => 1,
                    med   => 10,
                    low   => 600,
                    outageEnd=> 10 );
my %outTestDur =  ( high  => 60,
                    med   => 60,
                    low   => 0,
                    outageEnd=> 120 );

=pod
# percentage of testing period to wait after a test process abnormally exits
# note: 0.1 = 10%
my %TEST_FAIL_RETRY= (latency => 0.3,
                      bw      => 0.5);
=cut  
my $rcvBufferSize = 2048;  #max size for an incoming UDP message
my %outsockets = ();       #sockets keyed by dest node
#MARK_RELIABLE
# each result waiting to be acked has an id number and corresponding file
my $resultDBlimit = 12000;  #approx 24 hours of data at "normal" test rates
my $magic         = "0xDeAdBeAf";

my %testevents = ();
#queue for each test type containing tests that need to
# be run.
my %waitq = ( latency => [],
              bw      => [] );
my %runningtestPIDs = ();  # maps from PID's -> array of destnode, bw
my $status;  #keeps track of status of this bgmon. Possible values are:
             # anyscheduled_no, anyscheduled_yes
$status = "anyscheduled_no";

#*****************************************
# PROTOTYPES
sub sendOldResults;
sub spawnTest($$);
sub getRunningTestsCnt($);
sub parsedata($$$);
sub printTimeEvents;
sub saveTestToLocalDB($);
sub delLocalDBEntry($);
sub sendResults($$);
sub createtmpfilename($$);
sub createDBfilename();
sub detectHang($);
sub isMsgValid(\%);
sub updateTestEvent($);
sub addCmd($$);
sub updateOutageState($);
sub updateOutageState_lossCheck($);
sub initTestEv($$);
sub getstats;
sub diffstats($$);
sub printstats($$);
#*****************************************

my %opt = ();
getopts("s:c:a:p:e:d:i:hC",\%opt);

#if ($opt{h}) { exit &usage; }
#if (@ARGV > 1) { exit &usage; }

my ($server, $cmdport, $cmdackport, $sendport, $ackport,$expid,$workingdir,$iperfport, $nocontinuous);

if ($opt{s}) { $server = $opt{s}; } else { $server = "ops"; }
if ($opt{c}) { $cmdport = $opt{c}; } else { $cmdport = 5052; }
if ($opt{a}) { $ackport = $opt{a}; } else { $ackport = 5050; }
if ($opt{p}) { $sendport = $opt{p}; } else { $sendport = 5051; }
if ($opt{e}) { $expid = $opt{e}; } else { $expid = "none"; }
if ($opt{d}) { $workingdir = $opt{d}; chdir $workingdir; }
if ($opt{i}) { $iperfport = $opt{i}; } else { $iperfport = 5002; }
if ($opt{C}) { $nocontinuous = 1; }

my $debug = 1;

my $thismonaddr;
if( defined  $ARGV[0] ){
    $thismonaddr = $ARGV[0];
}else{
    $_ = `cat /var/emulab/boot/nodeid`;
    /plabvm(\d+)-/;
    if( defined $1 ){
        $thismonaddr = "plab$1" ;
    }else{
        $thismonaddr = `hostname`;
        chomp $thismonaddr;
    }
}
print "thismonaddr = $thismonaddr\n";
print "command-port = $cmdport\n";
print "result send port = $sendport\n";
print "ackport = $ackport\n";

# initialize seed for random number generation
srand (time ^ unpack "%L*", `uname -a`);

# Create a TCP socket to receive commands on
my $socket_cmd = IO::Socket::INET->new( LocalPort => $cmdport,
#TODO: added this as comment, in case it breaks: LocalHost => $thismonaddr,
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

my $subtimer_reset = 10;  # subtimer reaches 0 this many times thru poll loop
my $subtimer = $subtimer_reset;  #decrement every poll-loop.


sub handleincomingmsgs()
{
    my $inmsg;
    my $cmdHandle;

    my @ready = $sel->can_read($pollPer);
    foreach my $handle (@ready){
        my %sockIn;
        if( $handle eq $socket_ack ){
            $handle->recv( $inmsg, $rcvBufferSize );
            %sockIn = %{ deserialize_hash($inmsg) };
        }elsif( $handle eq $socket_cmd ){
            $cmdHandle = $handle->accept();
            $inmsg = <$cmdHandle>;
            chomp $inmsg;
            %sockIn = %{ deserialize_hash($inmsg) };
            if( !isMsgValid(%sockIn) ){
                close $cmdHandle;
                return 0; 
            }
            print $cmdHandle "ACK\n";
            
        }
#       print "received msg: $inmsg\n";

        if( !isMsgValid(%sockIn) ){ return 0; }

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
#            print time()." Ack for index $index. Deleting cache entry\n";
            print "rcved ACK, but UDP/tstamp not defined\n" 
                if( !defined $tstamp );
            print "rcved ACK, but dbfile/tstamp not defined\n" 
                if( !defined $results{tstamp} );
            delLocalDBEntry($index)
                if( $tstamp eq $results{tstamp} );
        }
        elsif( $cmdtype eq "EDIT" ){
            my $linkdest = $sockIn{dstnode};
            my $testtype = $sockIn{testtype};
            my $newtestper = $sockIn{testper};
            my $duration =$sockIn{duration};
            my $managerID=$sockIn{managerID};
            initTestEv($linkdest,$testtype);
            my $testev = \%{ $testevents{$linkdest}{$testtype} };

#            print time()." EDIT:\n";
#            print( "linkdest=$linkdest\n".
#                   "testype =$testtype\n".
#                   "testper=$newtestper\n" );
#            print "duration=".$sockIn{duration}."\n" 
#                if( defined $sockIn{duration} );

            #add new cmd to queue
            addCmd( $testev, Cmd->new($managerID, $newtestper, $duration) );
        }
        elsif( $cmdtype eq "INIT" ){
            print time()." INIT: ";
            my $testtype = $sockIn{testtype};
            my @destnodes 
                = split(" ",$sockIn{destnodes});
#           my $testper = $sockIn{testper};
            my $newtestper = $sockIn{testper};
            my $duration =$sockIn{duration};
            my $managerID=$sockIn{managerID};
            
            #distribute start times from offset 0 to testper/2
#TODO: HANDLE OFFSET SOMEWHERE!!
            my $offsetinc = 0;
            if( (scalar @destnodes) != 0 ){
                $offsetinc = $newtestper/(scalar @destnodes)/2.0;
            }
            my $offset = 0;
            foreach my $linkdest (@destnodes){
                initTestEv($linkdest,$testtype);
                my $testev = \%{ $testevents{$linkdest}{$testtype} };
                #add new cmd to queue
                addCmd( $testev, 
                        Cmd->new($managerID, $newtestper, $duration) );
                $offset += $offsetinc;
            }
            print " $testtype  $newtestper\n";
        }
        elsif( $cmdtype eq "STOPALL" ){
            print time()." STOPALL\n";
            my $managerID=$sockIn{managerID} if( defined $sockIn{managerID} );

            #delete those entries of this managerID
            for my $destaddr ( keys %testevents ) {
                for my $testtype ( keys %{ $testevents{$destaddr} } ){
                    my $testev = \%{ $testevents{$destaddr}{$testtype} };
                    #update scheduled tests in case something changed
                    if( defined $testev->{cmdq} ){
                        $testev->{cmdq}->rmCmds( $managerID );
                        updateTestEvent( $testev );
                    }
                }
            }
        }
        elsif( $cmdtype eq "DIE" ){
            die "Received termination command. Exiting.\n";
        }
        elsif( $cmdtype eq "GETSTATUS" ){
            if( defined $cmdHandle ){
                print $cmdHandle "$status\n";
            }
        }
        if( defined $cmdHandle ){
            close $cmdHandle;
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

    #set status of this bgmon
    if( keys %testevents != 0){
        $status = "anyscheduled_yes";
    }else{
        $status = "anyscheduled_no";
    }

    #re-try wait Q every $subtimer_reset times through poll loop
    #try to run tests on queue
    if( $subtimer == 0 ){       

        foreach my $testtype (keys %waitq){

            if( scalar(@{$waitq{$testtype}}) != 0 ){
#                print time_all()." $testtype QUEUE: ";
#                foreach my $node (@{$waitq{$testtype}}){
#                    print "$node ";
#                }
#                print "\n";
            }
            #
            # Run next scheduled test
            spawnTest( undef, $testtype );
        }
    }


    # Reap all dead children.
    #mark flags of finished tests
#    my @deadpids;
    use POSIX ":sys_wait_h";
    while( (my $pid = waitpid(-1,&WNOHANG)) > 0 ){
        my $destaddr = $runningtestPIDs{$pid}[0];
        my $testtype = $runningtestPIDs{$pid}[1];
        my $testev = \%{ $testevents{$destaddr}{$testtype} };
        # handle the case where a test (iperf) times out and bgmon kills it
        if( $testev->{"timedout"} == 1 ){
            $testev->{"flag_scheduled"} = 0;
            $testev->{"timedout"} = 0;
            
        }elsif( defined $testev ){
            $testev->{"flag_finished"} = 1;
        }
        $testev->{"pid"} = 0;
        delete $runningtestPIDs{$pid};  
    } 

    #
    # Check status of running tests.
    # * if a continuous test has produced more output, sechedule an event
    # * check for running tests which are taking too long and kill them
    #
    foreach my $pid (keys %runningtestPIDs){
        my $destaddr = $runningtestPIDs{$pid}[0];
        my $testtype = $runningtestPIDs{$pid}[1];
        my $testev = \%{ $testevents{$destaddr}{$testtype} };
	my $killit = 0;

	if ($testev->{"continuous"}) {
	    my $filename = $testev->{"outfile"};
	    my $fsize = $testev->{"lastsize"};
	    my $cursize = (stat($filename))[7];
	    print time_all().": c$testtype test: fn=$filename, fs=$cursize, lastfs=$fsize\n" if ($debug > 1);
	    if (defined($cursize)) {
		if ($cursize > $fsize) {
		    $testev->{"new_results"} = $fsize;
		    $testev->{"tstamp"} = time_all();
		} else {
		    undef $testev->{"new_results"};
		}
	    } else {
		# something is horribly wrong, kill the process
		$killit = "no output file";
	    }
	}
        if( $testtype eq "bw" &&
            time_all() >
            $testev->{"tstamp"} + 
            $iperftimeout )    
        {
            # bw test is running too long, so kill it
	    $killit = "timeout";
	}

	if ($killit) {
            kill 'TERM', $pid;
            print time_all()." $testtype $killit: killed $destaddr, ".
                "pid=$pid\n";

            $testev->{"timedout"} = 1;

            #delete tmp filename
	    my $filename = $testev->{"outfile"};
            unlink($filename) or warn "can't delete temp file";
	    undef $testev->{"new_results"};
	    undef $testev->{"lastsize"};

            my %results = 
                ("sourceaddr" => $thismonaddr,
                 "destaddr" => $destaddr,
                 "testtype" => $testtype,
                 "result" => $ERRID{timeout},
                 "tstamp" => $testev->{tstamp},
                 "magic"  => "$magic",
                 "ts_finished" => time()
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
            if( $testev->{"flag_finished"} == 1 ||
		($testev->{"continuous"} &&
		 defined($testev->{"new_results"})) ){
		my @raw_lines;

                #read raw results from temp file
		my $filename = $testev->{"outfile"};
		if (!$testev->{"flag_finished"} ||
		    !$testev->{"continuous"}) {
		    open FILE, "< $filename"
			or warn "can't open file $filename";
		    if ($testev->{"new_results"}) {
			seek(FILE, $testev->{"new_results"}, 0);
		    }
		    @raw_lines = <FILE>;
		    if (!$testev->{"flag_finished"}) {
			$testev->{"lastsize"} = tell(FILE);
		    }
		    close FILE;
		}

		if ($testev->{"flag_finished"}) {
		    unlink($filename) or warn "can't delete temp file";
		    undef $testev->{"new_results"};
		    undef $testev->{"lastsize"};
		} else {
		    print "c$testtype test: read ", scalar(@raw_lines),
		    " lines, size=", $testev->{"lastsize"}, "\n" if ($debug > 1);
		}

		#
		# XXX should pass each line individually for a continuous
		# test as it might represent multiple probes.  However, we
		# currently don't have per-line timestamps so the multiple
		# lines would all have the same timestamp and all but one
		# will be discarded anyway.
		#
		if (@raw_lines) {
		    my $raw;
		    foreach my $line (@raw_lines){
			$raw = $raw.$line;
		    }

		    #parse raw data
		    my $parsedData = parsedata($testtype,
					       $testev->{"continuous"},
					       $raw);
		    $testev->{"results_parsed"} = $parsedData;

		    my %results = 
			("sourceaddr" => $thismonaddr,
			 "destaddr" => $destaddr,
			 "testtype" => $testtype,
			 "result" => $parsedData,
			 "tstamp" => $testev->{tstamp},
			 "magic"  => "$magic",
			 "ts_finished" => time()
			 );

		    #MARK_RELIABLE
		    #save result to local DB
		    my $index = saveTestToLocalDB(\%results);

		    #send result to remote DB
		    sendResults(\%results, $index);
		}

                #reset flags
		if ($testev->{"flag_finished"}) {
		    $testev->{"flag_finished"} = 0;
		    $testev->{"flag_scheduled"} = 0;
		}

		# XXX avoid for continuous til I understand
		if (@raw_lines) {
		    # Check for outage
		    if( $testtype eq "latency" ){
			updateOutageState( $destaddr );
		    }elsif( $testtype eq "outage" ){
			updateOutageState_lossCheck( $destaddr );
		    }
		}
            }

            #schedule new tests
            if( $testev->{"flag_scheduled"} == 0 && 
                $testev->{"testper"} > 0 )
            {
                
                if( $testev->{limitTime} > 0 &&
                    time() >= $testev->{limitTime} )
                {
                    my $oldper = $testev->{testper};
		    print time().": Ending test $testtype for $destaddr\n";
		    $testev->{"end_stats"} = getstats();
		    printstats("End", $testev->{"end_stats"});
		    my $diff = diffstats($testev->{"start_stats"},
					 $testev->{"end_stats"});
		    printstats("Total", $diff);
                    # Rate increase expired. Get new value from Q
                    $testev->{cmdq}->cleanQueue();  #rid expired values
                    $testev->{testper} = 0;  #reset existing period
		    undef $testev->{continuous};
                    updateTestEvent($testev);
                    print "resetting period from $oldper";
                    print " to ".$testev->{testper}."\n";
                }

                if( time_all() < 
                    $testev->{"timeOfNextRun"} + $testev->{"testper"} )
                {
                    #if time of next run is in the future, set it to that
                    $testev->{"timeOfNextRun"} += $testev->{"testper"};
                }else{
		    if ( ($testev->{"timeOfNextRun"}  == 0) &&
			 ( $testev->{"managerID"} eq "automanagerclient" ) &&
			 ($testtype eq "bw" ) ) {
			# init the test based on random initial time
			my $range = $testev->{"testper"} - 2 * $iperfduration;
			my $random_init = int(rand($range));
			$testev->{"timeOfNextRun"} =  time_all() + $random_init;
		    } else {
			#if time of next run is in the past, set to current time
			$testev->{"timeOfNextRun"}
			= time_all();
		    }
                }

                $testev->{"flag_scheduled"} = 1;
            }

            #check for new tests ready to run
            if( $testev->{"timeOfNextRun"} <= time_all() &&
                $testev->{"flag_scheduled"} == 1 && 
                $testev->{"pid"} == 0 )
            {
                #run test
		print time().": Starting test $testtype for $destaddr\n";
		$testev->{"start_stats"} = getstats();
		printstats("Start", $testev->{"start_stats"});
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
        $count++;
        if( scalar(@ids) == 0 ){
            last;
        }
        my $index = $ids[int( rand scalar(@ids) )];
        my %results = %{ deserialize_hash($db{$index}) };
        next if( (defined $cacheLastSentAt{$index})  &&
                 time() - $cacheLastSentAt{$index} < 
                 $cacheIndxWaitPer );
        next if( !defined $results{ts_finished} );
        if (!exists($results{"magic"}) || $results{"magic"} ne $magic) {
            # Hmm, something went wrong!
            print "Old results for index $index are scrogged; deleting!\n";
            delLocalDBEntry($index);
            @ids = keys %db;
#           next;
        }

        $iterSinceLastRun = 0;

        #don't send recently completed tests
        next if( time() - $results{ts_finished} < 10 );
        print "sending old result:  $index\n";
        sendResults(\%results, $index);

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
    if( defined $linkdest ){
        foreach my $element ( @{$waitq{$testtype}} ){
            if( $element eq $linkdest ){
                $flag_duplicate = 1;
            }
        }
        if( $flag_duplicate == 0 ){
            unshift @{$waitq{$testtype}}, $linkdest;
#           print time_all()." added $linkdest to Q $testtype\n";
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
    my $fpingTimeout = $testevents{$linkdest}{$testtype}{fpingTimeout};
    my $contin = $testevents{$linkdest}{$testtype}{continuous};
#    print time()." running $linkdest / $testtype\n";

  FORK:{
      my $filename = createtmpfilename($linkdest,$testtype);
      if( my $pid = fork ){
          #parent
          #save child pid in test event
          $testevents{$linkdest}{$testtype}{"pid"} = $pid;
          $testevents{$linkdest}{$testtype}{"tstamp"} = time_all();
	  $testevents{$linkdest}{$testtype}{"outfile"} = $filename;
	  $testevents{$linkdest}{$testtype}{"lastsize"} = 0;
          $runningtestPIDs{$pid} = [$linkdest, $testtype];
          
      }elsif( defined $pid ){
          #child

          #############################
          ###ADD MORE TEST TYPES HERE###
          #############################
          if( $testtype eq "latency" ){
              #command line for "LATENCY TEST"
#             print "##########latTest\n";
	      my $duration = 0;
	      my $period = $testevents{$linkdest}{$testtype}{"testper"};
	      if ($contin) {
		  $duration = int($testevents{$linkdest}{$testtype}{"limitTime"}) - time() + 1;
		  if ($duration < 0 || $duration > (24*60*60)) {
		      $contin = 0;
		  }
	      }
	      if ($contin) {
		  # run ping for $duration seconds with the specified interval
		  open FILE, ">$filename"; close FILE;
		  exec "sudo ".
		      "ping -w $duration -i $period $linkdest >$filename 2>&1"
			  or die "can't exec: $!\n";
	      } else {
		  #one ping, using fping
		  # (2 total attempts, 10 sec timeout between)
		  exec "sudo $workingdir".
		      "fping -t$fpingTimeout -s -r1 $linkdest >& $filename"
			  or die "can't exec: $!\n";
	      }
	  }elsif( $testtype eq "bw" ){
              #command line for "BANDWIDTH TEST"
#             print "###########bwtest\n";
              exec "$workingdir".
                  "iperf -c $linkdest -t $iperfduration -p $iperfport >$filename"
                      or die "can't exec: $!";
          }elsif( $testtype eq "outage" ){
              exec "$workingdir".
                  "fping -c16 -p250 $linkdest >& $filename"
                      or die "can't exec: $!\n";
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
        if( defined $testevents{$destaddr}{$type} &&
            $testevents{$destaddr}{$type}{"pid"} > 0 ){
            #we have a running process, so inc it's counter
            $testcount++;
        }
    }
#    print "testcount = $testcount\n";
    return $testcount;
}
#############################################################################
sub parsedata($$$)
{
    my $type = $_[0];
    my $iscontin = $_[1];
    my $raw = $_[2];
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
	if ($iscontin) {
	    # "traditional" ping output
	    if( /icmp_seq=\d+ ttl=\d+ time=([\d\.]+) ms/ ) {
		$parsed = "$1" if ( $1 ne "0.000" );
	    }elsif( /unknown host/ ){
		$parsed = $ERRID{unknownhost};
	    }elsif( /100% packet loss/ ){
		$parsed = $ERRID{timeout};
	    }
	} else {
	    # fping output
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
#       print "parsed=$parsed\n";
    }elsif( $type eq "outage" ){
        #print "parsing Outage data\n";
        if( /loss = \d+\/\d+\/([\d.]+)%/ ){
            $parsed = $1;
        }else{
            $parsed = "";
        }
        if( defined($parsed) && 
            $parsed < 100 )
        {
            $parsed = "lossy";
        }elsif( $parsed == 100 ){
            $parsed = "down";
        }
        #print "parsed data = $parsed\n";
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

#
# return a warning (bw, for now) if a test has not been run for a while
# after when it is scheduled to be
sub detectHang($)
{
    my ($nodeid) = @_;
    my $TIMEOUT_NUM_PER = 10;

    if( defined $testevents{$nodeid}{bw} &&
        $testevents{$nodeid}{bw}{flag_scheduled} == 1 &&
        time_all() > $testevents{$nodeid}{bw}{timeOfNextRun} +
        $testevents{$nodeid}{bw}{testper} * $TIMEOUT_NUM_PER
        && $testevents{$nodeid}{bw}{testper} >0 )
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

#
#  Adjust latency measurement frequency if a path is detected to have
#   gone down. Also adjusts the timeout frequency of fping to allow for
#   a more frequent probing.
#
=pod
state transitions

CurrentState   Input                 NextState
----------------------------------------------
normal         gotErr&PrevSucc       lossCheck
               else                  normal
lossCheck      pingSucc
               |  lossCheck=lossy    normal
               lossCheck=down        highFreq
               else                  lossCheck
highFreq       <60sec&ERR            highFreq
               >60sec&ERR            medFreq
               SUCCESS               outageEnd
medFreq        <10min&ERR            medFreq
               >10min&ERR            lowFreq
               SUCCESS               outageEnd
lowFreq        ERR                   lowFreq
               SUCCESS               outageEnd
outageEnd      <120sec&SUCCESS       outageEnd
               >120sec&SUCCESS       normal
               ERR                   highFreq
=cut

#
#   CALLED FROM A OUTAGETEST RESULT
sub updateOutageState_lossCheck($)
{
    my ($destaddr) = @_;
    my $latref = \%{ $testevents{$destaddr}{latency} };
    my $outref = \%{ $testevents{$destaddr}{outage} };
    my $curstate = $latref->{outagestate};
    if( $curstate eq "lossCheck" ){
        if( $outref->{results_parsed} eq "lossy" ){
            # STATE TRANSITION -> normal
            # ** but start outageEnd monitoring for a time
            # Delete outageManID entry in cmdqueue
            #  set period to 0
            addCmd( $latref, Cmd->new($outManID,
                                      $outTestFreq{outageEnd},
                                      $outTestDur{outageEnd}) );
            $latref->{outagestate} = "normal";
            $latref->{outStateTime} = 0;
            $latref->{fpingTimeout} = $fpingTimeoutDef;
            print "NEW STATE: normal\n";
        }elsif( $outref->{results_parsed} eq "down" ){
            # STATE TRANSITION -> highFreq
            addCmd( $latref, Cmd->new($outManID,
                                      $outTestFreq{high},
                                      0) );  #forever, since internally managed
            $latref->{outagestate} = "highFreq";
            $latref->{outStateTime} = time();
            $latref->{fpingTimeout} = $outTestFreq{high}*1000/2;
            print "NEW STATE: highFreq\n";
        }
    }
}

#
#   MUST BE CALLED WITH A $TESTEV POINTING TO A LATENCY TESTTYPE HASH
sub updateOutageState($)
{
    my ($destaddr) = @_;
    my $testev = \%{ $testevents{$destaddr}{latency} };
    my $curstate = $testev->{outagestate};
    my $nextstate = $curstate;
    if( $testev->{results_parsed} > 0 ){
        #valid result, so note the time that this was seen
        $testev->{lastValidLatTime} = time();
    }

    # SWITCH ON "outagestate"
    if( $curstate eq "normal" ){
        if( isWorkingPathDown($testev) > 0 ){
            # STATE TRANSITION -> lossCheck
            $nextstate = "lossCheck";
            # run outage check
            $testevents{$destaddr}{outage}{testper} = 0;
            $testevents{$destaddr}{outage}{timeOfNextRun} = time_all()-1;
            $testevents{$destaddr}{outage}{flag_scheduled} = 1;
            $testevents{$destaddr}{outage}{pid} = 0;
            $testevents{$destaddr}{outage}{flag_finished} =0;
            $testevents{$destaddr}{outage}{timedout} = 0;
        }
    }elsif( $curstate eq "lossCheck" ){
        if( isWorkingPathDown($testev) < 0 ){
            # STATE TRANSITION -> normal
            # Delete outageManID entry in cmdqueue
            #  set period to 0
            addCmd( $testev, Cmd->new($outManID,0,0) );
            $nextstate = "normal";
            $testev->{outStateTime} = 0;
            $testev->{fpingTimeout} = $fpingTimeoutDef;
        }
    }elsif( $curstate eq "highFreq" ){
        if( isWorkingPathDown($testev) > 0 &&
            time() - $testev->{outStateTime} > $outTestDur{high} )
        {
            # STATE TRANSITION -> medFreq
            addCmd( $testev, Cmd->new($outManID,
                                      $outTestFreq{med},
                                      0) );  #forever, since internally managed
            $nextstate = "medFreq";
            $testev->{outStateTime} = time();
            $testev->{fpingTimeout} = $outTestFreq{med}*1000/2;
        }elsif( isWorkingPathDown($testev) < 0 ){
            # STATE TRANSITION -> outageEnd
            $nextstate = outSetState_outageEnd($testev);
        }
    }elsif( $curstate eq "medFreq" ){
        if( isWorkingPathDown($testev) > 0 &&
            time() - $testev->{outStateTime} > $outTestDur{med} )
        {
            # STATE TRANSITION -> lowFreq
            addCmd( $testev, Cmd->new($outManID,
                                      $outTestFreq{low},
                                      0) );  #forever, since internally managed
            $nextstate = "lowFreq";
            $testev->{outStateTime} = time();
            $testev->{fpingTimeout} = $fpingTimeoutDef;
        }elsif( isWorkingPathDown($testev) < 0 ){
            # STATE TRANSITION -> outageEnd
            $nextstate = outSetState_outageEnd($testev);
        }
    }elsif( $curstate eq "lowFreq" ){
        if( isWorkingPathDown($testev) < 0 ){
            # STATE TRANSITION -> outageEnd
            $nextstate = outSetState_outageEnd($testev);
        }
    }elsif( $curstate eq "outageEnd" ){
        if( isWorkingPathDown($testev) < 0 &&
            time() - $testev->{outStateTime} > $outTestDur{outageEnd} )
        {
            # STATE TRANSITION -> normal
            # Delete outageManID entry in cmdqueue
            #  set period to 0
            addCmd( $testev, Cmd->new($outManID,0,0) );
            $nextstate = "normal";
            $testev->{outStateTime} = 0;
            $testev->{fpingTimeout} = $fpingTimeoutDef;
        }elsif( isWorkingPathDown($testev) > 0 ){
            # STATE TRANSITION -> highFreq
            $nextstate = "highFreq";
            $testev->{outStateTime} = time();
            $testev->{fpingTimeout} = $outTestFreq{high}*1000/2;
        }
    }

    # save new state
    if( $curstate ne $nextstate ){
        print "NEW STATE: $nextstate\n";
    }
    $testev->{outagestate} = $nextstate;
}

sub outSetState_outageEnd($)
{
    my ($testev) = @_;
    addCmd( $testev, Cmd->new($outManID,
                              $outTestFreq{outageEnd},
                              0) );  #forever, since internally managed
#    $testev->{outagestate} = "outageEnd";
    $testev->{outStateTime} = time();
    $testev->{fpingTimeout} = $outTestFreq{outageEnd}*1000/2;
    return "outageEnd";
}

sub isWorkingPathDown($)
{
    my ($testev) = @_;
    my $secSinceUp = time() - $testev->{lastValidLatTime};
    if( $testev->{results_parsed} < 0 && 
        $secSinceUp < $outDet_maxPastSuc*60*60 )
    {
        #path is down and was up recently
        return $secSinceUp;  #return seconds since last being up
    }else{
        return -1;
    }
}

#
# update testevents with latest from queue.
#   input: ref to testev
#
# Called from places which change the cmdqueue:
#   receive an EDIT/INIT/STOPALL, test expired, outage detected/measured
#
sub updateTestEvent($)
{
    my ($testev) = @_;
    my $curPer = $testev->{testper};
    my $head = $testev->{cmdq}->head();

    if( !defined $head ){
        #if no tests in queue, stop testev.
        print "NO HEAD for ".$testev->{testper}."\n";
        $testev->{flag_scheduled} = 0;
        $testev->{testper} = 0;
    }
    #compare currently running test info with head of queue
    elsif( !defined $curPer ||
           $curPer == 0 ||
#          $head->period() < $curPer )
           $head->period() != $curPer )
    {
        #replace running info with that from head:
        #  testper, limitTime, flag_scheduled, timeOfNextRun, managerID
        print "replacing period $curPer with ".$head->period()."\n";
        $testev->{testper} = $head->period();
        if( $head->duration() == 0 ){
            $testev->{limitTime} = 0;
        }else{
            $testev->{limitTime} = time_all() + $head->timeleft();
        }
        $testev->{flag_scheduled} = 0;
        $testev->{timeOfNextRun} = 0;

	# See if we should change the continuous status
	print "testper=", $testev->{testper},
	      ", limit=", $testev->{limitTime}, "\n";
	if (!$nocontinuous &&
	    $testev->{limitTime} > 0 && $testev->{testper} < $CONT_THRESHOLD) {
	    $testev->{continuous} = 1;
	} else {
	    undef $testev->{continuous};
	}

        $testev->{managerID} = $head->managerid();
        print $testev->{cmdq}->getQueueInfo();
    }
}

#
# add a command to the queue of a testev
#
sub addCmd($$)
{
    my ($testev, $cmd) = @_;
    $testev->{cmdq}->add($cmd);
    #update scheduled tests in case something changed
    updateTestEvent( $testev );
}

sub initTestEv($$)
{
    my ($dest,$type) = @_;

    if( !defined $testevents{$dest}{$type} ){
        $testevents{$dest}{$type} = {
                                     cmdq => Cmdqueue->new(),
                                     flag_scheduled => 0,
                                     flag_finished  => 0,
                                     testper        => 0,
                                     timeOfNextRun  => 0,
                                     limitTime      => 0,
                                     pid            => 0,
                                     timedout       => 0,
                                     outagestate    => "normal",
                                     lastValidLatTime => 0,
                                     fpingTimeout   => $fpingTimeoutDef
                                     };
    }
}

sub getstats()
{
    my %h;

    my ($utime,$stime,$cutime,$cstime);
    if (open(FD, "</proc/self/stat")) {
	($utime,$stime,$cutime,$cstime)	= (split(/\s+/, <FD>))[13..16];
	close(FD);
    }
    $h{utime} = $utime;
    $h{stime} = $stime;
    $h{cutime} = $cutime;
    $h{cstime} = $cstime;
    return \%h;
}

sub diffstats($$)
{
    my ($rs,$re) = @_;

    my %before = %{$rs};
    my %after = %{$re};
    my %diff;
    foreach my $key (keys(%before)) {
	$diff{$key} = $after{$key} - $before{$key};
    }
    return \%diff;
}

sub printstats($$)
{
    my ($hdr,$stats) = @_;

    print("$hdr: utime=", $stats->{utime},
	  ", stime=", $stats->{stime},
	  ", cutime=", $stats->{cutime},
	  ", cstime=", $stats->{cstime}, "\n");
}
