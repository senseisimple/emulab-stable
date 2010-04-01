#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for Intel EtherExpress 510T switches
#

# Special Note:
# Because this code is not urgently needed, and it is unknown if it ever
# will be needed again, it has not been debugged since moving it into 
# this module. So don't count on it. It will need to be debugged before
# actually using it for anything useful.

package snmpit_intel;
use strict;

$| = 1;				# Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

#
# These are the commands that can be passed to the portControl function
# below
#
my %cmdOIDs = 
(
    "enable" => ["ifAdminStatus","up"],
    "disable"=> ["ifAdminStatus","down"],
    "100mbit"=> [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","disable",
		 ".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed100Mbit"],
    "10mbit" => [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","disable",
		 ".1.3.6.1.4.1.343.6.10.2.4.1.10.1.1","speed10Mbit"],
    "full"   => [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","disable",
		 ".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","full"],
    "half"   => [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","disable",
		 ".1.3.6.1.4.1.343.6.10.2.4.1.11.1.1","half"],
    "auto"   => [".1.3.6.1.4.1.343.6.10.2.4.1.12.1.1","enable"]
);


#
# Creates a new object.
#
# usage: new($classname,$devicename,$debuglevel)
#        returns a new object, blessed into the snmpit_intel class.
#
sub new {

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

    if ($self->{DEBUG}) {
	print "snmpit_intel module initializing... debug level $self->{DEBUG}\n"
	;   
    }

    #
    # This variable is used to determined how deep in locking the VLAN edit
    # buffer we are. This is so that we can have 'nested' calls to vlanLock,
    # and only the first one will grab the lock from the switch, and only
    # the last call to vlanUnlock will unlock on the switch.
    #
    $self->{LOCKLEVEL} = 0;

    #
    # Set up SNMP module variables, and connect to the device
    #
    $SNMP::debugging = ($self->{DEBUG} - 2) if $self->{DEBUG} > 2;
    my $mibpath = '/usr/local/share/snmp/mibs';
    &SNMP::addMibDirs($mibpath);
    &SNMP::addMibFiles("$mibpath/SNMPv2-SMI.txt", "$mibpath/SNMPv2-TC.txt", 
	               "$mibpath/SNMPv2-MIB.txt", "$mibpath/IANAifType-MIB.txt",
		       "$mibpath/IF-MIB.txt",
		       "$mibpath/INT_GEN.MIB", 
		       "$mibpath/INT_S500.MIB",
		       "$mibpath/INT_VLAN.MIB");
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		# parses default list of Mib modules 
    $SNMP::use_enums = 1;		#use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});

    $self->{SESS} = new SNMP::Session(DestHost => $self->{NAME},
	Community => $self->{COMMUNITY});

    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "ERROR: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }

    bless($self,$class);
    return $self;
}

#
# Set a variable associated with a port. The commands to execute are given
# in the cmdOIs hash above
#
# usage: portControl($self, $command, @ports)
#        returns 0 on success.
#        returns number of failed ports on failure.
#        returns -1 if the operation is unsupported
#
sub portControl($$@) {
    my $self = shift;

    my $cmd = shift;
    my @ports = @_;

    $self->debug("portControl: $cmd -> (@ports)\n");

    my @p = map { portnum($_) } @ports;

    #
    # Find the command in the %cmdOIDs hash (defined at the top of this file).
    # Some commands involve multiple SNMP commands, so we need to make sure
    # we get all of them
    #
    if (defined $cmdOIDs{$cmd}) {
	my @oid = @{$cmdOIDs{$cmd}};
	my $errors = 0;

	while (@oid) {
	    my $myoid = shift @oid;
	    my $myval = shift @oid;
	    $errors += $self->UpdateField($myoid,$myval,@ports);
	}
	return $errors;
    } else {
	#
	# Command not supported
	#
	print STDERR "Unsupported port control command '$cmd' ignored.\n";
	return -1;
    }
}

