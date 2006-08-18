#!/usr/bin/perl
#
# Based on pairwise characteristics of planet-* nodes (real planetlab nodes)
# in a pelab experiment, set the delay characteristics for the corresponding
# elab-* nodes.
#
# Elab nodes are in a "cloud" allowing you to set node-to-node characteristics
# for all nodes within the cloud.  To do this for node N to node M:
#
#	tevc -e pid/eid now elabc-elab-N MODIFY DEST=<elab-M-elabc IP> \
#		[ BANDWIDTH=<kbits/sec> ] [ DELAY=<ms> ] [ PLR=<prob> ]
#
# The characteristics are applied to the node->LAN pipe for node N.
# Since this is one-way, you will have to apply the usual tricks to
# convert round-trip delays and PLR.
#

my $TEVC = "/usr/testbed/bin/tevc";
my $NLIST = "/usr/testbed/bin/node_list";
my $pprefix = "planet-";
#my $pprefix = "plab-";

# XXX Need to configure this stuff!
use lib '/usr/testbed/lib';
use libtbdb;
use Socket;
use Getopt::Std;
use Class::Struct;
use libxmlrpc;
use strict;

my ($initvalLat, $initvalBw);
################
# Define a structure to hold the values used in initial condition reporting
struct( initvalres => {
    srcnode => '$',
    dstnode => '$',
    ave_exp => '$',
    numSamples => '$',
    numErrValSamples => '$',
    numLastSeqErr =>'$',
    tstampLastSample => '$' } );
#'################

#
# Every source host has a list of <dest-IP,bw,delay,plr> tuples, one
# element per possible destination
#
my %shapeinfo;

my $showonly = 0;
my $starttime = 0;

# Default values.  Note: delay and PLR are round trip values.
my $DEF_BW = 10000;	# Kbits/sec
my $DEF_DEL = 0;	# ms
my $DEF_PLR = 0.0;	# prob.

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";

my $now     = time();

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
my %options = ();
if (! getopts("S:n", \%options)) {
    usage();
}
if (defined($options{"n"})) {
    $showonly = 1;
}
if (defined($options{"S"})) {
    my $high = time();
    my $low = $high - (23 * 60 * 60); # XXX

    $starttime = $options{"S"};
    if ($starttime && ($starttime < $low || $starttime > $high)) {
	die("Bogus timestamp $starttime, should be in [$low - $high]\n");
    }
}
if (@ARGV != 2) {
    print STDERR "usage: init-elabnodes pid eid\n";
    exit(1);
}
my ($pid,$eid) = @ARGV;

# Get DB password and connect.
my $DBPWD   = `cat $PWDFILE`;
if ($DBPWD =~ /^([\w]*)\s([\w]*)$/) {
    $DBPWD = $1;
}
else{
    fatal("Bad chars in password!");
}
TBDBConnect($DBNAME, $DBUSER, $DBPWD) == 0
    or die("Could not connect to pelab database!\n");

# RPC STUFF ##############################################
my $TB         = "/usr/testbed";
my $ELABINELAB = 0;
my $RPCSERVER  = "boss.emulab.net";  #?
my $RPCPORT    = "3069";
#my $RPCCERT    = "/etc/outer_emulab.pem"; #?
my $RPCCERT = "~/.ssl/emulab.pem";
my $MODULE = "node";
my $METHOD = "getlist";

libxmlrpc::Config({"server"  => $RPCSERVER,
		    "verbose" => 0,
		    "portnum" => $RPCPORT});
# END RPC STUFF ##########################################



#
# XXX figure out how many pairs there are, and for each, who the
# corresponding planetlab node is.  Can probably get all this easier
# via XMLRPC...
#
my @nodelist = split('\s+', `$NLIST -m -e $pid,$eid`);
chomp(@nodelist);
print "NODELIST:\n@nodelist\n";
my $nnodes = grep(/^${pprefix}/, @nodelist);
if ($nnodes == 0) {
    print STDERR "No planetlab nodes in $pid/$eid?!\n";
    exit(1);
}

# Preload the site indicies rather then doing fancy joins.
my %site_mapping = ();
my %node_mapping = ();
my %ix_mapping   = ();
my %ip_mapping   = ();

