#!/usr/bin/perl -w
#
# The linktest script.
# -- if there is a problem, write a file to /proj/<exp directory>/tbdata/linktest
#
# Assumption: all nodes came up already (part of swapin).
#
# @author: davidand
# @created: 10/13/03
use strict;
use lib qw (/proj/utahstud/users/davidand/lib);
use Statistics::Descriptive qw(:all);

use Class::Struct;
use constant PATH_CRUDE_LOG => "/tmp/linktest.crude";
use constant PATH_CRUDE_LOG_DECODED => "/tmp/linktest.crude.d";
use constant PATH_RUDE_CFG => "/tmp/linktest.rude";
use constant PATH_RATE_LOG => "/tmp/linktest.pathrate";

#### globals ####################################
my $synch;    # node having synch server

# programs used by the tests not on the path by default
my $ns_cmd;
my $crude_cmd;
my $rude_cmd; 
my $pathrate_snd_cmd; 
my $pathrate_rcv_cmd; 
my $hostname;
my $eid;
my $pid;
my $gid;

# directed graph representing expt topology
# verts sorted by hostname
my @verts;
# edges sorted by src . dst
my @edges;

# struct for representing an edge
struct ( edge => {
    src => '$',
    dst => '$',
    bw  => '$',
    delay => '$',
    loss => '$'
});

# log path(s)
my $problem_path;

#### main    ####################################
&init;
&debug_top;
&start_sync;

# wait for all nodes to finish parsing.
&barrier;

# now do the tests.
&routing_test;
&link_test;
&latloss_test;
#&bw_test;        # works but analysis not done yet

#### procs   ####################################

# initialize paths, etc.
sub init {
    # temporarily hardcoded for linux
    $ns_cmd = "/proj/utahstud/users/davidand/linux/ns-2.26/ns";
    $crude_cmd = "/proj/utahstud/users/davidand/bin_rhl/crude";
    $rude_cmd =  "/proj/utahstud/users/davidand/bin_rhl/rude";
    $pathrate_snd_cmd = "/proj/utahstud/users/davidand/bin_rhl/pathrate_snd";
    $pathrate_rcv_cmd = "/proj/utahstud/users/davidand/bin_rhl/pathrate_rcv";


    my $nick = `cat /var/emulab/boot/nickname`;
    ($hostname, $eid, $pid) = split /\./, $nick;
    chomp $hostname;
    chomp $eid;
    chomp $pid;
    $gid = $pid;

    &get_topo;

    $problem_path = "/proj/$pid/exp/$eid/tbdata/linktest";

    @verts = sort { $a cmp $b } @verts;
    $synch = $verts[0]; # synch is first alphabetical node

    @edges = sort { $a->src . $a->dst cmp $b->src . $b->dst } @edges;

}    

# invoke ns and parse the representation into @verts and @edges
sub get_topo {
    # 1. get ns file.
    # pattern: /groups/<pid>/<gid>/exp/<eid>

    # is that not working? workaround:
    my $ns_file = "/proj/$pid/exp/$eid/tbdata/$eid.ns";
    &debug($ns_file);
    
    # 2. run it through ns.
    my @ns_output = `$ns_cmd $ns_file`;
    foreach my $line (@ns_output) {
	chomp ($line);
	
	# load the output from ns.
	# the file format is simple:
	# expr := h <node name>
	#      || l <src node> <dst node> <bw (Mb/s)> <latency (s)> <loss (%)>
	if( $line =~ /^h (\S+)/ ) {
	    push @verts, $1
	} elsif ( $line =~ /^l (\S+)\s+(\S+)\s+(\d+)\s+(\d\.\d+)\s+(\d\.\d+)/) {
	    my $newEdge = new edge;
	    $newEdge->src($1);
	    $newEdge->dst($2);
	    $newEdge->bw($3);
	    $newEdge->delay($4);
	    $newEdge->loss($5);
	    push @edges, $newEdge;
	}
    }
}

sub debug {
    print "@_\n"; 
}

sub debug_top {
    foreach my $vert (@verts) {
	&debug( $vert);
    }
    foreach my $edge (@edges) {
	&debug( $edge->src . " " . $edge->dst . " " . $edge->bw
		. " " . $edge->delay . " " . $edge->loss
		);
    }
}

sub error {
    print "ERROR: @_\n";
}


# highest alphabetical node gets the sync server.
# if that's "me", start it up.
sub start_sync {
    if($synch eq $hostname) {
	# TODO: use the right synch server specified by tb-set-sync-server
	my @psoutput = `ps -le | grep emulab-sy`;
	if(@psoutput==0) {
	    &debug ("starting synch");
	    system "/usr/local/etc/emulab/emulab-syncd";

	    # TODO: barrier gets a timeout after which it may kill the 
	    # synch server, releasing all nodes. this happens when
	    # a node has failed to come up!

	} else {
	    &debug ("existing synch found");
	}
    }
}

