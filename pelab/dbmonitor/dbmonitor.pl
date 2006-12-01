#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Periodically extract the latest path characteristics from the PELAB DB
# for paths from us (rather, the planetlab node we represent) to all other
# nodes in the experiment.  These data are then fed to the appropriate
# delay-agents.
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

my $MANAGERCLIENT = "/usr/testbed/sbin/managerclient.pl";
my $TEVC = "/usr/testbed/bin/tevc";
my $NODEMAP = "/var/tmp/node-mapping";
my $pprefix = "planet";
my $eprefix = "elab";

# XXX Need to configure this stuff!
use lib '/usr/testbed/lib';
use libtbdb;
use Socket;
use Getopt::Std;
use POSIX qw(strftime);

$| = 1;

#
# Every source host has a list of <dest-IP,bw,delay,plr> tuples, one
# element per possible destination
#
my %shapeinfo;

sub usage()
{
    print STDERR
"usage: dbmonitor.pl [-1dns] [-i interval] [-S starttime] pid eid\n".
" -1   run once only (set initial values and exit)\n".
" -d   turn on debugging\n".
" -n   don't actually set characteristics, just print values from DB\n".
" -r   is running 'remote' on nodes, handle only the node in question\n".
" -s   silent mode, don't print any non-errors\n".
" -B   do not mess with bgmon settings\n".
"\n".
" -i interval   periodic interval at which to query DB (seconds)\n".
" -b interval   set bgmon bandwidth testing interval\n".
" -l interval   set bgmon latency testing interval\n".
" -S starttime  start time for values pulled from DB (seconds since epoch)\n";
    exit(1);
}

my $optlist  = "Bb:ndi:l:prs1S:U";
my $showonly = 0;
my $debug = 0;
my $interval = (1 * 60);
my $latinterval = $interval;
my $bwinterval = (10 * 60);
my $verbose = 1;
my $onceonly = 0;
my $starttime = 0;
my $timeskew = 0;
my $bidir = 1;
my $remote = 0;
my $tweakbgmon = 1;

# Default values.  Note: delay and PLR are round trip values.
my $DEF_BW =  50;	# Kbits/sec
my $DEF_DEL = 1000;	# ms
my $DEF_PLR = 0.0;	# prob.

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBHOST  = "localhost";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";

#
# Check the environment for default interval settings
#
if (exists($ENV{DBMONITOR_INTERVAL}) &&
    $ENV{DBMONITOR_INTERVAL} > 0) {
    $interval = int($ENV{DBMONITOR_INTERVAL});
}
if (exists($ENV{DBMONITOR_LATINTERVAL}) &&
    $ENV{DBMONITOR_LATINTERVAL} > 0) {
    $latinterval = int($ENV{DBMONITOR_LATINTERVAL});
}
if (exists($ENV{DBMONITOR_BWINTERVAL}) &&
    $ENV{DBMONITOR_BWINTERVAL} > 0) {
    $bwinterval = int($ENV{DBMONITOR_BWINTERVAL});
}


#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"n"})) {
    $showonly = 1;
}
if (defined($options{"d"})) {
    $debug++;
}
if (defined($options{"i"})) {
    $interval = int($options{"i"});
}
if (defined($options{"b"})) {
    $bwinterval = int($options{"b"});
}
if (defined($options{"l"})) {
    $latinterval = int($options{"l"});
}
# XXX compat: ignore -p option
if (defined($options{"p"})) {
    ;
}
if (defined($options{"r"})) {
    $remote = 1;
    $PWDFILE = "/local/pelab/pelabdb.pwd";
    $DBHOST = "users.emulab.net";
}
if (defined($options{"s"})) {
    $verbose = 0;
}
if (defined($options{"1"})) {
    $onceonly = 1;
}
if (defined($options{"U"})) {
    $bidir = 0;
}
if (defined($options{"S"})) {
    my $high = time();
    my $low = $high - (23 * 60 * 60); # XXX

    $starttime = $options{"S"};
    if ($starttime && ($starttime < $low || $starttime > $high)) {
	die("Bogus timestamp $starttime, should be in [$low - $high]\n");
    }
    $timeskew = $high - $starttime;
}
if (defined($options{"B"})) {
    $tweakbgmon = 0;
}

if (@ARGV != 2) {
    usage();
}
my ($pid,$eid) = @ARGV;

