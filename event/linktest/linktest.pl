#!/usr/bin/perl -w -T
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#

use strict;
use Class::Struct;
use POSIX qw(uname);
use POSIX qw(strftime);
use IO::Handle;
use English;
use Socket;

my $LINKTEST_VERSION = "1.2";

#
# XXX config stuff that does not belong on the client-side
#
#my $PROJROOT      = "@PROJROOT_DIR@";

#
# Linktest test script. This script is set up to run as root on
# experiment nodes. It is invoked by the Linktest daemon after the
# daemon receives a Linktest "START" event. The script runs tests
# using ping, rude/crude (a real-time packet emitter/collector) and a
# locally hacked iperf to test all links in the experiment. If the
# results show a 99% chance that the experiment is configured
# incorrectly, an error is logged to the experiment directory in
# tbdata/linktest.  Valid ranges have been calibrated ahead of time.
#
sub usage() 
{
    print("Usage: linktest.pl\n".
	  " [STARTAT=<test step, 1-4>]\n".
	  " [STOPAT=<test step, 1-4>]\n".
	  " [COMPAT=<version or 0>]\n".
	  " [DOARP=<1=yes, 0=no>]\n".
	  " [REPORTONLY=<1=yes, 0=no>]\n".
	  " [NODES=<n1,n2...>\n".
	  " [DEBUG=<debugging level. 1=on, 0=off>]\n".
	  " [LOGRUN=<command/used/for/logging>]\n".
	  " [LOGDIR=<path/to/log/root>]\n".
	  " [BINDIR=<path/to/binary/files>]\n".
	  " [VARDIR=<path/to/config/files>]\n".
	  " [EVENTSERVER=<eventserver hostname>]\n");
    print("    <test step>: 1=conn/latency, 2=routing, 3=loss, 4=BW\n".
	  "    COMPAT=<version>: remain compatible with version <version> or earlier\n".
	  "    DOARP=1: run a single-ping pass to create ARP entries\n".
	  "    REPORTONLY=1: report stats only, do not pass judgement\n".
	  "    NODES: comma-separated list of virtnames to run on\n".
	  "    LOGRUN: Command to run for high-level logging. The log message as its only argument.".
	  "    LOGDIR: Path used to store stderr logs of individual test utilities.\n".
	  "    BINDIR: Path must contain emulab-rude emulab-crude emulab-iperf\n".
	  "    VARDIR: Path must contain...\n".
	  "    EVENTSERVER: Hostname of event server\n");
    exit(0);
}


##############################################################################
# Constants
##############################################################################

# log files used by tests.
use constant CRUDE_DAT => "/tmp/crude.dat"; # binary data
use constant RUDE_CFG  => "/tmp/rude.cfg";
use constant IPERF_DAT => "/tmp/iperf.dat";

# Packet size for iperf (1470 default).  Reduce to avoid problems with veths
use constant IPERF_PKTSIZE => 1450;

# max time to run iperf
use constant BW_TEST_MAXDURATION => 10;

# iperf test limits.
use constant LIMIT_BW_HI  => 100000000;
use constant LIMIT_BW_MED =>  10000000;
use constant LIMIT_BW_LO  =>   1000000;
use constant LIMIT_BW_MIN =>     64000;
use constant LIMIT_BW_LOSS => 0.20;

# Make sure that we dont get bogged down in being too accurate! 
# Make sure the error is a certain significance before we start reporting it.
use constant INSIGNIFICANT_LAT_ERROR_LO => 0.50;  # ms
use constant INSIGNIFICANT_LAT_ERROR_HI => 3.50;  # ms
use constant INSIGNIFICANT_BW_ERROR_HI  => 0.015; # percent.
use constant INSIGNIFICANT_BW_ERROR_LO  => 0.03;  # percent.
use constant INSIGNIFICANT_BW_ERROR_LO_Windows  => 0.10;  # Lower expectations.

# slow send rate (for bw from LIMIT_BW_MIN to LIMIT_BW_LO)
use constant SLOW_SEND => 100;
use constant FAST_SEND => 250;
use constant LOSS_TEST_DURATION => 4;	# In seconds.

# misc contstants
use constant BSD => "FreeBSD";
use constant LINUX => "Linux";
use constant RTPROTO_STATIC => "Static";
use constant RTPROTO_SESSION => "Session";
use constant EVENT_COMPLETE => "COMPLETE";
use constant EVENT_REPORT => "REPORT";
use constant EVENT_LOG => "LOG";
use constant PING_SEND_COUNT => 10;
use constant SYNC_NAMESPACE => "linktest";

# crude should have higher priority than rude on the same machine
use constant RUDE_PRI => 3;
use constant CRUDE_PRI => 5;

# test levels
use constant TEST_LATENCY => 1; # direct link connectivity & latency
use constant TEST_RT_STATIC => 2;   # prior plus static routing
use constant TEST_LOSS => 3;   # prior plus loss
use constant TEST_BW => 4; # prior plus bandwidth

# test names
use constant NAME_RT_STATIC => "Routing";
use constant NAME_LATENCY => "Latency";
use constant NAME_LOSS => "Loss";
use constant NAME_BW => "Bandwidth";

# error suffix for logs
use constant SUFFIX_ERROR => ".error";
use constant DEBUG_ALL => 2; # debug level for all debug info, not just msgs.

# exit codes
use constant EXIT_ABORTED => -1;
use constant EXIT_NOT_OK => 1;
use constant EXIT_OK => 0;

# Protos
sub TimeStamp();
sub PATH_NICE();

# struct for representing a link.
struct ( edge => {
    name => '$',
    src => '$',
    srcip => '$',
    dst => '$',
    dstip => '$',
    bw  => '$',
    testbw  => '$',
    bwtime  => '$',
    delay => '$',
    loss => '$',
    queuetype => '$',
    mac => '$',
    mpxstyle => '$',
    dstyle => '$',
    symlanignore => '$'});

struct ( host => {
    name => '$',
    visited => '$',
    links => '@',
    isvnode => '$',
    pname => '$',
    phost => '$',
    ptype => '$',
    osid => '$',
    os => '$',
    osvers => '$',
    osfeatures => '$'});

# fixes emacs colorization woes introduced by above struct definition.
# struct ( unused => { foo => '$'});

use constant TRUE => 1;
use constant FALSE => 0;

##############################################################################
# Globals
##############################################################################

my $topology_file;    # location of the topology input file.
my $ptopology_file;   # location of the physical topology input file.
my $synserv;    # synch server node
my $rtproto;    # routing protocol
my $hostname;   # this hosts name
my $exp_id;     # experiment id
my $proj_id;    # project id
my $gid;        # group id
my $platform;   # name of platform
my $startat=1;  # which test to start at
my $stopat=99;  # which test to stop at
my %kill_list;  # PIDS that are running as background procs, needing cleanup.
my $debug_level = 0; # enable debug statements
                     # 1 = print debug statements.
                     # 2 = show STDOUT and STDERR
my $arpit = 1;	# do a single ping to force ARP exchange
my $reportonly = 0; # just report the values seen, no pass/fail judgement
my $printsched = 0; # just print the test schedule
my $compat = 1.1;   # be compatible (wrt. synch) with old versions
my $barriers_hit = 1;
my $barr_count;   # used by synserv host, nubmer of hosts -1
my $log_file;    # common logfile for information saved over time.

my @hosts; # hosts: list of text strings containing host names.
           # sorted alphabetically
my %hostmap;
my $numvnodes = 0;
my %vhostmap;
my %linkmembers;
my @links; # links: list of edge structs.
           # sorted alphabetically by src . dst

my $expt_path;  # experiment path (ie, tbdata) set by init.
my $linktest_path;   # log path (ie tbdata/linktest) set by init.
my $simname = "ns";
my $swapper = "";
my $swapperid = 0;
my $swappergid = 0;
my $token = -1;
my $error_count = 0;
my $stage_error_count = 0;
my $total_error_count = 0;

my $warn_partial_test = 0;
my $warn_unshaped_links = 0;

my $listener_iperf;
my $listener_crude;

##############################################################################
# Main control
##############################################################################

# un-taint path
$ENV{'PATH'} = '/bin:/usr/bin:/usr/local/bin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

$| = 1; #Turn off line buffering on output

# Make sure that files written into the experiment subdir are group writable.
umask(0002);

our $LOGRUN = "";
our $PROJROOT = "";
our $VARDIR = "";
our $BINDIR = "";
our $EVENTSERVER = "";

#
# Parse command arguments. Since Linktest is run via the event system,
# parse out pairs of <symbol>=<value>.
#
foreach my $arg (@ARGV) {
    if($arg =~ /STOPAT=(\d)/) {
	$stopat=$1;
    }
    if($arg =~ /STARTAT=(\d)/) {
	$startat=$1;
    }
    if($arg =~ /COMPAT=(\d(?:\.\d+))/) {
	$compat=$1/1.0;
    }
    if($arg =~ /DOARP=(\d)/) {
	$arpit=$1;
    }
    if($arg =~ /REPORTONLY=(\d)/) {
	$reportonly=$1;
    }
    if($arg =~ /DEBUG=(\d)/) {
	$debug_level=$1;
    }
    if($arg =~ /PRINTSCHED=(\d)/) {
	$printsched=$1;
    }
    if($arg =~ /TOKEN=(\d+)/) {
	$token=$1;
    }
    if($arg =~ /SWAPPER=(\w+)/) {
	$swapper=$1;
	(undef,undef,$swapperid,$swappergid) = getpwnam($swapper);
    }
    if($arg =~ /LOGRUN=(.+)/) {
	$LOGRUN = $1;
    }
    if($arg =~ /LOGDIR=(.+)/) {
	$PROJROOT = $1;
    }
    if($arg =~ /VARDIR=(.+)/) {
	$VARDIR = $1;
    }
    if($arg =~ /BINDIR=(.+)/) {
	$BINDIR = $1;
    }
    if($arg =~ /EVENTSERVER=(.+)/) {
	$EVENTSERVER = $1;
    }
}

$compat = 99
    if ($compat == 0);

# XXX no arp test in 1.1 and before
if ($compat < 1.2) {
    $arpit = 0;
}

# path to applications and files
our $PATH_NICKNAME = "$VARDIR/boot/nickname";
our $PATH_KEYFILE = "$VARDIR/boot/eventkey";
our $PATH_RUDE = "$BINDIR/emulab-rude";
our $PATH_CRUDE = "$BINDIR/emulab-crude";
our $PATH_IPERF = "$BINDIR/emulab-iperf";
our $PATH_RCTOPO = "$BINDIR/rc/rc.topomap";
our $PATH_EMULAB_SYNC = "$BINDIR/emulab-sync";
our $PATH_LTEVENT = "$BINDIR/ltevent";
our $PATH_TEVC = "$BINDIR/tevc";
our $RUN_PATH = "$BINDIR"; # where the linktest-ns runs.
our $PATH_SCHEDFILE = "$VARDIR/logs/linktest.sched";
our $PATH_SYNCSERVER = "$VARDIR/boot/syncserver";
our $PATH_TOPOFILE = "$VARDIR/boot/ltmap";
our $PATH_PTOPOFILE = "$VARDIR/boot/ltpmap";

