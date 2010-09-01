#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#

#
# library of functions to manipulate the Apcon layer 1 switch by CLI
#
package apcon_clilib;

use Exporter;
@ISA = ("Exporter");
@EXPORT = qw( createExpectObject
              parseConnections
              parseNames
              parseClasses
              parseZones
              parseClassPorts
              parseZonePorts
              doCLICmd
              getRawOutput
              getAllNamedPorts 
              getPortName
              getNamedPorts
              getNamedConnections
              addCls
              addZone
              addClassPorts
              connectMulticast
              connectDuplex
              connectSimplex
              namePorts
              unnamePorts
              disconnectPorts 
              removePortName
              setPortRate
              getPortInfo
);

use strict;

$| = 1;

use English;
use Expect;

# some constants
my $APCON_SSH_CONN_STR = "ssh -l admin apcon1";

# This seems to be a bad practice... It will be better if we cancel 
# the password on the switch.
my $APCON1_PASSWD = "daApcon!";
my $CLI_PROMPT = "apcon1>> ";
my $CLI_UNNAMED_PATTERN = "[Uu]nnamed";
my $CLI_UNNAMED_NAME = "unnamed";
my $CLI_NOCONNECTION = "A00";
my $CLI_TIMEOUT = 10000;

# commands to show something
my $CLI_SHOW_CONNECTIONS = "show connections raw\r";
my $CLI_SHOW_PORT_NAMES  = "show port names *\r";

# mappings from port control command to CLI command
my %portCMDs =
(
    "enable" => "00",
    "disable"=> "00",
    "1000mbit"=> "9f",
    "100mbit"=> "9b",
    "10mbit" => "99",
    "auto"   => "00",
    "full"   => "94",
    "half"   => "8c",
    "auto1000mbit" => "9c",
    "full1000mbit" => "94",
    "half1000mbit" => "8c",
    "auto100mbit"  => "9a",
    "full100mbit"  => "92",
    "half100mbit"  => "8a",
    "auto10mbit"   => "99",
    "full10mbit"   => "91",
    "half10mbit"   => "89",
);


#
# Create an Expect object that spawns the ssh process 
# to switch.
#
sub createExpectObject()
{
    # default connection string:
    my $spawn_cmd = $APCON_SSH_CONN_STR;
    if ( @_ ) {
	$spawn_cmd = shift @_;
    }

    # Create Expect object and initialize it:
    my $exp = new Expect();
    if (!$exp) {
	# upper layer will check this
	return undef;
    }
    $exp->raw_pty(0);
    $exp->log_stdout(0);
    $exp->spawn($spawn_cmd)
	or die "Cannot spawn $spawn_cmd: $!\n";
    $exp->expect($CLI_TIMEOUT,
		 ["admin\@apcon1's password:" => sub { my $e = shift;
						       $e->send($APCON1_PASSWD."\n");
						       exp_continue;}],
		 ["Permission denied (password)." => sub { die "Password incorrect!\n";} ],
		 [ timeout => sub { die "Timeout when connect to switch!\n";} ],
		 $CLI_PROMPT );
    return $exp;
}


#
# parse the connection output
# return two hashes for query from either direction
#
sub parseConnections($) 
{
    my $raw = shift;

    my @lines = split( /\n/, $raw );
    my %dst = ();
    my %src = ();

    foreach my $line ( @lines ) {
	if ( $line =~ /^([A-I][0-9]{2}):\s+([A-I][0-9]{2})\W*$/ ) {
	    if ( $2 ne $CLI_NOCONNECTION ) {
		$src{$1} = $2;
		if ( ! (exists $dst{$2}) ) {
		    $dst{$2} = {};
		}

		$dst{$2}{$1} = 1;
	    }
	}
    }

    return (\%src, \%dst);
}


#
# parse the port names output
# return the port => name hashtable
#
sub parseNames($)
{
    my $raw = shift;

    my %names = ();

    foreach ( split ( /\n/, $raw ) ) {
	if ( /^([A-I][0-9]{2}):\s+(\w+)\W*/ ) {
	    if ( $2 !~ /$CLI_UNNAMED_PATTERN/ ) {
		$names{$1} = $2;
	    }
	}
    }

    return \%names;
}


#
# parse the show classes output
# return the classname => 1 hashtable, not a list.
#
sub parseClasses($)
{
    my $raw = shift;
    
    my %clses = ();

    foreach ( split ( /\n/, $raw ) ) {
	if ( /^Class\s\d{1,2}:\s+(\w+)\s+(\w+)\W*$/ ) {
	    $clses{$2} = 1;
	}
    }

    return \%clses;
}