if ($interval < 1 || $latinterval < 1 || $bwinterval < 1) {
    print STDERR "Bad interval value $interval/$latinterval/$bwinterval\n";
    exit(1);
}

#
# Run interval is the smaller of the desired latency and BW intervals.
#
if ($latinterval < $interval) {
    $interval = $latinterval;
}
if ($bwinterval < $interval) {
    $interval = $bwinterval;
}

# Get DB password and connect.
my $DBPWD   = `cat $PWDFILE`;
if ($DBPWD =~ /^([\w]*)\s([\w]*)$/) {
    $DBPWD = $1;
}
else{
    die("Bad chars in password!\n");
}

my @nodelist = read_nodelist();
my $nnodes = grep(/^$pprefix-/, @nodelist);
if ($nnodes == 0) {
    print STDERR "No planetlab nodes in $pid/$eid?!\n";
    exit(1);
}

#
# For BGMON updates.  We only do them if not remote.
#
my $bgmon_interval;
my $bgmon_update;
my @bgmon_nodes;
if ($remote && $tweakbgmon) {
    warn("*** Remote monitors cannot adjust bgmon\n");
    $tweakbgmon = 0;
}

#
# In remote mode (-r), a monitor runs on every node and queries the DB
# for just its own paths.
#
# In local mode, a single monitor runs on ops and manages the paths of
# all emulation nodes in an experiment.  In local mode, we attempt to
# turn up the frequency of measurements on the plab node monitor.
#

#
# Map nodes to site/node indicies
#
my %vnode_to_dbix =  ();
my %vnode_to_pnode = ();
my %ix_mapping = ();
my %ip_mapping = ();
my %inet_mapping = ();

TBDBConnect($DBNAME, $DBUSER, $DBPWD, $DBHOST) == 0
    or die("Could not connect to pelab database!\n");

foreach my $mapping (@nodelist) {
    if ($mapping =~ /^($pprefix-\d+)=(\w*)$/) {
	my $vnode = $1;
	my $pnode = $2;

	# Grab the indices.
	my $query_result =
	    DBQueryFatal("select site_idx,node_idx from site_mapping ".
			 "where node_id='$pnode'");

	if (!$query_result->numrows) {
	    die("Could not map $pnode to its site index!\n");
	}
	my ($site_index, $node_index) = $query_result->fetchrow_array();

	$vnode_to_pnode{$vnode} = $pnode;
	$vnode_to_dbix{$vnode} = $site_index;

	if ($vnode =~ /^$pprefix-(\d+)$/) {
	    $ix_mapping{$vnode} = $1;
	} else {
	    die("Could not map $vnode to its index!\n");
	}

	# Grab the IP address and save.
	my (undef,undef,undef,undef,@ips) = gethostbyname("$pnode");
	if (!@ips) {
	    die("Could not map $pnode to its ipaddr\n");
	}
	$ip_mapping{$pnode} = inet_ntoa($ips[0]);

	if ($tweakbgmon) {
	    push(@bgmon_nodes, $pnode);
	}
    } elsif ($mapping =~ /^($eprefix-\d+)=(\w*)$/) {
	$vnode_to_pnode{$1} = $2;
    }
}
TBDBDisconnect();
undef @nodelist;

#
# Now figure out who we represent and who all the possible destinations are.
# If remote is set, we only worry about connections for $me, otherwise we
# handle all connections.  Names are kept as virtual planetlab names.
#
my $me;
if ($remote) {
    my ($elabname) = split('\.', `hostname`);
    if ($elabname =~ /^$eprefix-(\d+)$/) {
	$me = "$pprefix-$1";
	if (!exists($vnode_to_pnode{$me})) {
	    die("Bogus host name $elabname");
	}
	if (!exists($vnode_to_dbix{$me})) {
	    die("No DB indices for $me");
	}
	if ($debug) {
	    print STDERR "I am $elabname (", $vnode_to_pnode{$elabname},
	    ") representing $me (", $vnode_to_pnode{$me},
	    ")\n";
	}
    } else {
	die("Bogus host name $elabname");
    }
}

my @them = ();
foreach my $vnode (keys(%vnode_to_pnode)) {
    if ($vnode =~ /^$pprefix-(\d+)$/) {
	my $ix = $1;
	if (defined($me) && $vnode eq $me) {
	    next;
	}
	if (!exists($vnode_to_dbix{$vnode})) {
	    die("No DB indices for destination $vnode");
	}
	push(@them, $vnode);
	if ($debug) {
	    my $ename = "$eprefix-$ix";
	    print STDERR "Destination $ename (", $vnode_to_pnode{$ename},
	                 ") representing $vnode (", $vnode_to_pnode{$vnode},
	    		 ")\n";
	}
    }
}

