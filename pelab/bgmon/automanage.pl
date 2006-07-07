#!/usr/bin/perl -w

use strict;
use lib '/usr/testbed/lib';
use libxmlrpc;
use English;
use Getopt::Std;
use IO::Socket::INET;
use IO::Select;

my ($constrFilename, $expid, $bwdutycycle, $port);
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
my $SITEDIFFTHRESHOLD = 1;   #number of site differences between period
                             #calculations that trigger an update
my $IPERFDURATION = 10;      #duration in seconds of iperf test
my %allnodes;
my %deadnodes;

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
if ($opt{p}) { $port = $opt{p}; } else{ $port = 5060; }


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
open FILE, "< $constrFilename"
    or die "cannot open file $constrFilename";
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


###########################################################
#
# Main Loop
#
#my $f_firsttime = 1;
while(1)
{
    %deadnodes = ();

    #update node list
#    print "getnodeinfo\n";
    getnodeinfo();
#    sleep(10);

#    print "choosenodes\n";
    choosenodes();
#    sleep(10);

#    print "modifytests\n";
    modifytests();
#    sleep(10);

#    printchosennodes();

    outputErrors();
#   sleep( 10 );

    sleep( 60 );
#    $f_firsttime = 0;
}


###########################################################

sub getnodeinfo
{
    #retrieve list of nodes
    my $rval = libxmlrpc::CallMethod($MODULE, $METHOD,
				     {"class" => "pcplabphys"});
    if( defined $rval ){
	%allnodes = %$rval;
    }else{ return; }

    #populate sitenodes
    foreach my $node (keys %allnodes){
	my $siteid = $allnodes{$node}{site};
 	@{$sitenodes{$siteid}} = ();
	push @{$sitenodes{$siteid}}, $node;
#       print @{$sitenodes{$siteid}}."\n";
    }
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
#
sub choosenodes
{
    foreach my $site (keys %sitenodes){
#	print "site $site\n";
#	my $bestnode = "NONE";
	my $bestnode = choosebestnode($site);
	if( !defined $bestnode ){ print "BESTNODE NOT DEF!!!\n"; }
	if( "NONE" ne $bestnode &&
	    !defined $intersitenodes{$site} )
	{
	    print "SECTION 1: adding $bestnode at $site\n";
	    # ** This section handles when a site is seen for the 1st time

	    #set new node to represent this site		
	    $intersitenodes{$site} = $bestnode;
	}
	elsif( ("NONE" eq $bestnode) && defined $intersitenodes{$site} )
	{
	    print "SECTION 2: removing tests to $site / ".
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
	       $intersitenodes{$site} ne $bestnode
	       #&& isnodeinconstrset($bestnode) 
	       )
	{
	    print "SECTION 3: node change at $site from ".
		"$intersitenodes{$site} to $bestnode\n";
	    # ** This section handles when a "bestnode" at a site changes
	    
	    #  Stop sigs to other nodes using old "bestnode" value
	    if( defined $intersitenodes{$site} ){
		foreach my $srcsite (keys %intersitenodes){
		    stoppairtest( $intersitenodes{$srcsite},
				  $intersitenodes{$site} );
		}
	    }
	    
	    #set new node to represent this site		
	    $intersitenodes{$site} = $bestnode;
	    
	    foreach my $srcsite (keys %intersitenodes){
		edittest( $intersitenodes{$srcsite},
			  $intersitenodes{$site},
			  $test_per{bw},
			  "bw" );
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
#	#TODO:: why does this give an error?
#	print "node = $node\n";
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
=pod
	if(isnodeinconstrset($node)){
	    print "node $node is in constr set ";
	    if( $allnodes{$node}{free} == 1 ){
		print "and free";
	    }
	    print "\n";
	}
=cut     
        #this command acts like a bgmon "ping" - used to 
        #determine if bgmon running correctly
        my %cmd = ( expid    => $expid,
		    cmdtype  => "EDIT",
		    dstnode  => "NOADDR",
		    testtype => "bw",
		    testper  => 0 );
	if( $allnodes{$node}{free} == 1 && 
	    isnodeinconstrset($node) )
	{
#	    print "choosing best node for site $site\n";
	    #first time thru loop...
	    if( $bestnode eq "NONE" ){
		#set this to be best node
		$bestnode = $node;
	    }else{
		if( ($allnodes{$node}{cpu} < $allnodes{$bestnode}{cpu}
		    - $CPUUSAGETHRESHOLD) &&
		    (edittest($node,"NOADDR",0,"bw") == 1) )
		{
		    print "setting new bestnode\n";
		    print '$allnodes{$node}{cpu}'."  $allnodes{$node}{cpu}\n";
		    print '$allnodes{$bestnode}{cpu}'.
			"  $allnodes{$bestnode}{cpu}\n";
		    $bestnode = $node;
		}
	    }
	}
    }
#    print "bestnode for $site = $bestnode\n";
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
#
sub updateTests
{
    my $srcnode;
    my $bw_destnodes;
    my $destnode;
    #init bandwidth
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

    #init latency: fully connected, but only one direction each path
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
	initnode($srcnode, $initstrs{$srcnode}, $test_per{latency}, "latency");
    }
    
}

#
# Stop all tests from a node
#
sub stopnode($)
{
    my ($node) = @_;

    if( isnodeinconstrset($node) ){
	my %cmd = ( expid    => $expid,
		    cmdtype  => "STOPALL" );
	print "stopnode $node called\n";
	sendcmd($node,\%cmd);
    }
}

#
#
#
sub edittest($$$$)
{
    my ($srcnode, $destnode, $testper, $testtype) = @_;
    if ($srcnode eq $destnode ){
	return -1;
    }
    print "editing test: $srcnode\n".
	  "              $destnode\n".
	  "		 $testtype\n".
	  "		 $testper\n";

    my %cmd = ( expid    => $expid,
		cmdtype  => "EDIT",
		dstnode  => $destnode,
		testtype => $testtype,
		testper  => $testper );

    return sendcmd($srcnode,\%cmd);
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
    print "stopping pair tests from $srcnode to $destnode\n";
}

#
# destnodes is space-separated string
#
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
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



sub sendcmd($$)
{
    my $node = $_[0];
    my $hashref = $_[1];
    my %cmd = %$hashref;

    my $sercmd = serialize_hash( \%cmd );
    my $f_success = 0;
    my $max_tries = 3;
    do{
	$socket = IO::Socket::INET->new( PeerPort => $port,
					 Proto    => 'tcp',
					 PeerAddr => $node,
					 Timeout  => 1);
	if( defined $socket ){
	    $sel->add($socket);
	    print $socket "$sercmd\n";
	    #todo: wait for ack;
	    # timeout period?
	    $sel->add($socket);
	    my ($ready) = $sel->can_read(1);
	    if( defined($ready) && $ready eq $socket ){
		my $ack = <$ready>;
		chomp $ack;
		if( $ack eq "ACK" ){
		    $f_success = 1;
		    print "Got ACK from $node for command\n";
		    close $socket;
		}else{
		    $max_tries--;
		}
	    }else{
		$max_tries--;
	    }
	    $sel->remove($socket);
	    close($socket);
	}else{
	    select(undef, undef, undef, 0.2);
	    $max_tries--;
	}
    }while( $f_success != 1 && $max_tries != 0 );

    if( $f_success == 0 && $max_tries == 0 ){
	$deadnodes{$node} = 1;
	print "DID NOT GET ACK from $node for command $sercmd\n";
	return -1;
    }elsif( $f_success == 1 ){
	#success!
	return 1;
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
}


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
