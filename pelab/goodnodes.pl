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
use lib '/usr/testbed/lib';
use libtbdb;
use libwanetmondb;

my $allnodeFile = "/proj/tbres/plab-reliable-list";
my $NLIST = "/usr/testbed/bin/node_list";
my $pprefix = "plab";
my $windowHrsDef = 6;

# Turn off line buffering on output
$| = 1;

sub usage {
        print "Usage: $0 [-e pid/eid] [-f blacklistfilename] [-t type]".
            "[-0 starttime] [-1 endtime] <numNodes>\n";
        return 1;
}
my ($pid, $eid);
my $blacklistfilename;
my $type = "";
my ($t0, $t1);
my %opt = ();
getopts("0:1:e:f:t:", \%opt);
if ($opt{e}) { ($pid,$eid) = split('/', $opt{e}); }
if ($opt{f}) { $blacklistfilename = $opt{f}; }
if ($opt{t}) { $type = $opt{t}; }
if ($opt{0}) { $t0 = $opt{0}; } else { $t0 = time()-$windowHrsDef*60*60; }
if ($opt{1}) { $t1 = $opt{1}; }
elsif($opt{0}) { $t1 = $t0+$windowHrsDef*60*60; }
else { $t1 = time(); }
if (@ARGV !=1) { exit &usage; }
my $numnodes = $ARGV[0];
my @allnodes = ();      #nodes to consider, in order of desirablility (?)
my %chosenBySite = ();  #indexed by siteidx, maps to plabxxx
my %nodeIds = ();       #indexed by plabxxx => (siteid, nodeid)
my %expnodes = ();  #nodes making up eid/pid
my %blacknodes = ();#nodes not allowed to be chosen (deleted from allnodes)
my %connMatrix = (); # {srcsite}{dstsite} => 1/0 mapping 
my %connRating = (); # site => rating value
my $allnodesIndex = 0;

#
# Get list of possible nodes
#
open FILE, "< $allnodeFile"
    or die "Can't open file";
my @allnodesinfo = <FILE>;
foreach my $nodeinfo (@allnodesinfo){
    my @fields = split " ",$nodeinfo;
    #we only want the plabxxx value (for now)
    push @allnodes, $fields[0];
    #print "$fields[0]\n";
}
close FILE;

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

#
# prototypes
sub addNew($);
sub checkConn($$);
sub isFullyConn($);
#
#


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
# build set of nodes to test for fully-connectedness.
#
for( $allnodesIndex=0; $allnodesIndex < scalar(@allnodes); $allnodesIndex++ ){
    if( !addNew($allnodesIndex) ){
    }
    if( scalar( keys %chosenBySite ) == $numnodes ){
#        print "got $numnodes nodes\n";
        last;
    }
}

#
#
# MAIN LOOP
#
#
my ($lowestRatedSite, $lowestRating) = ();
do{    
    ($lowestRatedSite, $lowestRating) = fullyConnTest();
#    print "lowestRatedSite=$lowestRatedSite, lowestRating=$lowestRating\n";
    if( !isFullyConn($lowestRating) ){
        #modify set
        # delete worst node
#        print "deleting $chosenBySite{$lowestRatedSite} ".
#            "with rating=$lowestRating\n";
        delete $chosenBySite{$lowestRatedSite};
        # add new node
        while( scalar(keys %chosenBySite) < $numnodes )
        {
            if( $allnodesIndex < scalar(@allnodes)-1 ){
                $allnodesIndex++;
                #addNew will add to chosenBySite if good N.
                addNew($allnodesIndex); 
            }else{
                die "COULD NOT MAKE FULLY CONNECTED SET!\n";
            }
        }
    }
}while( !isFullyConn($lowestRating) ); #test if fully-connected

print "FULLY CONNECTED (n=$numnodes)!\n";
print "allnodeindex=$allnodesIndex of ".scalar(@allnodes)."\n";

#printChosenNodes();

print "FINAL SET:\n";
foreach my $siteidx (keys %chosenBySite){
    print "$chosenBySite{$siteidx}\n";
}



#print "before FCT\n";
#fullyConnTest();


###############################################################################

sub isFullyConn($){
    my ($lowestRating) = @_;
    if( $lowestRating == ($numnodes-1)*2 ){
        return 1;
    }else{
        return 0;
    }
}


#
# Tests the nodes listed in %chosenBySite for fully-connectedness
# Returns the siteid of the lowest rated (connectedness) node, and its rating
#
sub fullyConnTest{

    %connRating = ();
 
    my @sites = keys %chosenBySite;
#    print "nodes=@nodes\n";

    for( my $i=0; $i<scalar(@sites)-1; $i++ ){
        my $srcsite = $sites[$i];

        for( my $j=$i+1; $j<scalar(@sites); $j++ ){
            my $dstsite = $sites[$j];

            if( !defined($connMatrix{$srcsite}{$dstsite}) ){ 
                my ($latConn,$bwConnF,$bwConnB)=checkConn($srcsite,$dstsite);
#                print "$srcsite => $dstsite, ($latConn,$bwConnF,$bwConnB)\n";

                $connMatrix{$srcsite}{$dstsite} = $latConn + $bwConnF;
                $connMatrix{$dstsite}{$srcsite} = $latConn + $bwConnB;
            }

            #a node's rating is defined as the number of successful
            # bw tests it is the dst for, plus the number of latency
            # tests it is involved in.
            #
            $connRating{$dstsite} += $connMatrix{$srcsite}{$dstsite};
            $connRating{$srcsite} += $connMatrix{$dstsite}{$srcsite};
        }        
    }


    my $lowestRatedSite;
    my $lowestRating;
    #
    # find lowest rating
    #
    foreach my $site (keys %chosenBySite){
        if( !defined $lowestRating ){
            $lowestRating = $connRating{$site};
            $lowestRatedSite = $site;
            next;
        }
        
        if( $connRating{$site} < $lowestRating ){
            $lowestRating = $connRating{$site};
            $lowestRatedSite = $site;
#            print "new lowest rating $lowestRating from $node\n";
        }
    }

#    print "lowestRatedSite=$lowestRatedSite, $lowestRating\n";
    return ($lowestRatedSite, $lowestRating);
}