my $msg = "DB Monitor starting at ";
if ($starttime) {
    $msg .= "$starttime (now - $timeskew)\n";
} else {
    $msg .= time() . " (now)\n";
}
logmsg($msg);

#
# For bgmon.  These are the interval and duration of increased probing
# on the nodes.  For simplicity, duration is computed as an integral
# multiple of the update interval, no less than 1 minute.
#
if ($tweakbgmon) {
    $bgmon_update = int(60 / $interval + 0.5);
    $bgmon_update = 1 if ($bgmon_update == 0);
}

#
# Play nice and crank back bgmon if we get killed
#
$SIG{TERM} = \&bgmon_stop;
$SIG{INT} = \&bgmon_stop;

#
# Periodically get DB info for all possible targets and
# send it to the appropriate delay agents.
#
my $startat = time();
my $intervals = 0;
while (1) {
    my $elapsed = time() - $startat;
    if ($tweakbgmon && ($intervals % $bgmon_update) == 0) {
	# XXX +60 to ensure overlap
	my $duration = $interval * $bgmon_update + 60;

	logmsg("==== Set DB update lat_freq/bw_freq/duration to ".
	       "$latinterval/$bwinterval/$duration at +$elapsed\n");

	bgmon_update($latinterval, $bwinterval, $duration);
    }
    logmsg("==== DB check at +$elapsed\n");
    if (get_plabinfo($me, @them) && !$showonly) {
	send_events();
    }
    last
	if ($onceonly);
    sleep($interval);
    $intervals++;
}

bgmon_stop();

#
# Get the list of nodes in an experiment via XML RPC.
#
# XXX currently, client nodes lack the XML RPC libraries, so we use
# a pre-written file.
#
sub read_nodelist()
{
    my @nodelist;

    if ($remote) {
	@nodelist = split('\s+', `cat $NODEMAP`);
	chomp(@nodelist);
	return @nodelist;
    }

    require libxmlrpc;

    my $rval = libxmlrpc::CallMethod("experiment", "info",
				     { "proj" => "$pid",
				       "exp"  => "$eid",
				       "aspect" => "mapping"});
    if (defined($rval)) {
	my $gotplab = 0;

	#
	# Generate vname=pnode pairs.
	# Also record inet/inet2/intl status of plab nodes
	#
	foreach my $node (keys %$rval) {
	    my $str = "$node=" . $rval->{$node}{"pnode"};
	    push(@nodelist, $str);
	    if ($node =~ /^$pprefix/) {
		my $auxtype = $rval->{$node}{"auxtype"};
		if ($auxtype =~ /inet2/) {
		    $auxtype = "inet2";
		} else {
		    $auxtype = "inet";
		}
		$inet_mapping{$node} = $auxtype;
		$gotplab++;
	    }
	}

	#
	# No planetlab machines in the experiment.  We might be doing
	# emulation only, so look for a list of plab nodes in the
	# user environment.
	#
	if ($gotplab == 0) {
	    my %plist;

	    my $rval = libxmlrpc::CallMethod("experiment", "metadata",
					     { "proj" => "$pid",
					       "exp"  => "$eid" });
	    if (defined($rval)) {
		my $aref = $rval->{"user_environment"};
		foreach my $env (@$aref) {
		    if ($env->{"name"} =~ /pelab-elab-(\d+)-mapping/) {
			my $node = "planet-$1";
			$plist{$node} = $env->{"value"};
			my $str = "$node=" . $plist{$node};
			push(@nodelist, $str);
		    }
		}

		# build a node string to use in the node.getlist call
		my $nodestr = join(',', values(%plist));

		# sslxmlrpc_client.py -m node getlist class=pcplabphys
		my $rval = libxmlrpc::CallMethod("node", "getlist",
						 { "proj" => "$pid",
						   "nodes" => "$nodestr",
						   "class"  => "pcplabphys" });
		if (defined($rval)) {
		    while (my ($node,$pnode) = each(%plist)) {
			my $auxtype;
			if (defined($rval->{$pnode})) {
			    $auxtype = $rval->{$pnode}{"auxtypes"};
			    if ($auxtype =~ /inet2/) {
				$auxtype = "inet2";
			    } else {
				$auxtype = "inet";
			    }
			} else {
			    $auxtype = "inet";
			}
			$inet_mapping{$node} = $auxtype;
			$gotplab++;
		    }
		}
	    }
	}
    } elsif ($debug) {
	print STDERR "XMLRPC experiment.info on $pid/$eid failed\n";
    }

    return @nodelist;
}

