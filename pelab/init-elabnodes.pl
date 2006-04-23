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

# XXX Need to configure this stuff!
use lib '/usr/testbed/lib';
use libtbdb;
use Socket;

my $showonly = 0;

# default values
my $DEF_BW = 10000;	# Kbits/sec
my $DEF_DEL = 0;	# ms
my $DEF_PLR = 0.0;	# prob.

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";

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

#
# Every source host has a list of <dest-IP,bw,delay,plr> tuples, one
# element per possible destination
#
my %shapeinfo;

if (@ARGV != 2) {
    print STDERR "usage: init-elabnodes pid eid\n";
    exit(1);
}
my ($pid,$eid) = @ARGV;

#
# XXX figure out how many pairs there are, and for each, who the
# corresponding planetlab node is.  Can probably get all this easier
# via XMLRPC...
#
my @nodelist = split('\s+', `$NLIST -m -e $pid,$eid`);
chomp(@nodelist);
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

    @{$shapeinfo{"elab-$ix"}} = ();

    foreach my $dstvnode (keys(%node_mapping)) {
	next
	    if ($srcvnode eq $dstvnode);

	my $dst       = $ip_mapping{$node_mapping{$dstvnode}};
	my $src_site  = $site_mapping{$node_mapping{$srcvnode}};
	my $dst_site  = $site_mapping{$node_mapping{$dstvnode}};
	my $dstix     = $ix_mapping{$dstvnode};

	my ($del,$plr,$bw);
	my $query_result =
	    DBQueryFatal("select latency,loss,bw from pair_data ".
			 "where srcsite_idx='$src_site' and ".
			 "      dstsite_idx='$dst_site' ".
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
	    $bw = $DEF_BW
		if ($bw == 0);
	}

	print "elab-$srcix -> elab-$dstix: ".
	    "real=$dst, bw=$bw, del=$del, plr=$plr\n";

	# XXX need to lookup "elab-$dstix"
	$dst = "10.0.0.$dstix";
	
	push(@{$shapeinfo{"elab-$srcix"}}, [ $dst, $bw, $del, $plr ]);
    }
}