#
# parse the show zones output
# return the zonename => 1 hashtable, not a list
#
sub parseZones($)
{
    my $raw = shift;

    my %zones = ();

    foreach ( split ( /\n/, $raw) ) {
	if ( /^\d{1,2}:\s+(\w+)\W*$/ ) {
	    $zones{$1} = 1;
	}
    }

    return \%zones;
}


#
# parse the show class ports output
# return the ports list
#
sub parseClassPorts($)
{
    my $raw = shift;
    my @ports = ();

    foreach ( split ( /\n/, $raw) ) {
	if ( /^Port\s+\d+:\s+([A-I][0-9]{2})\W*$/ ) {
	    push @ports, $1;
	}
    }

    return \@ports;
}


#
# parse the show zone ports output
# same to parse_class_ports
#
sub parseZonePorts($)
{
    return parseClassPorts(@_[0]);
}


#
# helper to do CLI command and check the error msg
#
sub doCLICmd($$)
{
    my ($exp, $cmd) = @_;
    my $output = "";

    $exp->clear_accum(); # Clean the accumulated output, as a rule.
    $exp->send($cmd);
    $exp->expect($CLI_TIMEOUT,
		 [$CLI_PROMPT => sub {
		     my $e = shift;
		     $output = $e->before();
		  }]);

    $cmd = quotemeta($cmd);
    if ( $output =~ /^($cmd)\n(ERROR:.+)\r\n[.\n]*$/ ) {
	return (1, $2);
    } else {
	return (0, $output);
    }
}


#
# get the raw CLI output of a command
#
sub getRawOutput($$)
{
    my ($exp, $cmd) = @_;
    my ($rt, $output) = doCLICmd($exp, $cmd);
    if ( !$rt ) {    	
    	my $qcmd = quotemeta($cmd);
	if ( $output =~ /^($qcmd)/ ) {
	    return substr($output, length($cmd)+1);
	}		
    }

    return undef;
}


#
# get all name => prorts hash
# return the name => port list hashtable
#
sub getAllNamedPorts($)
{
    my $exp = shift;

    my $raw = getRawOutput($exp, $CLI_SHOW_PORT_NAMES);
    my $names = parse_names($raw); 

    my %nps = ();
    foreach my $k (keys %{$names}) {
	if ( !(exists $nps{$names->{$k}}) ) {
	    $nps{$names->{$k}} = ();
	}

	push @{$nps{$names->{$k}}}, $k;
    }

    return \%nps;
}


#
# get the name of a port
#
sub getPortName($$)
{
    my ($exp, $port) = @_;

    my $raw = getRawOutput($exp, "show port info $port\r");
    if ( $raw =~ /$port Name:\s+(\w+)\W*\n/ ) {
	if (  $1 !~ /$CLI_UNNAMED_PATTERN/ ) {
	    return $1;
	}
    }

    return undef;
}

#
# get the ports list by their name.
#
sub getNamedPorts($$)
{
    my ($exp, $pname) = @_;
    my @ports = ();

    my $raw = getRawOutput($exp, $CLI_SHOW_PORT_NAMES);
    foreach ( split /\n/, $raw ) {
	if ( /^([A-I][0-9]{2}):\s+($pname)\W*/ ) {
	    push @ports, $1;
	}
    }

    return \@ports;
}

#
# get connections of ports with the given name
# return two hashtabls whose format is same to parseConnections
#
sub getNamedConnections($$)
{
    my ($exp, $name) = @_;

    my $raw_conns = getRawOutput($exp, $CLI_SHOW_CONNECTIONS);
    my ($allsrc, $alldst) = parseConnections($raw_conns);
    my $ports = getNamedPorts($exp, $name);

    my %src = ();
    my %dst = ();

    #
    # There may be something special: a named port may connect to
    # a port whose name is not the same. Then this connection
    # should not belong to the 'vlan'. Till now the following codes
    # have not dealt with it yet.
    #
    # TODO: remove those connections containning ports don't belong
    #       to the 'vlan'.
    #
    foreach my $p (@$ports) {
	if ( exists($allsrc->{$p}) ) {
	    $src{$p} = $allsrc->{$p};
	} 

	if ( exists($alldst->{$p}) ) {
	    $dst{$p} = $alldst->{$p};
	}
    }

    return (\%src, \%dst);
}

