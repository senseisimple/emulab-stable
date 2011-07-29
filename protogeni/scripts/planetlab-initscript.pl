#!/usr/bin/perl -w
#
# GENIPUBLIC-COPYRIGHT
# Copyright (c) 2008-2011 University of Utah and the Flux Group.
# All rights reserved.
#
use strict;
use English;
use Getopt::Std;
use IO::Socket::INET;

# un-taint path
$ENV{'PATH'} = '/bin:/usr/bin:/usr/local/bin:/usr/site/bin';
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Turn off line buffering on output
#
$| = 1;

# Protos
sub Reflector();

print STDERR "Emulab initialization continuing. ...\n";
system("date > /tmp/emulab_boot.log");

if (0) {
    Reflector();
    exit(0);
}

#
# All we have is our hostname and our slice name, so Emulab has to
# figure out from that, what to do. 
#
exit(0);

sub Reflector()
{
    my ($socket,$received_data);
    my ($peeraddress,$peerport);

    # we call IO::Socket::INET->new() to create the UDP Socket and bound
    # to specific port number mentioned in LocalPort and there is no need
    # to provide LocalAddr explicitly as in TCPServer.

    $socket = new IO::Socket::INET (LocalPort => '31576',
				    Proto => 'udp')
	or die "ERROR in Socket Creation : $!\n";

    while(1) {
	# read operation on the socket
	$socket->recv($recieved_data,1024);

	#get the peerhost and peerport at which the recent data received.
	$peer_address = $socket->peerhost();
	$peer_port = $socket->peerport();
	print "($peer_address , $peer_port) said : $recieved_data";

	#send the data to the client at which the read/write operations done
	#recently.
	$data = "data from server\n";
	$socket->send($data);
    }
    $socket->close();
}
