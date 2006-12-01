#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
#TODO: 
# - analyze for nodes which have trouble (bgmon.pl dies, for ex.)
#


#run with: perl -mDescriptive -I./Statistics getstats.pl

#use lib './Statistics/Basic/';
#use lib './Statistics/';
use lib Statistics::Descriptive;
use strict;

my @data1 = (1, 1, 2, 2, 3, 10, 11, 10, 9, 2, 3, 2);
my @times1;
$times1[0] = 0;
for( my $i=1; $i<@data1; $i++){
    push @times1,$times1[$i-1]+1;
    #artificially make an outage
    if( $i  == 4 ){
	$times1[$i] += 10;
    }
    if( $i  == 6 ){
	$times1[$i] += 15;
    }
}

my $stat = Statistics::Descriptive::Sparse->new();

$stat->add_data(@data1);
my $MEAN = $stat->mean();
my $STD_DEV = $stat->standard_deviation();
print "mean    = $MEAN\n";
print "std dev = $STD_DEV\n";



printOutliers($MEAN, $STD_DEV);
printOutages();



#
#seach for outliers
#
sub printOutliers($$)
{
    my ($mean, $std_dev) = @_;
    my $i = 0;
    foreach my $data (@data1) {
	if( abs($data-$mean) > 2*$std_dev ){
	    print "outlier $data at t=$times1[$i]\n";
	}
	$i++;
    }
}


#
# print periods where there are no samples
#
sub printOutages()
{
    my $percentile_theshold = .9;
    my $perMean;
    my $perStddev;
    my $per_stat = Statistics::Descriptive::Full->new();
    for( my $i=0; $i<@times1-1; $i++ ){
	$per_stat->add_data( $times1[$i+1] - $times1[$i] );
    }
    
    for( my $i=1; $i<@times1; $i++ ){
	if( $per_stat->percentile($times1[$i] - $times1[$i-1])
	    > $percentile_theshold )
	{
	    print "found an outage from $times1[$i-1] to $times1[$i]\n";
	}
    }
    

#    $perMean   = $per_stat->mean();
#    $perStddev = $per_stat->standard_deviation();
#    print "times : @times1\n";
#    print "mean of sample time diffs    = $perMean\n";
#    print "std of sample time diffs dev = $perStddev\n";

}



# TODO
# find points in the data where the data changes its trend significantly
#
sub printTrendChanges()
{
    my @trendchanges;
    my $rate_threshold;
}



