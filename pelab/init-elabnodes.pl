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
my @nodelist = split('\s+', `$NLIST -v -e $pid,$eid`);
chomp(@nodelist);
my $nnodes = grep(/^${pprefix}/, @nodelist);
if ($nnodes == 0) {
    print STDERR "No planetlab nodes in $pid/$eid?!\n";
    exit(1);
}

#
# Get planetlab info for each planetlab node...
#
foreach (@nodelist) {
    if (/^${pprefix}(\d+)/) {
	get_plabinfo($1);
    }
}

#
# ...and send events to set the characteristics
#
send_events();

exit(0);

sub send_events()
{
    foreach my $src (keys %shapeinfo) {
	foreach my $rec (@{$shapeinfo{$src}}) {
	    my ($dst,$bw,$del,$plr) = @{$rec};
	    my $cmd = "$TEVC -e $pid/$eid now elabc-$src MODIFY ".
		"DEST=$dst BANDWIDTH=$bw DELAY=$del PLR=$plr";
	    print "$cmd... ";
	    if (system("$cmd") != 0) {
		print "FAILED\n";
	    } else {
		print "OK\n";
	    }
	}
    }
}

#
# XXX testing...
# Replace with real code that looks up IP of "planet-$ix" and
# then looks in DB for info.
#
sub get_plabinfo($)
{
    my ($ix) = @_;

    @{$shapeinfo{"elab-$ix"}} = ();
    foreach my $i (1 .. $nnodes) {
	next if ($i == $ix);

	my $dst = "10.0.0.$ix";
	my $bw = $i * 1000;
	my $del = $i;
	my $plr = $i / 100;

	push(@{$shapeinfo{"elab-$ix"}}, [ $dst, $bw, $del, $plr ]);
    }
}
