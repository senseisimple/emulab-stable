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
# - + Nodes must have both latency and bw data in the past XXX hours (XXX<24)
#   + The set will not contain more than one node from a site
# - Run this on ops.

#use diagnostics;
use strict;
use Getopt::Std;
use lib '/usr/testbed/lib';
use libtbdb;


my $pastHourWindow = 6;
my $allnodeFile = "/proj/tbres/plab-reliable-list";
my $PWDFILE = "/usr/testbed/etc/pelabdb.pwd";
my $DBNAME  = "pelab";
my $DBUSER  = "pelab";
my $NLIST = "/usr/testbed/bin/node_list";
my $pprefix = "node-";
#
# Turn off line buffering on output
#
$| = 1;
sub usage {
        print "Usage: $0 [-e pid/eid] <numNodes>\n";
        return 1;
}
my ($pid, $eid);
my %opt = ();
if (! getopts("e:", \%opt)) { exit &usage; }
if ($opt{e}) { ($pid,$eid) = split('/', $opt{e}); }
if (@ARGV !=1) { exit &usage; }
my $numnodes = $ARGV[0];
my @allnodes = ();    #nodes to consider, in order of desirablility (?)
my %chosenBySite = ();  #indexed by siteidx, maps to plabxxx
my %nodeIds = ();       #indexed by plabxxx => (siteid, nodeid)
my $earliesttime = time() - $pastHourWindow*60*60;
my %expnodes = ();  #nodes making up eid/pid
my %connMatrix = (); # {srcplabxxx}{dstplabxxx} => 1/0 mapping 
my %connRating = (); # plabxxx => rating value
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



#
# Get DB password and connect.
#
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
    my @expnodelist = split('\s+', `$NLIST -m -e $pid,$eid`);
    chomp(@expnodelist);
    foreach my $mapping (@expnodelist) {
        if ($mapping =~ /^(${pprefix}[\d]+)=([\w]*)$/) {
            my $vnode = $1;
            my $pnode = $2;
#            print "$vnode ($pnode)\n";
            $expnodes{$pnode} = 1;  #set this node
        }
    }
    #delete nodes from allnodes not found in given experiment
    for( my $i=0; $i < scalar(@allnodes); $i++ ){
        if( !defined $expnodes{$allnodes[$i]} ){
#            print "removing $allnodes[$i] from set\n";
            splice( @allnodes, $i, 1 );
            $i--;
        }else{
#            print "$allnodes[$i]\n";
        }
    }
}


#
# build set of nodes to test for fully-connectedness.
#
for( $allnodesIndex=0; $allnodesIndex < scalar(@allnodes); $allnodesIndex++ ){
    if( !addNew($allnodesIndex) ){
    }
    if( scalar( keys %chosenBySite ) == $numnodes ){
        print "got $numnodes nodes\n";
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
    if( !isFullyConn($lowestRating) ){
        #modify set
        # delete worst node
        print "deleting $chosenBySite{$lowestRatedSite} ".
            "with rating=$lowestRating\n";
        delete $chosenBySite{$lowestRatedSite};
        # add new node
        while( scalar(keys %chosenBySite) < $numnodes )
        {
            if( $allnodesIndex < scalar(@allnodes) ){
                $allnodesIndex++;
            }else{
                die "COULD NOT MAKE FULLY CONNECTED SET!\n";
            }
            addNew($allnodesIndex); #addNew will add to chosenBySite if good N.
        }
    }
}while( !isFullyConn($lowestRating) ); #test if fully-connected

print "FULLY CONNECTED (n=$numnodes)!\n";
print "allnodeindex=$allnodesIndex of ".scalar(@allnodes)."\n";


#TODO: print out Fullyconnected set.
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
 
    foreach my $srcsiteid (keys %chosenBySite){
        my $srcnode = $chosenBySite{$srcsiteid};
        foreach my $dstsiteid (keys %chosenBySite){
            next if( $srcsiteid == $dstsiteid );
            my $dstnode = $chosenBySite{$dstsiteid};

            if( !defined($connMatrix{$srcnode}{$dstnode}) ){ 
                my ($latConn, $bwConnF, $bwConnB) 
                    = checkConn($srcnode, $dstnode);
#                print "$srckey => $dstkey, lat=$latConn, ".
#                    "bwF=$bwConnF, bwB=$bwConnB\n";
                print "$srcnode => $dstnode, ($latConn,$bwConnF,$bwConnB)\n";

                $connMatrix{$srcnode}{$dstnode} = $latConn + $bwConnF;
                $connMatrix{$dstnode}{$srcnode} = $latConn + $bwConnB;

                #
                #a node's rating is defined as the number of successful
                # bw tests it is the dst for, plus the number of latency
                # tests it is involved in.
                #
                $connRating{$dstnode} += $latConn + $bwConnF;
                $connRating{$srcnode} += $latConn + $bwConnB;
            }
        }
    }


    my $lowestRatedSite;
    my $lowestRating;
    #
    # find lowest rating
    #
    foreach my $site (keys %chosenBySite){
#        print "looking at site $site\n";
        my $node = $chosenBySite{$site};
        if( !defined $lowestRating ){
            $lowestRating = $connRating{$node};
            $lowestRatedSite = $site;
            next;
        }
        
        if( $connRating{$node} < $lowestRating ){
            $lowestRating = $connRating{$node};
            $lowestRatedSite = $site;
            print "new lowest rating $lowestRating from $node\n";
        }
    }

#    print "lowestRatedSite=$lowestRatedSite, $lowestRating\n";
    return ($lowestRatedSite, $lowestRating);
}




