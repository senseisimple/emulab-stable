#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#


package libwanetmon;

use strict;
use Exporter;
use vars qw(@ISA @EXPORT);
use IO::Socket::INET;
use IO::Select;
#use lib '/usr/testbed/lib';
#use event;
require Exporter;

@ISA    = "Exporter";
our @EXPORT = qw ( 
	       %deadnodes
	       %ERRID
	       deserialize_hash
	       serialize_hash
	       sendcmd
	       sendcmd_evsys
	       time_all
	       setcmdport
	       setexpid
	       stopnode
	       stopnode_evsys
	       edittest
	       edittest_evsys
	       killnode
	       getstatus
	       );
our @EXPORT_OK = qw(
		    );


# These errors define specifics of when a measurement value cannot be
# reported due to some error in the network or at the remote end.
our %ERRID;
$ERRID{timeout} = -1;
$ERRID{ttlexceed} = -2; # was an error for "ping", but is not seen in fping.
$ERRID{unknown} = -3;   # general error, which cannot be classified into others
$ERRID{unknownhost} = -4;
$ERRID{ICMPunreachable} = -5;  #used for all ICMP errors (see fping.c for strs)
$ERRID{iperfHostUnreachable} = -6;  #for iperf: error flagged by: write1 failed: Broken pipe

our %deadnodes;

my $socket;
my $sel = IO::Select->new();
my $port;
my $expid;




sub setcmdport($)
{
    $port = $_[0];
#    print "libwanetmon: port=$port\n";
}