my $schedfile = $PATH_SCHEDFILE;
if ($printsched) {
    open(SCHED, ">$schedfile") or
    die("Could not open schedule log file $schedfile");
} else {
    unlink($schedfile);
}

#
# Parse the nickname file to obtain the host name, 
# experiment ID and the project ID.
#
my $fname = $PATH_NICKNAME;
die("Could not locate $fname\n") unless -e $fname;
my @results = &read_file($fname);
($hostname, $exp_id, $proj_id) = split /\./, $results[0];
chomp $hostname;
chomp $exp_id;
chomp $proj_id;

# taint check pid/eid
if ($proj_id =~ /([-\w]*)/) {
    $proj_id = $1;
}
if ($exp_id =~ /([-\w]*)/) {
    $exp_id = $1;
}
$gid = $proj_id;

#
# Set path variables storing the experiment logging path,
# the current ns file and the output file for topology info.
#
$expt_path = "$PROJROOT/$proj_id/exp/$exp_id/tbdata";
$linktest_path = "$expt_path/linktest";
$topology_file = $PATH_TOPOFILE;
$ptopology_file = $PATH_PTOPOFILE;

#
# Determine what OS we are.  Used for handling the occasional difference
# in the location of or options to system utilities (e.g., ping).
#
($platform) = POSIX::uname();

#
# First, everyone parses the topo files to see who is participating in
# this run.  All instances will be started simultaneuously from the event
# system, so we throw a random sleep in here to keep nodes from pounding
# the NFS server.
#
sleep(int(rand(5)));
&my_system($PATH_RCTOPO, "reconfig");
&get_topo($topology_file, $ptopology_file);
&debug_top();

#
# Finally, see who the master is for synchronization.
# We prefer the synch server node for the experiment, but if that node
# is not participating, we choose the first node on the host list.
#
$synserv = "";
my $ssname = $PATH_SYNCSERVER;
if ($ssname) {
    @results = &read_file($ssname);
    ($synserv) = split/\./, $results[0];
    chomp $synserv;
}
if (@hosts > 0 && (!$synserv || !exists($hostmap{$synserv}))) {
    $synserv = $hosts[0];
}
&debug("Synch master is $synserv\n");

#
# If the current node is the special node, do some housekeeping
# and initialize the barrier count.
#
if(&is_special_node()) {
    #
    # If the shared path used by Linktest for logging and temporary
    # files already exists, clear its contents for this run.
    #
    if( -e $linktest_path ) {
	die("Path $linktest_path is not a directory\n") 
	    unless -d $linktest_path;

	opendir (DIR,$linktest_path)
	    ||  die("Could not open $linktest_path: $!");
	my @dirfiles = grep (/error$/,readdir(DIR));
	foreach (@dirfiles) {
	    &do_unlink("$linktest_path/$_");
	}
	closedir(DIR);

    } else {
	# 
	# The shared path does not exist, create it.
	#
	mkdir (&check_filename($linktest_path),0777) 
	    || die("Could not create directory $linktest_path: $!");
	chown($swapperid, $swappergid, $linktest_path);
    }

    $barr_count = @hosts;
    $barr_count--;
}

#
# If there are no links to test, there is nothing to do!
# Do this after we have identified the synch server.
#
if (@links == 0) {
    &debug("No links to test!\n");

    my $msg = "Linktest skipped, no links";
    &sim_event(EVENT_LOG,$msg);
    &debug("\n$msg\n\n");
    &post_event(EVENT_COMPLETE,"ERROR=0 CTOKEN=$token");
    exit(EXIT_OK);
}

#
# If it has been determined that we are not a part of the run,
# exit now so we don't screw up the barrier synchs.  Note that post_event
# will only happen if we are the synch server and we can only be the
# synch server if no other node is participating in the run either.
#
if (!exists($hostmap{$hostname})) {
    &debug("$hostname not participating in this linktest run.\n");
    &sim_event(EVENT_LOG,"Linktest skipped, no nodes participating");
    &post_event(EVENT_COMPLETE,"ERROR=0 CTOKEN=$token");
    exit(EXIT_OK);
}

#
# All nodes remove local temporary files remaining from the last
# run, if any.
# 
&do_unlink(CRUDE_DAT);
&do_unlink(IPERF_DAT);
&do_unlink(RUDE_CFG);

#
# Start up listeners; they run over the lifetime of Linktest to
# reduce the number of barrier synchronizations and startup
# delays. Always give the collectors a second to start up.
#
my $listeners = 0;
if (&dotest(TEST_BW)) {
    if ($printsched) {
	&schedlog("start iperf listener");
    } else {
	$listener_iperf = &start_listener(PATH_NICE, "-n", "-10",
					  $PATH_IPERF,"-s","-f","b","-u",
					  "-w","200000","-l",IPERF_PKTSIZE);
	$listeners++;
    }
}
if (&dotest(TEST_LOSS)) {
    if ($printsched) {
	&schedlog("start crude listener");
    } else {
	$listener_crude = &start_listener($PATH_CRUDE,"-l",CRUDE_DAT,
					  "-P",CRUDE_PRI);
	$listeners++;
    }
}
if ($listeners) {
    sleep(1);
}

#
# Finally, synchronize so that all nodes have completed startup
# tasks. When all nodes reach this barrier, the topology input file
# has been written, local variables are initialized, background
# listeners have been started, and temporary files have been cleared.
#
if ($printsched) {
    &schedlog("barrier $barriers_hit: post-startup");
}
&barrier;

#
# Beginning of the tests.
#
my $msg = "Linktest Version $LINKTEST_VERSION";
&sim_event(EVENT_LOG, $msg);
&post_event(EVENT_REPORT, $msg);
&debug("\n$msg\n\n");

#
# Print out any warnings that alter the behavior of the run
#
if ($warn_partial_test) {
    my $msg = "*** WARNING: some hosts do not support linktest,".
	      " skipping links between those hosts";
    &sim_event(EVENT_LOG, $msg);
    &post_event(EVENT_REPORT, $msg);
    &debug("\n$msg\n\n");
}
if ($warn_unshaped_links && &dotest(TEST_BW)) {
    my $msg = "*** WARNING: tb-set-noshaping used on one or more links,".
	      " skipping BW tests for those links.";
    &sim_event(EVENT_LOG, $msg);
    &post_event(EVENT_REPORT, $msg);
    &debug("\n$msg\n\n");
}

if (defined($rtproto) && $rtproto eq RTPROTO_SESSION) {
    my $msg = "Session routing active; waiting a bit to let routes stabilize";
    &sim_event(EVENT_LOG,$msg);
    &debug("\n$msg\n\n");
    sleep(30);
}

if ($arpit) {
    my $stamp = TimeStamp();
    my $msg   = "Pre-test ARP on all nodes ... $stamp";
    &post_event(EVENT_REPORT,$msg);
    &sim_event(EVENT_LOG,$msg);
    # Ick, this barrier makes sure the above message gets into the log
    # first, so as not to confuse Mike
    if ($printsched) {
	&schedlog("barrier $barriers_hit: pre-arp test");
    }
    &barrier();
    &debug("\n$msg\n\n");
    &force_arp;
}

if(&dotest(TEST_LATENCY)) {
    my $stamp = TimeStamp();
    my $msg   = "Testing Single Hop Connectivity and Latency ... $stamp";
    &post_event(EVENT_REPORT,$msg);
    &sim_event(EVENT_LOG,$msg);
    # Ick, this barrier makes sure the above message gets into the log
    # first, so as not to confuse Mike
    if ($printsched) {
	&schedlog("barrier $barriers_hit: pre-latency test");
    }
    &barrier();
    &debug("\n$msg\n\n");
    &latency_test;
    &report_status(NAME_LATENCY);
}

if(&dotest(TEST_RT_STATIC)
    && defined($rtproto)
    && ($rtproto eq RTPROTO_STATIC || $rtproto eq RTPROTO_SESSION)) {
    my $msg;
    my $stamp = TimeStamp();

    if ($total_error_count) {
	$msg = "Skipping Routing tests because of previous errors!";
    }
    else {
	$msg = "Testing Routing ... $stamp";
    }
    &post_event(EVENT_REPORT,$msg);
    &sim_event(EVENT_LOG,$msg);
    # Ick, this barrier makes sure the above message gets into the log
    # first, so as not to confuse Mike
    if ($printsched) {
	&schedlog("barrier $barriers_hit: pre-routing test");
    }
    &barrier();
    &debug("\n$msg\n\n");
    if (! $total_error_count) {
	&static_rt_test; # nodes not covered by 1hop test
	&report_status(NAME_RT_STATIC);
    }
    else {
	if ($printsched) {
	    &schedlog("barrier $barriers_hit: post-routing test");
	}
	&barrier();
    }
}

if(&dotest(TEST_LOSS)) {
    my $stamp = TimeStamp();
    my $msg = "Testing Loss ... $stamp";
    &post_event(EVENT_REPORT,$msg);
    &sim_event(EVENT_LOG,$msg);
    # Ick, this barrier makes sure the above message gets into the log
    # first, so as not to confuse Mike
    if ($printsched) {
	&schedlog("barrier $barriers_hit: pre-loss test");
    }
    &barrier();
    &debug("\n$msg\n\n");
    &loss_test; 
    &report_status(NAME_LOSS);
}

if(&dotest(TEST_BW)){
    my $stamp = TimeStamp();
    my $msg = "Testing Bandwidth ... $stamp";
    &post_event(EVENT_REPORT,$msg);
    &sim_event(EVENT_LOG,$msg);
    # Ick, this barrier makes sure the above message gets into the log
    # first, so as not to confuse Mike
    if ($printsched) {
	&schedlog("barrier $barriers_hit: pre-bandwidth test");
    }
    &barrier();
    &debug("\n$msg\n\n");
    &bw_test;
    &report_status(NAME_BW);
}

&cleanup;

if ($printsched) {
    &schedlog("barrier $barriers_hit: post-test");
}
&barrier();

$msg = "Linktest Done";
&sim_event(EVENT_LOG,$msg);
&debug("\n$msg\n\n");

#
# Send an event indicating that Linktest has completed normally.
#
&post_event(EVENT_COMPLETE,"ERROR=$total_error_count CTOKEN=$token");

