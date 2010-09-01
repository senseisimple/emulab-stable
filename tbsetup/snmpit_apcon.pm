#!/usr/bin/perl -W

#
# EMULAB-LGPL
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for Apcon 2000 series layer 1 switch
#

package snmpit_apcon;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libtestbed;
use apcon_clilib;

#
# All functions are based on snmpit_hp class.
#
# NOTES: This device is a layer 1 switch that has no idea
# about VLAN. We use the port name on switch as VLAN name here. 
# So in this module, vlan_id and vlan_number are the same 
# thing. vlan_id acts as the port name.
#
# Another fact: the port name can't exist without naming
# a port. So we can't find a VLAN if no ports are in it.
#

#
# Creates a new object.
#
# usage: new($classname,$devicename,$debuglevel,$community)
#        returns a new object.
#
# We actually donot use SNMP, the SNMP values here, like
# community, are just for compatibility.
#
sub new($$$;$) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $name = shift;
    my $debugLevel = shift;
    my $community = shift;  

    #
    # Create the actual object
    #
    my $self = {};

    #
    # Set the defaults for this object
    # 
    if (defined($debugLevel)) {
	$self->{DEBUG} = $debugLevel;
    } else {
	$self->{DEBUG} = 0;
    }
    $self->{BLOCK} = 1;
    $self->{CONFIRM} = 1;
    $self->{NAME} = $name;

    #
    # Get config options from the database
    #
    my $options = getDeviceOptions($self->{NAME});
    if (!$options) {
	warn "ERROR: Getting switch options for $self->{NAME}\n";
	return undef;
    }

    $self->{MIN_VLAN}         = $options->{'min_vlan'};
    $self->{MAX_VLAN}         = $options->{'max_vlan'};

    if ($community) { # Allow this to over-ride the default
	$self->{COMMUNITY}    = $community;
    } else {
	$self->{COMMUNITY}    = $options->{'snmp_community'};
    }

    # other global variables
    $self->{DOALLPORTS} = 0;
    $self->{DOALLPORTS} = 1;
    $self->{SKIPIGMP} = 1;

    if ($self->{DEBUG}) {
	print "snmpit_apcon initializing $self->{NAME}, " .
	    "debug level $self->{DEBUG}\n" ;   
    }

    #
    # Create the Expect object
    #
    # We'd better set a long timeout on Apcon switch
    # to keep the connection alive.
    $self->{SESS} = createExpectObject();
    if (!$self->{SESS}) {
	warn "WARNNING: Unable to connect via SSH to $self->{NAME}\n";
	return undef;
    }

    # TODO: may need this:
    #$self->readifIndex();

    return $self;
}


#
# Set a variable associated with a port. The commands to execute are given
# in the apcon_clilib::portCMDs hash
#
# usage: portControl($self, $command, @ports)
#	 returns 0 on success.
#	 returns number of failed ports on failure.
#	 returns -1 if the operation is unsupported
#
sub portControl ($$@) {
    my $self = shift;

    my $cmd = shift;
    my @ports = @_;

    $self->debug("portControl: $cmd -> (@ports)\n");

    my $errors = 0;
    foreach my $port (@ports) { 
	my $rt = setPortRate($self->{SESS}, $port, $cmd);
	if ($rt) {
	    if ($rt =~ /^ERROR: port rate unsupported/) {
		#
                # Command not supported
		#
		$self->debug("");
		return -1;
	    }

	    $errors++;
	}
    }

    return $errors;
}


