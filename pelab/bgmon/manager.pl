#!/usr/bin/perl

use lib '/usr/testbed/lib';
use event;
use Getopt::Std;
use strict;


# A node from each site to take part in the measurements
my @expnodes;


sub usage 
{
    warn "Usage: $0 <input_file> [-s server] [-p port] [-a]\n";
    return 1;
}

if( scalar(@ARGV) < 1 ){ exit &usage; }

#*****************************************
#my $filename_defaults = "expnodes.txt";
my $filename_defaults = $ARGV[0];
#my $filename_defaults = "destlist";
my %DEF_PER = (
	       "latency" => 300,
#	       "latency" => 0.4,
#	       "latency" => 0,
#	       "bw"=>2450  # start any test every 50 sec, * ~49 connections.
#	       "bw"=>0
	       "bw"=>950   # start any test every 50 sec, * 19 connections
#	       "bw"=>200
	       );
my %settings;  #misc options
$settings{"allpairs"} = 1;  #if 0 interprets input nodes as pairs
my @expnodes;
#*****************************************

my %opt = ();
getopt(\%opt,"s:p:h:a");

if ($opt{h}) { exit &usage; }
#if ($opt{a}) { 
if (@ARGV > 1) { exit &usage; }



#read the experiment nodes
open FILE, "< $filename_defaults" 
    or die "can't open file $filename_defaults";
while( <FILE> ){
    chomp;
    if( $_ =~ m/plab/ ){ 
	push @expnodes, $_;
    }
}
close FILE;

my ($server,$port);
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }

my $tuple = address_tuple_alloc();
if (!$tuple) { die "Could not allocate an address tuple\n"; }



#MAIN
#TODO: Control Loop
my $cmd;
while( $cmd ne "q" )
{
    $cmd = <STDIN>;
    chomp $cmd;
  SWITCH: {
      if( $cmd eq "start" ){ start(); last SWITCH; }
      if( $cmd eq "stop" ){ stop(); last SWITCH; }
      if( $cmd eq "reset" ){ reset(); last SWITCH; }
      if( $cmd eq "tmp" ){ tmp(); last SWITCH; }
      if( $cmd eq "update"){ update(); last SWITCH; }
      if( $cmd eq "set"){ adjsettings(); last SWITCH; }
      if( $cmd eq "auto"){ setAutoNodes(); last SWITCH; }
      my $nothing = 1;
  }
}


=pod
sub tmp
{
#    @expnodes = ("plab231","plab231","plab71","plab122");
    foreach my $node (@expnodes){
	%$tuple = ( objtype   => "BGMON",
		    objname   => $node,
		    eventtype => "INIT",
		    expt      => "__none");
	my $notification = event_notification_alloc($handle,$tuple);
	if (!$notification) { die "Could not allocate notification\n"; }

	#build string of destination nodes
	my $destnodes;
	foreach my $dest (@expnodes){
	    #add destination nodes to this source node, but not "self"
	    if( $dest ne $node ){
		$destnodes = $destnodes." ".$dest;
	    }
	}
	if( 0 == event_notification_put_string( $handle,
						$notification,
						"destnodes",
						$destnodes ) )
	{ warn "Could not add attribute to notification\n"; }

	#build string of test types and defaults
	my $testdefs;
	foreach my $key (keys %DEF_PER){
	    $testdefs = $testdefs." ".$key." ".$DEF_PER{$key};
	}
	if( 0 == event_notification_put_string( $handle,
						$notification,
						"testdefs",
						$testdefs ) )
	{ warn "Could not add attribute to notification\n"; }

	#send notification
	if (!event_notify($handle, $notification)) {
	    die("could not send test event notification");
	}
    }
}
=cut