#
# add the site of given node at given index to result set of "%chosenBySite"
# if data exists for that node
#  returns 1 if successful, 0 if fail.
#  fail: 
#    - no results
#    - already a node under its siteix in %chosenBySite
#
sub addNew($){
    my ($index) = @_;
    my $nodeid = $allnodes[$index];
    my $f_valid = 1;

#    print "adding index $index=$nodeid of ".scalar(@allnodes)-1."\n";;

    #get node and site IDxs
    my ($siteidx,$nodeidx) = getNodeIDs($nodeid);

    #** check if this node satisfies the constraints **

    my $qstr = "select latency from pair_data ".
                     "force index (unixstamp) ".
                     "where latency is not NULL and ".
                     "dstsite_idx=$siteidx and ".
                     "unixstamp > $t0 and ".
                     "unixstamp < $t1 ".
                     "limit 1";
    my @results = getRows($qstr);
    if( !scalar(@results) ){
#        warn("No latency results from $nodeid\n");
        return 0;
    }

    if( $f_valid == 1 ){
        #add node to set if one doesn't already exist from its site
        if( !defined $chosenBySite{$siteidx} ){
            $chosenBySite{$siteidx} = $nodeid;
        }else{
            #already exists
            $f_valid = 0;
        }
#        print "added $nodeid / $siteidx,$nodeidx\n";
    }

    return $f_valid;    
}


#
# param: plabxx, output: [siteidx, nodeidx]
#
sub getNodeIDs($){
    my ($nodeid) = @_;

    #if it's already added to hash, don't look it up again
    if( defined $nodeIds{$nodeid} ){
        return @{$nodeIds{$nodeid}};
    }

    # Grab the site and node index.
    my $qstr = "select * from site_mapping ".
               "where node_id='$nodeid'";
    my @results = getRows($qstr);

    if( !scalar(@results) ){
        die("Could not map $nodeid to its site or node index!\n");
    }

    my $site_idx = $results[0]->{site_idx};
    my $node_idx = $results[0]->{node_idx};

    $nodeIds{$nodeid} = [$site_idx,$node_idx];
#    print "added to nodeIds: $nodeid => @{$nodeIds{$nodeid}}\n";
    return ($site_idx,$node_idx);
}

#
# returns an array of values indicating the connectedness of the passed path
# - latConn is 0 or 1 (1 if either forward or backward rtt exists)
# - bwConnF is 0, 1 / for bw from src to dst
# - bwConnB is 0, 1 / for bw from dst to src
#
sub checkConn($$){
    my ($srcsite, $dstsite) = @_;
    my ($latConn, $bwConnF, $bwConnB) = (0,0,0);
    my $latTestStr= "> 0";
#    my $latTestStr= "is not null";
    my $bwTestStr = "> 0";
    
    my $qstr = "select * from pair_data ".
                     "force index (unixstamp) ".
                     "where (latency $latTestStr  and ".
                     "unixstamp > $t0 and ".
                     "unixstamp < $t1) and ".
                     "((srcsite_idx=$srcsite and ".
                     "dstsite_idx=$dstsite) or ".
                     "(srcsite_idx=$dstsite and ".
                     "dstsite_idx=$srcsite)) ".
                     "limit 1";
    my @results = getRows($qstr);
#    print "getRows (latency) finished for query\n$qstr\n";
    if( !scalar(@results) ){
    }else{
        $latConn = 1;
    }
    
    $qstr = "select * from pair_data ".
        "force index (unixstamp) ".
            "where bw $bwTestStr and ".
            "unixstamp > $t0 and ".
            "unixstamp < $t1 and ".
            "srcsite_idx=$srcsite and ".
            "dstsite_idx=$dstsite ".
            "limit 1";
    @results = getRows($qstr);
    if( !scalar(@results) ){
    }else{
        $bwConnF = 1;
    }

    $qstr = "select * from pair_data ".
        "force index (unixstamp) ".
            "where bw $bwTestStr and ".
            "unixstamp > $t0 and ".
            "unixstamp < $t1 and ".
            "srcsite_idx=$dstsite and ".
            "dstsite_idx=$srcsite ".
            "limit 1";
    @results = getRows($qstr);
    if( !scalar(@results) ){
    }else{
        $bwConnB = 1;
    }
#    print "($latConn, $bwConnF, $bwConnB)\n";
    return ($latConn, $bwConnF, $bwConnB);
}



sub printChosenNodes{

    my @nodes = values %chosenBySite;

    for( my $i=0; $i<scalar(@nodes)-1; $i++ ){
        my $src = $nodes[$i];
        for( my $j=$i+1; $j<scalar(@nodes); $j++ ){
            my $dst = $nodes[$j];
            print "$src=>$dst :: $connMatrix{$src}{$dst} ".
                "and $connMatrix{$dst}{$src}\n";
        }
    }

    foreach my $node (@nodes){
        print "$node: $connRating{$node}\n";
    }
}

