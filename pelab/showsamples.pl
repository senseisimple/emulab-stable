#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Show latency and/or BW samples between a pair of sites based on indices.
#
#
my $NLIST = "/usr/testbed/bin/node_list";
my $pprefix = "planet-";

# XXX Need to configure this stuff!
use lib '/usr/testbed/lib';
use libtbdb;
use Socket;
use Getopt::Std;
use Class::Struct;
use libxmlrpc;
use strict;

my $starttime = 0;
my $dolatency = 0;
my $dobw = 0;
my $doloss = 0;
my $bidir = 0;
my $number = 0;

my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";

my $pid;
my $eid;

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
my %options = ();
if (! getopts("Bbdle:S:n:", \%options)) {
    usage();
}
if (defined($options{"e"})) {
    ($pid,$eid) = split('/', $options{"e"});
}
if (defined($options{"B"})) {
    $bidir = 1;
}
if (defined($options{"d"})) {
    $dolatency = 1;
}
if (defined($options{"b"})) {
    $dobw = 1;
}
if (defined($options{"l"})) {
    $doloss = 1;
}
if (defined($options{"n"})) {
    $number = $options{"n"};
}
if (defined($options{"S"})) {
    my $high = time();
    my $low = $high - (23 * 60 * 60); # XXX

    $starttime = $options{"S"};
    if ($starttime && ($starttime < $low || $starttime > $high)) {
	die("Bogus timestamp $starttime, should be in [$low - $high]\n");
    }
}
if (!defined($pid) && @ARGV != 2) {
    print STDERR "usage: showsamples.pl [-Bbdl] <srcix>|all <dstix>|all\n";
    print STDERR "   show database records for given site indices\n";
    print STDERR "       -B        show both srcix -> dstix and dstix -> srcix\n";
    print STDERR "       -b        show bandwidth\n";
    print STDERR "       -d        show delay (the default)\n";
    print STDERR "       -l        show loss\n";
    print STDERR "       -n <num>  show only the last <num> records\n";
    print STDERR "       -S <time> show records no later than unix timestamp <time>\n\n";
    print STDERR "usage: showsamples.pl -e pid/eid\n";
    print STDERR "   show mapping of name->ix for all plab nodes in <pid>/<eid>\n";
    exit(1);
}
my ($srcix,$dstix) = @ARGV;
if ($srcix eq "all") {
    undef $srcix;
}
if ($dstix eq "all") {
    undef $dstix;
}

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
# XXX figure out how many pairs there are, and for each, who the
# corresponding planetlab node is.  Can probably get all this easier
# via XMLRPC...
#
if (defined($pid)) {
    my @nodelist = split('\s+', `$NLIST -m -e $pid,$eid`);
    chomp(@nodelist);
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
	    
	    print "$vnode ($pnode) is site $site_index\n";
	}
    }
    exit(0);
}

if (!$dolatency && !$dobw && !$doloss) {
    $dolatency = 1;
}
if ($starttime == 0) {
    $starttime = time();
}

get_plabinfo($srcix, $dstix);

exit(0);

#
# Grab data from DB.
#
sub get_plabinfo($$)
{
    my ($srcix,$dstix) = @_;

    my $fields;
    my $where2;
    if ($dolatency) {
	$fields = "latency";
	$where2 = "latency is not NULL";
    }
    if ($dobw) {
	if ($fields) {
	    $fields .= ",";
	    $where2 .= " or ";
	}
	$fields .= "bw";
	$where2 .= "bw is not NULL";
    }
    if ($doloss) {
	if (!$fields) {
	    $fields .= ",";
	    $where2 .= " or ";
	}
	$fields .= "loss";
	$where2 .= "loss is not NULL";
    }

    my $wherespec;
    if (defined($srcix)) {
	$wherespec = "(srcsite_idx='$srcix'";
	if (defined($dstix)) {
	    $wherespec .= " and dstsite_idx='$dstix'";
	    if ($bidir) {
		$wherespec .=
		    " or srcsite_idx='$dstix' and dstsite_idx='$srcix'";
	    }
	}
	$wherespec .= ")";
    } elsif (defined($dstix)) {
	$wherespec = "(dstsite_idx='$dstix')";
    }
    if (defined($wherespec)) {
	$wherespec .= " and ";
    }
    $wherespec .= "unixstamp <= $starttime ";

    print "$fields for ";
    if (defined($srcix)) {
	print "site $srcix ";
    } else {
	print "all sites ";
    }
    print "and ";
    if (defined($dstix)) {
	print "site $dstix";
    } else {
	print "all sites";
    }
    print ":\n";

    my $limit = "";
    if ($number > 0) {
	$limit = "limit $number";
    }

    my $query =
	"select srcsite_idx,dstsite_idx,$fields,unixstamp ".
	"from pair_data where $wherespec ".
	" and ($where2) ".
	"order by unixstamp desc,srcsite_idx,dstsite_idx $limit";
#    print "query is: $query\n";
    my $query_result = DBQueryFatal($query);
    if (!$query_result->numrows) {
	return;
    }

    printf "%-6s %-6s ", "src", "dst";
    if ($dolatency) {
	printf "%-6s ", "delay";
    }
    if ($dobw) {
	printf "%-6s ", "bw";
    }
    if ($doloss) {
	printf "%-4s ", "loss";
    }
    printf "%-25s\n", "date";

    my @vals;
    while (@vals = $query_result->fetchrow_array()) {
	printf "%-6s %-6s ", $vals[0], $vals[1];
	shift @vals;
	shift @vals;
	if ($dolatency) {
	    printf "%6.1f ", $vals[0];
	    shift @vals;
	}
	if ($dobw) {
	    printf "%6d ", $vals[0];
	    shift @vals;
	}
	if ($doloss) {
	    printf "%4.2f ", $vals[0];
	    shift @vals;
	}
	printf "%s\n", scalar(localtime($vals[0])); 
    }
}