foreach my $mapping (@nodelist) {
    if ($mapping =~ /^(${pprefix}[\d]+)=([\w]*)$/) {
	my $vnode = $1;
	my $pnode = $2;

	# Grab the site index.
	my $query_result =
	    DBQueryFatal("select site_idx from site_mapping ".
			 "where node_id='$pnode'");

	if (! $query_result->numrows) {
	    die("Could not map $pnode to its site index!\n");
	}
	my ($site_index) = $query_result->fetchrow_array();
	
	print "Mapping $vnode to $pnode\n";

	$node_mapping{$vnode} = $pnode;
	$site_mapping{$pnode} = $site_index;

	if ($vnode =~ /^${pprefix}(\d+)/) {
	    $ix_mapping{$vnode} = $1;
	}
	else {
	    die("Could not map $vnode to its index!\n");
	}

	# Grab the IP address and save.
	my (undef,undef,undef,undef,@ips) = gethostbyname("$pnode");

	if (!@ips) {
	    die("Could not map $pnode to its ipaddr\n");
	}
	$ip_mapping{$pnode} = inet_ntoa($ips[0]);
    }
}

my $msg = "Intializing conditions from time ";
if ($starttime) {
    $msg .= "$starttime (now - " . ($now - $starttime) . " seconds)\n";
} else {
    $msg .= $now . " (now)\n";
    $starttime = $now;
}
print($msg);

#
# Get planetlab info for each planetlab node...
#
foreach my $vnode (keys(%node_mapping)) {
    get_plabinfo($vnode);
}

#
# ...and send events to set the characteristics
#
send_events()
    if (!$showonly);

exit(0);

sub send_events()
{
    foreach my $src (keys %shapeinfo) {
	foreach my $rec (@{$shapeinfo{$src}}) {
	    my ($dst,$bw,$del,$plr) = @{$rec};
	    my $cmd = "$TEVC -e $pid/$eid now elabc-$src MODIFY ".
		"DEST=$dst BANDWIDTH=$bw DELAY=$del PLR=$plr";
	    print "elabc-$src: DEST=$dst BANDWIDTH=$bw DELAY=$del PLR=$plr...";
	    if (system("$cmd") != 0) {
		print "[FAILED]\n";
	    } else {
		print "[OK]\n";
	    }
	}
    }
}