sub send_events()
{
    my $msg;

    foreach my $src (keys %shapeinfo) {
	foreach my $rec (@{$shapeinfo{$src}}) {
	    my ($doit,$dst,$bw,$del,$plr) = @{$rec};
	    next
		if (!$doit);

	    my $cmd = "$TEVC -e $pid/$eid now elabc-$src MODIFY ".
		"DEST=$dst BANDWIDTH=$bw DELAY=$del PLR=$plr";
	    $msg = "elabc-$src: DEST=$dst BANDWIDTH=$bw DELAY=$del PLR=$plr...";
	    if (system("$cmd") != 0) {
		warn("*** '$cmd' failed\n");
		$msg .= "[FAILED]\n";
	    } else {
		$msg .= "[OK]\n";
	    }
	    logmsg($msg);
	}
    }
}

sub fetchone($$$$$)
{
    my ($me, $src_site, $dstvnode, $dst_site, $mynow) = @_;

    #
    #
    #
    my $siteclause;
    if ($bidir) {
	$siteclause =
	    "((srcsite_idx='$src_site' and dstsite_idx='$dst_site') or ".
		" (dstsite_idx='$src_site' and srcsite_idx='$dst_site')) ";
	
    } else {
	$siteclause = 
	    "(srcsite_idx='$src_site' and dstsite_idx='$dst_site') ";
    }

    my $dateclause = "";
    if (defined($mynow)) {
	$dateclause = "and unixstamp <= $mynow ";
    } else {
	$mynow = time();
    }

    #
    # BW and latency records are separate and there are, in general,
    # a lot more latency measurements than BW measurements.  So we
    # make two queries grabbing the latest of each.
    #
    # We also now grab loss data as a loss of 1.0 indicates that bgmon
    # has determined that a link is down.
    #
    my ($del,$plr,$bw);
    my ($del_stamp,$plr_stamp,$bw_stamp) = (0,0,0);
    my $query_result =
	DBQueryFatal("select latency,unixstamp from pair_data ".
		     " where ".
		     $siteclause .
		     " and latency is not null ".
		     $dateclause .
		     " order by unixstamp desc limit 1");
    if ($query_result->numrows) {
	($del, $del_stamp) = $query_result->fetchrow_array();
    }
    $query_result =
	DBQueryFatal("select bw,unixstamp from pair_data ".
		     " where ".
		     $siteclause .
		     " and bw is not null ".
		     $dateclause .
		     " order by unixstamp desc limit 1");
    if ($query_result->numrows) {
	($bw, $bw_stamp) = $query_result->fetchrow_array();
    }
    $query_result =
	DBQueryFatal("select loss,unixstamp from pair_data ".
		     " where ".
		     $siteclause .
		     " and loss is not null ".
		     $dateclause .
		     " order by unixstamp desc limit 1");
    if ($query_result->numrows) {
	($plr, $plr_stamp) = $query_result->fetchrow_array();
    }
    
    if ($showonly || $debug) {
	print "DB state for $me ($src_site) -> $dstvnode ($dst_site):\n";
	print "  (del=",
	      $del_stamp ? "$del\@$del_stamp" : "NONE",
	      ", bw=",
              $bw_stamp ? "$bw\@$bw_stamp" : "NONE",
              ", plr=",
              $plr_stamp ? "$plr\@$plr_stamp" : "NONE",
	      ")\n";
    }

    #
    # Check for down links.
    # A link is definitely down if the DB says so (plr==1) and there have
    # been no other valid latency/bw readings since.
    #
    if ($plr_stamp && $plr == 1) {
	if ($del_stamp < $plr_stamp && $bw_stamp < $plr_stamp) {
	    print STDERR "DB reports link as down\n"
    		if ($debug);
	} else {
	    $plr = $DEF_PLR;
	    $plr_stamp = $mynow;
	}
    }

    #
    # A down link may also be reflected by either delay or bandwidth
    # being set to < 0.  If only one is set negative, look at the most
    # recent of delay/bw to determine whether to mark the link as
    # down.
    #
    elsif (($del_stamp && $del < 0) || ($bw_stamp && $bw < 0)) {
	#
	# Both are negative
	#
	if ($del_stamp && $del < 0 && $bw_stamp && $bw < 0) {
	    print STDERR "marking as down, no valid bw/del measurements: ".
		         "bw=$bw($bw_stamp), del=$del($del_stamp)\n"
			     if ($debug);
	    $plr = 1;
	    $plr_stamp = $mynow;
	}

	#
	# Erroneous BW measurement that is more recent than delay measurement
	#
	elsif ($bw_stamp && $bw < 0 && $bw_stamp >= $del_stamp) {
	    print STDERR "marking as down, most recent bw measurement bad: ".
		         "bw=$bw($bw_stamp), del=$del($del_stamp)\n"
			     if ($debug);
	    $plr = 1;
	    $plr_stamp = $mynow;
	}

	#
	# Erroneous delay measurement that is more recent than BW.
	#
	# XXX some nodes don't allow pings, but are actually up.
	# So if we get a delay value of -1 but a legitimate BW value,
	# we make a more detailed query to determine if the node has
	# "always" (in the last N hours of data in the DB) returned -1.
	#
	elsif ($del_stamp && $del < 0 && $del_stamp >= $bw_stamp) {
	    my $query_result =
		DBQueryFatal("select count(*) from pair_data ".
			     " where ".
			     $siteclause .
			     " and latency is not null and latency >= 0 ".
			     $dateclause);
	    my ($count) = $query_result->fetchrow_array();
	    if ($count > 0) {
		print STDERR "marking as down, most recent del measurement bad: ".
		    "bw=$bw($bw_stamp), del=$del($del_stamp)\n"
			if ($debug);
		$plr = 1;
		$plr_stamp = $mynow;
	    }
	}
    }
	
    #
    # Now that we have taken care of loss conditions, set default values
    # for those for which we did not get readings.
    #
    if ($del_stamp == 0 || $del < 0) {
	print STDERR "NOTE: no current latency data for ".
		     "$me ($src_site) --> $dstvnode ($dst_site)\n".
		     "    defaulting to ${DEF_DEL}ms\n";
	$del = $DEF_DEL;
	$del_stamp = $mynow;
    }
    if ($plr_stamp == 0) {
	# XXX this condition is common
	if (0) {
	    print STDERR "NOTE: no current loss data for ".
			 "$me ($src_site) --> $dstvnode ($dst_site)\n".
			 "    defaulting to ${DEF_PLR}\n";
	}
	$plr = $DEF_PLR;
	$plr_stamp = $mynow;
    }
    # undef or zero--zero BW is not very useful
    if ($bw_stamp == 0 || $bw <= 0) {
	print STDERR "NOTE: no current bandwidth data for ".
		     "$me ($src_site) --> $dstvnode ($dst_site)\n".
		     "    defaulting to ${DEF_BW}Kbps\n";
	$bw = $DEF_BW;
	$bw_stamp = $mynow;
    }

    return ($del, $plr, $bw);
}

