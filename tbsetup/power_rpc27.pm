#!/usr/bin/perl -wT
use English;
use strict;
use Socket;
use IO::Handle;

#
# A little perl module to power cycle something attached to an RPC27.
# Thats a serially controlled, power controller.
#
# XXX The little secretkey handshake is coded in here. If this changes
# in capture, you have to change it here too. Look for the "pack"
# statement below. 
#

# Turn off line buffering on output
$| = 1;

# Set for more output.
my $debug = 1;

# RPC27 Prompt string
my $RPC27_PROMPT	= "RPC27>";
my $RPC27_REBOOT	= "reboot";

#
# Main routine.
#
# usage: power_rpc27(controller, outlet)
#
sub power_rpc27($$)
{
    my($controller, $outlet) = @_;
    my($TIP, $i);

    #
    # Form the connection to the controller via a "tip" line to the
    # capture process. Once we have that, we can just talk to the
    # controller directly.
    # 
    if (! ($TIP = tipconnect($controller))) {
	print STDERR "*** Could not form TIP connection to $controller\n";
	return 1;
    }

    #
    # Send a couple of newlines to get the command prompt, and then wait
    # for it to print out the command prompt. This loop is set for a small
    # number since if it cannot get the prompt quickly, then something has
    # gone wrong/
    #
    print $TIP "\n";
    for ($i = 0; $i < 5; $i++) {
	my $line = <$TIP>;
	if ($line =~ /^$RPC27_REBOOT/) {
	    last;
	}
	print $TIP "\n";
    }
    #
    # Okay, got a prompt. Send it the string:
    #
    print $TIP "$RPC27_REBOOT $outlet\n";

    if ($debug) {
	print "power_rpc27: Cycled power to outlet $outlet on $controller.\n";
    }
    
    close($TIP);
}

#
# Connect up to the capture process. This should probably be in a library
# someplace.
# 
sub tipconnect($)
{
    my($controller) = $_[0];
    my($server, $portnum, $keylen, $keydata);
    my($inetaddr, $paddr, $proto);
    my(%powerid_row);
    local *TIP;

    my $query_result =
	DBQueryWarn("select * from tiplines where node_id='$controller'");

    if ($query_result->numrows < 1) {
	print STDERR "*** No such tipline: $controller\n";
	return 1;
    }    
    %powerid_row = $query_result->fetchhash();

    $server  = $powerid_row{'server'};
    $portnum = $powerid_row{'portnum'};
    $keylen  = $powerid_row{'keylen'};
    $keydata = $powerid_row{'keydata'};

    if ($debug) {
	print "tipconnect: $server $portnum $keylen $keydata\n";
    }

    #
    # This stuff from the PERLIPC manpage.
    # 
    if (! ($inetaddr = inet_aton($server))) {
	print STDERR "*** Cannot map $server to IP address\n";
	return 0;
    }
    $paddr    = sockaddr_in($portnum, $inetaddr);
    $proto    = getprotobyname('tcp');

    if (! socket(TIP, PF_INET, SOCK_STREAM, $proto)) {
	print STDERR "*** Cannot create socket.\n";
	return 0;
    }

    if (! connect(TIP, $paddr)) {
	print STDERR
	    "*** Cannot connect to $controller on $server($portnum)\n";
	return 0;
    }
    TIP->autoflush(1);
    
    #
    # Okay, we got a connection. We have to send over the key. This is a
    # little hokey, since we have to make it look like the C struct. 
    #
    my $secretkey = pack("iZ256", $keylen, $keydata);

    if (! syswrite(TIP, $secretkey)) {
	print STDERR
	    "*** Cannot write key to $controller on $server($portnum)\n";
	return 0;
    }

    return(*TIP);    
}

1;
