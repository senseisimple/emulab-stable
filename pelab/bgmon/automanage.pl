#!/usr/bin/perl -w

use strict;
use lib '/usr/testbed/lib';
use libxmlrpc;
use libwanetmon;
use English;
use Getopt::Std;
use IO::Socket::INET;
use IO::Select;

$| = 1;

my ($constrFilename, $expid, $bwdutycycle, $port);
my $numsites;
my %test_per = (  # defaults
	       "latency" => 300,
	       "bw"      => 0,
	       );
$thisManagerID = "automanagerclient";
my %intersitenodes = (); #final list for fully-connected test
my @constrnodes;    #test constrained to these nodes
my %sitenodes;      #hash listing all sites => nodes
my $CPUUSAGETHRESHOLD = 10;  #should help prevent flip-flopping between
                             #"best" nodes at a site
                             #TODO: document normal range of values for CPU
my $SITEDIFFTHRESHOLD = 5;   #number of site differences between period
                             #calculations that trigger an update
my $IPERFDURATION = 5;      #duration in seconds of iperf test
my %allnodes;
my %deadsites;

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
	     " [-e pid/eid]".
	     " <bandwidth duty cycle 0-1>".
#	     " <number of sites or \"all\">\n".
	     "where: -B = Do not measure bandwidth\n".
	     "       -L = Do not measure latency\n".
	     "       -P = Do not measure latency to nodes not responding to pings\n";
	return 1;
}


my %opt = ();
getopts("B:L:P:f:l:e:s:p",\%opt);
#TODO: other options
if( $opt{f}) { $constrFilename = $opt{f}; }
if( $opt{l}) { $test_per{latency} = $opt{l}; }
if ($opt{e}) { $expid = $opt{e}; } else { $expid = "none"; }
if ($opt{p}) { $port = $opt{p}; } else{ $port = 5052; }

setcmdport($port);  #set the library's port
setexpid($expid);

if( !defined $ARGV[0] ){
    exit &usage;
}
$bwdutycycle = $ARGV[0];

my $lastupdated_numnodes = 0;

my $socket;
my $sel = IO::Select->new();

#FORWARD DECL'S
sub stopnode($);
sub outputErrors();

print "exp = $expid\n";
#############################################################################
#
# Initialization
#
libxmlrpc::Config({"server"  => $RPCSERVER,
		    "verbose" => 0,
#		    "cert"    => $RPCCERT,
		    "portnum" => $RPCPORT});

# Stop all nodes in constraint set
if( defined $constrFilename )
{
    open FILE, "< $constrFilename" or die "cannot open file $constrFilename";
    while( <FILE> ){
	chomp;
	if( $_ =~ m/plab/ ){ 
	    push @constrnodes, $_;
	    print "$_\n";
	}
    }
    close FILE;
    foreach my $node (@constrnodes){
#    print "stopping $node\n";
	stopnode($node);
    }
}
#stop all nodes from XML-RPC query
else{
    getnodeinfo();
    foreach my $site (keys %sitenodes){
	foreach my $node (@{$sitenodes{$site}}){
	    stopnode($node);
	}
    }
}


###########################################################
#
# Main Loop
#
my $f_firsttime = 1;
while(1)
{
#    %deadnodes = ();

    #update node list

    getnodeinfo();

    choosenodes();

#    if( $f_firsttime ){
#	print "FIRST TIME UPDATE\n";
#	updateTests();
#    }

    modifytests($f_firsttime);

#    printchosennodes();

    outputErrors();

    sleep( 60*5 );
    $f_firsttime = 0;
}


###########################################################

sub getnodeinfo
{
    #retrieve list of nodes
    %sitenodes = ();
    my $rval = libxmlrpc::CallMethod($MODULE, $METHOD,
				     {"class" => "pcplabphys"});
    if( defined $rval ){
	%allnodes = %$rval;
    }else{ return; }

    #remove old node-listing file
    my $nodesfilename = "allnodelisting_automanage.txt";
    unlink $nodesfilename or warn "can't delete node-listing file";
    open FILE, "> $nodesfilename"
	or warn "can't open file $nodesfilename\n";

    #populate sitenodes
    foreach my $node (keys %allnodes){
	my $siteid = $allnodes{$node}{site};
	push @{$sitenodes{$siteid}}, $node;
#       print @{$sitenodes{$siteid}}."\n";
	print FILE "$node\n";
    }
    close FILE;
}

sub printNodeInfo($)
{
    my ($node) = @_;
    foreach my $key (keys %{$allnodes{$node}} ){
	print "\t$key = $allnodes{$node}{$key}\n";
    }
}