#
# add node at given index to potential result set of "%chosenBySite"
# if data exists for that node
#  returns 1 if successful, 0 if fail.
#  fail: 
#    - no results
#    - already a node under its siteix
#
sub addNew($){
    my ($index) = @_;
    my $nodeid = $allnodes[$index];
    my $f_valid = 1;
    
    #get node and site IDxs
    my ($siteidx,$nodeidx) = getNodeIDs($nodeid);

    #check if this node satisfies the constraints

    # TODO: conditonally, check for valid results
    #check for existing data
    my $query_result =
        DBQueryFatal("select latency from pair_data ".
                     "where latency is not NULL and ".
                     "dstsite_idx=$siteidx and ".
                     "dstnode_idx=$nodeidx and ".
                     "unixstamp > $earliesttime ".
                     "limit 1");

    if (! $query_result->numrows) {
#        warn("No latency results from $nodeid\n");
        return 0;
    }
    my @latencies = $query_result->fetchrow_array();
    
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

    # Grab the site index.
    my $query_result =
        DBQueryFatal("select site_idx from site_mapping ".
                     "where node_id='$nodeid'");

    if (! $query_result->numrows) {
        die("Could not map $nodeid to its site index!\n");
    }
    my ($site_idx) = $query_result->fetchrow_array();

    # Grab the node index.
    $query_result =
        DBQueryFatal("select node_idx from site_mapping ".
                     "where node_id='$nodeid'");

    if (! $query_result->numrows) {
        die("Could not map $nodeid to its node index!\n");
    }
    my ($node_idx) = $query_result->fetchrow_array();

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
    my ($srcnode, $dstnode)    = @_;
    my ($srcsite, $srcnodeidx) = @{$nodeIds{$srcnode}};
    my ($dstsite,$dstnodeidx)  = @{$nodeIds{$dstnode}};
    my ($latConn, $bwConnF, $bwConnB) = (0,0,0);
    my $latTestStr= "> 0";
#    my $latTestStr= "is not null";
    my $bwTestStr = "> 0";
    
    my $qstr = "select * from pair_data ".
                     "where (latency $latTestStr  and ".
                     "unixstamp > $earliesttime) and ".                     
                     "((srcsite_idx=$srcsite and ".
                     "srcnode_idx=$srcnodeidx and ".
                     "dstsite_idx=$dstsite and ".
                     "dstnode_idx=$dstnodeidx) or ".
                     "(srcsite_idx=$dstsite and ".
                     "srcnode_idx=$dstnodeidx and ".
                     "dstsite_idx=$srcsite and ".
                     "dstnode_idx=$srcnodeidx)) ".
                     "limit 1";

    my $query_result =
        DBQueryFatal($qstr );
#    print $qstr."\n\n";
    if (! $query_result->numrows) {
#        warn("No latency results from $nodeid\n");
    }else{
        $latConn = 1;
    }
    
    $qstr = "select * from pair_data ".
            "where bw $bwTestStr and ".
            "unixstamp > $earliesttime and ".                     
            "srcsite_idx=$srcsite and ".
            "srcnode_idx=$srcnodeidx and ".
            "dstsite_idx=$dstsite and ".
            "dstnode_idx=$dstnodeidx ".                     
            "limit 1";
#    print $qstr."\n\n";
    $query_result =
        DBQueryFatal($qstr);
    if (! $query_result->numrows )
    {
    }else{
        $bwConnF = 1;
    }

    $query_result =
        DBQueryFatal("select * from pair_data ".
                     "where bw $bwTestStr and ".
                     "unixstamp > $earliesttime and ".                     
                     "srcsite_idx=$dstsite and ".
                     "srcnode_idx=$dstnodeidx and ".
                     "dstsite_idx=$srcsite and ".
                     "dstnode_idx=$srcnodeidx ".                     
                     "limit 1"
                     );
    if (! $query_result->numrows) {
    }else{
        $bwConnB = 1;
    }

    
    return ($latConn, $bwConnF, $bwConnB);
}