#
# Original purpose: Given VLAN indentifiers from the database, finds the 802.1Q VLAN
# number for them. If not VLAN id is given, returns mappings for the entire
# switch.
#
# On Apcon switch, no VLAN exists. So we use port name as VLAN name. This 
# funtion here actually either list all VLAN names or do the existance checking,
# which will set the mapping value to undef for nonexisted VLANs. 
# 
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to ... VLAN ids.
#        any VLANs not found have NULL VLAN numbers
#
sub findVlans($@) { 
    my $self = shift;
    my @vlan_ids = @_;
    my %mapping = ();
    my $id = $self->{NAME} . "::findVlans";
    my ($count, $name, $vlan_number, $vlan_name) = (scalar(@vlan_ids));
    $self->debug("$id\n");

    if ($count > 0) { @mapping{@vlan_ids} = undef; }

    # 
    # Get all port names, which are considered to be VLAN names.
    #
    my $vlans = getAllNamedPorts($self->{SESS});
    foreach $vlan_name (keys %{$vlans}) {
	if (!@vlan_ids || exists $mapping{$vlan_name}) {
	    $mapping{$vlan_name} = $vlan_name;
	}
    }

    return %mapping;
}


#
# See the comments of findVlans above for explanation.
#
# usage: findVlan($self, $vlan_id,$no_retry)
#        returns the VLAN number for the given vlan_id if it exists
#        returns undef if the VLAN id is not found
#
sub findVlan($$;$) { 
    my $self = shift;
    my $vlan_id = shift;

    #
    # Just for compatibility, we don't use retry for CLI.
    #
    my $no_retry = shift;
    my $id = $self->{NAME} . ":findVlan";

    $self->debug("$id ( $vlan_id )\n",2);
    
    my $ports = getNamedPorts($self->{SESS}, $vlan_id);
    if (@$ports) {
	return $vlan_id;
    }

    return undef;
}

#   
# Create a VLAN on this switch, with the given identifier (which comes from
# the database) and given 802.1Q tag number, which is ignored.
#
# usage: createVlan($self, $vlan_id, $vlan_number)
#        returns the new VLAN number on success
#        returns 0 on failure
#
sub createVlan($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $vlan_number = shift; # we ignore this on Apcon switch
    my $id = $self->{NAME} . ":createVlan";

    #
    # To acts similar to other device modules
    #
    if (!defined($vlan_number)) {
	warn "$id called without supplying vlan_number";
	return 0;
    }
    my $check_number = $self->findVlan($vlan_id,1);
    if (defined($check_number)) {
	warn "$id: recreating vlan id $vlan_id \n";
	return 0;
    }

    #
    # We do nothing here now.
    # We can be sure that the vlan_id won't be used in future for
    # another VLAN creation, but it is better if we can reserve
    # the vlan_id on switch.
    #   
    #
    # TODO: find a method to reserve the vlan_id as port name on 
    # switch. Maybe creating an empty class or zone, but then we
    # can only reserve as many as 48(or 46) names at most.
    #
    
    print "  Creating VLAN $vlan_id as VLAN #$vlan_number on " .
	    "$self->{NAME} ...\n";

    return $vlan_number;
}


#
# TODO: A+ TODO for Apcon switch. The result seems to be 'unsupported'.
# 
# gets the forbidden, untagged, and egress lists for a vlan
# sends back as a 3 element array of lists.  
#
sub getVlanLists($$) {
    my ($self, $vlan) = @_;
    my $ret = [0, 0, 0];
    return $ret;
}

#
# TODO: A+ TODO for Apcon switch. The result seems to be 'unsupported'.
#
# sets the forbidden, untagged, and egress lists for a vlan
# sends back as a 3 element array of lists.
# (Thats the order we saw in a tcpdump of vlan creation.)
# (or in some cases 6 elements for 2 vlans).
#
sub setVlanLists($@) {
    my ($self, @args) = @_;
    return 0;
}

#
# Put the given ports in the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: setPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $id = $self->{NAME} . "::setPortVlan";
    $self->debug("$id: $vlan_number ");
    $self->debug("ports: " . join(",",@ports) . "\n");
    
    $self->lock();

    # Check if ports are free
    foreach my $port (@ports) {
	if (getPortName($self->{SESS}, $port)) {
	    warn "ERROR: Port $port already in use.\n";
	    $self->unlock();
	    return 1;
	}
    }

    #
    # TODO: Shall we check whether Vlan exists or not?
    #
    
    my $errmsg = namePorts($self->{SESS}, $vlan_number, @ports);
    if ($errmsg) {
	warn "$errmsg";
	$self->unlock();
	return 1;
    }

    $self->unlock();

    return 0;
}