# synch all nodes
sub barrier {
    my $cmd;
    my $count = @verts - 1;
    if($synch eq $hostname) {
	$cmd = "emulab-sync -s $synch -n foo -i $count";
	&debug($cmd);
	system $cmd;
    } else {
	$cmd = "emulab-sync -s $synch -n foo";
	&debug($cmd);
	system $cmd;
    }
}

# routes test
# 
# This test checks whether each node can reach every other node.
# It uses ping.
#
# This will only work if $ns rtproto has been set!
sub routing_test {
    
    # fork processes to run the pings in parallel.
    foreach my $dst (@verts) {
	if(!($dst eq $hostname) ) {
	    my $pid = fork();
	    if(!$pid) {
		if(!&ping_node($dst,0)) {
		    &error("Could not ping $dst");
		}
		exit;
	    }
	}
    }
    wait;
}


# pings a node. 
# @param[0] := host to ping
# @param[1] := ttl, 0 for default
# @return: # of replies
#
# Since this is over the test net, it must accomodate loss.
# Startup tests support up to 10% loss. Loss is a binomial RV. 
# Probability of receiving no packets back after sending 10 packets: 0
# (technically: let X be a binomial RV for # of packets lost.
# Then p(X=10) = B(10;10,0.1) = 0
sub ping_node {
    my ($host,$ttl) = @_;

    # set deadline to prevent long waits
    my $cmd;
    if($ttl) {
	$cmd = "sudo ping -c 10 -q $host -i 0.1 -w 1 -t $ttl";
    } else {
	$cmd = "sudo ping -c 10 -q $host -i 0.1 -w 1";
    }
    &debug($cmd);

    # speed it up a bit by sending at 1/10 second intervals.
    # but not faster to avoid lost packets due to parallelism
    # TODO: what is the measurable upper limit?

    my @results = `$cmd`;
    foreach my $result (@results) {
	# find the results line we care about
	if($result =~ /(\d+) received/) {
	    return $1;
	}
    }
    return 0;
}

# direct link test
# 
# Attempt to reach linked nodes with TTL=1
sub link_test {
    # fork processes to run the pings in parallel.
    foreach my $edge (@edges) {
	if($edge->src eq $hostname ) {
	    my $pid = fork();
	    if(!$pid) {
		if(!&ping_node($edge->dst,1)) {
		    &error("could not reach " . $edge->dst . " directly");
		}
		exit;
	    }
	}
    }
    wait;
}
 
# link test
#
# Different from the routing test because it ensures that links are only 1 hop
# (sets ttl=1)
sub latloss_test {
    my %offset; # hash containing offset for destination hosts (from this host's perspective)
    my $this_offset; # a scalar contaiing this host's offset.

    # first need to start up crude so that it's running
    # then let the flows come on in for a while. after all flows
    # have been sent, shut down crude and examine the results.

    # TODO: only start if this host is a link destination
    my $crude_pid = fork();
    if(!$crude_pid) {
	# check that crude not already running
	my @result = `ps -le | grep crude`;
	if(@result) {
	    &debug(@result);
	    die("crude already running! cannot continue");
	}
	
	unlink PATH_CRUDE_LOG || die ("could not delete existing logfile");

	exec "$crude_cmd -l " . PATH_CRUDE_LOG;
    }

    # stores the nodes to send rude packets to in the config file
    &generate_rude;

    &barrier;

    # OK, this is the part that will need to be parallelized.
    # for now, since running simultaneously is definitely screwing up
    # the packet counts (even at 10packets/sec), give each node a turn to run.
    foreach my $node (@verts) {
	if($node eq $hostname) {
	    # get this host's offset
	    $this_offset = &get_offset($hostname);
	    &debug("my offset: " . $this_offset);

	    # get offset on all destination nodes for this host before running rude.
	    my $i;
	    for ($i = 0; $i < @edges; $i++) {
		my $edge = $edges[$i];
		if($edge->src eq $hostname) {
		    # TODO: only do following if hash entry is undefined
		    $offset{$edge->dst} = &get_offset($edge->dst);
		    &debug("got offset for " . $edge->dst . " = " . $offset{$edge->dst});
		}
	    }

	    for ($i = 0; $i < @edges; $i++) {
		my $edge = $edges[$i];
    
		if($edge->src eq $hostname) {
		    
		    my $rude_pid = fork();
		    if(!$rude_pid) {
			&generate_rude($i);
			my $cmd = "$rude_cmd -s " . PATH_RUDE_CFG;
			&debug ($cmd);
			exec $cmd;
		    }
		    waitpid($rude_pid,0);
		    &debug("rude completed on $node");
		}
	    }
	}
	&barrier;
    }
    
    # all nodes now kill crude
    kill 9, $crude_pid;
    &debug("crude completed");

    # analyze crude results
    &analyze_crude( $this_offset, %offset );
}