#
# Add a new class
#
sub addCls($$)
{
    my ($exp, $clsname) = @_;
    my $cmd = "add class I $clsname\r";

    return doCLICmd($exp, $cmd);
}

#
# Add a new zone
#
sub addZone($$)
{
    my ($exp, $zonename) = @_;
    my $cmd = "add zone $zonename\r";

    return doCLICmd($exp, $cmd);
}

#
# Add some ports to a class
#
sub addClassPorts($$@)
{
    my ($exp, $clsname, @ports) = @_;
    my $cmd = "add class ports $clsname ".join("", @ports)."\r";

    return doCLICmd($exp, $cmd);
}

# 
# Connect ports functions:
#

#
# Make a multicast connection $src --> @dsts
#
sub connectMulticast($$@)
{
    my ($exp, $src, @dsts) = @_;
    my $cmd = "connect multicast $src".join("", @dsts)."\r";

    return doCLICmd($exp, $cmd);
}

#
# Make a duplex connection $src <> $dst
#
sub connectDuplex($$$)
{
    my ($exp, $src, $dst) = @_;
    my $cmd = "connect duplex $src"."$dst"."\r";

    return doCLICmd($exp, $cmd);
}

#
# Make a simplex connection $src -> $dst
#
sub connectSimplex($$$)
{
    my ($exp, $src, $dst) = @_;
    my $cmd = "connect simplex $src"."$dst"."\r";

    return doCLICmd($exp, $cmd);
}


#
# Add some ports to a vlan, 
# it actually names those ports to the vlanname. 
#
sub namePorts($$@)
{
    my ($exp, $name, @ports) = @_;

    for( my $i = 0; $i < @ports; $i++ ) {    	
	my ($rt, $msg) = doCLICmd($exp, 
				     "configure port name $ports[$i] $name\r");

	# undo set name
	if ( $rt ) {
	    for ($i--; $i >= 0; $i-- ) {	    	
		doCLICmd($exp, 
			    "configure port name $ports[$i] $CLI_UNNAMED_NAME\r");
	    }
	    return $msg;
	}
    }

    return 0;
}


#
# Unname ports, the name of those ports will be $CLI_UNNAMED_NAME
#
sub unnamePorts($@)
{
    my ($exp, @ports) = @_;

    my $emsg = "";
    foreach my $p (@ports) {
	my ($rt, $msg) = doCLICmd($exp,
				     "configure port name $p $CLI_UNNAMED_NAME\r");
	if ( $rt ) {
	    $emsg = $emsg.$msg."\n";
	}
    }

    if ( $emsg eq "" ) {
	return 0;
    }

    return $emsg;
}


#
# Disconnect ports
# $sconns: the dst => src hashtable.
#
sub disconnectPorts($$) 
{
    my ($exp, $sconns) = @_;

    my $emsg = "";
    foreach my $dst (keys %$sconns) {
	my ($rt, $msg) = doCLICmd($exp,
				     "disconnect $dst".$sconns->{$dst}."\r");
	if ( $rt ) {
	    $emsg = $emsg.$msg."\n";
	}
    }

    if ( $emsg eq "" ) {
	return 0;
    }

    return $emsg;
}


#
# Remove a 'vlan', unname the ports and disconnect them
#
sub removePortName($$)
{
    my ($exp, $name) =  @_;

    # Disconnect ports:
    my ($src, $dst) = getNamedConnections($exp, $name);
    my $disrt = disconnectPorts($exp, $src);

    # Unname ports:
    my $ports = getNamedPorts($exp, $name);
    my $unrt = unnamePorts($exp, @$ports);
    if ( $unrt || $disrt) {
	return $disrt.$unrt;
    }

    return 0;
}


#
# Set port rate, for port control.
# Rates are defined in %portCMDs.
#
sub setPortRate($$$)
{
    my ($exp, $port, $rate) = @_;

    if ( !exists($portCMDs{$rate}) ) {
	return "ERROR: port rate unsupported!\n";
    }

    my $cmd = "configure rate $port $portCMDs{$rate}\r";
    my ($rt, $msg) = doCLICmd($exp, $cmd);
    if ( $rt ) {
	return $msg;
    }

    return 0;
}

#
# Get port info
#
sub getPortInfo($$)
{
    my ($exp, $port) = @_;

    # TODO: parse the output of show port xxxxx
    return [$port,0];
}