#
# Grab data from DB.
#
sub get_plabinfo($@)
{
    my ($me, @nodes) = @_;
    my @snodes;

    if (TBDBConnect($DBNAME, $DBUSER, $DBPWD, $DBHOST) != 0) {
	warn("Could not connect to pelab DB\n");
	return(0);
    }

    if (defined($me)) {
	@snodes = ($me);
    } else {
	@snodes = @nodes;
    }

    foreach $me (@snodes) {
	my $src_site = $vnode_to_dbix{$me};
	my $src_pname = $vnode_to_pnode{$me};
	my $src_ename;
	my $src_ix;

	($src_ename = $me) =~ s/$pprefix/$eprefix/;
	($src_ix = $src_ename) =~ s/$eprefix-//;

	# XXX should lookup "elab-$src_ix"
	my $src_ip = "10.0.0.$src_ix";

	my %lastinfo;
	if (exists($shapeinfo{$src_ename})) {
	    foreach my $rec (@{$shapeinfo{$src_ename}}) {
		my ($doit,$dst_ip,$bw,$del,$plr) = @$rec;
		$lastinfo{$src_ip}{$dst_ip} = [ $bw, $del, $plr ];
	    }
	}

	@{$shapeinfo{$src_ename}} = ();

	foreach my $dstvnode (@nodes) {

	    next
		if ($me eq $dstvnode);

	    my $dst_ip = $ip_mapping{$vnode_to_pnode{$dstvnode}};
	    my $dst_site = $vnode_to_dbix{$dstvnode};
	    my $dst_ename;
	    my $dst_ix;

	    ($dst_ename = $dstvnode) =~ s/$pprefix/$eprefix/;
	    ($dst_ix = $dst_ename) =~ s/$eprefix-//;

	    #
	    # If an explict replay start time was chosen add a time qualifier.
	    #
	    # Obviously, if we are operating in "unreal" time, we could do
	    # a much more exact job of setting characteristics.  Rather than
	    # periodic polling of the DB, we could compute a schedule of
	    # upcoming events based on the DB and apply them "when they
	    # happen".  But, we don't.
	    #
	    my $mynow;
	    if ($starttime) {
		my $mynow = time() - $timeskew;
	    }

	    my ($del,$plr,$bw) =
		fetchone($me, $src_site, $dstvnode, $dst_site, $mynow);

	    #
	    # Convert as necessary:
	    # * recorded delay is round-trip, so divide by 2 for one-way
	    # * BW is based on TCP payload, so compensate for dummynet's
	    #   use of ethernet.  Assuming full-sized ethernet packets
	    #   each with 54 bytes of overhead (14 + 20 + 20) that dummynet
	    #   will count, that is 1514 bytes for every 1460 that iperf
	    #   counts.  So we adjust by a factor of 1.036.
	    #
	    $del = int($del/2 + 0.5);
	    if ($bw != $DEF_BW) {
		$bw = int($bw * 1.036 + 0.5);
	    }

	    print "$src_ename -> $dst_ename: ".
		"real=$dst_ip, bw=$bw, del=$del, plr=$plr\n"
		    if ($showonly || $debug);
	
	    # XXX need to lookup "elab-$dst_ix"
	    $dst_ip = "10.0.0.$dst_ix";
	
	    my ($lbw, $ldel, $lplr);
	    if (exists($lastinfo{$src_ip}{$dst_ip})) {
		($lbw, $ldel, $lplr) = @{$lastinfo{$src_ip}{$dst_ip}};
		print STDERR "$src_pname->$dst_ip: old values: ".
		             "bw=$lbw, del=$ldel, plr=$lplr\n"
		    if ($debug);
	    } else {
		$lbw = $ldel = $lplr = -1;
	    }
	    
	    my $doit;
	    if ($lbw != $bw || $ldel != $del || $lplr != $plr) {
		$doit = 1;
	    } else {
		$doit = 0;
		print STDERR "$src_pname->$dst_ip: values unchanged\n"
		    if ($debug);
	    }
	    push(@{$shapeinfo{$src_ename}},
		 [ $doit, $dst_ip, $bw, $del, $plr ]);
	}
    }

    TBDBDisconnect();
    return(1);
}