#
# Remove the given ports from the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: delPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub delPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    $self->debug($self->{NAME} . "::delPortVlan $vlan_number ");
    $self->debug("ports: " . join(",",@ports) . "\n");
    
    $self->lock();

    #
    # disconnect connects
    #
    my ($src, $dst) = getNamedConnections($self->{SESS}, $vlan_number);
    my %sconns = ();
    foreach my $p (@ports) {

	if (exists($src->{$p})) {
	    
	    # As destination:
	    $sconns{$p} = $src->{$p};
	} else if (exists($dst->{$p})) {
	    
	    # As source:
	    foreach my $pdst (keys %{$dst->{$p}}) {
		$sconns{$pdst} = $p;
	    }
	}
    }
    
    my $errmsg = disconnectPorts($self->{SESS}, \%sconns);
    if ($errmsg) {
	warn "$errmsg";
	$self->unlock();
	return 1;
    }
    
    $errmsg = unnamePorts($self->{SESS}, @ports);
    if ($errmsg) {
	warn "$errmsg";
	$self->unlock();
	return 1;
    }    

    $self->unlock();

    return 0;
}

#
# Disables all ports in the given VLANS. Each VLAN is given as a VLAN
# 802.1Q tag value.
#
# This version will also 'delete' the VLAN because no ports use
# the vlan name any more. Same to removeVlan().
#
# usage: removePortsFromVlan(self,@vlan)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removePortsFromVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;
    my $errors = 0;
    my $id = $self->{NAME} . "::removePortsFromVlan";

    foreach my $vlan_number (@vlan_numbers) {
	if (removePortName($self->{SESS}, $vlan_number)) {
	    $errors++;
	}
    }
    return $errors;
}

#
# Removes and disables some ports in a given VLAN.
# The VLAN is given as a VLAN 802.1Q tag value.
#
# This function is the same to delPortVlan because
# disconnect a connection will disable its two endports
# at the same time.
#
# usage: removeSomePortsFromVlan(self,vlan,@ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removeSomePortsFromVlan($$@) {
    my ($self, $vlan_number, @ports) = @_;
    return $self->delPortVlan($vlan_number, @ports);
}

#
# Remove the given VLANs from this switch. Removes all ports from the VLAN,
# The VLAN is given as a VLAN identifier from the database.
#
# usage: removeVlan(self,int vlan)
#	 returns 1 on success
#	 returns 0 on failure
#
#
sub removeVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;
    my $name = $self->{NAME};
    my $errors = 0;

    foreach my $vlan_number (@vlan_numbers) {
	if (removePortName($self->{SESS}, $vlan_number)) {
	    $errors++;
	} else {
	    print "Removed VLAN $vlan_number on switch $name.\n";	
	}	
    }

    return ($errors == 0) ? 1:0;
}



#
# Determine if a VLAN has any members 
# (Used by stack->switchesWithPortsInVlan())
#
sub vlanHasPorts($$) {
    my ($self, $vlan_number) = @_;

    my $portset = getNamedPorts($self->{SESS}, $vlan_number);
    if (@$portset) {
	return 1;
    }
    return 0;
}

#
# List all VLANs on the device
#
# usage: listVlans($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listVlans($) {
    my $self = shift;

    my @list = ();
    my $vlans = getAllNamedPorts($self->{SESS});
    foreach my $vlan_id (keys %$vlans) {
	push @list, [$vlan_id, $vlan_id, $vlans->{$vlan_id}];
    }

    return @list;

    #$self->debug($self->{NAME} .":". join("\n",(map {join ",", @$_} @list))."\n");
    return @list;
}

#
# List all ports on the device
#
# usage: listPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listPorts($) {
    my $self = shift;
    my @ports = ();

    # TODO: call getPortInfo one by one for each port
    return @ports;
}