#
# Obtain a lock on the VLAN edit buffer. This must be done before VLANs
# are created or removed, or ports are assigned to VLANs. Will retry 60
# times before failing
#
# usage: vlanLock($self)
#        returns 1 on success
#        returns 0 on failure
#
sub vlanLock($) {
    my $self = shift;

    my $TokenOwner = '.1.3.6.1.4.1.343.6.11.4.5';
    my $TokenReq = '.1.3.6.1.4.1.343.6.11.4.6';
    my $TokenReqResult = '.1.3.6.1.4.1.343.6.11.4.7';
    my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
    my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';

    #
    # For nested locks - if we're called, but already locked, we just
    # report sucess and increase the lock level
    #
    if ($self->{LOCKLEVEL}) {
	$self->debug("vlanLock - already at lock level $self->{LOCKLEVEL}\n");
	$self->{LOCKLEVEL}++;
	return 1;
    }
    $self->{LOCKLEVEL} = 1;

    #
    # Some magic needed by the Intel switch
    #
    my $Num = pack("C*",0,0,0,0,1,1);

    #
    # Try max_tries times before we give up, in case some other process just
    # has it locked.
    #
    my $tries = 1;
    my $max_tries = 60;
    while ($tries <= $max_tries) {

	#
	# Attempt to grab the edit buffer
	#
	$self->{SESS}->set([[$TokenReq,0,$Num,"OCTETSTR"]]);
	my $RetVal = $self->{SESS}->get([[$TokenReqResult,0]]);
	while ($RetVal eq "notReady") {
	    $RetVal = $self->{SESS}->get([[$TokenReqResult,0]]);
	    $self->debug("*VLAN Token Claim Result is $RetVal\n");
	    select (undef, undef, undef, .25); #wait 1/4 second
	}

	$self->debug("VLAN Token Claim Result is $RetVal\n");

	if ($RetVal ne 'success') {
	    #
	    # Only print this message every five tries
	    #
	    if (!($tries % 5)) {
		print STDERR "VLAN edit buffer request failed - " .
		"try $tries of $max_tries.\n";
	    }
	} else {
	    last;
	}
	$tries++;

	sleep(1);
    }

    if ($tries > $max_tries) {
	#
	# Admit defeat and exit
	#
	print STDERR "ERROR: Failed to obtain VLAN edit buffer lock\n";
	return 0;
    } else {
	return 1;
    }

}

