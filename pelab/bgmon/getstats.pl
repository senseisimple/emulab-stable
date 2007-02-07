#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
#TODO: 
# - analyze for nodes which have trouble (bgmon.pl dies, for ex.)
# - fix stupid command line needed to run
#   run with: >>> perl -mDescriptive -I./Statistics getstats.pl

#use lib './Statistics/Basic/';
#use lib qw(modules);
#use lib qw(./modules/Statistics);

use lib Statistics::Descriptive;
use lib Statistics::Descriptive::Full;
use libtbdb;
use Getopt::Std;
use strict;

my $nodesep = "_";

my $tmplink = "285-1".$nodesep."81-1";

#from pairwise (nodes3.subset)
#my $tmplink = "198-2".$nodesep."226-2"; #lat
#my $tmplink = "195-1".$nodesep."224-1#"; #lat
#my $tmplink = "266-3".$nodesep."126-1"; #lat
#my $tmplink = "20-2".$nodesep."218-1"; #lat

#my $tmplink = "194-1".$nodesep."24-2"; #lat
#my $tmplink = "194-1".$nodesep."162-1"; #lat
#my $tmplink = "194-1".$nodesep."94-2";
#my $tmplink = "249-1".$nodesep."11-1";
#my $tmplink = "122-3".$nodesep."136-2";

#my $tmplink = "266-1".$nodesep."211-3";
#my $tmplink = "253-4".$nodesep."266-1";

#my $tmplink = "252-12".$nodesep."253-4";
#my $tmplink = "264-2".$nodesep."270-3";
#my $tmplink = "177-2".$nodesep."273-2"; 
#my $tmplink = "241-1".$nodesep."225-1"; 

my $starttime = 0;
my $endtime = time();
my $histbins = 20;  #number of bins for histogram
my $testtype;
my %data;  #hash of {srcsite-srcnode-dstsite-dstnode} to hashes by unixtime
my %hist;  #histogram of data
my $mean;
my $median;
my $stddev;
my @percentiles;  #values of the 95% boundaries
my @outages;  #start=>end times of the outage
my @changepoints;

my $DBNAME = "nodesamples";
my $DBUSER = "gebhardt";
my ($DB_data, $DB_sitemap);
TBDBConnect($DBNAME,$DBUSER);

sub usage
{
    warn "Usage: $0 <sec in past to start> <testtype> [-q quiet] [-h help]".
	"[srcplabnode] [dstplabnode]\n";
    return 1;
}

my %opt = ();
getopts("q",\%opt);
#if( $opt{h} ) { exit &usage; };

if( @ARGV < 2 ) { exit &usage; }
$starttime = $endtime - $ARGV[0];
$testtype = $ARGV[1];
print "Looking at samples from t=$starttime to $endtime of type $testtype\n";

if( defined $ARGV[2] && defined $ARGV[3] ){
    $tmplink = getidx($ARGV[2]).$nodesep.getidx($ARGV[3]);
}
print "link = $tmplink\n";

###############################################################################
#
# get data from database and store in a hash of hashes (indexed by unixtime)
#
my ($src, $dst) = split "$nodesep",$tmplink;
my $sth = DBQuery(
		  "SELECT * FROM pair_data WHERE ".
		  "unixstamp > $starttime and ".
		  "unixstamp < $endtime and ".
		  "srcsite_idx = ".${[split "-",$src]}[0] ." and ".
		  "srcnode_idx = ".${[split "-",$src]}[1] ." and ".
		  "dstsite_idx = ".${[split "-",$dst]}[0] ." and ".
		  "dstnode_idx = ".${[split "-",$dst]}[1] ." and ".
		  "$testtype != 0;" );

while( my $hr = $sth->fetchrow_hashref() ){
    my %row = %{$hr};
#    print "adding $row{$testtype}\n";
#    push @{$data{"$row{srcsite_idx}-$row{srcnode_idx}".
#		     "-$row{dstsite_idx}-$row{dstnode_idx}"}},
#	$row{unixstamp};
#    push @{$data{"$row{srcsite_idx}-$row{srcnode_idx}".
#		     "-$row{dstsite_idx}-$row{dstnode_idx}"}},
#	$row{$testtype};
    ${$data{"$row{srcsite_idx}-$row{srcnode_idx}$nodesep".
		"$row{dstsite_idx}-$row{dstnode_idx}"}}{$row{unixstamp}} =
		$row{$testtype};
}

#save a link data to a file
findstats($tmplink);
savelinkdatatofile($tmplink);
savegnuplotscript($tmplink);
if( !$opt{q} ){
    rungnuplot($tmplink);
}