#
# Just run managerclient.
# XXX we are just updating the latency measurement frequency right now.
#
sub bgmon_update($$$)
{
    my ($liv, $biv, $dur) = @_;
    my $livstr;
    my $bivstr;

    if ($liv == 0) {
	$livstr = "-L";
    } else {
	$livstr = "-l $liv";
    }
    if ($biv == 0) {
	$bivstr = "-B";
    } else {
	$bivstr = "-b $biv";
    }

    my $cmd = "$MANAGERCLIENT -i $pid/$eid -c start -a " .
	"$livstr $bivstr -d $dur " . join(' ', @bgmon_nodes);
    print STDERR "bgmon_update: $cmd\n"
	if ($debug);
    if (system("$cmd") != 0) {
	warn("*** '$cmd' failed\n");
    }
}

sub bgmon_stop()
{
    if ($tweakbgmon) {
	my $cmd = "$MANAGERCLIENT -i $pid/$eid -c stop";
	print STDERR "\nbgmon_stop: $cmd\n"
	    if ($debug);
	if (system("$cmd") != 0) {
	    warn("*** '$cmd' failed\n");
	}
    }
    exit(0);
}

sub logmsg($)
{
    my ($msg) = @_;

    print strftime("%b %e %H:%M:%S", localtime)." dbmonitor[$$]: $msg"
	if ($verbose || $debug);
}