#
# Release a lock on the VLAN edit buffer. As part of releasing, applies the
# VLAN edit buffer. If $self->{CONFIRM} is set, uses the saveWithConfirmOption,
# which requires the porcess to confirm the changes, or they are cancelled
# (so that you don't accidentally cut yourself off from the switch.)
#
# usage: vlanUnlock($self)
#        
sub vlanUnlock($;$) {
    my $self = shift;

    my $TokenRelease = '.1.3.6.1.4.1.343.6.11.4.8';
    my $TokenReleaseResult = '.1.3.6.1.4.1.343.6.11.4.9';
    my $TokenConfirmState = '.1.3.6.1.4.1.343.6.11.1.18';

    #
    # Allow for nested locks - only release the lock if our locklevel
    # is down to 1 (ie. we're the last level that cares about the lock)
    #
    if ($self->{LOCKLEVEL} > 1) {
	$self->debug("vlanUnock - at lock level $self->{LOCKLEVEL}\n");
	$self->{LOCKLEVEL}--;
	return 1;
    }
    $self->{LOCKLEVEL} = 0;

    my $save = ($self->{CONFIRM} ? "saveWithConfirmOption" : "save");

    #
    # Relse the token
    #
    $self->debug("Releasing Token with $save command\n");
    $self->{SESS}->set([[$TokenRelease,0,$save,"INTEGER"]]);
    my $RetVal = $self->{SESS}->get([[$TokenReleaseResult,0]]);
    $self->debug("VLAN Configuration Save Result is $RetVal\n");

    #
    # Wait for the switch to report that the token has been released
    #
    while ($RetVal eq "notReady") {
	$RetVal = $self->{SESS}->get([[$TokenReleaseResult,0]]);
	$self->debug("VLAN Configuration Save Result is $RetVal\n");
	select (undef, undef, undef, .25); #wait 1/4 second
    }

    if ($self->{CONFIRM}) {

	#
	# Wait for the switch to report that it's ready to recieve
	# confirmation of the release
	#
	$RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	$self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	while ($RetVal eq "notReady") {
	    sleep(2);
	    $RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	}

	#
	# Confirm the release
	#
	if ($RetVal eq "ready") {
	    $RetVal = $self->{SESS}->set([[$TokenConfirmState,0,"confirm",
		"INTEGER"]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	} # XXX: Should there be an 'else'?

	#
	# Wait for the switch to confirm the confirmation
	#
	while (!($RetVal =~ /Conf/i)) {
	    $RetVal = $self->{SESS}->get([[$TokenConfirmState,0]]);
	    $self->debug("VLAN Configuration Confirm Result is $RetVal\n");
	}

	#
	# Make sure the confirmation was confirmed
	#
	if ($RetVal ne "confirmedNewConf") {
	    warn("VLAN Reconfiguration Failed ($RetVal). No changes saved.\n");
	    return 0;
	}

    } else { # if $self->{CONFIRM}
	#
	# Just check the return value
	#
	if ($RetVal ne 'success') {
	    warn("VLAN Reconfiguration Failed: $RetVal. No changes saved.\n");
	    return 0;
	}
    }
}

#
# Given VLAN indentifiers from the database, finds the Intel-specific VLAN
# number for them. If not VLAN id is given, returns mappings for the entire
# switch.
# 
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to Intel VLAN numbers
#        any VLANs not found have NULL VLAN numbers
#
sub findVlans($@) { 
    my $self = shift;
    my @vlan_ids = @_;

    my $field = ["policyVlanName",0];

    my %mapping = ();
    @mapping{@vlan_ids} = undef;

    #
    # Find all VLAN names. Do one to get the first field...
    #
    $self->{SESS}->getnext($field);
    my ($name,$vlan_number,$vlan_name);
    do {
	($name,$vlan_number,$vlan_name) = @{$field};
	$self->debug("findVlan: Got $name $vlan_number $vlan_name\n");

	#
	# We only want the names - we ignore everything else
	#
	if ($name =~ /policyVlanName/) {
	    if (!@vlan_ids || exists $mapping{$vlan_name}) {
		$self->debug("Putting in mapping from $vlan_name to " .
		    "$vlan_number\n",2);
		$mapping{$vlan_name} = $vlan_number;
	    }
	}

	$self->{SESS}->getnext($field);
    } while ( $name =~ /^policyVlanName/) ;

    return %mapping;
}

#
# Given a VLAN identifier from the database, find the Intel-specific VLAN
# number that is assigned to that VLAN. Retries several times (to account
# for propagation delays) unless the $no_retry option is given.
#
# usage: findVlan($self, $vlan_id,$no_retry)
#        returns the VLAN number for the given vlan_id if it exists
#        returns undef if the VLAN id is not found
#
sub findVlan($$;$) { 
    my $self = shift;
    my $vlan_id = shift;
    my $no_retry = shift;

    my $max_tries;
    if ($no_retry) {
	$max_tries = 1;
    } else {
	$max_tries = 10;
    }

    #
    # We try this a few time, with 1 second sleeps, since it can take
    # a while for VLAN information to propagate
    #
    foreach my $try (1 .. $max_tries) {

	my %mapping = $self->findVlans($vlan_id);
	if (defined($mapping{$vlan_id})) {
	    return $mapping{$vlan_id};
	}

	#
	# Wait before we try again
	#
	if ($try != $max_tries) {
	    $self->debug("VLAN find failed, trying again\n");
	    sleep 1;
	}
    }
    #
    # Didn't find it
    #
    return undef;
}

#   
# Create a VLAN on this switch, with the given identifier (which comes from
# the database.) Picks its own switch-specific VLAN number to use. If ports
# are given, puts them in the newly-created VLAN.
#
# usage: createVlan($self, $vlan_id,@ports)
#        returns 1 on success
#        returns 0 on failure
#
sub createVlan($$;@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;

    my $okay = 0;

    if (!$self->vlanLock()) {
	return 0;
    }

    my $NextVLANId = '.1.3.6.1.4.1.343.6.11.1.6';
    my $CreateOID = ".1.3.6.1.4.1.343.6.11.1.9.1.3";
    my $RetVal = "Undef.";

    #
    # Intel provides us with a handy way to pick a VLAN number
    # NOTE: This will respect the MAX_VLAN variable, but not the MIN_VLAN
    # one
    #
    my $vlan_number = $self->{SESS}->get([[$NextVLANId,0]]);
    if ($vlan_number > $self->{MAX_VLAN}) {
	print STDERR "ERROR: Returned VLAN was too high ($vlan_number)\n";
	return 0;
    }

    #
    # Pick a name if one was not given
    #
    if (!$vlan_id) {
	$vlan_id = $vlan_number;
    }

    print "  Creating VLAN $vlan_id as VLAN #$vlan_number ... ";
    $RetVal = $self->{SESS}->set([[$CreateOID,$vlan_number,$vlan_id,
	"OCTETSTR"]]);
    if ($RetVal) {
	$okay = 1;
	print "Succeeded\n";
    } else {
	print "Failed\n";
	$okay = 0;
    }

    #
    # If we wre given any ports, put them into the VLAN now, so that
    # we can do this without unlocking/relocking.
    #
    if ($okay && @ports) {
	$self->debug("CreateVlan: port list is ".join(",",@ports)."\n");
	if ($self->setPortVlan($vlan_id,@ports)) {
	    $okay = 0;
	}
    }

    $self->vlanUnlock();

    return $okay;

}

#
# Put the given ports in the given VLAN. The VLAN is given as a VLAN
# identifier from the database.
#
# usage: setPortVlan($self,$vlan_id, @ports)
#        returns 0 on sucess.
#        returns the number of failed ports on failure.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;

    my $errors = 0;
           
    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 1;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #
    $self->debug("Raw port list: " . join(",",@ports) . "\n");
    my @portlist = map { portnum($_) } @ports;

    if (!$self->vlanLock()) {
	return 1;
    }

    foreach my $port (@portlist) {
	#
	# The value returned from portmap looks like: intel1:1.1 . It's
	# only the last part, the port number, that we acutally care about
	# (since intels don't have multiple cards, every port is considered
	# to be on port 1)
	#
	$port =~ /:1\.(\d+)$/;
	$port = $1;

	# 
	# Make sure the port didn't get mangled in conversion
	#
	if (!defined $port) {
	    print STDERR "Port not found, skipping\n";
	    $errors++;
	    next;
	}

	$self->debug("Putting port $port in VLAN $vlan_number\n");

	# 
	# Remove the port from all VLANs it's currently in (or we can end
	# up with it in multiple VLANs)
	#
	$self->removePortFromVlans($port);

	#
	# Do the acutal SNMP command
	#
	my $policyPortRuleCreateObj = "policyPortRuleCreateObj";
	my $RetVal = $self->{SESS}->set([$policyPortRuleCreateObj,
	    "$vlan_number.$port",$vlan_number,'OCTETSTR']);

	if (!$RetVal) {
	    print STDERR "$port VLAN change failed with $RetVal.\n";
	    $errors++;
	    next;
	}

	#
	# Ports going into the System vlan are being taken out of circulation,
	# so we disable them. Otherwise, we need to make sure they get enabled.
	#
	if ($vlan_id eq "System") {
	    $self->debug("Disabling " . join(',',@ports) . "...");
	    if ( my $rv = $self->portControl("disable",@ports) ) {
		print STDERR "Port disable had $rv failures.\n";
		$errors += $rv;
	    }
	} else {
	    $self->debug("Enabling "  . join(',',@ports) . "...");
	    if ( my $rv = $self->portControl("enable",@ports) ) {
		print STDERR "Port enable had $rv failures.\n";
		$errors += $rv;
	    }
	}
    }

    $self->vlanUnlock();

    return $errors;

}

#
# Remove a port from all VLANs of which it it currently a member
#
# usage: removePortFromVlans(self,int port)
#	 returns 0 on sucess.
#	 returns the number of failed VLANs on failure.
#
sub removePortFromVlans($$) {
    my $self = shift;
    my $port_number = shift;

    my $errors = 0;

    $self->debug("removePortFromVlans($port_number) called\n");
    my $field = ["policyPortRuleDeleteObj",0];

    if (!$self->vlanLock()) {
	return 1;
    }

    #
    # Do one to get the first field...
    #
    $self->{SESS}->getnext($field);
    my ($name,$index,$value);
    do {
	($name,$index,$value) = @{$field};
	$self->debug("removePortFromVlans: Got $name $index $value\n");

	if ($name eq "policyPortRuleDeleteObj") {
	    #
	    # This table is indexed by vlan.port
	    #
	    $index =~ /(\d+)\.(\d+)/;
	    my ($vlan,$port) = ($1,$2);

	    #
	    # If this was the port we're looking for, nuke it!
	    #
	    if ($port == $port_number) {
		my $RetVal = $self->{SESS}->set(["policyPortRuleDeleteObj",
		    $index,1,"INTEGER"]);
		if (!$RetVal) {
		    $errors++;
		}
	    }
	}
	$self->{SESS}->getnext($field);

    } while ( $name =~ /^policyPortRuleDeleteObj/) ;

    $self->vlanUnlock();

    return $errors;

}

#
# Remove all ports from the given VLAN. The VLAN is given as a VLAN
# identifier from the database.
#
# usage: removePortsFromVlan(self,int vlan)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removePortsFromVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    my $errors = 0;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 1;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    if (!$self->vlanLock()) {
	return 1;
    }

    my $field = ["policyPortRuleDeleteObj",0];

    #
    # Do one to get the first field...
    #
    $self->{SESS}->getnext($field);
    my ($name,$index,$value);
    do {
	($name,$index,$value) = @{$field};
	if ($name eq "policyPortRuleDeleteObj") {
	    $self->debug("removePortsFromVlan: Got $name $index $value\n");

	    #
	    # This table is indexed by vlan.port
	    #
	    $index =~ /(\d+)\.(\d+)/;
	    my ($vlan,$port) = ($1,$2);

	    if ($vlan == $vlan_number) {
		#
		# Remove the port from this VLAN
		#
		$self->debug("Removing $port from vlan $vlan\n");
		my $RetVal = $self->{SESS}->set(["policyPortRuleDeleteObj",
		    $index,1,"INTEGER"]);
		if (!$RetVal) {
		    $errors++;
		}

		#
		# Put the port in the System VLAN. We have to translate the
		# port into node:port form since that's what setPortVlan
		# expects.
		#
		my $portnum = portnum("$self->{NAME}:1.$port");
		$self->debug("Calling setPortVlan(System,$portnum)");
		my $errors += $self->setPortVlan("System",$portnum);
	    }
	}
	$self->{SESS}->getnext($field);

    } while ( $name =~ /^policyPortRuleDeleteObj/) ;

    $self->vlanUnlock();

    return $errors;

}