###############################################################################

sub findstats($)
{
    my ($link) = @_;
    my $stats = Statistics::Descriptive::Full->new();
    $stats->add_data( values %{$data{$link}} );
    $mean = $stats->mean();
    $median = $stats->median();
    $stddev = $stats->standard_deviation();
    print "mean    = $mean\n";
    print "Median  = $median\n";
    print "std dev = $stddev\n";

    $percentiles[0] = $stats->percentile(5);
    $percentiles[1] = $stats->percentile(95);
    print "5%      = $percentiles[0]\n";
    print "95%      = $percentiles[1]\n";


    #*** find outage periods
#    my $percentile_theshold = 96;
    my $meanThreshold = 5;  #periods Xmeans away from trimmed mean are outage
    my $perMean;
#    my $perStddev;
    my @times = sort {$a <=> $b} keys %{$data{$link}};
    my $per_stats = Statistics::Descriptive::Full->new();
    for( my $i=0; $i<@times-1; $i++ ){
	$per_stats->add_data( $times[$i+1] - $times[$i] );
    }
    $perMean = $per_stats->trimmed_mean( 0, 0.5 );
    for( my $i=0; $i<@times-1; $i++ ){
	my $diff = $times[$i+1] - $times[$i];
	if( $diff > $meanThreshold*$perMean )
	{
	    print "found an outage from ".
		$times[$i].
		" to ".
		$times[$i+1] . "\n";
	    push @outages, $times[$i];
	    push @outages, $times[$i+1];
	}
    }



    #get histogram
    %hist = $stats->frequency_distribution($histbins);


    #*** find change points

}


#
# save data to files
#    USES GLOBAL: %data and %hist
#
sub savelinkdatatofile($)
{
    my ($link) = @_;
    my @sortedtimes = sort {$a <=> $b} keys %{$data{$link}};
    my $filename = makedatafilename($link, $testtype);
    open FILE, "> $filename" or die "can't open file $filename";
    print "\nsaving LINK $link\n";
    my @sites = split "$nodesep",$link;
    print getnodeid($sites[0]) . "  to  " . getnodeid($sites[1])."\n";
    foreach my $time ( @sortedtimes ){
	my $adjtime = convertraw($time);
#	print FILE "$time   $adjtime   $data{$link}{$time}\n";
	print FILE "$time\t$adjtime\t$data{$link}{$time}\n";
    }
    close FILE;

    #save histogram data
    $filename = makehistfilename($link, $testtype);
    open FILE, "> $filename" or die "can't open file $filename";
    #TODO: save histogram data
    my @histx = sort {$a <=> $b} keys %hist;
    foreach (@histx){
	print FILE "$_   $hist{$_}\n";
    }
    close FILE;
}


###############################################################################

sub savegnuplotscript($)
{
    my ($link) = @_;
    my $filename = makegnuplotscriptfilename($link,$testtype);
    my @sites = split "$nodesep",$link;
    use Time::Format qw{%time};
    my $timeformat = 'Mon d, hh:mm:ss';
    my $script = 
	"set autoscale\n".
#	"set title \"$testtype between $sites[0] and $sites[1]".
	"set title \"$testtype between ".
	getsitename($sites[0])." and ".getsitename($sites[1]).
	" from $time{$timeformat,$starttime} to".
	" $time{$timeformat, $endtime}\"\n".
	"set xlabel \"time of sample (hh:mm:ss)\"\n".
	"set ylabel \"". ($testtype eq "bw" ? "kbps" :
			  ($testtype eq "latency" ? "milliseconds" : 
			   "unknown") ) .
	"\"\n".
	"set xdata time \n".
	'set timefmt "%s"'."\n".
	'set format x "%b%d\n%H:%M:%S"'."\n".
	makegnuplotoutages().
	"plot \"".makedatafilename($link,$testtype)."\"".
	" using 1:3".
	" title \'data points\'".
	makegnuplotstats().
	"\n".
	"pause -1\n".
	"set terminal postscript landscape enhanced mono lw 2 \"Helvetica\" ".
	"10\n".
	"set output \'".makedatafilename($link,$testtype).".ps\'\n".
	"replot\n".
	"set term pop\n"
#	"set terminal x11\n"
#	"set size 1,1\n"
	;

    open FILE, "> $filename" or die "can't open file $filename";
    print FILE $script;
    close FILE;

    #write histogram script
    $script = 
	"set autoscale\n".
	"set title \"$testtype histogram between ".
	getsitename($sites[0])." and ".getsitename($sites[1]).
	" from $time{$timeformat,$starttime} to".
	" $time{$timeformat, $endtime}\"\n".
	"set xlabel \"latency (ms) or bandwidth (kbps)\"\n".
	"set ylabel \"number of samples\"\n".
	"plot \"".makehistfilename($link,$testtype)."\"".
	" using 1:2 with boxes".
	"\n".
	"pause -1\n".
	"set terminal postscript landscape enhanced mono lw 2 \"Helvetica\" ".
	"10\n".
	"set output \'".makehistfilename($link,$testtype).".ps\'\n".
	"replot\n".
	"set term pop\n"
	;

    $filename = makegnuplotscripthistfilename($link,$testtype);
    open FILE, "> $filename" or die "can't open file $filename";
    print FILE $script;
    close FILE;
}