#
# Grab data from DB.
#
sub get_plabinfo($)
{
    my ($srcvnode) = @_;
    my $srcix = $ix_mapping{$srcvnode};

    @{$shapeinfo{"elab-$srcix"}} = ();

    foreach my $dstvnode (keys(%node_mapping)) {
	next
	    if ($srcvnode eq $dstvnode);

	#Only need these two for error messages. get_pathInitCond finds them
        # itself.
	my $dst       = $ip_mapping{$node_mapping{$dstvnode}};
	my $src_site  = $site_mapping{$node_mapping{$srcvnode}};
	my $dst_site  = $site_mapping{$node_mapping{$dstvnode}};
	my $dstix  = $ix_mapping{$dstvnode};

	# Get initial conditions
	($initvalLat, $initvalBw) = 
	     get_pathInitCond( $node_mapping{$srcvnode}, 
			       $node_mapping{$dstvnode},
			       24, 0.8 );
	bless $initvalLat, "initvalres";
#	print "initvalLat=$initvalLat\n";
	bless $initvalBw, "initvalres";
#	print "initvalBw=$initvalBw\n";
=pod
        print "\nLATENCY\n";
	printInitValResStruct($initvalLat);
	print "\nBW\n";
	printInitValResStruct($initvalBw);
=cut
	#TODO: is this print statement wanted here? Should it be conditional
        #      on available path measurements, like in the original?
	print "elab-$srcix -> elab-$dstix (on behalf of $dst):\n"
	    if ($showonly);

	my ($del,$plr,$bw) = ($initvalLat->ave_exp,
			   undef,  #TODO!!! LOSS RATE!!!
			   $initvalBw->ave_exp);

#	print "INIT COND: ($del,$plr,$bw)\n";

	# handle a path with no available measurements
	if($initvalLat->numSamples == $initvalLat->numErrValSamples){
	    $del = $DEF_DEL;
	    warn("*** Could not get latency ".
		 "$srcvnode ($src_site) --> $dstvnode ($dst_site)\n".
		 "    defaulting to ".
		 "${DEF_DEL}ms\n" );
	}
	if(!defined($plr)){
	    $plr = $DEF_PLR;
	    warn("*** Could not get lossrate ".
		 "$srcvnode ($src_site) --> $dstvnode ($dst_site)\n".
		 "    defaulting to ".
		 "${DEF_PLR}plr\n" );
	}
	if($initvalBw->numSamples == $initvalBw->numErrValSamples){	 
	    $bw = $DEF_BW;
	    warn("*** Could not get bandwidth ".
		 "$srcvnode ($src_site) --> $dstvnode ($dst_site)\n".
		 "    defaulting to ".
		 "${DEF_BW}bps\n" );
	}
	    



=pod
	my $dst       = $ip_mapping{$node_mapping{$dstvnode}};
	my $src_site  = $site_mapping{$node_mapping{$srcvnode}};
	my $dst_site  = $site_mapping{$node_mapping{$dstvnode}};

	my $query_result =
	    DBQueryFatal("select latency,loss,bw from pair_data ".
			 "where srcsite_idx='$src_site' and ".
			 "      dstsite_idx='$dst_site' and ".
			 "      unixstamp <= $starttime ".
			 "order by unixstamp desc limit 5");

	if (!$query_result->numrows) {
	    warn("*** Could not get pair data for ".
		 "$srcvnode ($src_site) --> $dstvnode ($dst_site)\n".
		 "    defaulting to ".
		 "${DEF_BW}bps, ${DEF_DEL}ms, ${DEF_PLR}plr\n");
	    ($del,$plr,$bw) = ($DEF_DEL, $DEF_PLR, $DEF_BW);
	} else {
	    my @vals;

	    print "elab-$srcix -> elab-$dstix (on behalf of $dst):\n"
		if ($showonly);
	    while (($del,$plr,$bw) = $query_result->fetchrow_array()) {
		print "  ($del, $plr, $bw)\n"
		    if ($showonly);
		push(@vals, [ $del, $plr, $bw ]);
	    }

	    #
	    # XXX This needs to be modified!
	    #
	    ($del,$plr,$bw) = @{$vals[0]};
	    $del = $DEF_DEL
		if (!defined($del));
	    $plr = $DEF_PLR
		if (!defined($plr));
	    $bw = $DEF_BW
		if ($bw == 0);	# undef or zero--zero BW is not very useful

	    $del = int($del / 2);
	}
=cut

	print "elab-$srcix -> elab-$dstix: ".
	    "real=$dst, bw=$bw, del=$del, plr=$plr\n";

	# XXX need to lookup "elab-$dstix"
	$dst = "10.0.0.$dstix";
	
	push(@{$shapeinfo{"elab-$srcix"}}, [ $dst, $bw, $del, $plr ]);
    }
}




