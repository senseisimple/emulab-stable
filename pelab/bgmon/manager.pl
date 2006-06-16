#!/usr/bin/perl

use Getopt::Std;
use strict;
use IO::Socket::INET;
use IO::Select;

# A node from each site to take part in the measurements
my @expnodes;

my %deadnodes;

sub usage 
{
    warn "Usage: $0 <input_file> [-p port] [-e pid/eid] [-l latency period] [-b bandwidth period] [-a]\n";
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
my @expnodes;
#*****************************************

my %opt = ();
getopts("s:p:h:e:l:b:a", \%opt);

if ($opt{h}) { exit &usage; }
if ($opt{e}) { $settings{"expt"} = $opt{e}; }
if ($opt{a}) { $settings{"allpairs"} = 1; }

# XXX: We should transfer the defaults into the %settings hash, then
#      override here.  However, the rest of the code references DEF_PER
#      and I am not inclined to fix that up right now.
if (defined($opt{l})) { $DEF_PER{"latency"} = $opt{l}; }
if (defined($opt{b})) { $DEF_PER{"bw"} = $opt{b}; }

if (@ARGV > 1) { exit &usage; }

#my $filename_defaults = "destlist";
#my $filename_defaults = "expnodes.txt";
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

my ($port);
if ($opt{p}) { $port = $opt{p}; } else{ $port = 5060; }

my $socket;

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
    outputErrors();
}


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


# destnodes is space-separated string
sub initnode($$$$)
{
    my ($node, $destnodes, $testper, $testtype) = @_;
    print "SENDING INIT: *$node*$destnodes*$testper*$testtype*\n";

    my %cmd = ( expid     => $settings{expt},
		cmdtype   => "INIT",
		destnodes => $destnodes,
		testtype  => $testtype,
		testper   => $testper
		);

    sendcmd($node,\%cmd);
=pod    
    $socket = IO::Socket::INET->new( PeerPort => $port,
				     Proto    => 'udp',
				     PeerAddr => $node );
#    my $sercmd = Storable::freeze \%cmd ;
    my $sercmd = serialize_hash( \%cmd );
    $socket->send($sercmd);
=cut

    print "sent initnode to $node\n";
    print "destnodes = $destnodes\n";
    print "testper = $testper\n";
    print "testtype = $testtype\n";
}

sub stopnode($)
{
    my ($node) = @_;
    my %cmd = ( expid    => $settings{expt},
		cmdtype  => "STOPALL" );
    sendcmd($node,\%cmd);
}

sub sendcmd($$)
{
    my $node = $_[0];
    my $hashref = $_[1];
    my %cmd = %$hashref;

    my $sercmd = serialize_hash( \%cmd );
    my $f_success = 0;
    my $max_tries = 5;
    do{
	$socket = IO::Socket::INET->new( PeerPort => $port,
					 Proto    => 'tcp',
					 PeerAddr => $node );
	$sel->add($socket);
	if( defined $socket ){	    
	    print $socket "$sercmd\n";
	    #todo: wait for ack;
	    # timeout period?
	    $sel->add($socket);
	    my ($ready) = $sel->can_read(1);
	    if( $ready eq $socket ){
		my $ack = <$ready>;
		chomp $ack;
		if( $ack eq "ACK" ){
		    $f_success = 1;
#		    print "Got ACK from $node for command\n";
		    close $socket;
		}else{
		    $max_tries--;
		}
	    }
	    $sel->remove($socket);
	    close($socket);
	}else{
	    select(undef, undef, undef, 0.2);
	    $max_tries--;
	}
    }while( $f_success != 1 && $max_tries != 0 );

    if( $max_tries == 0 ){
	$deadnodes{$node} = 1;
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


sub outputErrors()
{
    print "Nodes not responding to Command:\n";
    foreach my $node (keys %deadnodes){
	print "$node  ";
    }
    print "\n";
}


#
# Custom sub to turn a hash into a string. Hashes must not contain
# the substring of $separator anywhere!!!
#
sub serialize_hash($)
{
    my ($hashref) = @_;
    my %hash = %$hashref;
    my $separator = "::";
    my $out = "";

    for my $key (keys %hash){
	$out .= $separator if( $out ne "" );
	$out .= $key.$separator.$hash{$key};
    }
    return $out;
}