########################################################
#
# choose a node from each possible site
sub choosenodes
{
    foreach my $site (keys %sitenodes){
#	print "site $site\n";
	my $bestnode = choosebestnode($site);
#	if( !defined $bestnode ){ print "BESTNODE NOT DEF!!!\n"; }
#	if( $bestnode ne "NONE"){ print "bestnode for $site = $bestnode\n"; }
	if( $bestnode ne "NONE" &&
	    !defined $intersitenodes{$site} )
	{
	    print time()." SECTION 1: adding $bestnode at $site\n";
	    # ** This section handles when a site is seen for the 1st time

	    #set new node to represent this site
	    $intersitenodes{$site} = $bestnode;
	    initNewSiteNode($site,$bestnode)
		if( $f_firsttime == 0 );
	}
	elsif( ("NONE" eq $bestnode) && defined $intersitenodes{$site} )
	{
	    print time()." SECTION 2: removing tests to $site / ".
		"$intersitenodes{$site} \n";
	    # ** This section handles when a site has no nodes available

	    #no available node at this site, so remove site from hash
	    foreach my $srcsite (keys %intersitenodes){
		stoppairtest( $intersitenodes{$srcsite},
			      $intersitenodes{$site} );
	    }
	    delete $intersitenodes{$site};
	}
	elsif( defined $intersitenodes{$site} &&
	       ( $intersitenodes{$site} ne $bestnode ||
		 getstatus($bestnode) eq "anyscheduled_no"
	       )
	     )
	{
	    print time()." SECTION 3: node change/restart at $site from ".
		"$intersitenodes{$site} to $bestnode\n";
	    # ** This section handles when a "bestnode" at a site changes

	    #TODO: This logic should be fixed so the new tests are
	    #  started before old ones are stopped. This may help
	    #  prevent "holes" in the data collection to a site.
	    #  Stop sigs to other nodes using old "bestnode" value
	    if( defined $intersitenodes{$site} ){
		foreach my $srcsite (keys %intersitenodes){
		    stoppairtest( $intersitenodes{$srcsite},
				  $intersitenodes{$site} );
		}
	    }
	    
	    #set new node to represent this site
	    $intersitenodes{$site} = $bestnode;
	    initNewSiteNode($site);	    
	}
    }
}

sub initNewSiteNode($)
{
    my ($site) = @_;
#    $intersitenodes{$site} = $bestnode;

    # stop any tests remaining on this node.
    stopnode($intersitenodes{$site});

    # start tests to and from this new site
    foreach my $srcsite (keys %intersitenodes){
	edittest( $intersitenodes{$srcsite},
		  $intersitenodes{$site},
		  $test_per{bw},
		  "bw",
		  0,
		  $thisManagerID);
	edittest( $intersitenodes{$site},
		  $intersitenodes{$srcsite},
		  $test_per{bw},
		  "bw"
		  0,
		  $thisManagerID );
	my $r = rand;
	if( $r <= .5 ){
	    edittest( $intersitenodes{$srcsite},
		      $intersitenodes{$site},
		      $test_per{latency},
		      "latency" );
	}else{
	    edittest( $intersitenodes{$site},
		      $intersitenodes{$srcsite},
		      $test_per{latency},
		      "latency" );
	}
    }
}


#
# Re-adjust the test periods of connections based on number of nodes
#   Pass a non-zero parameter to force an initialization of all paths
sub modifytests
{
    my ($f_forceInit) = @_;

    my $numsites = scalar(keys %intersitenodes);
    my $bwper = ($numsites - 1) * $IPERFDURATION * 1/$bwdutycycle;
    #TODO: ?? dynamically change latency period, too?

    #update connections to use  newly calculated periods
    if( abs($lastupdated_numnodes - $numsites) > $SITEDIFFTHRESHOLD )
    {
	if( !$opt{B} ){       
	    $test_per{bw} = $bwper;
	    print "new BW per = $bwper\n";
	    $lastupdated_numnodes = $numsites;
	    updateTests(1,0);   #handles changing number of sites.
	                        # only update bandwidth
	}
    }

    if( defined $f_forceInit && $f_forceInit != 0 ){
	updateTests(1,1);
    }
}



sub printchosennodes
{
    foreach my $node (values %intersitenodes){
	print "site: ". $allnodes{$node}{site} . " = $node\n";
#	#TODO:: why does this give an error?
#	print "node = $node\n";
    }
}


sub choosebestnode($)
{
    my ($site) = @_;

    my $bestnode = "NONE";  #default to an error value
   
    my $flag_siteIncluded = 0;  #set if any node at site is in constraint set

=pod
    print "site: $site ";
    foreach my $node ( @{$sitenodes{$site}} ){
	print "$node ";
    }
    print "\n";
=cut
    foreach my $node ( @{$sitenodes{$site}} ){
=pod
	if(isnodeinconstrset($node)){
	    print "node $node is in constr set ";
	    if( $allnodes{$node}{free} == 1 ){
		print "and free";
	    }
	    print "\n";
	}
=cut
        if( isnodeinconstrset($node) ){
	    $flag_siteIncluded = 1;
#	    print "SETTING SITEINCLUDED=1 for $node at $site\n";
	}

	if( $allnodes{$node}{free} == 1 && isnodeinconstrset($node) ){
#	    print "choosebestnode: considering $node\n";
#	    print "choosing best node for site $site\n";
	    #first time thru loop...
	    if( $bestnode eq "NONE" ){
		#set this to be best node if it responds to a command
		
#		if( edittest($node,"NOADDR",0,"bw") == 1 ){
#		    $bestnode = $node;
#		}
		if( getstatus($node) ne "error" ){
		    $bestnode = $node;
		}
	    }else{
		if( ($allnodes{$node}{cpu} < $allnodes{$bestnode}{cpu}
		    - $CPUUSAGETHRESHOLD) &&
		    (edittest($node,"NOADDR",0,"bw") == 1) )
		{
		    print '$allnodes{$node}{cpu}'.
			"  $allnodes{$node}{cpu}\n";
		    print '$allnodes{best$node}{cpu}'.
			"  $allnodes{$bestnode}{cpu}\n";
		    $bestnode = $node;
		}
	    }
	}
    }



    if( $bestnode eq "NONE" && $flag_siteIncluded == 1){
#	print "^^^^^adding to deadsites: $site\n";
	$deadsites{$site} = 1;
    }else{
	delete $deadsites{$site};
    }
    return $bestnode;
}