exit(EXIT_OK);


##############################################################################
# Loss Test Functions
##############################################################################


# Writes the configuration file used by RUDE.
sub write_rude_cfg {
    my ($stream_id, $edge) = @_;
    my $sample_size = &get_loss_sample_size($edge);
    my $millis      = LOSS_TEST_DURATION * 1000;
    my @contents;
    
    #
    # Run for the desired time and then tell rude to not transmit, but
    # wait, for the edge one-way latency time.  This way rude will not
    # exit before the last packet has a chance to arrive at the destination.
    # Delaying here simplifies the barrier synchronization (we don't have
    # to do a sub-second delay in perl).  Note that a final, zero rate
    # MODIFY like we use here only works in our modified version of rude!
    # Stock rude will just exit after the last actual transmission; i.e.,
    # prior to the final one-way latency wait.
    #
    # START <when>
    # <start-offset-ms> <flowID> ON <src-port> \
    #    <dst-addr>:<dst-port> CONSTANT <packets per second> <packet size>
    # <stop-offset-ms> <flowID> MODIFY CONSTANT 0 <XX>
    # <stop-offset-ms+one-way-link-latency> <flowID> OFF
    #
    # For our purposes, the variables are:
    #	$sample_size	packet rate
    #	$millis		time to run
    #
    push @contents, "START NOW\n";
    # Let bind() choose the src port; any constant port may be already in use.
    push @contents, "0000 $stream_id ON 0 " 
	. $edge->dst . "-" . $edge->name
	    . ":10001 CONSTANT $sample_size 20\n";
    if ($edge->delay) {
	push @contents, "$millis $stream_id MODIFY CONSTANT 0 20\n";
	$millis += int($edge->delay);
    }
    push @contents, "$millis $stream_id OFF\n";

    &write_file(RUDE_CFG, @contents);

}

# Returns the sample size used by the Loss test.
# TODO: why this number? (from my ProbStats book.)
sub get_loss_sample_size {
    my $edge = shift @_;
    if($edge->loss > 0) {
	return &round( 2.5 / $edge->loss);
    } else {
	# just in case a slow link with no loss.
	return SLOW_SEND; 
    }
}



# returns TRUE if the link loss is valid for the linktest loss test.
sub valid_loss {
    my $edge = shift @_;
    if($edge->bw >= LIMIT_BW_MIN && $edge->bw < LIMIT_BW_LO) {
	if(&get_loss_sample_size($edge) > SLOW_SEND) {
	    return FALSE;
	} else {
	    return TRUE;
	}
    } elsif( $edge->bw >= LIMIT_BW_LO) {
	# also want an upper limit.
	if(&get_loss_sample_size($edge) > FAST_SEND) {
	    return FALSE;
	} else {
	    return TRUE;
	}
    } else {
	return FALSE;
    }
}


# This test uses RUDE and CRUDE to send a stream of packets
# in both directions over a duplex link.
sub loss_test {
    my %analyze;
    my %recv_cnt;
    my $stream_id = 1;
    my @edge_copy = @links;
    my $trun = 1;
    my $rude_arg = "";

    # XXX version 1.1 compatibility; used to start crude here and wait
    if ($compat < 1.2) {
	&debug("performing barrier synch for backward compatibility\n");
	&barrier();
    }

    #
    # XXX "concession" to vnodes: the stock rude is, well..."rude",
    # when it comes to CPU usage.  It spins between time intervals
    # which is rather painful for vnodes.  So we have a local version
    # which allows sleeping between intervals if the clock resolution
    # is sufficient.  We send 100 pps (10000us) and vnodes have a 1000HZ
    # (1000us) clock, which qualifies as sufficient.
    #
    # So, we add the extra rude option if conditions are met.
    #
    if ($numvnodes && $hostmap{$hostname}->isvnode) {
	my $hz = `/sbin/sysctl kern.clockrate 2>/dev/null`;
	if ($hz =~ /\shz = (\d+),/) {
	    $rude_arg = "-C $1";
	}
    }

    while(&has_elems(\@edge_copy)) {
	my ($edge,$other_edge) = &get_twoway_assign(\@edge_copy, 1);
	if(defined($edge) && defined($other_edge)) {
	    if($hostname eq $edge->src) {
		if(valid_loss($edge)) {
		    &write_rude_cfg($stream_id,$edge);
		    if ($printsched) {
			&schedlog("rude " . schedprint_link($edge) .
				  " (pps=" .
				  &get_loss_sample_size($edge) .
				  ", time=" .
				  LOSS_TEST_DURATION . "s, psize=20)");
		    } else {
			&my_system($PATH_RUDE,"-s", RUDE_CFG, "-P", RUDE_PRI,
				   $rude_arg);
			$analyze{$stream_id} = $other_edge;
		    }
		} else {
		    if ($printsched) {
			&schedlog("skipping loss test " .
				  schedprint_link($edge));
		    }
		    &debug("Skipping loss test for " .
			   &print_link($edge) . "\n");
		    &info("*** Skipping loss test on $hostname for " .
			  &print_link($edge) . "\n");
		}
	    } elsif ($hostname eq $other_edge->src) {
		if(valid_loss($other_edge)) {
		    &write_rude_cfg($stream_id,$other_edge);
		    if ($printsched) {
			&schedlog("rude " . schedprint_link($other_edge) .
				  " (pps=" .
				  &get_loss_sample_size($edge) .
				  ", time=" .
				  LOSS_TEST_DURATION . "s, psize=20)");
		    } else {
			&my_system($PATH_RUDE,"-s", RUDE_CFG, "-P", RUDE_PRI,
				   $rude_arg);
			$analyze{$stream_id} = $edge;
		    }
		} else {
		    if ($printsched) {
			&schedlog("skipping loss test " .
				  schedprint_link($other_edge));
		    }
		    &debug("Skipping loss test for " .
			   &print_link($other_edge) . "\n");
		    &info("*** Skipping loss test on $hostname for " .
			  &print_link($other_edge) . "\n");
		}
	    }
	}
	$stream_id++;
	if ($printsched) {
	    &schedlog("barrier $barriers_hit: loss: after run $trun");
	    $trun++;
	}
	&barrier();
    }

    # wait for any stragglers due to delay-- there is a  race
    # between the barrier sync on the control net and the expt net latency.
    sleep(1);

    # count packets received for each stream.
    my @results;
    if ($printsched) {
	@results = ();
    } else {
	@results = &my_tick($PATH_CRUDE,"-d",CRUDE_DAT);
    }
    my $result_count = @results;
    &debug("result_count from crude: $result_count\n");
    foreach (@results) {
	if(/ID=(\d+) /) {
	    $recv_cnt{$1}++;
	}
    }

    # analyze only links for which a stream was received.
    foreach my $key (keys %analyze) {
	my $edge = $analyze{$key};
	my $sent = (&get_loss_sample_size($edge) * LOSS_TEST_DURATION) + 1 ;
	my $received = $recv_cnt{$key};

	if ($reportonly) {
	    &info("    Loss result on $hostname for " .
		  &print_edge($edge) .
		  ": sent/recv = $sent/$received\n");
	    next;
	}

	if(!defined($received)) {
	    $received=0;
	    &error (NAME_LOSS,$edge,"No packets received from " . $edge->src);
	} else {
	    # this is a large sample test about proportion p.
	    # this is considered a valid statistical estimate for np >= 10.
	    my $p = 1 - $edge->loss;

	    my $p_hat = $received / $sent;
	    my $numerator = $p_hat - $p;
	    my $denominator = sqrt( abs( $p * (1 - $p_hat) / $sent) );

	    if ($edge->loss == 0) {
		#
		# Lets not worry about a single lost packet.
		#
		if ($received < ($sent - 1)) {
		    my $errmsg = "Unexpected loss (sent $sent, received=$received)";
		    &error(NAME_LOSS, $edge, $errmsg);
		}
	    } elsif($denominator == 0) {
		my $errmsg = "No packets lost (sent $sent, plr " . $edge->loss .")";
		&error(NAME_LOSS, $edge, $errmsg);
	    } else {

		
		my $z = $numerator / $denominator;
		my $reject_region = 2.58; # alpha = 0.1, normal distro by CLT
		if(abs($z) > $reject_region) {
		    my $errmsg = "sent $sent, received $received; expected proportion $p, measured proportion $p_hat";
		    &error(NAME_LOSS, $edge, $errmsg);
		}
	    }
	}

    }
    if (!$printsched) {
	kill_listener($listener_crude);
    }

    # wait for completion before next test.
    if ($printsched) {
	&schedlog("barrier $barriers_hit: loss: after test");
    }
    &barrier();
}

##############################################################################
# Latency Test Functions
##############################################################################

# returns whether the link latency is in a valid test range.
sub valid_latency {
    return TRUE;

}

# Pings a node and returns information.
# @param[0] := host to ping
# @param[1] := ttl, 0 for default
# @return: (received_count, avg_latency ms)
sub ping_node {
    my ($host,$ttl,$scount,$timo) = @_;
    my $count = 0;
    my $avg_latency = 0;
    my $stddev = 0;

    my $send_count = defined($scount) ? $scount : PING_SEND_COUNT;
    my $timeout = defined($timo) ? $timo : 3;
    my $send_rate = ($timeout > 1) ? (($timeout - 1) / $send_count) : 0;

    # set deadline to prevent long waits
    my $cmd;
    my $ttlarg = "";
    if($ttl) {
	if($platform eq BSD) {
	    $ttlarg = "-m $ttl";
	} elsif($platform eq LINUX) {
	    $ttlarg = "-t $ttl";
	} elsif($platform =~ /CYGWIN/) {
	    # The Windows system ping has a TTL arg; Cygwin ping doesn't. 
	    $ttlarg = "-i $ttl";
	}
    }
    my $srarg;
    if ($send_rate) {
	$srarg = "-i $send_rate";
    } else {
	$srarg = "";
    }
    if($platform eq BSD) {
	$cmd = "/sbin/ping -c $send_count -q $srarg -t $timeout $ttlarg $host";
    } elsif($platform eq LINUX) {
	$cmd = "/bin/ping -c $send_count -q $srarg -w $timeout $ttlarg $host";
    } elsif($platform =~ /CYGWIN/) {
	# Neither Windows nor Cygwin ping has either send rate or timeout.
	# Windows ping doesn't have -q, but it does have TTL, so use it.
	$cmd = "/cygdrive/c/WINDOWS/system32/ping.exe -n $send_count $ttlarg $host";
    }

    # note backticks passes SIGINT to child procs
    my @args = split(/\s+/,$cmd);
    my @results = &my_tick(@args);
    my $reslt_cnt = @results;
    my $result = $results[$reslt_cnt-2];
    if($platform eq BSD && $result =~ /(\d+) packets received/) {
	$count = $1;
    } elsif($platform eq LINUX && $result =~ /(\d+) received/) {
	$count = $1;
    } elsif($platform =~ /CYGWIN/ && 
	    $results[$reslt_cnt-3] =~ /Received = (\d+)/) {
	$count = $1;
    }

    if($count) {
	$result = $results[$reslt_cnt-1];
	if($result=~ /\d+\.\d+\/(\d+\.\d+)\/\d+\.\d+\/(\d+\.\d+)/) {
	    $avg_latency = $1;
	    $stddev = $2;
	} elsif($result=~ /Average = (\d+)ms/) {
	    $avg_latency = $1;
	    $stddev = 0.03;	# Stddev is not reported on Windows.
	}
    }
    return ($count, $avg_latency, $stddev);
}