#
# Remove the given VLAN from this switch. Removes all ports from the VLAN,
# so it's not necessary to call removePortsFromVlan() first. The VLAN is
# given as a VLAN identifier from the database.
#
# usage: removeVlan(self,int vlan)
#        returns 1 on success
#        returns 0 on failure
#
#
sub removeVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 0;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Need to lock the VLAN edit buffer
    #
    if (!$self->vlanLock()) {
	return 0;
    }

    #
    # First, remove all ports from this VLAN
    #
    if ($self->removePortsFromVlan($vlan_id)) {
	#
	# Uh oh, something went wrong. Unlock and bail
	#
	$self->vlanUnlock();
	return 0;
    }

    #
    # Perform the actual removal
    #
    my $DeleteOID = ".1.3.6.1.4.1.343.6.11.1.9.1.4";
    my $RetVal = "Undef.";

    print "  Removing VLAN #$vlan_number ... ";
    $RetVal = $self->{SESS}->set([[$DeleteOID,$vlan_number,"delete",
	"INTEGER"]]);
    my $ok;
    if ($RetVal) {
	print "Succeeded.\n";
	$ok = 1;
    } else {
	print "Failed.\n";
	$ok = 0;
    }

    #
    # Unlock whether successful or not
    #
    $self->vlanUnlock();

    if (! defined $RetVal) {
	print STDERR "VLAN #$vlan_number does not exist on this switch.\n";
    }

    return $ok;
}

