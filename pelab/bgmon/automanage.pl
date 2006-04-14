#!/usr/bin/perl -w

use lib '/usr/testbed/lib';
use libxmlrpc;
use event;
use English;
use Getopt::Std;

use strict;


my $constrFilename;
my $bwdutycycle;
my $numsites;
my %test_per = (  # defaults
	       "latency" => 300,
	       "bw"      => 0,
	       );
my %intersitenodes = (); #final list for fully-connected test
my @constrnodes;    #test constrained to these nodes
my %sitenodes;      #hash listing all sites => nodes
my $CPUUSAGETHRESHOLD = 10;  #should help prevent flip-flopping between
                             #"best" nodes at a site
my $SITEDIFFTHRESHOLD = 3;   #number of site differences between period
                             #calculations that trigger an update
my $IPERFDURATION = 10;      #duration in seconds of iperf test
my %allnodes;

# RPC STUFF ##############################################
my $TB         = "/usr/testbed";
my $ELABINELAB = 0;
my $RPCSERVER  = "boss.emulab.net";  #?
my $RPCPORT    = "3069";
#my $RPCCERT    = "/etc/outer_emulab.pem"; #?
my $RPCCERT = "~/.ssl/emulab.pem";
my $MODULE = "node";
my $METHOD = "getlist";
# END RPC STUFF ##########################################



sub usage 
{
	warn "Usage: $0 [-BLP] [-f constraint file] [-l latency test period]".
	     " <bandwidth duty cycle>".
#	     " <number of sites or \"all\">\n".
	     "where: -B = Do not measure bandwidth\n".
	     "       -L = Do not measure latency\n".
	     "       -P = Do not measure latency to nodes not responding to pings\n";
	return 1;
}

if( !defined $ARGV[0] ){
    exit &usage;
}
$bwdutycycle = $ARGV[0];
#if( !defined $ARGV[1] ){
#    exit &usage;
#}
#$numsites = $ARGV[1];
my %opt;
getopt("B:L:P:f:l:s:p",\%opt,);
if( $opt{f}) { $constrFilename = $opt{f}; }
if( $opt{l}) { $test_per{latency} = $opt{l}; }
#TODO: other options

my ($server,$port);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }


#############################################################################
#
# Initialization
#
libxmlrpc::Config({"server"  => $RPCSERVER,
		    "verbose" => 0,
#		    "cert"    => $RPCCERT,
		    "portnum" => $RPCPORT});

my $lastupdated_numnodes = 0;
# TODO: Stop all nodes in constraint set

###########################################################
#
# Main Loop
#
while(1)
{
    #update node list
    getnodeinfo();
    choosenodes();
    modifytests();

    select(undef, undef, undef, 5.0);
}


###########################################################

sub getnodeinfo
{
    #retrieve list of nodes
    my $rval = libxmlrpc::CallMethod($MODULE, $METHOD,
				     {"class" => "pcplabphys"});
    print "here";
    %allnodes = %$rval;

    #populate sitenodes
    foreach my $node (keys %allnodes){
	my $siteid = $allnodes{$node}{site};
#       print $siteid;
#       print $node;
	push @{$sitenodes{$siteid}}, $node;
#       print @{$sitenodes{$siteid}}."\n";
    }
}



########################################################
#
# choose a node from each possible site
#
sub choosenodes
{
    foreach my $site (keys %sitenodes){
	my $bestnode = choosebestnode($site);
	if( "NONE" eq $bestnode ){	 
	    # ** This section handles when a site has no nodes available
	    #no available node at this site, so remove site from hash
	    #TODO: send "stop" signals to all other nodes having this
	    #  node as the destination
	    delete $intersitenodes{$site};
	}
	else{
	    if( !defined $intersitenodes{$site} || 
		$intersitenodes{$site} ne $bestnode )
	    {
		# ** This section handles when a "bestnode" at a site changes

		# TODO: Stop sigs to other nodes using old "bestnode" value
		#set new node to represent this site
		$intersitenodes{$site} = $bestnode;
		# TODO: start other nodes using this new "bestnode"
		#       (This uses the EDIT bgmon signal - see bgmon.pl)
	    }
        }
    }
}


#
# Re-adjust the test periods of connections based on number of nodes
#
sub modifytests
{
    my $numsites = scalar(keys %intersitenodes);

    my $bwper = ($numsites - 1) * $IPERFDURATION * 1/$bwdutycycle;
    #TODO: ?? dynamically change latency period, too?

    #update connections to use  newly calculated periods
    if( abs($lastupdated_numnodes - $numsites) > $SITEDIFFTHRESHOLD )
    {
	if( !$opt{B} ){       
	    $test_per{bw} = $bwper;
	    print "new BW per = $bwper\n";
	}

	$lastupdated_numnodes = $numsites;
	updateTests();   #handles changing number of sites
    }  

}