#
# Ping each directly connected node once to force an ARP.
#
sub force_arp {
    my %waitlist;
    my @edge_copy = @links;

    while(&has_elems(\@edge_copy)) {
	my ($edge,$other_edge) = &get_twoway_assign(\@edge_copy, 0);
	if (defined($edge) && defined($other_edge)) {
	    if ($hostname eq $edge->src) {
		if ($printsched) {
		    &schedlog("ping " . schedprint_link($edge) .
			      " (pkts=1, pps=1, timo=1s)");
		    next;
		}
		my $pid = fork();
		if(!$pid) {
		    # call ping_node with ttl=1,count=1,timeout=1
		    &ping_node($edge->dst . "-" . $edge->name,1,1,1);
		    exit(EXIT_OK);
		} else {
		    $waitlist{$pid} = 1;
		}
	    }
	}
    }

    &wait_all(%waitlist);
    # wait for completion before next test.
    if ($printsched) {
	&schedlog("barrier $barriers_hit: arp: after test");
    }
    &barrier();
}

#
# Compute the packet header size used by the bandwidth shaper for its
# calculations.  We need this for various estimations.  Our argument is
# the link on which the packet is being sent.
#
# Header size depends on how the shaping is done:
#
# * Delay nodes running dummynet count IP/UDP/ethernet headers
#   (but *not* the 4 byte CRC)
#
# * End-node shaping ("linkdelays") on FreeBSD (dummynet again,
#   but at layer3) count only IP/UDP headers.  Linux end-node shaping
#   appears to be the same.
#
# * Veth encapsulation adds another 16 bytes to the overhead in the
#   non-linkdelay case.
#
sub header_size {
    my $edge = shift;

    # IP (20) + UDP (8)
    my $hsize = 20 + 8;

    if ($edge->dstyle ne "linkdelay") {
	# + eth (14)
	$hsize += 14;
	if ($edge->mpxstyle eq "veth") {
	    # + veth (16)
	    $hsize += 16;
	}
    }
    return $hsize;
}

#
# Compute the expected RTT value for a link.
#
# Facts from analysis in /users/davidand/public/calibrate.
# Came from 40 independent swapins (enough for normality assumption)
# (note that data actually is normal at any particular latency point, 
# according to described.lst)
#
# Best fit regression for the error as a function of total latency,
# according to sas.
# See regression1.lst and regression1.sas
# -0.00523(actual)     0.00003096 fbsd
# -0.00530(actual)     0.00003478 linux
# roughly identical, so use:
# -0.005(actual)
#
# Inherent delay in the system (with a delay node) is
# see described.lst and described.sas
# 0.337737  fbsd
# 0.362282  linux (median was 0.328000)
# round to:
# 0.333 ms
#
# Note, this has been measured and is in one of the emulab papers (Shashi)
#
# Also, described.lst provides good support for the notion that
# the distribution of latencies is normal. For Fbsd all of the 
# distributions were normal, and most were for Linux. So, use this
# assumption in order to send fewer test packets.
#
sub link_rtt {
    my ($edge,$other_edge,$psize) = @_;

    # XXX assume a ping packet
    $psize = 56
	if (!defined($psize));

    # the null hypothesis value, u.
    my $u = $edge->delay + $other_edge->delay;
			    
    # the calibration as a function of $u
    $u += 0.333 - 0.005 * $u / 2;

    #
    # Correct latency to account for transport delay.
    #
    # Strictly speaking, we need to account for this once the ethernet
    # bandwidth goes below 10Mb/sec which is the point at which the
    # media transport delay becomes significant to us (~1.2ms each way).
    # While calculating transport delay based on bandwidth seems easy
    # enough, it isn't really.  Two aspects of the current emulation
    # techniques make it more complicated.
    #
    # First, we always use 100Mb links to/from the delay nodes (or between
    # linkdelayed nodes).  Thus, in the absense of an explicit delay value,
    # as long as the transfer rate is less than the capped BW and the
    # individual packet size is sufficiently small (see next paragraph),
    # those packets will go across the wire with 100Mb ethernet latency
    # (about 120us) regardless of the BW setting.  This case has been
    # accounted for in our (empirically determined) base latency calculation
    # described above.
    #
    # Second, we also see quantization effects, from both Linux and BSD
    # emulations, once the bandwidth drops below a certain point.  For
    # example, in our ping-based latency test, a single ping packet counts
    # as 84 bytes or 672 bits (when using end-node shaping).  As long as the
    # ping rate does not exceed one packet per tick (1ms for end-node shaping)
    # and the allowed bit rate is greater than 672 bits/tick, then a packet
    # will never be delayed due to bandwidth shaping.  Otherwise, the
    # bandwidth shaper will delay each packet for one or more ticks til it
    # accumulates enough bandwidth credit to send the packet.  In our latency
    # test, the packet rate is never a problem (5 pings/sec), but the bandwidth
    # may be.  With a 1ms tick, once the bandwidth drops below 672 bits * 1000
    # ticks/sec == 672Kb/sec, packets will start to experience at least 1ms
    # of delay in each direction.
    #
    # So, given the IP payload size of a packet as a parameter, we calculate
    # the point at which transport time becomes significant and calculate
    # the added latency.
    #
    # ASSUMPTIONS:
    # * packet rate is no faster than 1 packet per tick
    # * < 1ms one-way latency is "insignificant"
    #
    my $bits_per_packet;
    my $bwthresh;

    $bits_per_packet = (&header_size($edge) + $psize) * 8;
    if ($edge->dstyle eq "linkdelay") {
	$bwthresh = $bits_per_packet * 1000;
    } else {
	$bwthresh = 10000000;
    }
    if ($edge->bw < $bwthresh) {
	$u += (1000 * $bits_per_packet / $edge->bw);
    }

    $bits_per_packet = (&header_size($other_edge) + $psize) * 8;
    if ($other_edge->dstyle eq "linkdelay") {
	$bwthresh = $bits_per_packet * 1000;
    } else {
	$bwthresh = 10000000;
    }
    if ($other_edge->bw < $bwthresh) {
	$u += (1000 * $bits_per_packet / $other_edge->bw);
    }

    #
    # XXX with dummynet, packets which are not queued for bandwidth shaping,
    # but go directly to the delay queue (either because there is no BW
    # specified or because there is not a backlog) may be delayed from [0-1]
    # tick short of the specified delay value depending on when during the
    # current tick the packet is queued.  So on average, it will be 1/2 tick
    # short for these packets on links with non-zero delay values.  With
    # endnode shaping where the tick value is 1ms, that will be on average
    # 1ms short for a round trip, enough that we will compensate for it here.
    #
    if ($edge->delay > 0 && $edge->dstyle eq "linkdelay" &&
	$hostmap{$edge->src}->os eq "FreeBSD") {
	$u -= 0.5;
    }
    if ($other_edge->delay > 0 && $other_edge->dstyle eq "linkdelay" &&
	$hostmap{$other_edge->src}->os eq "FreeBSD") {
	$u -= 0.5;
    }
    if ($u < 0.0) {
	$u = 0.0;
    }

    return $u;
}

# For directly connected hosts, checks latency using Ping.
sub latency_test {
    my %waitlist;
    my @edge_copy = @links;

    while(&has_elems(\@edge_copy)) {
	my ($edge,$other_edge) = &get_twoway_assign(\@edge_copy, 0);
	if(defined($edge) && defined($other_edge)) {
	    if($hostname eq $edge->src ) {
		# todo: consider ignoring latency if no delay node.
		if(&valid_latency($edge) && &valid_latency($other_edge)) {

		    #
		    # Tell ping to wait at least one round-trip time.
		    # XXX should be reconciled with expected values below.
		    #
		    my $ptimo = $edge->delay + $other_edge->delay;
		    $ptimo = int(($ptimo + 500) / 1000) + 1;
		    $ptimo = 3 if ($ptimo < 3);

		    if ($printsched) {
			&schedlog("ping " . schedprint_link($edge) .
				  " (pkts=" . PING_SEND_COUNT .
				  ", pps=" . (PING_SEND_COUNT/2) .
				  ", timo=${ptimo}s)");
			next;
		    }
		    my $pid = fork();
		    if(!$pid) {

			# call ping_node with ttl=1
			my ($result_cnt, $sample_avg, $sample_dev) =
			    &ping_node($edge->dst . "-" . $edge->name,
				       1, undef, $ptimo);
			
			if ($reportonly) {
			    my $u = &link_rtt($edge, $other_edge);

			    &info("    Latency result on $hostname for " .
				  &print_edge($edge) . 
				  ": count/avg/stddev = ".
				  "$result_cnt/$sample_avg/$sample_dev ".
				  "(expected $u)\n");
			    exit(EXIT_OK);
			}

			my $n = PING_SEND_COUNT;

			if($result_cnt == 0) {
			    my $errmsg = "No packets received (n=$n)\n";
			    &error(NAME_LATENCY, $edge, $errmsg);
			    exit(EXIT_NOT_OK);
			} else {
			    my $u = &link_rtt($edge, $other_edge);
			    
			    my $x_bar = $sample_avg;
			    my $numerator = $x_bar - $u;

			    my $S = $sample_dev;

			    # Very rarely, ping reports a 0.0 stddev.
			    $S = 0.03
				if ($S == 0.0);

			    my $denominator = $S / sqrt( abs( $n ) );

			    if($denominator == 0) {
				my $errmsg = "Invalid sample standard deviation (possible parse problem, please report). (n=$n, u=$u, x_bar=$x_bar, S=$S)";
				&error(NAME_LATENCY, $edge, $errmsg);
				exit(EXIT_NOT_OK);
			    } else {
				my $z = $numerator / $denominator;
				my $t_reject = 3.250; # alpha = 0.01, df=9

				if ((abs($z) > $t_reject) &&
				    (($x_bar < $u &&
				      (($u - $x_bar) >=
				       INSIGNIFICANT_LAT_ERROR_LO)) ||
				     ($x_bar > $u &&
				      (($x_bar - $u) >=
				       INSIGNIFICANT_LAT_ERROR_HI)))) {
				    &error(NAME_LATENCY, $edge,
					   "expected=$u, ".
					   "measured mean=$x_bar");
				    exit(EXIT_NOT_OK);
				}
			    }
			}
			exit(EXIT_OK);
		    } else {
			$waitlist{$pid} = 1;
		    }
		} else {
		    if ($printsched) {
			&schedlog("skipping latency test " .
				  schedprint_link($other_edge));
		    }
		    &debug("Skipping latency test for " . &print_link($edge) . " to " . &print_link($other_edge) . "\n");
		}
	    }
#
	}
    }

    &wait_all(%waitlist);
    # wait for completion before next test.
    if ($printsched) {
	&schedlog("barrier $barriers_hit: latency: after test");
    }
    &barrier();
}


