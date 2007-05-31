#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# - Find a set of fully connected nodes from the list of nodes at
#     boss:/proj/tbres/plab-reliable-list
# - Size of desired set given as a parameter
# - + The sites containing the nodes must have both latency and bw 
#     data in the past XXX hours (XXX<24)
#   + The set will not contain more than one node from a site
# - Run this on ops.


#
# TODO:
# - add options for time window
#


#use diagnostics;
use strict;
use English;
use Getopt::Std;
use lib '/usr/testbed/devel/johnsond/lib';
use libxmlrpc;

my $allnodeFile = "/share/planetlab/reliable_nodes";
my $NLIST = "/usr/testbed/bin/node_list";
my $pprefix = "plab";
my $windowHrsDef = 6;

# Turn off line buffering on output
$| = 1;

sub usage {
        print "Usage: $0 [-e pid/eid] [-f blacklistfilename] [-t type] [-v] ".
            "[-0 starttime] [-1 endtime] <numNodes>\n";
        return 1;
}
my ($pid, $eid);
my $blacklistfilename;
my $type = "";
my $verbose = 0;
my ($t0, $t1);
my %opt = ();
getopts("0:1:e:f:t:v", \%opt);
if ($opt{e}) {
    ($pid,$eid) = split('/', $opt{e});
} else {
    $pid = "tbres"; $eid = "pelabbgmon";
}
if ($opt{f}) { $blacklistfilename = $opt{f}; }
if ($opt{t}) { $type = $opt{t}; }
if ($opt{v}) { $verbose = 1; }
if ($opt{0}) { $t0 = $opt{0}; } else { $t0 = time()-$windowHrsDef*60*60; }
if ($opt{1}) { $t1 = $opt{1}; }
elsif($opt{0}) { $t1 = $t0+$windowHrsDef*60*60; }
else { $t1 = time(); }
if (@ARGV !=1) { exit &usage; }

#
# These are globals
#
my $numnodes = $ARGV[0];
my @allnodes = ();      #nodes to consider, in order of desirablility (?)
my %expnodes = ();  #nodes making up eid/pid
my %blacknodes = ();#nodes not allowed to be chosen (deleted from allnodes)

#
# Get list of possible nodes
#
open FILE, "< $allnodeFile"
    or die "Can't open file";
my @allnodesinfo = <FILE>;
foreach my $nodeinfo (@allnodesinfo){
    my @fields = split /\s+/, $nodeinfo;
    #we only want the plabxxx value (for now)
    push @allnodes, $fields[0];
    #print "$fields[0]\n";
}
close FILE;
if ($verbose) {
    print "Read " . scalar(@allnodes) . " nodes from reliable nodes file\n";
}

#
# get list of blacklisted nodes
#
if( defined $blacklistfilename ){
    open FILE, "< $blacklistfilename"
        or die "Can't open file";
    my @blacklist = <FILE>;
    chomp @blacklist;
    foreach my $node (@blacklist){
        $blacknodes{$node} = 1;
        print "blacknode: $node\n";
    }
    close FILE;
}


###############################################################################
# Keep nodes given in pid/eid in hash for quick access.
if( defined($pid) && defined($eid) ){
#    print "reading $pid/$eid nodes\n";
    #add exp nodes to a hash
    my @expnodelist = split('\s+', `$NLIST -H -e $pid,$eid`);
    chomp(@expnodelist);
    foreach my $node (@expnodelist) {
        if ($node =~ /^(${pprefix}\d+)=([\w,]*)$/) {
            my $pnode = $1;
            my $types = $2;
            my @types = split(/,/,$types);
            if ($type && ! grep(/^$type$/,@types)) {
                #print "Skipping $pnode ($type,$types)\n";
                next;
            }
#            print "$vnode ($pnode)\n";
            $expnodes{$pnode} = 1;  #set this node
        }
    }
    #delete nodes from allnodes not found in given experiment
    for( my $i=0; $i < scalar(@allnodes); $i++ ){
        if( !defined $expnodes{$allnodes[$i]} ||
            defined $blacknodes{$allnodes[$i]})
        {
#            print "removing $allnodes[$i] from set\n";
            splice( @allnodes, $i, 1 );
            $i--;
        }else{
#            print "$allnodes[$i]\n";
        }
    }
#    chomp @allnodes;
}else{
    #remove blacknodes
    for( my $i=0; $i < scalar(@allnodes); $i++ ){
        if( defined $blacknodes{$allnodes[$i]})
        {
#            print "removing $allnodes[$i] from set\n";
            splice( @allnodes, $i, 1 );
            $i--;
        }else{
#            print "$allnodes[$i]\n";
        }
    }
}

#print "allnodes[".scalar(@allnodes-1]=$allnodes

#print "%%%%%%\n@allnodes\n%%%%%%%%%%";


#
# Send candidate node list to flexlab xmlrpc server.
#

my ($DEF_HOST,$DEF_PORT,$DEF_URL) = ('ops.emulab.net','3993','/');

my $xurl = "http://${DEF_HOST}:${DEF_PORT}${DEF_URL}";
my $xargs = { 'size' => $numnodes,
              'nodefilter' => \@allnodes,
              'filtertype' => 1 };
my $respref = libxmlrpc::CallMethodHTTP($xurl,'flexlab.getFullyConnectedSet',
    $xargs);

print "rr = $respref\n";

if (!defined($respref)) {
    print STDERR "ERROR: did not get response from server!\n";
    exit(3);
}

my %resp = %$respref;

foreach my $k (keys(%resp)) {
    print "rk = $k, rv = $resp{$k}\n";
}

if (exists($resp{'httpcode'}) && $resp{'httpcode'} != 400) {
    print STDERR "ERROR: http code " . $resp{'httpcode'} . "; http msg '" . 
        $resp{'httpmsg'} . "'\n";
    exit(4);
}

if ($resp{'code'} ne 0) {
    print STDERR "ERROR: xmlrpc code " . $resp{'code'} . ", fault msg '" . 
	$resp{'output'} . "'\n";
    exit(5);
}

#
# Must be a success.
#
print "" . join(',',@{$resp{'value'}}). "\n";
exit(0);