sub setexpid($)
{
    $expid = $_[0];
#    print "libwanetmon: expid=$expid\n";
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



sub deserialize_hash($)
{
    my ($string) = @_;
    my $separator = "::";
    my %hashout;

    my @tokens = split( /$separator/, $string );

    for( my $i=0; $i<@tokens; $i+=2 ){
	$hashout{$tokens[$i]} = $tokens[$i+1];
#	print "setting $tokens[$i] => $tokens[$i+1]\n";
    }
    return \%hashout;
}



sub time_all()
{
    package main;
    require 'sys/syscall.ph';
    my $tv = pack("LL",());
    syscall( &SYS_gettimeofday, $tv, undef ) >=0
	or warn "gettimeofday: $!";
    my ($sec, $usec) = unpack ("LL",$tv);
    return $sec + ($usec / 1_000_000);
#    return time();
}


sub sendcmd($$)
{
    my $node = $_[0];
    my $hashref = $_[1];
    my %cmd = %$hashref;
    if( !defined $cmd{managerID} ){
	$cmd{managerID} = "default";
    }
    if( !defined $cmd{expid} ){
        $cmd{expid} = $expid;
    }

    my $sercmd = serialize_hash( \%cmd );
#    print "sercmd=$sercmd\n";
    my $f_success = 0;
    my $max_tries = 3;
    my $retval;
    do{
	$socket = IO::Socket::INET->new( PeerPort => $port,
					 Proto    => 'tcp',
					 PeerAddr => $node,
					 Timeout  => 1 );
	if( defined $socket ){
#	    $sel->add($socket);
	    print $socket "$sercmd\n";
	    $sel->add($socket);
	    my ($ready) = $sel->can_read(2);  #timeout (seconds) (?)
	    if( defined($ready) && $ready eq $socket ){
		my $ack;
		($ack, $retval) = <$ready>;
		if( defined $ack && ( (chomp($ack),$ack) eq "ACK") ){
		    $f_success = 1;
#		        print "Got ACK from $node for command\n";
		    close $socket;
		    
		}else{
		    $max_tries--;
		}
		chomp $retval 
		    if( defined $retval );
	    }else{
		$max_tries--;
	    }
	    $sel->remove($socket);
	    close($socket);
	}else{
	    select(undef, undef, undef, 0.2);
	    $max_tries--;
	}
    }while( $f_success != 1 && $max_tries != 0 );

    if( $f_success == 0 && $max_tries == 0 ){
	$deadnodes{$node} = 1;
#	print "DID NOT GET ACK from $node for command $sercmd\n";
	return -1;
    }elsif( $f_success == 1 ){
	#success!
	delete $deadnodes{$node};
	return (1,$retval);
    }

}




#
# input params:
# - name of command (EDIT, INIT, etc..)
# - hash of extra strings to add to event notification
# - handle to eventsystem "handle"
sub sendcmd_evsys($$$;$)
{
    my ($cmdname, $hashref,$handle,$manType) = @_;
    my %cmd = %$hashref;

    if( !defined $manType ){
	$manType = "managerclient";
    }

    print "manType = $manType\n";

    #
    # This is the evsys command to send
    #
    my $tuple = event::address_tuple_alloc();
    if (!$tuple) { die "Could not allocate an address tuple\n"; }

    %$tuple = ( objtype   => "WANETMON",
		objname   => $manType,
		eventtype => $cmdname,
		expt      => "__none",
		);

    my $notification = event::event_notification_alloc($handle,$tuple);
    if (!$notification) { die "Could not allocate notification\n"; }

    # set extra params
    foreach my $name (keys %cmd){
	if( 0 == event::event_notification_put_string( $handle,
						$notification,
						"$name",
						$cmd{$name} ) )
	{ warn "Could not add attribute to notification\n"; }
#	print "adding $name => ".$cmd{$name}."\n";
    }

    #send notification
    if (!event::event_notify($handle, $notification)) {
	die("could not send test event notification");
    }

    event::event_notification_free($handle, $notification);
}


sub stopnode($$)
{
    my ($node,$managerID) = @_;
    my %cmd = (
		managerID => $managerID,
		cmdtype  => "STOPALL" );
    sendcmd($node,\%cmd);
}


#
#
sub stopnode_evsys($$$)
{
    my ($node, $managerID, $handle) = @_;
    my %cmd = ( srcnode      => $node,
		managerID    => $managerID,
		cmdtype  => "STOPALL" );
    my $manType;
    if( $managerID eq "automanagerclient" ){
	$manType = $managerID;
    }else{
	$manType = "managerclient";
    }

    sendcmd_evsys("STOPALL",\%cmd,$handle,$manType);
}


sub killnode($$)
{
    my ($node,$managerID) = @_;
    my %cmd = (
                managerID=> $managerID,
		cmdtype  => "DIE" );
    sendcmd($node,\%cmd);
}


sub edittest($$$$$$)
{
    my ($srcnode, $destnode, $testper, $testtype, $duration, $managerID) = @_;

    if ($srcnode eq $destnode ){
	return -1;
    }

    my %cmd = ( expid    => $expid,
		cmdtype  => "EDIT",
		dstnode  => $destnode,
		testtype => $testtype,
		testper  => "$testper",
		duration => "$duration" );

    return ${[sendcmd($srcnode,\%cmd)]}[0];
}

sub edittest_evsys($$$$$$$)
{
    my ($srcnode, $destnode, $testper, $testtype, 
	$duration, $managerID, $handle) = @_;

    if ($srcnode eq $destnode ){
	return -1;
    }

    my %cmd = ( managerID    => $managerID,
		srcnode  => $srcnode,
		dstnode  => $destnode,
		testtype => $testtype,
		testper  => "$testper",
		 duration => "$duration" );

    my $manType;
    if( $managerID eq "automanagerclient" ){
	$manType = $managerID;
    }else{
	$manType = "managerclient";
    }

    sendcmd_evsys("EDIT",\%cmd,$handle,$manType);

    #return ${[sendcmd($srcnode,\%cmd)]}[0];
}


sub getstatus($){
    my ($node) = @_;
    my %cmd = ( expid    => $expid,
		cmdtype  => "GETSTATUS" );
    my @cmdresult = sendcmd($node,\%cmd);
    if( defined $cmdresult[1] ){
	return $cmdresult[1];
    }else{
	return "error";
    }
}




1;