##############################################################################
# Bandwidth Test Functions
##############################################################################

# Returns whether the link bandwidth is in a valid test range.
sub valid_bw {
    my $edge = shift @_;
    if($edge->bw >= LIMIT_BW_MIN
       && $edge->bw <= LIMIT_BW_HI
       && $edge->loss <= LIMIT_BW_LOSS
       ) {
	return TRUE;
    } else {
	return FALSE;
    }
}


# Checks bandwidth for directly connected links.
sub bw_test {
    my @analyze_list	= ();
    my @edge_copy	= @links;
    my $trun = 1;

    # lower expectations for windows
    my $bw_error_low = (($platform =~ /CYGWIN/) ? 
			INSIGNIFICANT_BW_ERROR_LO_Windows : 
			INSIGNIFICANT_BW_ERROR_LO);

    #
    # all nodes will execute the same reductions on the edge list
    # on their own so that the number of barriers is the same.
    #
    while (&has_elems(\@edge_copy)) {
	my ($edge,$redge) = &get_twoway_assign(\@edge_copy, 1);

	# Figure out what bw to use so as not to overflow the
	# system too badly. Take the max of the two edges and
	# add 10 percent.
	my $bw = 0;

	if (defined($edge) && defined($redge)) {
	    if($hostname eq $edge->dst) {
		#
		# iperf does a twoway test.
		#
		if (&valid_bw($edge)) {
		    $bw = $edge->bw
			if ($edge->bw > $bw);
		    
		    if (!$printsched) {
			push(@analyze_list, $edge);
		    }
		    &debug("    Starting bandwidth test on $hostname for " .
			  &print_link($edge) . "\n");
		}
		else {
		    if ($printsched) {
			&schedlog("skipping bandwidth test " .
				  schedprint_link($edge));
		    }
		    if ($edge->bw != LIMIT_BW_HI+1) {
			&debug("Skipping bandwidth test on $hostname for " .
			       &print_link($edge) . "\n");

			&info("*** Skipping bandwidth test on $hostname for " .
			      &print_link($edge) . "\n");
			&info("*** Bandwidth is out of range ".
			      "(" . LIMIT_BW_LO . " <= BW <= " .
			      LIMIT_BW_HI .") ". "or loss is too high (> " .
			      LIMIT_BW_LOSS . ").\n");
		    }
		}
		if (&valid_bw($redge)) {
		    $bw = $redge->bw
			if ($redge->bw > $bw);
		    
		    if (!$printsched) {
			push(@analyze_list, $redge);
		    }
		    &debug("    Starting bandwidth test on $hostname for " .
			  &print_link($redge) . "\n");
		}
		else {
		    if ($printsched) {
			&schedlog("skipping bandwidth test " .
				  schedprint_link($redge));
		    }
		    if ($redge->bw != LIMIT_BW_HI+1) {
			&debug("Skipping bandwidth test on $hostname for " .
			       &print_link($redge) . "\n");
		    
			&info("*** Skipping bandwidth test on $hostname for " .
			      &print_link($redge) . "\n");
			&info("*** Bandwidth is out of range ".
			      "(" . LIMIT_BW_LO . " <= BW <= " .
			      LIMIT_BW_HI .") ". "or loss is too high (> " .
			      LIMIT_BW_LOSS . ").\n");
		    }
		}

		# Okay, start the test.
		if (&valid_bw($edge) || &valid_bw($redge)) {
		    #
		    # Depending on the bw we are going to test at, set
		    # the duration:
		    #	high BW: 3 sec
		    #	med BW:  5 sec
		    #	low BW:  7 sec
		    # high loss: add 3 sec
		    #
		    my $duration = 5;
		    if ($bw >= LIMIT_BW_HI/3.0) {
			$duration = 3;
		    }
		    elsif ($bw < LIMIT_BW_MED) {
			$duration = 7;
		    }
		    if ($edge->loss > 0.10) {
			$duration += 3;
		    }
			
		    # Send a little faster to make sure its the delay
		    # node doing the throttling. 
		    my $bw = $bw + int($bw * 0.10);

		    #
		    # Even without packet loss, there is a good chance that
		    # iperf's end-of-stream (FIN) will be lost since we are
		    # over-driving the link.  The ACK for that FIN needs to
		    # be received before iperf's timer stops, so a long
		    # timeout (for resending the FIN) on a short duration
		    # run will lower the BW reading considerably.  So we
		    # attempt to keep the timeout as short as possible,
		    # taking into account the round-trip latency of the link.
		    # We may also need to adjust the run time of the test
		    # upward, but we do not exceed the indicated max duration. 
		    #
		    # 250 ms is the default timeout value for iperf.  We
		    # start with a candidate value of 50 ms which is chosen
		    # as it is sufficient for the 3% BW error on min duration
		    # (3 second) tests.  You should not pick a base acktime
		    # less than the resolution of the clock (10ms or 1ms).
		    #
		    my $acktime = 50;
		    my $clockres = ($edge->dstyle eq "linkdelay") ? 1 : 10;
		    my $minacktime = &link_rtt($edge, $redge);

		    #
		    # Ugh.  Since we are over-driving the link, our transmit
		    # queue is likely to be non-empty, delaying the FIN and
		    # thus further delaying the ACK.  So based on the edge BW
		    # and the default emulation queue size of 50, we estimate
		    # how long til we hit the wire and add that to the RTT.
		    #
		    my $psize = (&header_size($edge) + IPERF_PKTSIZE) * 8;
		    $minacktime += (($psize * 50/2) / $edge->bw) * 1000;
		    $minacktime = int($minacktime);

		    # must not be less than RTT or clock resolution
		    if ($minacktime < $clockres) {
			$minacktime = $clockres;
		    }
		    if ($acktime < $minacktime) {
			$acktime = $minacktime;
		    }

		    #
		    # If a single timeout would result in exceeding the
		    # target BW error, try lowering the timeout or lengthening
		    # the test.
		    #
		    my $maxacktime = $bw_error_low * $duration * 1000.0;
		    if ($acktime > $maxacktime) {
			$acktime = $minacktime;
			if ($acktime > $maxacktime) {
			    # compute duration necessary to achieve minacktime
			    my $dur = int(($minacktime + $bw_error_low*1000.0-1)
					  / ($bw_error_low * 1000.0) + 0.5);

			    # still doesn't fit, warn and try anyway
			    if ($dur > BW_TEST_MAXDURATION) {
				&debug("May see BW errors for " .
				       &print_link($edge) . "\n");
				&info("*** May see BW errors for " .
				      &print_link($edge) . "\n");
			    } else {
				$duration = $dur;
			    }
			}
		    }

		    # So we know what was sent in the analysis below.
		    $edge->testbw($bw);
		    $redge->testbw($bw);
		    $edge->bwtime($duration);
		    $redge->bwtime($duration);

		    if ($printsched) {
			&schedlog("iperf to " . $edge->src .
				  " on " . $edge->name .
				  " (bw=${bw}bps, time=${duration}s," .
				  " sbsize=200000, acktime=${acktime}ms)");
		    } else {
			&my_system(PATH_NICE, "-n", "-10", $PATH_IPERF,
				   "-c", $edge->src . "-" . $edge->name,
				   "-t", "$duration", "-f", "b",
				   "-r", "-u", "-w", "200000",
				   "-l", IPERF_PKTSIZE, "-b", "$bw",
				   "-x", "s", "-y", "c", "-A", $acktime,
				   "-L", "4444", "-o", IPERF_DAT);
		    }
		}
	    } 
	}
	if ($printsched) {
	    &schedlog("barrier $barriers_hit: bandwidth: after run $trun");
	    $trun++;
	}
	&barrier();
    }

    # read the log file.
    if (@analyze_list) {
	my @results = &read_file(IPERF_DAT);
	foreach my $edge (@analyze_list) {
	    my $found_results = 0;
	    
	    #
	    # Iperf result format:
	    #
	    #   [0]         [1]       [2]       [3]       [4]
	    # <datestamp>,<src-ip>,<src-port>,<dst-ip>,<dst-port>,
	    #
	    #  [5]      [6]       [7]      [8]
	    # <ID>,<start - end>,<bytes>,<bps-rate>,
	    #
	    #     [9]     [10]       [11]           [12]          [13]
	    # [ <jitter>,<drops>,<total-packets>,<%-dropped>,<pkts-out-order> ]
	    #
	    foreach my $result (@results) {
		my @stuff = split(",", $result);
		if (scalar(@stuff) < 9) {
		    die("Error parsing " . IPERF_DAT . "\n");
		}
		next
		    if (scalar(@stuff) == 9);
			
		my $myip     = $stuff[1];
		my $port     = $stuff[2];
		my $hisip    = $stuff[3];
		my $numsent  = $stuff[11];
		my $numpkts  = $stuff[11] - $stuff[10];
		my $duration = 0;

		if ($stuff[6] =~ /^([\d.]+)-([\d.]+)$/) {
		    $duration = abs($2 - $1);
		    # This was likely fixed by adding the -A option to iperf
		    ## Trim off excess; this is wrong.
		    #$duration = int($duration) * 1.0
		    #    if ($edge->loss > .10);
		}

		#
		# XXX Iperf uses *only* UDP payload length when calculating
		# the bandwidth. We want to add the rest of the overhead
		# before making the comparison below.
		#
		my $poh = &header_size($edge);
		my $bw = ((IPERF_PKTSIZE + $poh) * 8 * $numpkts) / $duration;
		
		#
		# iperf is a twoway test. Both edges represented in the file.
		#
		if (($hostname eq $edge->dst &&
		     $edge->dstip eq $myip && "$port" eq "4444" &&
		     $edge->srcip eq $hisip) ||
		    ($hostname eq $edge->src &&
		     $edge->dstip eq $myip && "$port" eq "5001" &&
		     $edge->srcip eq $hisip)) {
		    my $wanted   = $edge->bw;	# NS file amount
		    my $expected = $wanted;     # After applying loss
		    my $adjusted = $wanted;     # After applying extra 10%
		    my $diff     = abs($bw - $adjusted);
		    my $error    = undef;

		    #
		    # If there is loss on the channel, expected bandwidth
		    # goes down, but must take into account the fact that
		    # we added 10% above.
		    #
		    if ($edge->loss > 0) {
			# Loss will reduce expected BW by this much.
			$expected = $expected - ($expected * $edge->loss);

			# But we sent in 10% more then the max of both sides.
			$adjusted = ((IPERF_PKTSIZE + $poh) * 8 * $numsent)
			    / $duration;
			$adjusted -= ($edge->loss * $adjusted);

			# If that adjusted bandwidth is still higher then
			# the link BW setting, thats all we should get.
			if ($adjusted > $edge->bw) {
			    $adjusted = $edge->bw;
			}

			$diff = abs($bw - $adjusted);
			
			if ($reportonly) {
			    &info("    Bandwidth result on $hostname for " .
				  &print_edge($edge) .
				  ": wanted/expected/adjusted/actual = ".
				  "$wanted/$expected/$adjusted/$bw\n");
			}
		    }
		    else {
			if ($reportonly) {
			    &info("    Bandwidth result on $hostname for " .
				  &print_edge($edge) .
				  ": wanted/actual = $wanted/$bw\n");
			}
		    }

		    &debug("BW results on $hostname for " .
			   &print_edge($edge) . ": ".
			   "$bw/$wanted/$expected/$adjusted/$diff/".
			   "$numsent/$numpkts/$duration/" .
			   $edge->bwtime . "\n");

		    if ($reportonly) {
			$bw = $adjusted;
		    }

		    #
		    # The measurement tool does not give perfect results.
		    # However, it reports low all the time, so if it reports
		    # high, then the link is almost certainly bad.
                    #
		    if ($bw > $adjusted) {
			if ($diff > ($adjusted * INSIGNIFICANT_BW_ERROR_HI)) {
			    $error = "higher";
			}
		    }
		    elsif ($bw < $adjusted) {
			if ($diff > ($adjusted * $bw_error_low)) {
			    $error = "lower";
			}
		    }
		    if (defined($error)) {
			&error(NAME_BW, $edge,
			       "Measured $bw, Expected $adjusted bps");
		    }
		    $found_results = 1;
		    last;
		}
	    }
	    if (!$found_results) {
		&error(NAME_BW, $edge, "Could not find results!");
	    }
	}
    }
    if (!$printsched) {
	kill_listener($listener_iperf);
    }

    # wait for completion before termination so that all errors reported in.
    if ($printsched) {
	&schedlog("barrier $barriers_hit: bandwidth: after test");
    }
    &barrier();
}