#START
sub start
{
    print "starting.";
    if( $settings{allpairs} == 1 ){
	#start fully-connected tests amongst nodes

	foreach my $node (@expnodes){
	    #build string of destination nodes
	    #init bandwidth
	    my $bw_destnodes;
	    foreach my $dest (@expnodes){
		#add destination nodes to this source node, but not "self"
		if( $dest ne $node ){
		    $bw_destnodes = $bw_destnodes." ".$dest;
		}
	    }
	    initnode($node, $bw_destnodes, $DEF_PER{"bw"}, "bw");
	    #TODO! Distribute initialization times evenly
	}


	#INIT LATENCY

	#make connections "balanced" to prevent one node from running tests to
	# all the others
	my @destmat;
	#initialize matrix: 0=>no link, 1=>link, 2=>reverse or self link
	#index values are index of @expnodes
	my ($i,$j) = (0,0);
	for $i (0..$#expnodes){
	    push @destmat, [];
	    for $j (0..$#expnodes){
		if( $i == $j ){
		    $destmat[$i][$j] = 2;
		}else{
		    $destmat[$i][$j] = 0;
		}
	    }
	}

	#mark connections in matrix
	my $remaining_connections = @expnodes * (@expnodes-1);
    #    print "numexpnodes = ".scalar(@expnodes)."\n";
	($i, $j) = (0, 0);    
	while( $remaining_connections > 0 ){
	    #walk array of destination nodes looking for the first "no link"
	    #starting at the previous node
	    while( $destmat[$i][$j] != 0 ){
		$j = ($j+1) % @expnodes;
	    }
	    $destmat[$i][$j] = 1;
	    $destmat[$j][$i] = 2;
	    $remaining_connections -= 2;
    #	print "i,j, remaining = $i,$j,$remaining_connections\n";
	    $i = ($i+1) % @expnodes;
	}

=pod
	#print connection matrix
	print "\n";
	for $i (0..$#expnodes){
	    for $j (0..$#expnodes){
		print " $destmat[$i][$j] ";
	    }
	    print "\n";
	}
=cut

	#send latency data
	for $i (0..$#expnodes){
	    #build string of destination nodes
	    my $lat_destnodes;
	    for $j (0..$#expnodes){
		#add destination nodes to this source node, but not "self"
		if( $destmat[$i][$j] == 1 ){
		    $lat_destnodes = $lat_destnodes." ".$expnodes[$j];
		}
	    }
	    #send list
    #	print "$lat_destnodes\n";
	    initnode($expnodes[$i], $lat_destnodes, 
		     $DEF_PER{"latency"}, "latency");
	}
    }
    
    elsif( $settings{allpairs} == 0 ){
	#start tests by pairs
	print "starting single-pairs tests\n";
	my $i;
	print "expnodes = @expnodes\n";
	for( $i=0; $i < $#expnodes; $i=$i+ 2 ){
	    #start latency
	    print "src=$expnodes[$i], dst=$expnodes[$i+1]\n";
	    initnode( $expnodes[$i], $expnodes[$i+1], $DEF_PER{"latency"},
		      "latency");
	    initnode( $expnodes[$i], $expnodes[$i+1], $DEF_PER{"bw"}, "bw");
	}
    }
}


#STOP
sub stop
{
    print "stopping.\n";
    foreach my $node (@expnodes){
	stopnode( $node );
	print "stopping node: $node\n";
    }
}


#RESET
sub reset
{

}


# destnodes and testdefs are space-separated strings
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
    print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";
    %$tuple = ( objtype   => "BGMON",
		objname   => $node,
		eventtype => "INIT",
		expt      => "__none" );
    my $notification = event_notification_alloc($handle,$tuple);
    if (!$notification) { die "Could not allocate notification\n"; }

    #add destination nodes attribute
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "destnodes",
					    $destnodes ) )
    { warn "Could not add attribute to notification\n"; }

    #add tests and their default values
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "testper",
					    $testper ) )
    { warn "Could not add attribute to notification\n"; }

    #add test type
    if( 0 == event_notification_put_string( $handle,
					    $notification,
					    "testtype",
					    $testtype ) )
    { warn "Could not add attribute to notification\n"; }


    #send notification
    if (!event_notify($handle, $notification)) {
	die("could not send test event notification");
    }

    print "sent initnode to $node\n";
    print "destnodes = $destnodes\n";
    print "testper = $testper\n";
    print "testtype = $testtype\n";
}

sub stopnode($)
{
    my ($node) = @_;
    %$tuple = ( objtype   => "BGMON",
		objname   => $node,
		eventtype => "STOPALL",
		expt      => "__none" );
    my $notification = event_notification_alloc($handle,$tuple);
    if (!$notification) { die "Could not allocate notification\n"; }
    #send notification
    if (!event_notify($handle, $notification)) {
	die("could not send test event notification");
    }
}



sub updatenodes
{

    #TODO!!

    #re-read file of nodes

    #compare each to current list

    #stop those nodes which are no longer listed

    #start those nodes which are newly listed
}


sub adjsettings
{
    print "enter setting and value: <setting> <value>\n";
    my $input = <STDIN>;
    my @settingcmd = split " ",$input;
    if( scalar(@settingcmd) < 2 ){
	die "bad setting command";
    }
    $settings{$settingcmd[0]} = $settingcmd[1];
}