######################################################################
# Grab data from DB to create an initial condition structure
#
#  Notes:
#    + Data used in init cond is from any node at the sites containing
#      the given nodes.
#
sub get_pathInitCond($$$;$)
{
    my ($srcnode, $dstnode, $pasthours, $expAlpha) = @_;
    if( !defined $expAlpha) { $expAlpha = 0.6; }  #default alpha value
    my $lasttime = $starttime;
    #my $firsttime = $lasttime - (60*60*24);
    my $firsttime = $lasttime - (60*60*$pasthours);
    my $srcsite_idx    = $site_mapping{$srcnode};
    my $dstsite_idx    = $site_mapping{$dstnode};

    my $initvalLat = new initvalres;
    my $initvalBw = new initvalres;

    my @latsamples = ();
    my @bwsamples = ();
    my @latsamples_noerr;
    my @bwsamples_noerr;

    #
    # retreive data for last 24 hours for the given path and
    # populate result structures
    #
    my $sth = DBQuery(
		      "SELECT * ".
		      "FROM pair_data WHERE ".
		      "srcsite_idx = $srcsite_idx and ".
		      "dstsite_idx = $dstsite_idx and ".
		      "(latency IS NOT NULL or bw IS NOT NULL) and ".
		      "unixstamp > $firsttime and ".
		      "unixstamp <= $lasttime ".
		      "order by unixstamp asc ".
		      ";" );
    while( my $hr = $sth->fetchrow_hashref() ){
	my %row = %{$hr};
	if ( defined $row{latency} ){
	    push @latsamples, $row{latency};
	    $initvalLat->tstampLastSample($row{unixstamp});
	    $initvalLat->srcnode($srcnode) if !defined($initvalLat->srcnode);
	    $initvalLat->dstnode($dstnode) if !defined($initvalLat->dstnode);
	}
	if ( defined $row{bw} ){
	    push @bwsamples, $row{bw} ;
	    $initvalBw->tstampLastSample($row{unixstamp});
	    $initvalBw->srcnode($srcnode) if !defined($initvalBw->srcnode);
	    $initvalBw->dstnode($dstnode) if !defined($initvalBw->dstnode);
	}
    }
    #do another query in reverse path if given one doesn't have latency data
    if( scalar @latsamples == 0 ){
	my $sth = DBQuery(
		      "SELECT latency ".
		      "FROM pair_data WHERE ".
		      "srcsite_idx = $dstsite_idx and ".
		      "dstsite_idx = $srcsite_idx and ".
		      "(latency IS NOT NULL) and ".
		      "unixstamp > $firsttime and ".
		      "unixstamp <= $lasttime ".
		      "order by unixstamp asc ".
		      ";" );
	while( my $hr = $sth->fetchrow_hashref() ){
	    my %row = %{$hr};
	    if ( defined $row{latency} ){
		push @latsamples, $row{latency};
		$initvalLat->tstampLastSample($row{unixstamp});
		$initvalLat->srcnode($dstnode) 
		    if !defined($initvalLat->srcnode);
		$initvalLat->dstnode($srcnode) 
		    if !defined($initvalLat->dstnode);
	    }
	}
    }
#    print "latsamples = @latsamples\n";
#    print "bwsamples = @bwsamples\n";

    $initvalLat->numSamples(scalar @latsamples);
    $initvalBw->numSamples(scalar @bwsamples);

    
    # Find err vals

    #do for latency
    my $flag_errseriesEnd = 0;
    my $numErrs = 0;
    my $numSeriesErr = 0;
    for( my $i=$#latsamples; $i >=0; $i-- ){
	if( $latsamples[$i] < 0 ){
	    $numErrs++;
	    if( $flag_errseriesEnd == 0 ){
		$numSeriesErr++;
	    }
	}else{
	    $flag_errseriesEnd = 1;
	    unshift @latsamples_noerr, $latsamples[$i];
	}
    }
    $initvalLat->numLastSeqErr($numSeriesErr);
    $initvalLat->numErrValSamples($numErrs);
    #now for bw
    $flag_errseriesEnd = 0;
    $numErrs = 0;
    $numSeriesErr = 0;
    for( my $i=$#bwsamples; $i >=0; $i-- ){
	if( $bwsamples[$i] < 0 ){
	    $numErrs++;
	    if( $flag_errseriesEnd == 0 ){
		$numSeriesErr++;
	    }
	}else{
	    $flag_errseriesEnd = 1;
	    unshift @bwsamples_noerr, $bwsamples[$i];
	}
    }
    $initvalBw->numErrValSamples($numErrs);
    $initvalBw->numLastSeqErr($numSeriesErr);


    # Calculate Exponential average
    # TODO!! Change this called function such that the time between
    #        samples is factored into the weighting.
    $initvalLat->ave_exp( calcExpAve($expAlpha,\@latsamples_noerr) );
    $initvalBw->ave_exp( calcExpAve($expAlpha,\@bwsamples_noerr) );

=pod
    print "\nLatency\n@latsamples\n";
    printInitValResStruct($initvalLat);
    print "\nBW\n@bwsamples\n";
    printInitValResStruct($initvalBw);
=cut

    return ($initvalLat, $initvalBw);
}


sub printInitValResStruct($)
{
    my ($struct) = @_;

    print "printing InitValRes Structure\n";
    foreach my $var (keys %{$struct}){
	print "$var  =   \t".${$struct}{$var} ."\n" 
	    if defined ${$struct}{$var};
    }
}


sub calcExpAve($$\@)
{
    my ($alpha, $aref) = @_;
    my @values = @$aref;

    if( scalar(@values) == 0 ){ return 0; }

    my $lastAve = $values[0];  #start with first sample
    for( my $i=1; $i<@values; $i++ ){
	$lastAve = $alpha*$values[$i] + (1-$alpha)*$lastAve;
    }
    return $lastAve;
}