sub isnodeinconstrset($)
{
    my ($node) = @_;
    
    #if constraint set is empty, return true
    if( @constrnodes == 0 ){
	return 1;
    }else{
	#check if node exists in contraint set
	foreach my $cnode (@constrnodes){
	    if( $node eq $cnode ){
		return 1;
	    }
	}
	return 0;
    }
}



#
# update all nodes with new test periods and destination nodes
#   Two parameters: first is flag to update bw, second to update latency
sub updateTests($$)
{
    my ($f_updatebw, $f_updatelat) = @_;
    print "UPDATING TESTS\n";

    my $srcnode;
    my $bw_destnodes;
    my $destnode;
    #init bandwidth
    if( $f_updatebw ){
	foreach my $srcsite (keys %intersitenodes){
	    $srcnode = $intersitenodes{$srcsite};
	    $bw_destnodes = "";
	    foreach my $destsite (keys %intersitenodes){
#	    print "looking at site $destsite\n";
		if( defined $intersitenodes{$destsite}) {
		    $destnode = $intersitenodes{$destsite};
		}
		#add destination nodes to this source node, but not "self"
		if( $destnode ne $srcnode ){
		    $bw_destnodes .= " ".$destnode;
		}
	    }
	    initnode($srcnode, $bw_destnodes, $test_per{bw}, "bw");
	}
    }

    #init latency: fully connected, but only one direction each path
    if( $f_updatelat ){
	my %initstrs;  #build init strings for each site node
	my @sitekeys = keys %intersitenodes;
	for( my $i = 0; $i < @sitekeys-1; $i++ ){
	    for( my $j = $i+1; $j < @sitekeys; $j++ ){
		my $r = rand;
		if( $r <= .5 ){
		    $initstrs{$intersitenodes{$sitekeys[$i]}} .= 
			"$intersitenodes{$sitekeys[$j]} ";
		}else{
		    $initstrs{$intersitenodes{$sitekeys[$j]}} .= 
			"$intersitenodes{$sitekeys[$i]} ";
		}
	    }
	}
	# now send the inits to all nodes
	foreach my $srcsite (keys %intersitenodes){
	    $srcnode = $intersitenodes{$srcsite};
	    initnode($srcnode, $initstrs{$srcnode}, 
		     $test_per{latency}, "latency");
	}
    }
}

#
# Stop all tests from a node
#

sub stopnode($)
{
    my ($node) = @_;

    if( isnodeinconstrset($node) ){
=pod
	my %cmd = ( expid    => $expid,
		    cmdtype  => "STOPALL" );
	print "stopnode $node called\n";
	sendcmd($node,\%cmd);
=cut
        libwanetmon::stopnode($node);
        print "stopnode $node called\n";
    }
}





#
# stops the tests from given source and destination nodes
#
sub stoppairtest($$)
{
    my ($srcnode, $destnode) = @_;
    my $testper = 0;
    my @testtypes = ("latency","bw");

    foreach my $testtype (@testtypes){
	edittest($srcnode, $destnode, 0, $testtype);
    }
#    print "stopping pair tests from $srcnode to $destnode\n";
}

#
# destnodes is space-separated string
#
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;

    if( defined $destnodes ){
	print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";

	my %cmd = ( expid     => $expid,
		    cmdtype   => "INIT",
		    destnodes => $destnodes,
		    testtype  => $testtype,
		    testper   => $testper
		    );

	sendcmd($node,\%cmd);

	print "sent initnode to $node\n";
	print "destnodes = $destnodes\n";
	print "testper = $testper\n";
	print "testtype = $testtype\n";
    }
}


sub outputErrors()
{
    if( keys %deadnodes > 0 ){
	print "Nodes not responding:\n";
	foreach my $node (keys %deadnodes){
	    print "$node  ";
	}
	print "\n";
    }
    if( keys %deadsites > 0 ){
	print "Sites with no nodes available:\n";
	foreach my $site (keys %deadsites){
	    print "$site  ";
	}
	print "\n";
    }
    print "Sites with nodes available:\n";
    foreach my $site (keys %intersitenodes){
	print "$site ";
    }
    print "\n";
}