#
# XXX: Major cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    my ($OID,$val,@ports)= @_;

    my $Status = 0;
    my $err = 0;

    $self->debug("UpdateField: '@_'\n");

    foreach my $port (@ports) {
	#
	# Check the input format - the ports might already be in
	# switch:module.port form, or we may have to convert them to it with
	# portnum
	#
	if ($port =~ /:1\.(\d+)/) {
	    $port = $1;
	} else {
	    $port = portnum($port);
	    $port =~ /:1\.(\d+)/;
	    $port = $1;
	}
	$self->debug("Checking port $port for $val...");
	$Status = $self->{SESS}->get([$OID,$port]);
	if (!defined $Status) {
	    print STDERR "Port $port, change to $val: No answer from device\n";
	    return -1;			# Return error
	} else {
	    $self->debug("Okay.\n");
	    $self->debug("Port $port was $Status\n");
	    if ($Status ne $val) {
		$self->debug("Setting $port to $val...");
		# The empty sub {} is there to force it into async mode
		$self->{SESS}->set([[$OID,$port,$val,"INTEGER"]],sub {});
		if ($self->{BLOCK}) {
		    while ($Status ne $val) { 
			$Status = $self->{SESS}->get([[$OID,$port]]);
			$self->debug("Value for $port was $Status\n");
			# XXX: Timeout
		    }
		    #print "Okay.\n";
		} else {
		    #print "\n";
		}
	    }
	}

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

    my %Names = ();
    my %Members = ();

    #
    # First, we grab the names of all VLANs
    #
    my $field = ["policyVlanName",0];
    my ($varname,$index,$vlan_number);
    $self->{SESS}->getnext($field);
    do {
	($varname,$index,$vlan_number) = @{$field};
	$self->debug("listVlans: Got $varname $index $vlan_number\n");
	if ($varname =~ /policyVlanName/) {
	    $Names{$index} = $vlan_number
	}
	$self->{SESS}->getnext($field);
    } while ( $varname eq "policyVlanName");

    #
    # Next, we find membership, by port
    #
    $field = ["policyPortRuleVlanId",0];
    $self->{SESS}->getnext($field);
    do {
	($varname,$index,$vlan_number) = @{$field};
	$self->debug("listVlans: Got $varname $index $vlan_number\n");
	if ($varname =~ /policyPortRuleVlanId/) {
	    #
	    # This table is indexed by vlan.port
	    #
	    $index =~ /(\d+)\.(\d+)/;
	    my $port = $2;

	    #
	    # Find out the real node name. We just call it portX if
	    # it doesn't seem to have one.
	    #
	    my $node;
	    ($node = portnum($self->{NAME} . ":1." . $port)) ||
			($node = "port$port");

	    push @{$Members{$vlan_number}}, $node;
	}

	$self->{SESS}->getnext($field);

    } while ( $varname eq "policyPortRuleVlanId" );

    #
    # Build a list from the name and membership lists
    #
    my @list = ();

    foreach my $vlan_id ( sort {$a <=> $b} keys %Names ) {
	#
	# Special case - we don't want to list the System VLAN, as that's
	# where ports that are not in use go. (This is the same as ignoring
	# VLAN #1 on Cisco's
	#
	next if ($Names{$vlan_id} eq "System");

	#
	# Check for members to avoid pushing an undefined value into
	# the array (which causes warnings elsewhere)
	#
	if ($Members{$vlan_id}) {
	    push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
	} else {
	    push @list, [$Names{$vlan_id},$vlan_id,[]];
	}
    }

    $self->debug("LIST:\n".join("\n",@list)."\n",2);
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

    my %Able = ();
    my %Link = ();
    my %auto = ();
    my %speed = ();
    my %duplex = ();

    # XXX: do these get in the way?
    my $MinPorts = 1;
    my $MaxPorts = 24;

    my $ifTable = ["ifAdminStatus",0];

    #
    # Get the ifAdminStatus (enabled/disabled) and ifOperStatus
    # (up/down)
    #
    my ($varname, $port, $status);
    $self->{SESS}->getnext($ifTable);
    do {
	($varname,$port,$status) = @{$ifTable};
	$self->debug("$varname $port $status\n");
	if (($port >= $MinPorts) && ($port <= $MaxPorts)) {
	    if ($varname =~ /AdminStatus/) { 
		$Able{$port} = ($status =~/up/ ? "yes" : "no");
	    }
	    if ($varname =~ /OperStatus/) { 
		$Link{$port} = $status;
	    } 
	}
	$self->{SESS}->getnext($ifTable);
    } while ( $varname =~ /^i(f)(.*)Status$/) ;

    #
    # Get the port configuration, including speed, duplex, and whether or not
    # it is autoconfiguring
    #
    my $portConf = ["portConfSpeed",0];
    $self->{SESS}->getnext($portConf);
    do {
	$self->debug("$varname $port $status\n");
	($varname,$port,$status) = @{$portConf};
	$port =~ s/\./:/g;
	$port = (split(":",$port))[2];
	if (($port >= $MinPorts) && ($port <= $MaxPorts)) {
	    if ($varname =~ /Speed/) {
		$status =~ s/speed//;
		$status =~ s/autodetect/\(auto\)/i;
		$speed{$port} = $status;
	    } elsif ($varname =~ /Duplex/) { 
		$status =~ s/autodetect/\(auto\)/i;
		$duplex{$port} = $status;
	    } elsif ($varname =~ /AutoNeg/) {
		$auto{$port} = $status;
	    } 
	}
	$self->{SESS}->getnext($portConf);
    } while ( $varname =~ /^portConf(Speed|Duplex|AutoNeg)$/);

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %Able ) {
	my $port = portnum($self->{NAME} . ":1." . $id);

	#
	# Skip ports that don't seem to have anything interesting attached
	#
	if (!$port) {
	    $self->debug("$id does not seem to be connected, skipping\n");
	    next;
	}
	if (! defined ($speed{$id}) ) { $speed{$id} = " "; }
	if (! defined ($duplex{$id}) ) { $duplex{$id} = " "; }
	push @rv, [$port,$Able{$id},$Link{$id},$speed{$id},$duplex{$id}];
    }
    return @rv;

}