# 
# Get statistics for ports on the switch
#
# Unsupported operation on Apcon 2000-series switch via CLI.
#
# usage: getStats($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
#
sub getStats() {
    my $self = shift;

    warn "Port statistics are unavaialble on our Apcon 2000 switch.";
    return undef;
}


#
# Get the ifindex for an EtherChannel (trunk given as a list of ports)
#
# usage: getChannelIfIndex(self, ports)
#        Returns: undef if more than one port is given, and no channel is found
#           an ifindex if a channel is found and/or only one port is given
#
sub getChannelIfIndex($@) {
    warn "ERROR: Apcon switch doesn't support trunking";
    return undef;
}


#
# Enable, or disable,  port on a trunk
#
# usage: setVlansOnTrunk(self, modport, value, vlan_numbers)
#        modport: module.port of the trunk to operate on
#        value: 0 to disallow the VLAN on the trunk, 1 to allow it
#	 vlan_numbers: An array of 802.1Q VLAN numbers to operate on
#        Returns 1 on success, 0 otherwise
#
sub setVlansOnTrunk($$$$) {
    warn "ERROR: Apcon switch doesn't support trunking";
    return 0;
}

#
# Enable trunking on a port
#
# usage: enablePortTrunking2(self, modport, nativevlan, equaltrunking)
#        modport: module.port of the trunk to operate on
#        nativevlan: VLAN number of the native VLAN for this trunk
#	 equaltrunk: don't do dual mode; tag PVID also.
#        Returns 1 on success, 0 otherwise
#
sub enablePortTrunking2($$$$) {
    warn "ERROR: Apcon switch doesn't support trunking";
    return 0;
}

#
# Disable trunking on a port
#
# usage: disablePortTrunking(self, modport)
#        modport: module.port of the trunk to operate on
#        Returns 1 on success, 0 otherwise
#
sub disablePortTrunking($$;$) {
    warn "ERROR: Apcon switch doesn't support trunking";
    return 0;
}


#
# Prints out a debugging message, but only if debugging is on. If a level is
# given, the debuglevel must be >= that level for the message to print. If
# the level is omitted, 1 is assumed
#
# Usage: debug($self, $message, $level)
#
sub debug($$;$) {
    my $self = shift;
    my $string = shift;
    my $debuglevel = shift;
    if (!(defined $debuglevel)) {
	$debuglevel = 1;
    }
    if ($self->{DEBUG} >= $debuglevel) {
	print STDERR $string;
    }
}

my $lock_held = 0;

sub lock($) {
    my $self = shift;
    my $token = "snmpit_" . $self->{NAME};
    if ($lock_held == 0) {
	my $old_umask = umask(0);
	die if (TBScriptLock($token,0,1800) != TBSCRIPTLOCK_OKAY());
	umask($old_umask);
    }
    $lock_held = 1;
}

sub unlock($) {
	if ($lock_held == 1) { TBScriptUnlock();}
	$lock_held = 0;
}


#
# Enable Openflow
#
sub enableOpenflow($$) {
    warn "ERROR: Apcon switch doesn't support Openflow now";
    return 0;
}

#
# Disable Openflow
#
sub disableOpenflow($$) {
    warn "ERROR: Apcon switch doesn't support Openflow now";
    return 0;
}

#
# Set controller
#
sub setOpenflowController($$$) {
    warn "ERROR: Apcon switch doesn't support Openflow now";
    return 0;
}

#
# Set listener
#
sub setOpenflowListener($$$) {
    warn "ERROR: Apcon switch doesn't support Openflow now";
    return 0;
}

#
# Get used listener ports
#
sub getUsedOpenflowListenerPorts($) {
    my %ports = ();

    warn "ERROR: Apcon switch doesn't support Openflow now";
    return %ports;
}


#
# Check if Openflow is supported on this switch
#
sub isOpenflowSupported($) {
    return 0;
}

# End with true
1;
