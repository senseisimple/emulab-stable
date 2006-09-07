#!/usr/bin/perl -W

=pod

A Utility to be run on ops to send control commands to
a set of nodes. This is most useful to manually adjust measurement
frequency of specific nodes of interest.

Parameters/options (more details):
- port: remote port num to use for commands (TCP).
- latency/bandwidth periods: period in seconds between measurements.
- outputport: local port number when sending commands (TCP).
- (-a): when initializing the set of nodes given in <input_file>,
        schedule measurements in a fully connected fashion. Default
        is to init nodes in a pairwise fashion, in order listed in
        <input_file>.

After script starts, it awaits a user command at a console prompt.
The commands fully implemented are "start" and "stop", which send 
proper INIT or STOPALL commands, respectively, to the node set.

=cut

use Getopt::Std;
use strict;
use IO::Socket::INET;
use IO::Select;
use libwanetmon;

# A node from each site to take part in the measurements
my @expnodes;

sub usage 
{
    warn "Usage: $0 [-p port] [-e pid/eid]".
	" [-l latency period] [-b bandwidth period] [-a]".
        " [-d testduration]".
	" [-o outputport] [-L]   <input_file>\n".
	"where -a = measure all pairs\n".
	"      -L = do not init Latency\n".
	"      -B = do not init bandwidth\n";
    return 1;
}

if( scalar(@ARGV) < 1 ){ exit &usage; }

#*****************************************
my %DEF_PER = (
#	       "latency" => 5,
#	       "latency" => 300,
	       "latency" => 600,
#	       "latency" => 0.4,
#	       "latency" => 0,
#	       "bw"=>120,
#	       "bw"=>2450  # start any test every 50 sec, * ~49 connections.
	       "bw"=>7850  # start any test every 50 sec, * ~157 connections.
#	       "bw"=>0
#	       "bw"=>950   # start any test every 50 sec, * 19 connections
#	       "bw"=>200
	       );
my %settings;  #misc options
$settings{"allpairs"} = 0;  #if 0 interprets input nodes as pairs
$settings{"expt"} = "__none";

#*****************************************

my %opt = ();
getopts("s:p:o:h:e:d:l:b:aBL", \%opt);

if ($opt{h}) { exit &usage; }
if ($opt{e}) { $settings{"expt"} = $opt{e}; }
if ($opt{d}) { $settings{"testduration"} = $opt{d}; }
if ($opt{a}) { $settings{"allpairs"} = 1; }
if ($opt{L}) { $settings{"noLatency"} = 1; }
if ($opt{B}) { $settings{"noBW"} = 1; }


# XXX: We should transfer the defaults into the %settings hash, then
#      override here.  However, the rest of the code references DEF_PER
#      and I am not inclined to fix that up right now.
if (defined($opt{l})) { $DEF_PER{"latency"} = $opt{l}; }
if (defined($opt{b})) { $DEF_PER{"bw"} = $opt{b}; }

if (@ARGV > 1) { exit &usage; }

my $filename_defaults = $ARGV[0];

my $sel = IO::Select->new();

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

my ($port, $lport);
if ($opt{p}) { $port = $opt{p}; } else{ $port = 5052; }
if ($opt{o}) { $lport = $opt{o}; } else{ $lport = 5052; }
print "using remote port $port and local port $lport\n";
print "experiment $settings{expt}\n";
my $socket;

setcmdport($port);
setexpid($settings{expt});

#
# Prototypes
#
sub outputErrors();

#
#MAIN
#
my $cmd = "";
while( $cmd ne "q" )
{

    %deadnodes = ();
    
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
    outputErrors();
}


#START
sub start
{
    print "starting.";
    if( $settings{allpairs} == 1 ){
	#start fully-connected tests amongst nodes

	if( !defined $settings{noBW} ){
	    foreach my $node (@expnodes){
		#build string of destination nodes
		#init bandwidth
		my $bw_destnodes = "";
		foreach my $dest (@expnodes){
		    #add destination nodes to this source node, but not "self"
		    if( $dest ne $node ){
			$bw_destnodes = $bw_destnodes." ".$dest;
		    }
		}
		initnode($node, $bw_destnodes, $DEF_PER{"bw"}, "bw");
		#TODO! Distribute initialization times evenly
	    }
	}


	
	#INIT LATENCY
#	if( !defined $settings{noLatency} ){

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
			if (defined $lat_destnodes){
			    $lat_destnodes = $lat_destnodes." ".$expnodes[$j];
			}else{
			    $lat_destnodes = $expnodes[$j];
			}
		    }
		}
		#send list
		#	print "$lat_destnodes\n";
		initnode($expnodes[$i], $lat_destnodes, 
			 $DEF_PER{"latency"}, "latency");
	    }
#	}
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


# destnodes is space-separated string
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
    print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";

    my %cmd = ( expid     => $settings{expt},
		cmdtype   => "INIT",
		destnodes => $destnodes,
		testtype  => $testtype,
		testper   => $testper,
		duration  => $settings{testduration}
		);

    sendcmd($node,\%cmd);

    print "sent initnode to $node\n";
    print "destnodes = $destnodes\n";
    print "testper = $testper\n";
    print "testtype = $testtype\n";
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


sub outputErrors()
{
    print "Nodes not responding to Command:\n";
    foreach my $node (keys %deadnodes){
	print "$node  ";
    }
    print "\n";
}