##############################################################################
# Static Routing Connectivity Test Functions
##############################################################################

# Traverse the links between nodes to figure out which nodes are actually
# reachable.  First parameter is a reference to an array that should be filled
# out with node names.  The second parameter contains the name of the node to
# visit.
sub reachable_nodes {
    my ($nodes_ref, $currnode) = @_;

    $hostmap{$currnode}->visited(1);
    foreach my $edge (@{ $hostmap{$currnode}->links }) {
	my $nextnode;
	my $lan = $edge->name;

	if ($edge->src eq $currnode) {
	    $nextnode = $edge->dst;
	} 
	else {
	    $nextnode = $edge->src;
	}
	if ($hostmap{$nextnode}->visited == 0) {
	    if ($currnode ne $hostname) { # Don't add 1st hop nodes.
		push(@{$nodes_ref}, "$nextnode:$lan");
	    }
	    &reachable_nodes($nodes_ref, $nextnode);
	}
    }
    $hostmap{$currnode}->visited(2);
}

# Attempts to reach nodes that are not on a direct link
# with this host. IE, use TTL > 1. Pings are in parallel.
sub static_rt_test {
    my @nodes = ();
    my %okay  = ();
    my $maxtrials = ($rtproto eq RTPROTO_SESSION ? 2 : 1);

    &reachable_nodes(\@nodes, $hostname);
    &debug("Route test nodes: @nodes\n");
    
    #
    # Because of session routing, we run failed nodes twice, in the
    # hopes that the routes stabilize between the first and second runs.
    #
    for (my $trial = 0; $trial < $maxtrials; $trial++) {
	my %waitlist  = ();
	my $waitcount = 0;
	
	# fork processes to run the pings in parallel.
	foreach my $dst (@nodes) {
	    my ($host,$lan) = split(":", $dst);
	    my $dstname     = "${host}-${lan}";
	
	    if ($printsched) {
		&schedlog("ping $host on $lan" .
			  " (pkts=" . PING_SEND_COUNT .
			  ", pps=" . (PING_SEND_COUNT/2) .
			  ", timo=3s)");
	    
		$okay{$dst} = 1;
		$waitcount++;
	    } else {
		my $pid = fork();
		if (!$pid) {
		    my ($recv_cnt) = &ping_node($dstname, 0);
		    
		    if (!$recv_cnt) {
			&debug("Attempting to reach $dstname ... Failed!\n");
			exit(EXIT_NOT_OK);
		    }
		    &debug("Attempting to reach $dstname ... OK\n");
		    exit(EXIT_OK);
		}
		else {
		    $waitlist{$dst} = $pid;
		    $waitcount++;
		}
	    }

	    #
	    # If the count gets too high, lets stop and wait.
	    #
	    if ($waitcount > 10) {
		if ($printsched) {
		    &schedlog("pausing to wait for pings");
		}
		&debug("Pausing to wait for outstanding pings to clear ...\n");

		foreach my $name (keys(%waitlist)) {
		    my $wpid = $waitlist{$name};

		    waitpid($wpid, 0);
		    if ($? == 0) {
			$okay{$name} = 1;
		    }
		}
		$waitcount = 0;
		%waitlist  = ();
	    }
	}
	# Wait for stragglers.
	if ($printsched) {
	    &schedlog("waiting for outstanding pings");
	}
	&debug("Waiting for outstanding pings to clear ...\n");
	foreach my $name (keys(%waitlist)) {
	    my $wpid = $waitlist{$name};

	    waitpid($wpid, 0);
	    if ($? == 0) {
		$okay{$name} = 1;
	    }
	}

	#
	# See if any failed, and if so lets delay a bit more.
	#
	if ($trial < ($maxtrials - 1)) {
	    last 
		if (scalar(@nodes) == scalar(keys(%okay)));

	    my $seconds = 60 + int(scalar(@nodes) * 0.25);
		
	    &debug("Some nodes failed to respond during trial $trial!\n");
	    &debug("Waiting for $seconds seconds, and then trying again.\n");

	    sleep($seconds);
	}
    }
    #
    # Now look at the final results.
    #
    if (!$printsched) {
	foreach my $dst (@nodes) {
	    if (! exists($okay{$dst})) {
		my ($host,$lan) = split(":", $dst);
		my $dstname     = "${host}-${lan}";
		
		# only report failures
		if ($reportonly) {
		    &info("    Routing result on $hostname for $dstname: ".
			  "failed\n");
		} else {
		    &error(NAME_RT_STATIC, undef,
			   "$hostname could not ping $dstname");
		}
	    }
	}
    }

    # wait for completion before next test.
    if ($printsched) {
	&schedlog("barrier $barriers_hit: routing: after test");
    }
    &barrier();
}



##############################################################################
# Utility Functions
##############################################################################

# Convenience to print information about a link.
sub print_link {
    my $edge = shift @_;
    my $str = $edge->src . "-" . $edge->name . " to " .
	$edge->dst . "-" . $edge->name;

    $str .= " (" . ($edge->bw / 1000000)  . " Mbps, " .
	$edge->delay . "ms, " . (100 * $edge->loss) . "% loss, " .
	$edge->queuetype . ")";

    return $str;
}

# Print link with associated physical info
sub print_plink {
    my $edge = shift @_;
    my $str;

    if (!defined($hostmap{$edge->src}->phost) ||
	!defined($hostmap{$edge->dst}->phost) ||
	!defined($edge->mac)) {
	return &print_link($edge);
    }

    $str = $edge->src . "-" . $edge->name . " [" .
	$hostmap{$edge->src}->phost . "/" . $edge->mac . "]";
    $str .= " to " . $edge->dst . "-" . $edge->name . " [" .
	$hostmap{$edge->dst}->phost . "]";

    $str .= " (" . ($edge->bw / 1000000)  . " Mbps, " .
	$edge->delay . "ms, " . (100 * $edge->loss) . "% loss, " .
	$edge->queuetype . ")";

    return $str;
}

sub print_edge {
    my $edge = shift @_;
    my $str = $edge->src . "-" . $edge->name . " to " .
	$edge->dst . "-" . $edge->name;

    return $str;
}

