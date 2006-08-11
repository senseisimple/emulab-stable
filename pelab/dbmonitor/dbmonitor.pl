#!/usr/bin/perl
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
" -s   silent mode, don't print any non-errors\n".
"\n".
" -i interval   periodic interval at which to query DB (seconds)\n".
" -S starttime  start time for values pulled from DB (seconds since epoch)\n";
    exit(1);
}

my $optlist  = "ndi:ps1S:";
my $showonly = 0;
my $debug = 0;
my $interval = (1 * 60);
my $verbose = 1;
my $onceonly = 0;
my $starttime = 0;
my $timeskew = 0;

# Default values.  Note: delay and PLR are round trip values.
my $DEF_BW = 10000;	# Kbits/sec
my $DEF_DEL = 0;	# ms
my $DEF_PLR = 0.0;	# prob.

my $PWDFILE = "/local/pelab/pelabdb.pwd";
my $DBHOST  = "users.emulab.net";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";

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
# XXX compat: ignore -p option
if (defined($options{"p"})) {
    ;
}
if (defined($options{"s"})) {
    $verbose = 0;
}
if (defined($options{"1"})) {
    $onceonly = 1;
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
if (@ARGV != 2) {
    usage();
}
my ($pid,$eid) = @ARGV;

# Get DB password and connect.
my $DBPWD   = `cat $PWDFILE`;
if ($DBPWD =~ /^([\w]*)\s([\w]*)$/) {
    $DBPWD = $1;
}
else{
    die("Bad chars in password!\n");
}

#
# XXX figure out how many pairs there are, and for each, who the
# corresponding planetlab node is.  This file is just the output of
# node_list run on ops.  Could probably get all this easier via XMLRPC,
# if the XMLRPC interface worked in the PLAB-DEVBOX image...
#
my @nodelist = split('\s+', `cat $NODEMAP`);
chomp(@nodelist);
my $nnodes = grep(/^$pprefix-/, @nodelist);
if ($nnodes == 0) {
    print STDERR "No planetlab nodes in $pid/$eid?!\n";
    exit(1);
}

#
# Map nodes to site/node indicies
#
my %vnode_to_dbix =  ();
my %vnode_to_pnode = ();
my %ix_mapping   = ();
my %ip_mapping   = ();

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

	if ($vnode =~ /^$pprefix-(\d+)/) {
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
    } elsif ($mapping =~ /^($eprefix-\d+)=(\w*)$/) {
	$vnode_to_pnode{$1} = $2;
    }
}
TBDBDisconnect();
undef @nodelist;