sub printchosennodes
{
    foreach my $node (values %intersitenodes){
	print "site: ". $allnodes{$node}{site} . " = $node\n";
	#TODO:: why does this give an error?
	print "node = $node\n";
    }
}


sub choosebestnode($)
{
    my ($site) = @_;
    my $bestnode = "NONE";  #default to an error value
=pod
    print "$site ";
    foreach my $node ( @{$sitenodes{$site}} ){
	print $node. " ";
    }
    print "\n";
=cut
    foreach my $node ( @{$sitenodes{$site}} ){
	if( $allnodes{$node}{free} == 1 && isnodeinconstrset($node) ) {
	    #first time thru loop...
	    if( $bestnode eq "NONE" ){
		#set this to be best node
		$bestnode = $node;
	    }else{
		if( $allnodes{$node}{cpu} < $allnodes{$bestnode}{cpu}
		    + $CPUUSAGETHRESHOLD)
		{
		    $bestnode = $node;
		}
	    }
	}
    }
    print "bestnode for $site = $bestnode\n";
    return $bestnode;
}


sub isnodeinconstrset($)
{
    my ($node) = @_;
    
    #if constraint set is empty, return true
    if( ! defined(@constrnodes) ){
	return 1;
    }else{
	#TODO: check if node exists in contraint set
	return -1;
    }
}



#
# update all nodes with new test periods and destination nodes
#
sub updateTests
{
    my $srcnode;
    my $bw_destnodes;
    my $destnode;
    #init bandwidth
    foreach my $srcsite (keys %intersitenodes){
	$srcnode = $intersitenodes{$srcsite};
	foreach my $destsite (%intersitenodes){
	    if( defined $intersitenodes{$destsite}) {
		$destnode = $intersitenodes{$destsite};
	    }
	    #add destination nodes to this source node, but not "self"
	    if( $destnode ne $srcnode ){
		$bw_destnodes .= " ".$destnode;
	    }
	}
#	initnode($srcnode, $bw_destnodes, $test_per{bw}, "bw");

	#TODO! Distribute initialization times evenly
    }

    #TODO: LATENCY
}


#
# Stops all nodes in constraint set
#
sub stopall()
{
    foreach my $node (values %sitenodes){
	if( isnodeinconstrset($node) ){
	    %$tuple = ( objtype   => "BGMON",
			objname   => $node,
			eventtype => "STOPALL",
			expt      => "__none" );
	    my $notification = event_notification_alloc($handle,$tuple);
	    if (!$notification) { die "Could not allocate notification\n"; }

	    #send notification
	    if (!event_notify($handle, $notification)) {
		die("could not send test event notification");
	    }
	}
    }
}

#
# stops the tests from given source and destination nodes
#
sub stoppairtest($$)
{
    my ($srcnode, $destnode) = @_;
    print "stopping pair tests from $srcnode to $destnode\n";
    my $testper = 0;
    my @testtypes = ("latency","bw");
    %$tuple = ( objtype   => "BGMON",
		objname   => $srcnode,
		eventtype => "EDIT",
		expt      => "__none" );

    foreach my $testtype(@testtypes){

	my $notification = event_notification_alloc($handle,$tuple);
	if (!$notification) { die "Could not allocate notification\n"; }

	#add destination nodes attribute
	if( 0 == event_notification_put_string( $handle,
						$notification,
						"linkdest",
						$destnode ) )
	{ warn "Could not add attribute to notification\n"; }

	#add tests and their default values
	if( 0 == event_notification_put_string( $handle,
						$notification,
						"testper",
						$testper ) )
	{ warn "Could not add attribute to notification\n"; }

	#add test type
	if( 0 == event_notification_put_string( $handle,
						$notification,
						"testtype",
						$testtype ) )
	{ warn "Could not add attribute to notification\n"; }

	#send notification
	if (!event_notify($handle, $notification)) {
	    die("could not send test event notification");
	}
    }
}

#
# destnodes is space-separated string
#
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
    print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";
    %$tuple = ( objtype   => "BGMON",
		objname   => $node,
		eventtype => "INIT",
		expt      => "__none" );
    my $notification = event_notification_alloc($handle,$tuple);
    if (!$notification) { die "Could not allocate notification\n"; }

    #add destination nodes attribute
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "destnodes",
					    $destnodes ) )
    { warn "Could not add attribute to notification\n"; }

    #add tests and their default values
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "testper",
					    $testper ) )
    { warn "Could not add attribute to notification\n"; }

    #add test type
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "testtype",
					    $testtype ) )
    { warn "Could not add attribute to notification\n"; }

    #send notification
    if (!event_notify($handle, $notification)) {
	die("could not send test event notification");
    }

    print "sent initnode to $node\n";
    print "destnodes = $destnodes\n";
    print "testper = $testper\n";
    print "testtype = $testtype\n";
}
