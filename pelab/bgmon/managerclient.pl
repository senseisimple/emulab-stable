#!/usr/bin/perl -W

#
# Configure variables
#
use lib '/usr/testbed/lib';

use event;
use Getopt::Std;
use strict;
use libwanetmon;

# A node from each site to take part in the measurements
my @expnodes;

sub usage 
{
    warn "Usage: $0 [-p port] [-i managerID]".
	" [-l latency period] [-b bandwidth period] [-a]".
        " [-d testduration]".
	" [-e bgmonExpt]".
	" [-o outputport] [-L]".
	" [-f input_file]".
	" [-c initial_command]".
	" <list of nodes>\n".
	"where -a = measure all pairs\n".
	"      initial_command = \"start\" or \"stop\"\n".
	"      -L = do not init Latency\n".
	"      -B = do not init bandwidth\n";
    return 1;
}


#*****************************************

my %settings;  #misc options
$settings{"allpairs"} = 0;  #if 0, interprets input nodes as pairs
$settings{"expt"} = "__none";
$settings{"bgmonexpt"} = "tbres/pelabbgmon";
$settings{per_latency} = 600;
$settings{per_bw} = 0;

#*****************************************

my %opt = ();
my ($filename_default, $initCmd);
getopts("i:s:p:o:e:d:l:b:f:c:aBLh", \%opt);
my ($server,$port);
if ($opt{i}) { $settings{managerID} = $opt{i}; } 
else { $settings{managerID} = "default"; }
if ($opt{s}) { $server = $opt{s}; } else { $server = "localhost"; }
if ($opt{p}) { $port = $opt{p}; }
if ($opt{h}) { exit &usage; }
if ($opt{e}) { $settings{"bgmonexpt"} = $opt{e}; }
if ($opt{d}) { $settings{"testduration"} = $opt{d}; }
if ($opt{a}) { $settings{"allpairs"} = 1; }
if ($opt{L}) { $settings{"noLatency"} = 1; }
if ($opt{B}) { $settings{"noBW"} = 1; }
if ($opt{l}) { $settings{per_latency} = $opt{l}; }
if ($opt{b}) { $settings{per_bw} = $opt{b}; }
if ($opt{f}) { $filename_default = $opt{f}; } else{ $filename_default = ""; }
if ($opt{c}) { $initCmd = $opt{c}; } else{ $initCmd = ""; }

#if (@ARGV != 1) { exit &usage; }
if( @ARGV > 0 ){
    foreach my $node (@ARGV){
	push @expnodes, $node;
    }
}

print "bgmonexpt=".$settings{"bgmonexpt"}."\n";

my $URL = "elvin://$server";
if ($port) { $URL .= ":$port"; }

my $handle = event_register($URL,0);
if (!$handle) { die "Unable to register with event system\n"; }




#read the experiment nodes from a file
if( $filename_default ne "" ){
    open FILE, "< $filename_default" 
	or die "can't open file $filename_default";
    while( <FILE> ){
	chomp;
	if( $_ ne "" ){
            push @expnodes, $_;
            print "$_\n";
        }
    }
    close FILE;
}


sub doCmd($);


#
# TODO: ?   How to get list of deadnodes back?
# 

#
#MAIN
#

# handle given command on command-line
if( $initCmd ne "" ){
    doCmd($initCmd);
    exit 0;   #exit
}

# handle interactive commands
my $cmd = "";
while( $cmd ne "q" )
{

#  TODO: handle Deadnodes
#    %deadnodes = ();
    print "> ";
    $cmd = <STDIN>;
    chomp $cmd;
    doCmd($cmd);
    
    #  TODO: re-do outputErrors!!
    #outputErrors();
}




sub doCmd($)
{
    my ($cmd) = @_;

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
		initnode($node, $bw_destnodes, $settings{per_bw}, "bw");
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
		if( defined $lat_destnodes && $lat_destnodes ne "" ){
		    initnode($expnodes[$i], $lat_destnodes, 
			     $settings{per_latency}, "latency");
		}
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
	    initnode( $expnodes[$i], $expnodes[$i+1], 
		      $settings{per_latency}, "latency");
	    initnode( $expnodes[$i], $expnodes[$i+1], 
		      $settings{per_bw}, "bw");
	}
    }
}



#STOP
sub stop
{
    print "stopping.\n";
    foreach my $node (@expnodes){
	stopnode_evsys( $node, $settings{managerID}, $handle );
	print "stopping node: $node\n";
    }
}



# destnodes is space-separated string
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
    print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";

    my %cmd = ( managerID => $settings{managerID},
		srcnode   => $node,
		dstnodes => $destnodes,
		testtype  => $testtype,
		testper   => "$testper",
		duration  => "$settings{testduration}",
		expid     => "$settings{bgmonexpt}"
		);

#    sendcmd($node,\%cmd);
    sendcmd_evsys("INIT",\%cmd,$handle);

    print "sent initnode to $node\n";
    print "destnodes = $destnodes\n";
    print "testper = $testper\n";
    print "testtype = $testtype\n";
    print "expid = ".$cmd{expid}."\n";
}

