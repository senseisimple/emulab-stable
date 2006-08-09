#!/usr/bin/perl -w

package libwanetmon;

use strict;
use Exporter;
use vars qw(@ISA @EXPORT);
use IO::Socket::INET;
use IO::Select;
require Exporter;

@ISA    = "Exporter";
@EXPORT = qw ( 
	       %deadnodes
	       %ERRID
	       deserialize_hash
	       serialize_hash
	       sendcmd
	       time_all
	       setcmdport
	       setexpid
	       stopnode
	       edittest
	       killnode
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
}

sub setexpid($)
{
    $expid = $_[0];
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

    my $sercmd = serialize_hash( \%cmd );
    my $f_success = 0;
    my $max_tries = 3;
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
		my $ack = <$ready>;
#		chomp $ack;
		if( defined $ack && ( (chomp($ack),$ack) eq "ACK") ){
		    $f_success = 1;
#		        print "Got ACK from $node for command\n";
		    close $socket;
		    
		}else{
		    $max_tries--;
		}
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
	return 1;
    }

}


sub stopnode($)
{
    my ($node) = @_;
    my %cmd = ( expid    => $expid,
		cmdtype  => "STOPALL" );
    sendcmd($node,\%cmd);
}

sub killnode($)
{
    my ($node) = @_;
    my %cmd = ( expid    => $expid,
		cmdtype  => "DIE" );
    sendcmd($node,\%cmd);
}


sub edittest($$$;$)
{
    my ($srcnode, $destnode, $testper, $testtype, $limitTime) = @_;
    if( !defined $limitTime ){
	$limitTime = 0;
    }
    if ($srcnode eq $destnode ){
	return -1;
    }

    my %cmd = ( expid    => $expid,
		cmdtype  => "EDIT",
		dstnode  => $destnode,
		testtype => $testtype,
		testper  => $testper,
		limitTime=> $limitTime);

    return sendcmd($srcnode,\%cmd);
}





1;