#
# Now figure out who we represent and who all the possible destinations are.
# Names are kept as virtual planetlab names.
#
my $me;
my ($elabname) = split('\.', `hostname`);
if ($elabname =~ /^$eprefix-(\d)+$/) {
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

my @them = ();
foreach my $vnode (keys(%vnode_to_pnode)) {
    if ($vnode =~ /^$pprefix-(\d)+$/) {
	my $ix = $1;
	if ($vnode eq $me) {
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
# Periodically get DB info for all possible targets and
# send it to the appropriate delay agents.
#
while (1) {
    if (get_plabinfo($me, @them) && !$showonly) {
	send_events();
    }
    last
	if ($onceonly);
    sleep($interval);
}

exit(0);

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

#
# Grab data from DB.
#
sub get_plabinfo($@)
{
    my ($me, @nodes) = @_;

    if (TBDBConnect($DBNAME, $DBUSER, $DBPWD, $DBHOST) != 0) {
	warn("Could not connect to pelab DB\n");
	return(0);
    }

    my $src_site = $vnode_to_dbix{$me};
    my $src_ename;
    ($src_ename = $me) =~ s/$pprefix/$eprefix/;
    my $src_pname = $vnode_to_pnode{$me};

    my %lastinfo;
    if (exists($shapeinfo{$src_ename})) {
	foreach my $rec (@{$shapeinfo{$src_ename}}) {
	    my ($doit,$dst,$bw,$del,$plr) = @$rec;
	    $lastinfo{$dst} = [ $bw, $del, $plr ];
	}
    }
    @{$shapeinfo{$src_ename}} = ();

    foreach my $dstvnode (@nodes) {
	my $dst_ip = $ip_mapping{$vnode_to_pnode{$dstvnode}};
	my $dst_site = $vnode_to_dbix{$dstvnode};
	my $dst_ename;
	my $dst_ix;

	($dst_ename = $dstvnode) =~ s/$pprefix/$eprefix/;
	($dst_ix = $dst_ename) =~ s/$eprefix-//;

	#
	# If an explict replay start time was chosen add a time qualifier.
	#
	# Obviously, if we are operating in "unreal" time, we could do a
	# much more exact job of setting characteristics.  Rather than
	# periodic polling of the DB, we could compute a schedule of
	# upcoming events based on the DB and apply them "when they happen".
	# But, we don't.
	#
	my $dateclause = "";
	if ($starttime) {
	    my $mynow = time() - $timeskew;
	    $dateclause = "and unixstamp <= $mynow ";
	}

	#
	# BW and latency records are separate so we just grab the last 20
	# and hope we get a legit value for both.
	# Note that there are no loss measurements right now.
	#
	my ($del,$plr,$bw);
	my ($del_stamp,$plr_stamp,$bw_stamp);
	my $query_result =
	    DBQueryFatal("select latency,loss,bw,unixstamp from pair_data ".
			 " where ".
			 "  srcsite_idx='$src_site' and ".
			 "  dstsite_idx='$dst_site' and ".
			 "   (latency is not null or ".
			 "    loss is not null or bw is not null) ".
			 $dateclause .
			 " order by unixstamp desc limit 20");

	if (!$query_result->numrows) {
	    warn("*** Could not get pair data for ".
		 "$me ($src_site) --> $dstvnode ($dst_site)\n".
		 "    defaulting to ".
		 "${DEF_BW}bps, ${DEF_DEL}ms, ${DEF_PLR}plr\n");
	    ($del,$plr,$bw) = ($DEF_DEL, $DEF_PLR, $DEF_BW);
	    $del_stamp = $plr_stamp = $bw_stamp = time();
	} else {
	    print "$src_ename -> $dst_ename (on behalf of $dst_ip):\n"
		if ($showonly || $debug);

	    while (my ($_del,$_plr,$_bw,$_stamp) =
		   $query_result->fetchrow_array()) {
		print "  ($_del, $_plr, $_bw, $_stamp)\n"
		    if ($showonly || $debug);
		if (!defined($del) && defined($_del)) {
		    $del = $_del;
		    $del_stamp = $_stamp;
		}
		if (!defined($plr) && defined($_plr)) {
		    $plr = $_plr;
		    $plr_stamp = $_stamp;
		}
		if (!defined($bw) && defined($_bw)) {
		    $bw = $_bw;
		    $bw_stamp = $_stamp;
		}
	    }

	    #
	    # XXX This needs to be modified!
	    #
	    if (!defined($del)) {
		$del = $DEF_DEL;
		$del_stamp = time();
	    }
	    if (!defined($plr)) {
		$plr = $DEF_PLR;
		$plr_stamp = time();
	    }
	    # undef or zero--zero BW is not very useful
	    if ($bw == 0) {
		$bw = $DEF_BW;
		$bw_stamp = time();
	    }

	    #
	    # Check for down links, reflected by either delay or bandwidth
	    # being set to -1.  If only one is set to -1, look at the most
	    # recent of delay/bw to determine whether to mark the link as
	    # down.
	    #
	    if ($del == -1 || $bw == -1) {
		if (($del == -1 && $bw == -1) ||
		    ($del == -1 && $del_stamp >= $bw_stamp) ||
		    ($bw == -1 && $bw_stamp >= $del_stamp)) {
		    print STDERR "marking as down: bw=$bw($bw_stamp), del=$del($del_stamp)\n"
			if ($debug);
		    $plr = 1;
		}
		$del = $DEF_DEL
		    if ($del == -1);
		$bw = $DEF_BW
		    if ($bw == -1);
	    }

	    $del = int($del / 2 + 0.5);
	}

	print "$src_ename -> $dst_ename: ".
	    "real=$dst_ip, bw=$bw, del=$del, plr=$plr\n"
		if ($showonly || $debug);

	# XXX need to lookup "elab-$dst_ix"
	$dst_ip = "10.0.0.$dst_ix";
	
	my ($lbw, $ldel, $lplr);
	if (exists($lastinfo{$dst_ip})) {
	    ($lbw, $ldel, $lplr) = @{$lastinfo{$dst_ip}};
	    print STDERR "$src_pname->$dst_ip: old values: bw=$lbw, del=$ldel, plr=$lplr\n"
		if ($debug);
	} else {
	    $lbw = $ldel = $lplr = -2;
	}
	    
	my $doit;
	if ($lbw != $bw || $ldel != $del || $lplr != $plr) {
	    $doit = 1;
	} else {
	    $doit = 0;
	    print STDERR "$src_pname->$dst_ip: values unchanged\n"
		if ($debug);
	}
	push(@{$shapeinfo{$src_ename}}, [ $doit, $dst_ip, $bw, $del, $plr ]);
    }

    TBDBDisconnect();
    return(1);
}

sub logmsg($)
{
    my ($msg) = @_;

    print strftime("%b %e %H:%M:%S", localtime)." dbmonitor[$$]: $msg"
	if ($verbose || $debug);
}