#
# Handles reading the topology map(s).
# Note: the topo map file can be huge, so we read it line at a time
# rather than en masse with read_file.
#
sub get_topo {
    my ($top_file, $ptop_file) = @_;

    die "Attempted to open missing file $top_file\n" 
	unless -e $top_file;

    open TOPO, $top_file || die ("Could not open $top_file\n");
    while(<TOPO>) {
	# load the output from ns.
	# the file format is simple:
	# expr := h <node name>
	#      || l <src node> <dst node> <bw (Mb/s)> <latency (s)> <loss (%)>
	if( /^h (\S+)/ ) {
	    push @hosts, $1;
	    my $newHost = new host;
	    $newHost->name($1);
	    $newHost->visited(0);
	    # assume the node supports linktest unless told otherwise
	    $newHost->osfeatures("linktest");
	    $hostmap{$1} = $newHost;
	}
	elsif (/^l (\S+)\s+(\S+)\s+(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\S+)\s+(\S+)/ ||
	       /^l (\S+)\s+(\S+)\s+(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\S+)/) {
	    my $newEdge = new edge;
	    $newEdge->name($6);
	    $newEdge->src($1);
	    $newEdge->srcip(hostip("$1-$6"));
	    $newEdge->dst($2);
	    $newEdge->dstip(hostip("$2-$6"));
	    $newEdge->bw($3);
	    $newEdge->delay($4 * 1000); # units of ms
	    $newEdge->loss($5);
	    my $qt = defined($7) ? $7 : "droptail";
	    $newEdge->queuetype($qt);
	    $newEdge->mpxstyle("none");
	    $newEdge->dstyle("dnode");
	    $newEdge->symlanignore(0);
	    push @links, $newEdge;

	    push @{ $hostmap{$newEdge->src}->links }, $newEdge;
	    push @{ $hostmap{$newEdge->dst}->links }, $newEdge;
	} elsif (/^r static/i) {
	    $rtproto = RTPROTO_STATIC;
	} elsif (/^r ospf/i) {
	    $rtproto = RTPROTO_SESSION;
	} elsif (/^r none/i) {
	    ;
	} elsif (/^s ([-\w\(\)]+)/i) {
	    $simname = $1;
	} else {
	    print "Bad line in map: $_\n";
	}
    }
    close(TOPO);

    #
    # Augment with physical info if present
    #
    if (open(TOPO, $ptop_file)) {
	my $vers = 1;
	while(<TOPO>) {
	    my @row = split;
	    if ($row[0] eq "V") {
		if ($row[1] > 2) {
		    print "Unknown version $row[1] of physical topo info\n";
		    last;
		}
		$vers = $row[1];
		next;
	    }
	    if ($row[0] eq "H") {
		if (!exists($hostmap{$row[1]})) {
		    print "Unknown host $row[1]\n";
		    next;
		}
		my $host = $hostmap{$row[1]};
		$host->pname($row[2]);
		$host->phost($row[3]);
		$host->ptype($row[4]);
		$host->osid($row[5]);
		$host->os($row[6]);
		$host->osvers($row[7]);
		if ($vers > 1) {
		    $host->osfeatures($row[8]);
		}
		# if pname != phost, we have a vnode
		if ($row[2] ne $row[3]) {
		    $host->isvnode(1);
		    $numvnodes++;
		} else {
		    $host->isvnode(0);
		}
		# map virtual to physical host names
		$vhostmap{$row[1]} = $row[3];
		next;
	    }
	    if ($row[0] eq "L") {
		if (!exists($hostmap{$row[1]})) {
		    print "Unknown host $row[1] in link info\n";
		    next;
		}
		foreach my $edge (@{ $hostmap{$row[1]}->links }) {
		    if ($row[1] eq $edge->src && $row[2] eq $edge->dst &&
			$row[3] eq $edge->name) {
			$edge->mac($row[4]);
			$edge->mpxstyle($row[5]);
			$edge->dstyle($row[6]);

			#
			# If the link is not doing BW shaping
			# (tb-set-noshaping) then reflect this as best
			# we can.  We set the BW above the max so that
			# the test will be skipped.  We retain the base
			# shaping style (dnode or linkdelays) for the
			# benefit of other tests in the code.
			#
			if ($edge->dstyle =~ /(\w+)-nobw$/) {
			    $edge->bw(LIMIT_BW_HI+1);
			    $edge->dstyle($1);
			    $warn_unshaped_links++;
			}

			last;
		    }
		}
		next;
	    }
	    print "Unknown ltpmap record '$row[0]'\n";
	}
	close(TOPO);

	#
	# Arrange to run linktest only on nodes which support it.
	# We will ignore all others (with a warning)
	#
	my %badhosts = ();
	my @goodhosts = ();
	foreach my $vert (@hosts) {
	    my $host = $hostmap{$vert};
	    if ($host->osfeatures !~ /linktest/) {
		$badhosts{$vert} = 1;
		delete($hostmap{$vert});
	    } else {
		push(@goodhosts, $vert);
	    }
	}
	if (scalar(keys(%badhosts)) > 0) {
	    $warn_partial_test++;
	    @hosts = @goodhosts;

	    #
	    # Now remove all links involving these hosts and remove
	    # the hosts from the hostmap.
	    #
	    my @goodlinks = ();
	    foreach my $edge (@links) {
		if (!$badhosts{$edge->src} && !$badhosts{$edge->dst}) {
		    push(@goodlinks, $edge);
		}
	    }
	    @links = @goodlinks;

	    # recompute the good hosts' edge lists
	    foreach my $host (@hosts) {
		$hostmap{$host}->links([]);
	    }
	    foreach my $edge (@links) {
		push @{ $hostmap{$edge->src}->links }, $edge;
		push @{ $hostmap{$edge->dst}->links }, $edge;
	    }
	}
    }

    my %symlan;
    my %linklist;

    #
    # Now that we have filtered out any uninvolved hosts and links,
    # we can compute the assorted auxilliary maps
    #
    my %linkmap;
    foreach my $edge (@links) {
	my $name = $edge->name;
	my $src = $edge->src;

	# keep track of LAN members
	if (!defined($linklist{$name})) {
	    $linklist{$name} = [ $src ];
	    $linkmembers{$name} = 1;
	} elsif (!grep(/^$src$/, @{$linklist{$name}})) {
	    push(@{$linklist{$name}}, $src);
	    $linkmembers{$name}++;
	}

	# keep track of symmetric LANs
	if (!defined($linkmap{$name})) {
	    @{$linkmap{$name}} = ($edge->bw, $edge->delay, $edge->loss,
				  $edge->queuetype);
	    $symlan{$name} = 1;
	} elsif (exists($symlan{$name})) {
	    my ($bw, $del, $plr, $qs) = @{$linkmap{$name}};
	    if ($bw ne $edge->bw || $del ne $edge->delay ||
		$plr ne $edge->loss || $qs ne $edge->queuetype) {
		delete $symlan{$name};
	    }
	}
    }

    #
    # For every symlan, make sure there are an even number of nodes
    # so we can do pairwise tests.  We add the first node a second time
    # if necessary to assure this.
    #
    # From this list, we then mark the edges in symmetric LANs that can
    # be ignored for "once-only" style tests.  get_twoway_assign will
    # ignore these links when passed the appropriate option.
    #
    foreach my $lan (keys %symlan) {
	my @list = @{$linklist{$lan}};
	if (scalar(@list) % 2) {
	    push(@list, $list[0]);
	}
	$symlan{$lan} = \@list;
    }
    foreach my $edge (@links) {
	if (!exists($symlan{$edge->name})) {
	    next;
	}
	my @list = @{$symlan{$edge->name}};
	my $good = 0;
	for (my $i = 0; $i < @list; $i += 2) {
	    if ($edge->src eq $list[$i] && $edge->dst eq $list[$i+1] ||
		$edge->dst eq $list[$i] && $edge->src eq $list[$i+1]) {
		$good = 1;
		last;
	    }
	}
	if (!$good) {
	    $edge->symlanignore(1);
	}
    }

    #
    # Finally, put the lists in sorted order.
    #
    @hosts = sort { $a cmp $b } @hosts;
    @links = sort { $a->src . $a->dst . $a->name 
			cmp $b->src . $b->dst . $a->name } @links;
}

# Send an info message.
sub info {
    my($msg) = @_;
    
    &post_event2(EVENT_REPORT, "    " . $msg);
    &sim_event2(EVENT_LOG, "    " . $msg);
}

# prints out the topology read in from the NS file
sub debug_top {
    &debug("nodes:\n");
    foreach my $vert (@hosts) {
	if (exists($hostmap{$vert}) && defined($hostmap{$vert}->pname)) {
	    my $host = $hostmap{$vert};
	    &debug( " " . $vert . " " . $host->pname . " " .
		    $host->phost . " " . $host->ptype . " " .
		    $host->os . " " . $host->osvers . " " .
		    $host->osfeatures . "\n");
	} else {
	    &debug( " " . $vert . "\n");
	}
    }
    &debug("links:\n");
    foreach my $edge (@links) {
	if (defined($edge->mac)) {
	    &debug( " " . $edge->src . " " . $edge->dst . " " . $edge->name
		    . " " . $edge->bw . " " . $edge->delay . " " . $edge->loss
		    . " "
		    . $edge->mac . " " . $edge->mpxstyle . " " . $edge->dstyle
		    . "\n"
		    );

	} else {
	    &debug( " " . $edge->src . " " . $edge->dst . " " . $edge->bw
		    . " " . $edge->delay . " " . $edge->loss . "\n"
		    );
	}
    }
    &debug("routing protocol: $rtproto\n") if defined($rtproto);
}

# log to expt problem directory.
sub error {
    my($test,$edge,$msg) = @_;

    $error_count += 1;

    my $output = "$test error: ";
    if (defined($edge)) {
	$output .= &print_plink($edge) . " ";
    }
    $output .= ": $msg\n\n";

    &debug($output);    
    &append_file($linktest_path . "/" . $hostname . SUFFIX_ERROR,
		 $output);
}

sub report_status {
    my ($test) = @_;
    $test = lc($test);

    if ($hostname eq $synserv) {
	if ($stage_error_count) {
	    my $msg = "  Some $test tests had errors!";
	    &post_event(EVENT_REPORT,$msg);
	    &sim_event(EVENT_LOG,$msg);
	    &debug("\n$msg\n");
	}
	elsif (!$reportonly) {
	    my $msg = "  All $test tests were successful!";
	    &post_event(EVENT_REPORT,$msg);
	    &sim_event(EVENT_LOG,$msg);
	    &debug("\n$msg\n");
	}
    }
    $total_error_count += $stage_error_count;
    $error_count = 0;
    $stage_error_count = 0;
}

sub barrier {
    my $rc;
    
    if ($printsched) {
	$barriers_hit++;
	return;
    }

    if ($hostname eq $synserv) {
	return
	    if (! $barr_count);

	$rc = &my_system($PATH_EMULAB_SYNC,"-i",$barr_count,
			 "-n",SYNC_NAMESPACE,
			 "-e",$error_count);
    }
    else {
	$rc = &my_system($PATH_EMULAB_SYNC,"-n",
			 SYNC_NAMESPACE,"-e",$error_count);
    }

    # All peers get error notification.
    if ($rc) {
	$stage_error_count += 1;
    }
}

sub debug {
    return unless $debug_level;
    print "@_";
}

sub schedlog {
    return unless $printsched;

    print SCHED strftime("%b %e %H:%M:%S", localtime), " [$hostname]: ",
	        "@_", "\n";
}

sub schedprint_link {
    my $edge = shift;
    my $str = "to " . $edge->dst . " on " . $edge->name;
    return $str;
}

# returns one edge at a time, reserving two nodes.
sub get_assign {
    my ($todo_ref) = @_; # must maintain sorted order invariant
    my $task = undef;
    my @thisrun;

    # build a fresh hash to see which nodes are in use.
    my %inuse;
    foreach (@hosts) {
	$inuse{$_}=0;
    }

    for(my $i=0;$i<@{$todo_ref};$i++) {
	my $edge = @{$todo_ref}[$i];
	if(defined($edge) && !($inuse{$edge->src} || $inuse {$edge->dst})) {
	    $inuse{$edge->src} = 1;
	    $inuse{$edge->dst} = 1;
	    push @thisrun,$edge;
	    @{$todo_ref}[$i] = undef;
	}
    }

    # figure out the tasks for this particular host.
    foreach my $edge (@thisrun) {
	if($hostname eq $edge->src || $hostname eq $edge->dst ) {
	    $task = $edge;
	}
    }

    # each machine should reduce the todo list the same order due to
    # alphabetic sorting of info from the ns file.
    # only thing left to do is return this machines assignment for processing.
    return $task; # or undef if no jobs left for this host.
}