#   
# Get statistics for ports on the switch
#   
# usage: getPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
#
sub getStats($) {
    my $self = shift;

    my $MinPorts = 1;
    my $MaxPorts = 24;

    my $ifTable = ["ifInOctets",0];
    my %inOctets=();
    my %inUcast=();
    my %inNUcast=();
    my %inDiscard=();
    my %inErr=();
    my %inUnkProt=();
    my %outOctets=();
    my %outUcast=();
    my %outNUcast=();
    my %outDiscard=();
    my %outErr=();
    my %outQLen=();

    #
    # Walk the whole stats tree, and fill these ha
    #
    $self->{SESS}->getnext($ifTable);
    my ($varname, $port, $value);
    do {
	($varname,$port,$value) = @{$ifTable};
	$self->debug("getStats: Got $varname, $port, $value\n");
	if (($port >= $MinPorts) && ($port <= $MaxPorts)) {
	    if ($varname =~ /InOctets/) {
		$inOctets{$port} = $value;
	    } elsif ($varname =~ /InUcast/) {
		$inUcast{$port} = $value;
	    } elsif ($varname =~ /InNUcast/) {
		$inNUcast{$port} = $value;
	    } elsif ($varname =~ /InDiscard/) {
		$inDiscard{$port} = $value;
	    } elsif ($varname =~ /InErrors/) {
		$inErr{$port} = $value;
	    } elsif ($varname =~ /InUnknownP/) {
		$inUnkProt{$port} = $value;
	    } elsif ($varname =~ /OutOctets/) {
		$outOctets{$port} = $value;
	    } elsif ($varname =~ /OutUcast/) {
		$outUcast{$port} = $value;
	    } elsif ($varname =~ /OutNUcast/) {
		$outNUcast{$port} = $value;
	    } elsif ($varname =~ /OutDiscard/) {
		$outDiscard{$port} = $value;
	    } elsif ($varname =~ /OutErrors/) {
		$outErr{$port} = $value;
	    } elsif ($varname =~ /OutQLen/) {
		$outQLen{$port} = $value;
	    }
	}
	$self->{SESS}->getnext($ifTable);
    } while ( $varname =~ /^i[f](In|Out)/) ;

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %inOctets ) {
	my $port = portnum($self->{NAME} . ":1." . $id);

	#
	# Skip ports that don't seem to have anything interesting attached
	#
	if (!$port) {
	    $self->debug("$id does not seem to be connected, skipping\n");
	    next;
	}
	push @rv, [$port,$inOctets{$id},$inUcast{$id},$inNUcast{$id},
		$inDiscard{$id},$inErr{$id},$inUnkProt{$id},$outOctets{$id},
		$outUcast{$id},$outNUcast{$id},$outDiscard{$id},$outErr{$id},
		$outQLen{$id}];

    }
    return @rv;

}

