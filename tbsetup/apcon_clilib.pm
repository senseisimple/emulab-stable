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
@EXPORT = qw( parse_connections
              parse_names
              parse_classes
              parse_zones
              parse_class_ports
              parse_zone_ports
              get_all_vlans
              get_port_vlan
              get_vlan_ports
              get_vlan_connections
              add_cls
              add_zone
              add_class_ports
              connect_multicast
              connect_duplex
              connect_simplex
              create_vlan
              add_vlan_ports
              remove_vlan);

use strict;

$| = 1;

use English;

# some constants
my $CLI_PROMPT = "apcon1>> ";
my $CLI_UNNAMED_PATTERN = "[Uu]nnamed";
my $CLI_UNNAMED_NAME = "unnamed";
my $CLI_NOCONNECTION = "A00";

# commands to show something
my $CLI_SHOW_CONNECTIONS = "show connections raw\r";
my $CLI_SHOW_PORT_NAMES  = "show port names *\r";


#
# parse the connection output
# return two hashes for query from either direction
#
sub parse_connections($) 
{
    my $raw = shift;

    my @lines = split( /\n/, $raw );
    my %dst = ();
    my %src = ();

    foreach my $line ( @lines ) {
	if ( $line =~ /^([A-I][0-9]{2}):\s+([A-I][0-9]{2})\W*$/ ) {
	    if ( $2 ne $CLI_NOCONNECTION ) {
		$src{$2} = $1;
		if ( ! (exists $dst{$1}) ) {
		    $dst{$1} = {};
		}

		$dst{$1}{$2} = 1;
	    }
	}
    }

    return (\%src, \%dst);
}


#
# parse the port names output
# return the port => name hashtable
#
sub parse_names($)
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
sub parse_classes($)
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
sub parse_zones($)
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
sub parse_class_ports($)
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
sub parse_zone_ports($)
{
    return parse_class_ports(@_[0]);
}


#
# helper to do CLI command and check the error msg
#
sub _do_cli_cmd($$)
{
    my ($exp, $cmd) = @_;
    my $output = "";

    $exp->clear_accum(); # Clean the accumulated output, as a rule.
    $exp->send($cmd);
    $exp->expect(10000,
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
sub get_raw_output($$)
{
    my ($exp, $cmd) = @_;
    my ($rt, $output) = _do_cli_cmd($exp, $cmd);
    if ( !$rt ) {    	
    	my $qcmd = quotemeta($cmd);
	if ( $output =~ /^($qcmd)/ ) {
	    return substr($output, length($cmd)+1);
	}		
    }

    return undef;
}


#
# get all vlans and their ports
# return the vlanname => port list hashtable
#
sub get_all_vlans($)
{
    my $exp = shift;

    my $raw = get_raw_output($exp, $CLI_SHOW_PORT_NAMES);
    my $names = parse_names($raw); 

    my %vlans = ();
    foreach my $k (keys %{$names}) {
	if ( !(exists $vlans{$names->{$k}}) ) {
	    $vlans{$names->{$k}} = ();
	}

	push @{$vlans{$names->{$k}}}, $k;
    }

    return \%vlans;
}


#
# get the vlanname of a port
#
sub get_port_vlan($$)
{
    my ($exp, $port) = @_;

    my $raw = get_raw_output($exp, "show port info $port\r");
    if ( $raw =~ /$port Name:\s+(\w+)\W*\n/ ) {
	if (  $1 !~ /$CLI_UNNAMED_PATTERN/ ) {
	    return $1;
	}
    }

    return undef;
}

#
# get the ports list of a vlan
#
sub get_vlan_ports($$)
{
    my ($exp, $vlan) = @_;
    my @ports = ();

    my $raw = get_raw_output($exp, $CLI_SHOW_PORT_NAMES);
    foreach ( split /\n/, $raw ) {
	if ( /^([A-I][0-9]{2}):\s+($vlan)\W*/ ) {
	    push @ports, $1;
	}
    }

    return \@ports;
}

#
# get connections within a vlan
# return two hashtabls whose format is same to parse_connections
#
sub get_vlan_connections($$)
{
    my ($exp, $vlan) = @_;

    my $raw_conns = get_raw_output($exp, $CLI_SHOW_CONNECTIONS);
    my ($allsrc, $alldst) = parse_connections($raw_conns);
    my $ports = get_vlan_ports($exp, $vlan);

    my %src = ();
    my %dst = ();

    #
    # There may be something special: a vlan port may connect to
    # a port that doesn't belong to the vlan. Then this connection
    # should not belong to the vlan. Till now the following codes
    # have not dealt with it yet.
    #
    # TODO: remove those connections containning ports don't belong
    #       to the vlan.
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
sub add_cls($$)
{
    my ($exp, $clsname) = @_;
    my $cmd = "add class I $clsname\r";

    return _do_cli_cmd($exp, $cmd);
}

#
# Add a new zone
#
sub add_zone($$)
{
    my ($exp, $zonename) = @_;
    my $cmd = "add zone $zonename\r";

    return _do_cli_cmd($exp, $cmd);
}

#
# Add some ports to a class
#
sub add_class_ports($$@)
{
    my ($exp, $clsname, @ports) = @_;
    my $cmd = "add class ports $clsname ".join("", @ports)."\r";

    return _do_cli_cmd($exp, $cmd);
}

# 
# Connect ports functions:
#

#
# Make a multicast connection $src --> @dsts
#
sub connect_multicast($$@)
{
    my ($exp, $src, @dsts) = @_;
    my $cmd = "connect multicast $src".join("", @dsts)."\r";

    return _do_cli_cmd($exp, $cmd);
}

#
# Make a duplex connection $src <> $dst
#
sub connect_duplex($$$)
{
    my ($exp, $src, $dst) = @_;
    my $cmd = "connect duplex $src"."$dst"."\r";

    return _do_cli_cmd($exp, $cmd);
}

#
# Make a simplex connection $src -> $dst
#
sub connect_simplex($$$)
{
    my ($exp, $src, $dst) = @_;
    my $cmd = "connect simplex $src"."$dst"."\r";

    return _do_cli_cmd($exp, $cmd);
}

#
# Create a new vlan, it actually does nothing.
# Maybe I should find a way to reserve the name.
#
sub create_vlan($$)
{
    # do nothing, but it is perfect if we can reserve the name here.
}

#
# Add some ports to a vlan, 
# it actually names those ports to the vlanname. 
#
sub add_vlan_ports($$@)
{
    my ($exp, $vlan, @ports) = @_;

    for( my $i = 0; $i < @ports; $i++ ) {    	
	my ($rt, $msg) = _do_cli_cmd($exp, 
				     "configure port name $ports[$i] $vlan\r");

	# undo set name
	if ( $rt ) {
	    for ($i--; $i >= 0; $i-- ) {	    	
		_do_cli_cmd($exp, 
			    "configure port name $ports[$i] $CLI_UNNAMED_NAME\r");
	    }
	    return $msg;
	}
    }

    return 0;
}

#
# Remove a vlan, unname the ports and disconnect them
#
sub remove_vlan($$)
{
    # TODO: not implemented yet.
}

#
# Obsoleted:
# I found a better way to name the 'vlan'.
# Ports can share the same name, so we can name
# the ports in a vlan the same name, which is the vlan name.
# However, extra work is required to parse the port names.
#
# Make a name from the ports of a VLAN
# The naming rule is "vlan"+(sorted ports), e.g.:
# A vlan has A01, A03, B02, its name is 'vlanA01A03B02'.
#
sub make_vlan_name(@)
{
    my @ports = shift;

    return "vlan".join("", sort @ports);
}