sub makegnuplotoutages()
{
    my $s = "";
    for( my $i=0; $i<@outages; $i++ ){
	$s .=
	    "set arrow from \"$outages[$i]\"".
	    ", graph 0 to \"$outages[$i]\", graph 1 nohead lt 0\n";
    }
    return $s;
}

sub makegnuplotstats()
{
    my $s = "";
    if( defined $median ){
	$s .= ", $median title \'Median\'";
    }
    if( defined $percentiles[0] ){
	$s .= ", $percentiles[0] title \'5% range\'";
    }
    if( defined $percentiles[1] ){
	$s .= ", $percentiles[1] title \'95% range\'";
    }
#    if( defined $stddev ){
#	my $upper = $mean + 2*$stddev;
#	my $lower = $mean - 2*$stddev;
#	$s .= ", $upper title \'2*Std Dev upper\'";
#	$s .= ", $lower title \'2*Std Dev lower\'";
#    }
    return $s;
}


sub makedatafilename($$)
{
    my ($link, $testtype) = @_;
    my @sites = split "$nodesep",$link;
    $link = getsitename($sites[0])."_".getsitename($sites[1]);
    return "$link.$testtype.dat";
}

sub makehistfilename($$)
{
    my ($link, $testtype) = @_;
    my @sites = split "$nodesep",$link;
    $link = getsitename($sites[0])."_".getsitename($sites[1]);
    return "$link.$testtype.hist";
}

sub makegnuplotscripthistfilename($$)
{
    my ($link, $testtype) = @_;
    my @sites = split "$nodesep",$link;
    $link = getsitename($sites[0])."_".getsitename($sites[1]);
    return "$link.$testtype.hist.gnuplot";
}

sub makegnuplotscriptfilename($$)
{
    my ($link, $testtype) = @_;
    my @sites = split "$nodesep",$link;
    $link = getsitename($sites[0])."_".getsitename($sites[1]);
    return "$link.$testtype.gnuplot";
}

sub rungnuplot($)
{
    my ($link) = @_;
    my $scriptfilename = makegnuplotscriptfilename($link, $testtype);
    `gnuplot $scriptfilename`;
    $scriptfilename = makegnuplotscripthistfilename($link, $testtype);
    `gnuplot $scriptfilename`;
}

sub convertraw($)
{
    my ($unixtime) = @_;
    use Time::Format qw{%time};
#    return ($unixtime - $endtime);
    return $time{'mon_dd_hh:mm:ss',$unixtime};
}


#
# input idx parameter must be of the format site_idx-node_idx 
#
sub getsitename($)
{
    my ($idx) = @_;
    my ($site_idx, $node_idx) = @{[split "-",$idx]};
    my $sth = DBQuery("select site_name from site_mapping where site_idx=".
		      "$site_idx;");
    my %row = %{ $sth->fetchrow_hashref() };
    return $row{"site_name"};
}

sub getnodeid($)
{
    my ($idx) = @_;
    my ($site_idx, $node_idx) = @{[split "-",$idx]};
    my $sth = DBQuery("select node_id from site_mapping where site_idx=".
		      "$site_idx and node_idx=$node_idx;");
    my %row = %{ $sth->fetchrow_hashref() };
    return $row{"node_id"};
}

sub getidx($)
{
    my ($nodeid) = @_;
    my ($site_idx, $node_idx);
    my $sth = DBQuery("select site_idx, node_idx from site_mapping ".
		      "where node_id=".
		      "\"$nodeid\";");
    my %row = %{ $sth->fetchrow_hashref() };
    $site_idx = $row{"site_idx"};
    $node_idx = $row{"node_idx"};
    my $ret =$site_idx."-".$node_idx;
    return $ret;
}