#
# usage: getFields(self,ports,oids)
#        ports: Reference to a list of ports, in any allowable port format
#        oids: A list of OIDs to reteive values for
#
# On sucess, returns a two-dimensional list indexed by port,oid
#
sub getFields($$$) {
    my $self = shift;
    my ($ports,$oids) = @_;


    my @ifindicies = map { portnum($_) =~ /:1\.(\d+)$/; $1 } @$ports;
    my @oids = @$oids;

    
    #
    # Put together an SNMP::VarList for all the values we want to get
    #
    my @vars = ();
    foreach my $ifindex (@ifindicies) {
	foreach my $oid (@oids) {
	    push @vars, ["$oid","$ifindex"];
	}
    }

    #
    # If we try to ask for too many things at once, we get back really bogus
    # errors. So, we limit ourselves to an arbitrary number that, by
    # experimentation, works.
    #
    my $maxvars = 16;
    my @results = ();
    while (@vars) {
	my $varList = new SNMP::VarList(splice(@vars,0,$maxvars));
	my $rv = $self->{SESS}->get($varList);
	push @results, @$varList;
    }
	    
    #
    # Build up the two-dimensional list for returning
    #
    my @return = ();
    foreach my $i (0 .. $#ifindicies) {
	foreach my $j (0 .. $#oids) {
	    my $val = shift @results;
	    $return[$i][$j] = $$val[2];
	}
    }

    return @return;
}