sub generate_rude {
    my ($i) = @_;

    # TODO: parameterize test length and packet rate.

    # TODO: skip this step if no incoming links.

    open CFG, ">" . PATH_RUDE_CFG || die( "could not open " . PATH_RUDE_CFG . ":$!");
    printf CFG "START NOW\n";

    # generate a rude job using position in the sorted array for flow numbers.
    my $edge = $edges[$i];
#    print "debug, i=$i, dst=" . $edge->dst;

    my $line = sprintf "0000 %04d ON 3001 " . ($edge->dst) . ":10001 CONSTANT 400 20\n", $i;
    printf CFG $line;
    $line = sprintf "1000 %04d OFF\n",$i;
    printf CFG $line;

    
    close CFG;
    system "cat " . PATH_RUDE_CFG;
}

# analyze the crude log containing multiple flows of which this host is the recipient.
sub analyze_crude {
    my ($host_offset, %offset) = @_;

    my %diff; # hash containing lists of latency diffs, indexed by flowid.
    my %cnt;  # hash containing scalar indicating number of packets received.


    # decode the log
    my $cmd = "$crude_cmd -d " . PATH_CRUDE_LOG . " > " . PATH_CRUDE_LOG_DECODED;
    system $cmd;

    # populate %diff and %loss
    open LOG,"" . PATH_CRUDE_LOG_DECODED || die("could not open " . PATH_CRUDE_LOG . ":$!");
    while( <LOG> ) {
	if(/ID=(\d+).*Tx=(\d+\.\d+).*Rx=(\d+\.\d+)/) {
	    # times in seconds.microseconds
	    my $flowid = $1;
	    my $Ts = $2;
	    my $Tr = $3;
	    
	    # get edge info
	    my $edge = $edges[$flowid];
	    my $Os = $offset{$edge->src} / 1000; # convert units from ms to microseconds
#	    &debug("Tr, Os: $Tr $Os");
	    my $Or = $host_offset / 1000;

	    # TODO: correct for clock offset
	    my $owd = $Tr + $Or - ( $Ts + $Os);
	    
	    # Save the delay..
	    push @{$diff{$flowid}},$owd;
	    
	    # Save the packet count
	    $cnt{$flowid}++;
	}
    }
    close LOG;

    # statistical info, for now just print
    foreach my $flowid ( keys %diff) {
	my $edge = $edges[$flowid];
	
	&debug("flow: $flowid from " . $edge->src . " to " . $edge->dst);
	&debug("\treceive count: $cnt{$flowid}");
	my $stat = Statistics::Descriptive::Full->new();
	$stat->add_data(@{$diff{$flowid}});
	&debug("\tmean latency (ms): " . ($stat->mean() * 1000 ) );
    }

}

# get ntpq offset of the specified host
sub get_offset {
    my ($host) = @_;
    my @result = `/usr/sbin/ntpq -c rl $host | grep ^offset`;
    if( $result[0] =~ /^offset=(-*\d+\.\d+)/) {
	return $1;
    }
    return 0;
}

sub bw_test {

    # first need to start up pathrate_snd so that it's running
    # then let the receive commands come on in for a while. after all flows
    # have been sent, shut down pathrate_snd and examine the results.

    # TODO: only start if this host is a link destination
    my $snd_pid = fork();
    if(!$snd_pid) {
	# check that pathrate not already running
	my @result = `ps -le | grep pathrate`;
	if(@result) {
	    &debug(@result);
	    die("pathrate already running! cannot continue");
	}
	exec "$pathrate_snd_cmd -i";
    }

    unlink PATH_RATE_LOG || die ("could not delete existing logfile");

    # wait for a second to give pathrate time to startup. (race?)
    sleep(1);

    # wait for all nodes to get pathrate_snd started up
    &barrier;

    # OK, this is the part that will need to be parallelized.
    foreach my $node (@verts) {
	# wait for this host's turn
	if($node eq $hostname) {

	    my $i;
	    for ($i = 0; $i < @edges; $i++) {
		my $edge = $edges[$i];
		if($edge->src eq $hostname) {

		    my $rcv_pid = fork();
		    if(!$rcv_pid) {
			my $cmd = "$pathrate_rcv_cmd -s " . $edge->dst . " -Q -q -N "
			    . PATH_RATE_LOG;
			&debug ($cmd);
			exec $cmd;
		    }
		    waitpid($rcv_pid,0);
		}
	    }
	    

	}
	&barrier;
    }
    
    # all nodes now kill pathrate_snd
    kill 9, $snd_pid;
    &debug("pathrate_snd completed");

    # analyze results
    &analyze_pathrate;

}

sub analyze_pathrate {
    system "cat " . PATH_RATE_LOG;
}