# returns two edges at a time, reserving two nodes.
sub get_twoway_assign {
    my ($todo_ref, $onceperlink) = @_;
    my $task = undef;
    my $other_task = undef;
    my @thisrun;

    #
    # For vnodes, we allow up to two pairs of tests on the same physical node
    #
    my $tlimit = 4;

    # build a fresh hash to see which nodes are in use.
    my %inuse;
    map { $inuse{$_} = 0 } @hosts;
    my %vinuse;
    if ($numvnodes > 0) {
	map { $vinuse{$vhostmap{$_}} = 0 } @hosts;
    }

    for(my $i=0;$i<@{$todo_ref};$i++) {
	my $edge = @{$todo_ref}[$i];
	my ($src,$dst);

	# assigned in a previous call
	if (!defined($edge)) {
	    next;
	}

	#
	# No node can be used more than once in a pass.
	#
	if ($inuse{$edge->src} || $inuse{$edge->dst}) {
	    next;
	}

	#
	# If vnodes are in use, no vnode host can be used in more than
	# a small number of tests at once.
	#
	if ($numvnodes > 0 &&
	    ($vinuse{$vhostmap{$edge->src}} >= $tlimit ||
	     $vinuse{$vhostmap{$edge->dst}} >= $tlimit)) {
	    next;
	}

	# we should ignore this symmetric lan link (for future calls too)
	if ($onceperlink && $edge->symlanignore) {
	    @{$todo_ref}[$i]=undef;
	    next;
	}

	$inuse{$edge->src}++;
	$inuse{$edge->dst}++;
	if ($numvnodes) {
	    $vinuse{$vhostmap{$edge->src}}++;
	    $vinuse{$vhostmap{$edge->dst}}++;
	}

	push @thisrun, $edge;
	@{$todo_ref}[$i]=undef;

	# get the other side
	for(my $j=$i;$j<@{$todo_ref};$j++) {
	    my $otheredge = @{$todo_ref}[$j];
	    if(defined($otheredge)
	       && $edge->src eq $otheredge->dst
	       && $edge->dst eq $otheredge->src
	       && $edge->name eq $otheredge->name) {
		push @thisrun,$otheredge;
		@{$todo_ref}[$j] = undef;
	    }
	}
    }

    # figure out the tasks for this particular host.
    foreach my $edge (@thisrun) {
	if($hostname eq $edge->src || $hostname eq $edge->dst) {
	    $task = $edge;
	    last;
	}
    }
    if(defined($task)) {
	foreach my $edge (@thisrun) {
	    if($task->dst eq $edge->src && $task->src eq $edge->dst) {
		$other_task = $edge;
		last;
	    }
	}
    }

    return ($task,$other_task); # or undef if no jobs left for this machine.
}

sub has_elems {
    my ($todo_ref) = @_;
    foreach (@{$todo_ref}) {
	if(defined($_)) {
	    return 1;
	}
    }
    return 0;
}


sub round {
    my($number) = shift;
    return int($number + .5);
}

# wait for all procs in the list argument to exit
sub wait_all {
    my (%list_ref) = @_;
    while (scalar(%list_ref)) {
	my $pid = wait();
	if ($? >> 8) {
	    $error_count += 1;
	}
	delete $list_ref{$pid};
    }
}

#
# post_event sends an event to the ltevent process running on ops (the
# controlling process for linktest). The REPORT events are displayed to
# the user.
#
sub post_event {
    my ($event,$args) = map { $1 if (/(.*)/) } @_;
    if($hostname eq $synserv) {
	if ($LOGRUN ne "") {
	    system("$LOGRUN '$args'");
	} elsif ($EVENTSERVER ne "") {

	    if ($printsched) {
		&schedlog("syncserver ltevent $event");
		return;
	    }

	    system($PATH_LTEVENT,
		   "-s",
		   $EVENTSERVER,
		   "-e",
		   "$proj_id/$exp_id",
		   "-k", 
		   $PATH_KEYFILE,
		   "-x",
		   "$event",
		   "$args");
	} else {
	    print $args . "\n";
	}
    }
}

sub post_event2 {
    my ($event,$args) = map { $1 if (/(.*)/) } @_;

    if ($LOGRUN ne "") {
	system("$LOGRUN '$args'");
    } elsif ($EVENTSERVER ne "") {
	if ($printsched) {
	    &schedlog("ltevent $event");
	    return;
	}

	system($PATH_LTEVENT,
	       "-s",
	       $EVENTSERVER,
	       "-e",
	       "$proj_id/$exp_id",
	       "-k", 
	       $PATH_KEYFILE,
	       "-x",
	       "$event",
	       "$args");
    } else {
	print $args . "\n";
    }
}

#
# sim_event sends an event to the event scheduler. EVENT_LOG events
# deposit their message into the event scheduler log file. Kinda silly
# if ya ask me!
#
sub sim_event {
    my ($event,$args) = map { $1 if (/(.*)/) } @_;
    if($hostname eq $synserv && $EVENTSERVER ne "") {

	if ($printsched) {
	    &schedlog("syncserver tevc $event");
	    return;
	}

	system($PATH_TEVC,
	       "-e", "$proj_id/$exp_id",
	       "now",
	       $simname,
	       "$event",
	       "$args");
    }
}

sub sim_event2 {
    my ($event,$args) = map { $1 if (/(.*)/) } @_;

    if ($EVENTSERVER ne "") {
	if ($printsched) {
	    &schedlog("tevc $event");
	    return;
	}

	system($PATH_TEVC,
	       "-e", "$proj_id/$exp_id",
	       "now",
	       $simname,
	       "$event",
	       "$args");
    }
}

# cleanup any child procs.
sub cleanup {
    my @pidlist = keys(%kill_list);
    
    &debug("Cleaning up @pidlist\n");
    kill 9, @pidlist;
}

sub dotest {
    my $level = shift @_;
    if($level >= $startat && $level <= $stopat) {
	return TRUE;
    } else {
	return FALSE;
    }
}

# an alternative to backticks to pass taint mode.
sub my_tick {
    # first arg has to be a file, so at least check that here.
    my $fname = &check_filename(shift @_);
    my @args = map { $1 if (/(.*)/) } @_;

    my @results;

    open(FROM, "-|") or exec $fname, @args;
    while( <FROM>) {
	push @results,$_;
    };
    close FROM;
    return @results;
}

#
# Opens a file, reads its contents, and returns the contents in a list.
#
# @param The filename as a string.
#
sub read_file {
    my @results;
    my $filename = &check_filename($_[0]);

    die "Attempted to open missing file $filename\n" 
	unless -e $filename;

    open FILE, $filename || die ("Could not open $filename\n");
    while(<FILE>) {
	chomp;
	push @results, $_;
    };
    close FILE;
    return @results;
	
}



# Use my_system instead of system
# for longer-running tasks for which output is redirected to null and
# the procid is saved in the kill list.
#
# @param: accepts a list of arguments for exec.
sub my_system {
    my $retval = 0;

    &check_filename($_[0]);

    foreach my $param (@_) {
	&debug($param . " ");
    }
    &debug("\n");
    if(my $pid =fork) {
	$kill_list{$pid} = $pid;
	waitpid($pid,0);
	$retval = $? >> 8;
	delete($kill_list{$pid});
    } else {
	if($debug_level < DEBUG_ALL) {
	    open(STDOUT, "/dev/null") ;
	    open(STDERR, $linktest_path . "/" . $hostname . SUFFIX_ERROR) ;
	}
	my @args = map { $1 if (/(.*)/) } @_;
	exec(@args);
    }
    return $retval;
}

# permutation of my_system to start but not wait for child procs.
sub start_listener {
    &check_filename($_[0]);

    if (my $pid = fork()) {
	$kill_list{$pid} = $pid;
	return $pid;
    }
    else {
	if($debug_level < DEBUG_ALL) {
	    open(STDOUT, "/dev/null") ;
	    open(STDERR, $linktest_path . "/" . $hostname . SUFFIX_ERROR) ;
	}
	exec(@_);
    }
}

sub kill_listener {
    my $pid = $_[0];

    return
	if (! exists($kill_list{$pid}));

    kill 9, $pid;
    waitpid($pid, 0);
    delete($kill_list{$pid});
}

sub check_filename {
    my $fname = shift @_;
    
    # taint check: /something/something.out
    if($fname =~ /^(\/?(?:[\/\w-]*(?:\.\w+)?)*)$/) {
	return "$1";
    } else {
	die("Taint detected: $fname\n");
    }

}

sub write_file{
    my ($fname,@list) = @_;
    my $untainted_filename = &check_filename($fname);
    &do_unlink($untainted_filename);

    open FILE,">$untainted_filename"  
	|| die("could not open $untainted_filename for writing: $!");
    foreach (@list) {
	print FILE $_;
    }
    close FILE;

}


sub do_unlink {
    my $ut_fname = &check_filename(shift @_);
    my $res;

    if( -e $ut_fname) {
	&debug("unlink $ut_fname\n");
	$res = unlink $ut_fname;
	if(!$res) {
	    die("Could not delete $ut_fname: $!");
	}
    }
}

# phaseout- want better logs in a format.
#sub log {
#    my $msg = shift @_;
#    &debug($msg);
#    &append_file($log_file,$msg);
#}

sub append_file {
    my $fname = &check_filename(shift @_);
    open FILE,">>$fname" 
	|| die ("Could not append to $fname: $!");
    if ($swapperid) {
	chown($swapperid, $swappergid, $fname);
    }
    print FILE "@_";
    close FILE;
}

sub is_special_node {
    if($hostname eq $synserv) {
	return TRUE;
    } else {
	return FALSE;
    }
}

sub linux_version {
    my $vers = "linux";
    if (-e "/etc/redhat-release") {
	my $foo = `cat /etc/redhat-release`;
	if ($foo =~ /Red Hat Linux release 9/) {
	    $vers = "linux9";
	}
    }
    return $vers;
}

sub hostip {
    my $host = shift(@_);
    
    my (undef,undef,undef,undef,@ipaddrs) = gethostbyname($host);
    return inet_ntoa($ipaddrs[0]);
}

sub TimeStamp()
{
    return POSIX::strftime("%H:%M:%S", localtime());
}

sub PATH_NICE()
{
    return "/bin/nice" if (-x "/bin/nice");
    return "/usr/bin/nice" if (-x "/usr/bin/nice");
    return "nice";
}