#   
# Prints out a debugging message, but only if debugging is on. If a level is
# given, the debuglevel must be >= that level for the message to print. If
# the level is omitted, 1 is assumed
#   
# Usage: debug($self, $message, $level)
#   
sub debug($$:$) {
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

#
# Enable Openflow
#
sub enableOpenflow($$) {
    my $self = shift;
    my $vlan = shift;
    my $RetVal;
    
    #
    # Intel switch doesn't support Openflow yet.
    #
    warn "ERROR: Intel swith doesn't support Openflow now";
    return 0;
}

#
# Disable Openflow
#
sub disableOpenflow($$) {
    my $self = shift;
    my $vlan = shift;
    my $RetVal;
    
    #
    # Intel switch doesn't support Openflow yet.
    #
    warn "ERROR: Intel swith doesn't support Openflow now";
    return 0;
}

#
# Set controller
#
sub setController($$$) {
    my $self = shift;
    my $vlan = shift;
    my $controller = shift;
    my $RetVal;
    
    #
    # Intel switch doesn't support Openflow yet.
    #
    warn "ERROR: Intel swith doesn't support Openflow now";
    return 0;
}

#
# Set listener
#
sub setListener($$$) {
    my $self = shift;
    my $vlan = shift;
    my $listener = shift;
    my $RetVal;
    
    #
    # Intel switch doesn't support Openflow yet.
    #
    warn "ERROR: Intel swith doesn't support Openflow now";
    return 0;
}

#
# Check if Openflow is supported on this switch
#
sub isOpenflowSupported($) {
    #
    # Intel switch doesn't support Openflow yet.
    #
    return 0;
}


# End with true
1;

