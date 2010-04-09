#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for Cisco Catalyst 6509 switches
#
# TODO: Standardize returning 0 on success/failure
# TODO: Fix uninitialized variable warnings in getStats()
#

package snmpit_cisco;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;
use Socket;
use libtestbed;

#
# These are the commands that can be passed to the portControl function
# below
#
my %cmdOIDs =
(
    "enable"  => ["ifAdminStatus","up"],
    "disable" => ["ifAdminStatus","down"],
    "1000mbit"=> ["portAdminSpeed","s1000000000"],
    "100mbit" => ["portAdminSpeed","s100000000"],
    "10mbit"  => ["portAdminSpeed","s10000000"],
    "full"    => ["portDuplex","full"],
    "half"    => ["portDuplex","half"],
    "auto"    => ["portAdminSpeed","autoDetect",
		 "portDuplex","auto"]
);

#
# Ports can be passed around in three formats:
# ifindex: positive integer corresponding to the interface index (eg. 42)
# modport: dotted module.port format, following the physical reality of
#	Cisco switches (eg. 5.42)
# nodeport: node:port pair, referring to the node that the switch port is
# 	connected to (eg. "pc42:1")
#
# See the function convertPortFormat below for conversions between these
# formats
#
my $PORT_FORMAT_IFINDEX  = 1;
my $PORT_FORMAT_MODPORT  = 2;
my $PORT_FORMAT_NODEPORT = 3;

#
# Creates a new object.
#
# usage: new($classname,$devicename,$debuglevel,$community)
#        returns a new object, blessed into the snmpit_cisco class.
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
    $self->{BULK} = 1;
    $self->{NAME} = $name;

    #
    # Get config options from the database
    #
    my $options = getDeviceOptions($self->{NAME});
    if (!$options) {
	warn "ERROR: Getting switch options for $self->{NAME}\n";
	return undef;
    }

    $self->{SUPPORTS_PRIVATE} = $options->{'supports_private'};
    $self->{MIN_VLAN}         = $options->{'min_vlan'};
    $self->{MAX_VLAN}         = $options->{'max_vlan'};

    if (($self->{MAX_VLAN} > 1024) && ($self->{MIN_VLAN} < 1000)) {
	warn "ERROR: Some Cisco switches forbid creation of user vlans ".
	     "with 1000 < vlan number <= 1024\n";
	return undef;
    }

    if ($community) { # Allow this to over-ride the default
	$self->{COMMUNITY}    = $community;
    } else {
	$self->{COMMUNITY}    = $options->{'snmp_community'};
    }

    #
    # We have to change our behavior depending on what OS the switch runs
    #
    if (!($options->{'type'} =~ /^(\w+)(-modhack(-?))?(-ios)?$/)) {
	warn "ERROR: Incorrectly formatted switch type name: ",
	     $options->{'type'}, "\n";
	return undef;
    }
    $self->{SWITCHTYPE} = $1;
    if (!$self->{SWITCHTYPE}) {
	warn "ERROR: Unable to determine type of switch $self->{NAME} from " .
             "string '$options->{type}'\n";
	return undef;
    }

    if ($2) {
        $self->{NON_MODULAR_HACK} = 1;
    } else {
        $self->{NON_MODULAR_HACK} = 0;
    }

    if ($4) {
	$self->{OSTYPE} = "IOS";
    } else {
	$self->{OSTYPE} = "CatOS";
    }

    if ($self->{DEBUG}) {
	print "snmpit_cisco module initializing... debug level $self->{DEBUG}\n";
    }

    #
    # Find the class of switch - look for 4 digits in the switch type
    #
	if ($self->{SWITCHTYPE} =~ /(\d{2})(\d{2})/) {
	   # Special case, that 2960
	   if ($1 == "29" && $2 == "60") {
	      $self->{SWITCHCLASS} = "2960";
	   } else {
		  $self->{SWITCHCLASS} = "${1}00";
	   }
    } else {
        warn "snmpit: Unable to determine switch class for $name\n";
        $self->{SWITCHCLASS} = "6500";
    }
    if ($self->{DEBUG}) {
	print "snmpit_cisco picked class $self->{SWITCHCLASS}\n";
    }

    #
    # Set up SNMP module variables, and connect to the device
    #
    $SNMP::debugging = ($self->{DEBUG} - 2) if $self->{DEBUG} > 2;
    my $mibpath = '/usr/local/share/snmp/mibs';
    &SNMP::addMibDirs($mibpath);
    # We list all MIBs we use, so that we don't depend on a correct .index file
    my @mibs = ("$mibpath/SNMPv2-SMI.txt", "$mibpath/SNMPv2-TC.txt",
	    "$mibpath/SNMPv2-MIB.txt", "$mibpath/IANAifType-MIB.txt",
	    "$mibpath/IF-MIB.txt", "$mibpath/RMON-MIB.txt",
	    "$mibpath/CISCO-SMI.txt", "$mibpath/CISCO-TC.txt",
	    "$mibpath/CISCO-VTP-MIB.txt", "$mibpath/CISCO-PAGP-MIB.txt",
	    "$mibpath/CISCO-PRIVATE-VLAN-MIB.txt");
	    
    if ($self->{OSTYPE} eq "CatOS") {
	push @mibs, "$mibpath/CISCO-STACK-MIB.txt";
        # The STACK mib contains some code for copying config via TFTP
        $self->{TFTPWRITE} = 1;
    } elsif ($self->{OSTYPE} eq "IOS") {
	push @mibs, "$mibpath/CISCO-STACK-MIB.txt",
                    "$mibpath/CISCO-VLAN-MEMBERSHIP-MIB.txt";
        # Backwards compatability: for some reason, some older installations
        # seem to have a different filename for this file. The version of
        # the filename ending in '-MIB' is the "correct" one, but try
        # loading the older file if they don't have the newer one. If they
        # don't have either one, we'll not fail here, only when they try to
        # acutally use this MIB, and most sites won't actually use it.
        if (-e "$mibpath/CISCO-CONFIG-COPY-MIB.txt") {
            push @mibs, "$mibpath/CISCO-CONFIG-COPY-MIB.txt";
            $self->{TFTPWRITE} = 1;
        } elsif (-e "$mibpath/CISCO-CONFIG-COPY.txt") {
            push @mibs, "$mibpath/CISCO-CONFIG-COPY.txt";
            $self->{TFTPWRITE} = 1;
        } else {
            $self->{TFTPWRITE} = 0;
        }
    } else {
	warn "ERROR: Unsupported switch OS $self->{OSTYPE}\n";
	return undef;
    }

    if ($self->{SWITCHCLASS} == 2900) {
        # There is a special MIB with some 2900 stuff in it
	push @mibs, "$mibpath/CISCO-C2900-MIB.txt";
    }

    &SNMP::addMibFiles(@mibs);
    
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		  # parses default list of Mib modules 
    $SNMP::use_enums = 1;	  # use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});
    $self->{SESS} =
	    new SNMP::Session(DestHost => $self->{NAME},Version => "2c",
		    Community => $self->{COMMUNITY});
    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "ERROR: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }

    #
    # Connecting an SNMP session doesn't necessarily mean you can actually get
    # packets to and from the switch. Test that by grabbing an OID that should
    # be on every switch. Let it retry a bunch, to hide transient failures
    #

    my $OS_details = snmpitGetFatal($self->{SESS},["sysDescr",0],30);
    print "Switch $self->{NAME} is running $OS_details\n" if $self->{DEBUG};

    #
    # The bless needs to occur before readifIndex(), since it's a class 
    # method
    #
    bless($self,$class);

    $self->readifIndex();

    return $self;
}

#
# Set a variable associated with a port. The commands to execute are given
# in the cmdOIs hash above
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

    #
    # Find the command in the %cmdOIDs hash (defined at the top of this file)
    #
    if (defined $cmdOIDs{$cmd}) {
	my @oid = @{$cmdOIDs{$cmd}};
	my $errors = 0;

	#
	# Convert the ports from the format they were given in to the format
	# required by the command
	#
	my $portFormat;
	if ($cmd =~ /(en)|(dis)able/) {
	    $portFormat = $PORT_FORMAT_IFINDEX;
	} else { 
	    $portFormat = $PORT_FORMAT_MODPORT;
	}
	my @portlist = $self->convertPortFormat($portFormat,@ports);

	#
	# Some commands involve multiple SNMP commands, so we need to make
	# sure we get all of them
	#
	while (@oid) {
	    my $myoid = shift @oid;
	    my $myval = shift @oid;

            # 
            # We have to do some translation to a different mib for 2900
            # switches
            #
            if ($self->{SWITCHCLASS} == 2900) {
                if ($myoid eq "portAdminSpeed") {
                    $myoid = "c2900PortAdminSpeed";
                } elsif ($myoid eq "portDuplex") {
                    $myoid = "c2900PortDuplexState";
                    # Have to translate the value too
                    if ($myval eq "full") { $myval = "fullduplex"; }
                    elsif ($myval eq "half") { $myval = "halfduplex"; }
                    elsif ($myval eq "auto") { $myval = "autoNegotiate"; }
                }
            }

	    $errors += $self->UpdateField($myoid,$myval,@portlist);
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
# Convert a set of ports to an alternate format. The input format is detected
# automatically. See the declarations of the constants at the top of this
# file for a description of the different port formats.
#
# usage: convertPortFormat($self, $output format, @ports)
#        returns a list of ports in the specified output format
#        returns undef if the output format is unknown
#
# TODO: Add debugging output, better comments, more sanity checking
#
sub convertPortFormat($$@) {
    my $self = shift;
    my $output = shift;
    my @ports = @_;


    #
    # Avoid warnings by exiting if no ports given
    # 
    if (!@ports) {
	return ();
    }

    #
    # We determine the type by sampling the first port given
    #
    my $sample = $ports[0];
    if (!defined($sample)) {
	warn "convertPortFormat: Given a bad list of ports\n";
	return undef;
    }

    my $input;
    SWITCH: for ($sample) {
	(/^\d+$/) && do { $input = $PORT_FORMAT_IFINDEX; last; };
	(/^\d+\.\d+$/) && do { $input = $PORT_FORMAT_MODPORT; last; };
	(/^$self->{NAME}\.\d+\/\d+$/) && do { $input = $PORT_FORMAT_MODPORT;
		@ports = map {/^$self->{NAME}\.(\d+)\/(\d+)$/; "$1.$2";} @ports; last; };
	$input = $PORT_FORMAT_NODEPORT; last;
    }

    #
    # It's possible the ports are already in the right format
    #
    if ($input == $output) {
	$self->debug("Not converting, input format = output format\n",2);
	return @ports;
    }

    if ($input == $PORT_FORMAT_IFINDEX) {
	if ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting ifindex to modport\n",2);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting ifindex to nodeport\n",2);
	    return map portnum($self->{NAME}.":".$self->{IFINDEX}{$_}), @ports;
	}
    } elsif ($input == $PORT_FORMAT_MODPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting modport to ifindex\n",2);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting modport to nodeport\n",2);
	    return map portnum($self->{NAME} . ":$_"), @ports;
	}
    } elsif ($input == $PORT_FORMAT_NODEPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting nodeport to ifindex\n",2);
	    return map $self->{IFINDEX}{(split /:/,portnum($_))[1]}, @ports;
	} elsif ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting nodeport to modport\n",2);
	    return map { (split /:/,portnum($_))[1] } @ports;
	}
    }

    #
    # Some combination we don't know how to handle
    #
    warn "convertPortFormat: Bad input/output combination ($input/$output)\n";
    return undef;

}

#
# Obtain a lock on the VLAN edit buffer. This must be done before VLANS
# are created or removed. Will retry 5 times before failing
#
# usage: vlanLock($self)
#        returns 1 on success
#        returns 0 on failure
#
sub vlanLock($) {
    my $self = shift;

    my $EditOp = 'vtpVlanEditOperation'; # use index 1
    my $BufferOwner = 'vtpVlanEditBufferOwner'; # use index 1

    #
    # Try max_tries times before we give up, in case some other process just
    # has it locked. NOTE: snmpitSetWarn is going to retry something like
    # 10 times, so we don't need to try the look _too_ many times.
    #
    my $tries = 1;
    my $max_tries = 10;
    while ($tries <= $max_tries) {
    
	#
	# Attempt to grab the edit buffer
	#
	my $grabBuffer = snmpitSetWarn($self->{SESS},
            [$EditOp,1,"copy","INTEGER"]);

	#
	# Check to see if we were sucessful
	#
	$self->debug("Buffer Request Set gave " .
		(defined($grabBuffer)?$grabBuffer:"undef.") . "\n");
	if (! $grabBuffer) {
	    #
	    # Only print this message if we've tried at least twice, to
            # cut down on error messages
	    #
	    if ($tries >= 2) {
		print STDERR "$self->{NAME}: VLAN edit buffer request failed - " .
			     "try $tries of $max_tries.\n";
                #
                # Try to find out who is holding the lock. Let's only try a
                # couple times, since if it's failing due to an unresponsive
                # switch, there's no point in sending a ton of these get
                # requests.
                #
                my $owner = snmpitGetWarn($self->{SESS}, [$BufferOwner, 1], 2);
                if ($owner) {
                    print STDERR "$self->{NAME}: VLAN lock is held by $owner\n";
                } else {
                    print STDERR "$self->{NAME}: No owner of the VLAN lock\n";
                }

	    }

	} else {
	    last;
	}
	$tries++;

	sleep(3);
    }

    if ($tries > $max_tries) {
	#
	# Admit defeat and exit
	#
	print STDERR "ERROR: Failed to obtain VLAN edit buffer lock\n";
	return 0;
    } else {
	#
	# Set the owner of the buffer to be the machine we're running on
	#
	my $me = `/usr/bin/uname -n`;
	chomp $me;
	snmpitSetWarn($self->{SESS},[$BufferOwner,1,$me,"OCTETSTR"]);

	return 1;
    }

}

#
# Release a lock on the VLAN edit buffer. As part of releasing, applies the
# VLAN edit buffer.
#
# usage: vlanUnlock($self)
#
sub vlanUnlock($) {
    my $self = shift;

    #
    # OIDs of the operations we'll be using in this function
    #
    my $EditOp = 'vtpVlanEditOperation'; # use index 1
    my $ApplyStatus = 'vtpVlanApplyStatus'; # use index 1

    print "    Applying VLAN changes on $self->{NAME} ...";

    #
    # Send the command to apply what's in the edit buffer
    #
    my $ApplyRetVal = snmpitSetWarn($self->{SESS},[$EditOp,1,"apply","INTEGER"]);
    if (!defined($ApplyRetVal)) {
        $self->debug("Apply set: '$ApplyRetVal'\n");
    } else {
        $self->debug("Apply returned undef\n");
    }

    if (!defined($ApplyRetVal) || $ApplyRetVal != 1) {
        print " FAILED\n";
	warn("**** ERROR: Failure attempting to apply VLAN changes ($ApplyRetVal) on $self->{NAME}\n");
    } else {

        #
        # No point in trying to do this part if the switch rejected our request
        # to apply the edit buffer changes.
        #
        # Loop waiting for the switch to tell us that it's finished applying the
        # edits
        #
        $ApplyRetVal = snmpitGetWarn($self->{SESS},[$ApplyStatus,1]);
        if (!defined($ApplyRetVal)) {
            $self->debug("Apply set: '$ApplyRetVal'\n");
        } else {
            $self->debug("Apply returned undef\n");
        }
        while ($ApplyRetVal eq "inProgress") { 
            # Rate-limit our polling
            select(undef,undef,undef,.1);
            $ApplyRetVal = snmpitGetWarn($self->{SESS},[$ApplyStatus,1]);
            $self->debug("Apply gave $ApplyRetVal\n");
            print ".";
        }

        #
        # Tell the caller what happened
        #
        if ($ApplyRetVal ne "succeeded") {
            print " FAILED\n";
            warn("**** ERROR: Failure applying VLAN changes on $self->{NAME}:".
		 " $ApplyRetVal\n");
        } else { 
            print " Succeeded\n";
            $self->debug("Apply Succeeded.\n");
        }
    }

    #
    # Try to release the lock, even if the previous part failed - we don't
    # want to keep holding it
    #
    my $snmpvar = [$EditOp,1,"release",'INTEGER'];
    my $RetVal = snmpitSetWarn($self->{SESS},$snmpvar);
    if (! $RetVal ) {
        warn("*** ERROR: ".
	     "Failed to unlock VLAN edit buffer on $self->{NAME}\n");
        return 0;
    }
    $self->debug("Release: '$RetVal'\n");
    
    return $ApplyRetVal;
}

# 
# Check to see if the given (cisco-specific) VLAN number exists on the switch
#
# usage: vlanNumberExists($self, $vlan_number)
#        returns 1 if the VLAN exists, 0 otherwise
#
sub vlanNumberExists($$) {
    my $self = shift;
    my $vlan_number = shift;

    my $VlanName = "vtpVlanName";

    #
    # Just look up the name for this VLAN, and see if we get an answer back
    # or not
    #
    my $rv = snmpitGetWarn($self->{SESS},[$VlanName,"1.$vlan_number"]);
    if (!$rv or $rv eq "NOSUCHINSTANCE") {
	return 0;
    } else {
    	return 1;
    }
}

#
# Given VLAN indentifiers from the database, finds the cisco-specific VLAN
# number for them. If not VLAN id is given, returns mappings for the entire
# switch.
# 
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to Cisco VLAN numbers
#        any VLANs not found have NULL VLAN numbers
#
sub findVlans($@) { 
    my $self = shift;
    my @vlan_ids = @_;

    my $VlanName = "vtpVlanName"; # index by 1.vlan #

    #
    # Walk the tree to find the VLAN names
    # TODO - we could optimize a bit, since, if we find all VLAN, we can stop
    # looking, potentially saving us a lot of time. But, it would require a
    # more complex walk.
    #
    my %mapping = ();
    @mapping{@vlan_ids} = undef;
    my ($rows) = snmpitBulkwalkFatal($self->{SESS},[$VlanName]);
    foreach my $rowref (@$rows) {
	my ($name,$vlan_number,$vlan_name) = @$rowref;
	#
	# We get the VLAN number in the form 1.number - we need to strip
	# off the '1.' to make it useful
	#
	$vlan_number =~ s/^1\.//;

	$self->debug("Got $name $vlan_number $vlan_name\n",2);
	if (!@vlan_ids || exists $mapping{$vlan_name}) {
	    $self->debug("Putting in mapping from $vlan_name to $vlan_number\n",2);
	    $mapping{$vlan_name} = $vlan_number;
	}
    }

    return %mapping;
}

#
# Given a VLAN identifier from the database, find the cisco-specific VLAN
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
# the database.) If $vlan_number is given, attempts to use it when creating
# the vlan - otherwise, picks its own Cisco-specific VLAN number.
#
# usage: createVlan($self, $vlan_id, $vlan_number, [,$private_type
# 		[,$private_primary, $private_port]])
#        returns the new VLAN number on success
#        returns 0 on failure
#        if $private_type is given, creates a private VLAN - if private_type
#        is 'community' or 'isolated', then the assocated primary VLAN and
#        promiscous port must also be given
#
sub createVlan($$;$$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $vlan_number = shift;

    my ($private_type,$private_primary,$private_port);
    if (@_) {
	$private_type = shift;
	if ($private_type ne "primary") {
	    $private_primary = shift;
	    $private_port = shift;
	}
    } else {
	$private_type = "normal";
    }


    my $okay = 1;

    my $VlanType = 'vtpVlanEditType'; # vlan # is index
    my $VlanName = 'vtpVlanEditName'; # vlan # is index
    my $VlanSAID = 'vtpVlanEditDot10Said'; # vlan # is index
    my $VlanRowStatus = 'vtpVlanEditRowStatus'; # vlan # is index

    #
    # If they gave a VLAN number, make sure it doesn't exist
    #
    if ($vlan_number) {
	if ($self->vlanNumberExists($vlan_number)) {
	    print STDERR "ERROR: VLAN $vlan_number already exists\n";
	    return 0;
	}
    }
    
    #
    # We may have to do this multiple times - a few times, we've had the
    # Cisco give no errors, but fail to actually create the VLAN. So, we'll
    # make sure it gets created, and retry if it did not. Of course, we don't
    # want to try forever, though....
    #
    my $max_tries = 3;
    my $tries_remaining = $max_tries;
    while ($tries_remaining) {
	#
	# Try to wait out transient failures
	#
	if ($tries_remaining != $max_tries) {
	    print STDERR "VLAN $vlan_id creation failed, trying again " .
		"($tries_remaining tries left)\n";
	    sleep 5;
	}
	$tries_remaining--;

	if (!$self->vlanLock()) {
	    next;
	}

	if (!$vlan_number) {
	    #
	    # Find a free VLAN number to use. Get a list of all VLANs on the
            # switch, then look through for a free one
	    #
            my %vlan_mappings = $self->findVlans();

            #
            # Convert the mapping to a form we can use
            #
            my @vlan_numbers = values(%vlan_mappings);
            my @taken_vlans;
            foreach my $num (@vlan_numbers) {
                $taken_vlans[$num] = 1;
            }

            #
            # Pick a VLAN number
            #
	    $vlan_number = $self->{MIN_VLAN};
            while ($taken_vlans[$vlan_number]) {
                $vlan_number++;
            }
	    if ($vlan_number > $self->{MAX_VLAN}) {
		#
		# We must have failed to find one
		#
		print STDERR "ERROR: Failed to find a free VLAN number\n";
		next;
	    }
	}

	$self->debug("Using Row $vlan_number\n");

	#
	# SAID is a funky security identifier that _must_ be set for VLAN
	# creation to suceeed.
	#
	my $SAID = pack("H*",sprintf("%08x",$vlan_number + 100000));

	print "  Creating VLAN $vlan_id as VLAN #$vlan_number on " .
		"$self->{NAME} ... ";

	#
	# Perform the actual creation. Yes, this next line MUST happen all in
	# one set command....
	#
	my ($statusRow, $typeRow, $nameRow, $saidRow) = 
               ([$VlanRowStatus,"1.$vlan_number", "createAndGo","INTEGER"],
		[$VlanType,"1.$vlan_number","ethernet","INTEGER"],
		[$VlanName,"1.$vlan_number",$vlan_id,"OCTETSTR"],
		[$VlanSAID,"1.$vlan_number",$SAID,"OCTETSTR"]);
	my @varList = ($vlan_number > 1000) ?  ($statusRow, $nameRow)
			    : ($statusRow, $typeRow, $nameRow, $saidRow);
	my $RetVal = snmpitSetWarn($self->{SESS}, new SNMP::VarList(@varList));
	print "",($RetVal? "Succeeded":"Failed"), ".\n";

	#
	# Check for success
	#
	if (!$RetVal) {
	    print STDERR "VLAN Create '$vlan_id' as VLAN $vlan_number " .
		    "failed.\n";
	    $self->vlanUnlock();
	    next;
	} else {
	    #
	    # Handle private VLANs - Part I: Stuff that has to be done while we
	    # have the edit buffer locked
	    #
	    if ($self->{SUPPORTS_PRIVATE} && $private_type ne "normal") {
		#
		# First, set the private VLAN type
		#
		my $PVlanType = "cpvlanVlanEditPrivateVlanType";
		print "    Setting private VLAN type to $private_type ... ";
		$RetVal = snmpitSetWarn($self->{SESS},
                    [$PVlanType,"1.$vlan_number",$private_type, 'INTEGER']);
		print "",($RetVal? "Succeeded":"Failed"), ".\n";
		if (!$RetVal) {
		    $okay = 0;
		}
		if ($okay) {
		    #
		    # Now, if this isn't a primary VLAN, associate it with its
		    # primary VLAN
		    #
		    if ($private_type ne "primary") {
			my $PVlanAssoc = "cpvlanVlanEditAssocPrimaryVlan";
			my $primary_number = $self->findVlan($private_primary);
			if (!$primary_number) {
			    print "    **** Error - Primary VLAN " .
			    	"$private_primary could not be found\n";
			    $okay = 0;
			} else {
			    print "    Associating with $private_primary (#$primary_number) ... ";
			    $RetVal = snmpitSetWarn($self->{SESS},
                                [$PVlanAssoc,"1.$vlan_number",
                                 $primary_number,"INTEGER"]);
			    print "", ($RetVal? "Succeeded":"Failed"), ".\n";
			    if (!$RetVal) {
				$okay = 0;
			    }
			}
		    }
		}
	    }

	    $RetVal = $self->vlanUnlock();
	    $self->debug("Got $RetVal from vlanUnlock\n");

	    #
	    # Unfortunately, there are some rare circumstances in which it
	    # seems that we can't trust the switch to tell us the truth.
	    # So, let's use findVlan to see if it really got created.
	    #
	    if (!$self->findVlan($vlan_id)) {
		print STDERR "*** Switch reported success, but VLAN did not " .
			     "get created - trying again\n";
		next;	     
	    }
	    if ($self->{SUPPORTS_PRIVATE} && $private_type ne "normal" &&
		$private_type ne "primary") {

		#
		# Handle private VLANs - Part II: Set up the promiscuous port -
		# this has to be done after we release the edit buffer
		#

		my $SecondaryPort = 'cpvlanPromPortSecondaryRemap';

		my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,
		    $private_port);

		if (!$ifIndex) {
		    print STDERR "    **** ERROR - unable to find promiscous " .
			"port $private_port!\n";
		    $okay = 0;
		}

		if ($okay) {
		    print "    Setting promiscuous port to $private_port ... ";

		    #
		    # Get the existing bitfield used to maintain the mapping
		    # for the port
		    #
		    my $bitfield = snmpitGetFatal($self->{SESS},
                        [$SecondaryPort,$ifIndex]);
		    my $unpacked = unpack("B*",$bitfield);

		    #
		    # Put this into an array of 1s and 0s for easy manipulation
		    # We have to pad this out to 128 bits, because it's given
		    # back as the empty string if no bits are set yet.
		    #
		    my @bits = split //,$unpacked;
		    foreach my $bit (0 .. 127) {
			if (!defined $bits[$bit]) {
			    $bits[$bit] = 0;
			}
		    }

		    $bits[$vlan_number] = 1;

		    # Pack it back up...
		    $unpacked = join('',@bits);

		    $bitfield = pack("B*",$unpacked);

		    # And save it back...
		    $RetVal = snmpitSetFatal($self->{SESS},
                        [$SecondaryPort,$ifIndex,$bitfield, "OCTETSTR"]);
		    print "", ($RetVal? "Succeeded":"Failed"), ".\n";

		}
	    }
	    if ($okay) {
		return $vlan_number;
	    } else {
		return 0;
	    }
	}
    }

    print STDERR "*** Failed to create VLAN $vlan_id after $max_tries tries " .
		 "- giving up\n";
    return 0;
}

#
# Put the given ports in the given VLAN. The VLAN is given as a cisco-specific
# VLAN number
#
# usage: setPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $errors = 0;

    if (!$self->vlanNumberExists($vlan_number)) {
	print STDERR "ERROR: VLAN $vlan_number does not exist on switch"
	. $self->{NAME} . "\n";
	return 1;
    }

    #
    # If this switch supports private VLANs, check to see if the VLAN we're
    # putting it into is a secondary private VLAN
    #
    my $privateVlan = 0;
    if ($self->{SUPPORTS_PRIVATE}) {
	$self->debug("Checking to see if vlan is private ... ");
	my $PrivateType = "cpvlanVlanPrivateVlanType";
	my $type = snmpitGetFatal($self->{SESS},[$PrivateType,"1.$vlan_number"]);
	$self->debug("type is $type ... ");
	if ($type eq "community" ||  $type eq "isolated") {
	    $self->debug("It is\n");
	    $privateVlan = 1;
	} else {
	    $self->debug("It isn't\n");
	}
    }

    my $PortVlanMemb;
    my $format;
    if ($self->{OSTYPE} eq "CatOS") {
	if (!$privateVlan) {
	    $PortVlanMemb = "vlanPortVlan"; #index is ifIndex
	    $format = $PORT_FORMAT_MODPORT;
	} else {
	    $PortVlanMemb = "cpvlanPrivatePortSecondaryVlan";
	    $format = $PORT_FORMAT_IFINDEX;
	}
    } elsif ($self->{OSTYPE} eq "IOS") {
	$PortVlanMemb = "vmVlan"; #index is ifIndex
	$format = $PORT_FORMAT_IFINDEX;
    }

    #
    # We'll keep track of which ports suceeded, so that we don't try to
    # enable/disable, etc. ports that failed.
    #
    my @okports = ();
    my ($index, $retval);
    my %BumpedVlans = ();

    foreach my $port (@ports) {
	$self->debug("Putting port $port in VLAN $vlan_number\n");
	#
	# Check to see if it's a trunk ....
	#
	($index) = $self->convertPortFormat($PORT_FORMAT_IFINDEX, $port);
	$retval = snmpitGetWarn($self->{SESS},
			["vlanTrunkPortDynamicState",$index]);
	if (!$retval) {
	    $errors++;
	    next;
	}
	$self->debug("Port trunk mode is $retval\n");
	
	if (!(($retval eq "on") || ($retval eq "onNoNegotiate"))) {
	    #
	    # Convert ports to the correct format
	    #
	    ($index) = $self->convertPortFormat($format, $port);

	    # 
	    # Make sure the port didn't get mangled in conversion
	    #
	    if (!defined $index) {
		print STDERR "Port not found, skipping\n";
		$errors++;
		next;
	    }
	    my $snmpvar = [$PortVlanMemb,$index,$vlan_number,'INTEGER'];
	    #
	    # Check to see if we are already in a VLAN
	    #
	    $retval = snmpitGet($self->{SESS},[$PortVlanMemb,$index]);
	    if (($retval ne "NOSUCHINSTANCE") &&
		("$retval" ne "$vlan_number") && ("$retval" ne "1")) {
		$BumpedVlans{$retval} = 1;
	    }
	    #
	    # Do the acutal SNMP command
	    #
	    $retval = snmpitSetWarn($self->{SESS},$snmpvar);
	} else {
	    #
	    # We're here if it a trunk
	    #
	    $retval = $self->setVlansOnTrunk($port, 1, $vlan_number);
	}
	if (!$retval) {
	    $errors++;
	    next;
	} else {
	    push @okports, $port;
	}
    }

    #
    # Ports going into VLAN 1 are being taken out of circulation, so we
    # disable them. Otherwise, we need to make sure they get enabled.
    #
    if ($vlan_number == 1) {
	$self->debug("Disabling " . join(',',@okports) . "...");
	if ( my $rv = $self->portControl("disable",@okports) ) {
	    print STDERR "Port disable had $rv failures.\n";
	    $errors += $rv;
	}
    } else {
	$self->debug("Enabling "  . join(',',@okports) . "...");
	if ( my $rv = $self->portControl("enable",@okports) ) {
	    print STDERR "Port enable had $rv failures.\n";
	    $errors += $rv;
	}
    }

    # When removing things from the control vlan for a firewall,
    # need to tell stack to shake things up to flush FDB on neighboring
    # switches.
    #
    my @bumpedlist = keys ( %BumpedVlans );
    if (@bumpedlist) {
	@{$self->{DISPLACED_VLANS}} = @bumpedlist;
    }

    return $errors;
}

#
# Remove all ports from the given VLANs, which are given as Cisco-specific
# VLAN numbers
#
# usage: removePortsFromVlan(self,int vlans)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removePortsFromVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;

    #
    # Make sure the VLANs actually exist
    #
    foreach my $vlan_number (@vlan_numbers) {
	if (!$self->vlanNumberExists($vlan_number)) {
	    print STDERR "ERROR: VLAN $vlan_number does not exist\n";
	    return 1;
	}
    }

    #
    # Make a hash of the vlan number for easy lookup later
    #
    my %vlan_numbers = ();
    @vlan_numbers{@vlan_numbers} = @vlan_numbers;

    #
    # Get a list of the ports in the VLAN
    #
    my $VlanPortVlan;
    if ($self->{OSTYPE} eq "CatOS") {
	$VlanPortVlan = "vlanPortVlan"; #index is ifIndex
    } elsif ($self->{OSTYPE} eq "IOS") {
	$VlanPortVlan = "vmVlan"; #index is ifIndex
    }
    my @ports;

    #
    # Walk the tree to find VLAN membership
    #
    my ($rows) = snmpitBulkwalkFatal($self->{SESS},[$VlanPortVlan]);
    foreach my $rowref (@$rows) {
	my ($name,$modport,$port_vlan_number) = @$rowref;
	$self->debug("Got $name $modport $port_vlan_number\n");
	if ($vlan_numbers{$port_vlan_number}) {
	    push @ports, $modport;
	}
    }

    $self->debug("About to remove ports " . join(",",@ports) . "\n");
    if (@ports) {
	return $self->setPortVlan(1,@ports);
    } else {
	return 0;
    }
}

#
# Remove some ports from the given VLAN, which are given as Cisco-specific
# VLAN numbers. Do not specify trunked ports here.
#
# usage: removeSomePortsFromVlan(self,int vlan, ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removeSomePortsFromVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    #
    # Make sure the VLANs actually exist
    #
    if (!$self->vlanNumberExists($vlan_number)) {
	print STDERR "ERROR: VLAN $vlan_number does not exist\n";
	return 1;
    }

    #
    # Make a hash of the ports for easy lookup later.
    #
    my %ports = ();
    @ports{@ports} = @ports;

    #
    # Get a list of the ports in the VLAN
    #
    my $VlanPortVlan;
    if ($self->{OSTYPE} eq "CatOS") {
	$VlanPortVlan = "vlanPortVlan"; #index is ifIndex
    } elsif ($self->{OSTYPE} eq "IOS") {
	$VlanPortVlan = "vmVlan"; #index is ifIndex
    }
    my @remports;

    #
    # Walk the tree to find VLAN membership
    #
    my ($rows) = snmpitBulkwalkFatal($self->{SESS},[$VlanPortVlan]);
    foreach my $rowref (@$rows) {
	my ($name,$modport,$port_vlan_number) = @$rowref;
	my ($trans) = convertPortFormat($PORT_FORMAT_NODEPORT,$modport);
	if (!defined $trans) {
	    $trans = ""; # Guard against some uninitialized value warnings
	}

	$self->debug("Got $name $modport ($trans) $port_vlan_number\n");
	
	push(@remports, $modport)
	    if ("$port_vlan_number" eq "$vlan_number" &&
		exists($ports{$trans}));
    }

    $self->debug("About to remove ports " . join(",",@remports) . "\n");
    if (@remports) {
	return $self->setPortVlan(1,@remports);
    } else {
	return 0;
    }
}

#
# Remove the given VLAN from this switch. This presupposes that all of its
# ports have already been removed with removePortsFromVlan(). The VLAN is
# given as a Cisco-specific VLAN number
#
# usage: removeVlan(self,int vlan)
#	 returns 1 on success
#	 returns 0 on failure
#
#
sub removeVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;

    my $errors = 0;

    foreach my $vlan_number (@vlan_numbers) {
        #
        # Need to lock the VLAN edit buffer
        #
        if (!$self->vlanLock()) {
            return 0;
        }

	#
	# Make sure the VLAN actually exists
	#
	if (!$self->vlanNumberExists($vlan_number)) {
	    print STDERR "ERROR: VLAN $vlan_number does not exist\n";
	    return 0;
	}

	#
	# Perform the actual removal
	#
	my $VlanRowStatus = 'vtpVlanEditRowStatus'; # vlan is index

	print "  Removing VLAN #$vlan_number on $self->{NAME} ... ";
	my $RetVal = snmpitSetWarn($self->{SESS},
            [$VlanRowStatus,"1.$vlan_number","destroy","INTEGER"]);
	if ($RetVal) {
	    print "Succeeded.\n";
	} else {
	    print "Failed.\n";
	    $errors++;
	}

        #
        # Unlock whether successful or not
        #
        $self->vlanUnlock();

    }

    if ($errors) {
	return 0;
    } else {
	return 1;
    }

}

#
# TODO: Cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    # returns 0 on success, # of failed ports on failure
    $self->debug("UpdateField: '@_'\n");
    my ($OID,$val,@ports)= @_;
    my $Status = 0;
    my $err = 0;
    foreach my $port (@ports) {
	my ($trans) = convertPortFormat($PORT_FORMAT_NODEPORT,$port);
	if (!defined $trans) {
	    $trans = ""; # Guard against some uninitialized value warnings
	}
	$self->debug("Checking port $port ($trans) for $val...");
	$Status = snmpitGetWarn($self->{SESS},[$OID,$port]);
	if (!defined $Status) {
	    warn "Port $port ($trans), change to $val: No answer from device\n";
	    return -1;		# return error
	} else {
	    $self->debug("Okay.\n");
	    $self->debug("Port $port was $Status\n");
	    if ($Status ne $val) {
		$self->debug("Setting $port to $val...");

		# For 2960, we must ensure that field updates use
                # module 1 *not* 0
		# This transforms a request like
		#	portAdminSpeed.0.8.s100000000
		# to
		#	portAdminSpeed.1.8.s100000000
		#
		if ($self->{SWITCHCLASS} == "2960" &&
			$port =~ /^(\d+)\.(\d+)$/ && $1 == 0) {
			$port = "1.$2";
		}

		# Don't use async
		my $result = snmpitSetWarn($self->{SESS},
                    [$OID,$port,$val,"INTEGER"]);
		$self->debug("Set returned '$result'\n") if (defined $result);
		if ($self->{BLOCK}) {
		    my $n = 0;
		    while ($Status ne $val) {
			$Status = snmpitGetWarn($self->{SESS},[$OID,$port]);
			$self->debug("Value for $port was $Status\n");
			select (undef, undef, undef, .25); # wait .25 seconds
			$n++;
			if ($n > 20) {
			    $err++;
			    $self->debug("Timing out...");
			    last;
			}
		    }
		    $self->debug("Okay.\n");
		} else {
		    $self->debug("\n");
		}
	    }
	}
    }
    # returns 0 on success, # of failed ports on failure
    $err;
}

#
# Determine if a VLAN has any ports on this switch
#
# usage: vlanHasPorts($self, $vlan_number)
# returns 1 if the vlan exists and has ports
#
sub vlanHasPorts($$) {
    my ($self, $vlan_number)= @_;

    if ($self->vlanNumberExists($vlan_number)) {

	my $VlanPortVlan;
	if ($self->{OSTYPE} eq "CatOS") {
	    $VlanPortVlan = ["vlanPortVlan"]; #index is ifIndex
	} elsif ($self->{OSTYPE} eq "IOS") {
	    $VlanPortVlan = ["vmVlan"]; #index is ifIndex
	}
	#
	# Walk the tree for the VLAN members
	#
	my ($rows) = snmpitBulkwalkFatal($self->{SESS},$VlanPortVlan);
	$self->debug("Vlan members walk got " . scalar(@$rows) . " rows\n");
	foreach my $rowref (@$rows) {
	    my ($name,$modport,$number) = @$rowref;
	    $self->debug("Got $name $modport $number\n",3);
	    if ($number == $vlan_number) { return 1; }
	}
	return 0;
    }
}

#
# The next section is a helper function for checking which vlans
# and which ports or trunks, setting and clearing them, etc.
#

my %vtrunkOIDS = (
    0 => "vlanTrunkPortVlansEnabled",
    1 => "vlanTrunkPortVlansEnabled2k",
    2 => "vlanTrunkPortVlansEnabled3k",
    3 => "vlanTrunkPortVlansEnabled4k"
);

# precompute 1k 0 bits as bitfield
my $p1k = pack("x128");

my ($VOP_CLEAR, $VOP_SET, $VOP_CLEARALL, $VOP_CHECK) = (0, 1, 2, 3);

#
# vlanTrunkUtil($self, $op, $ifIndex, @vlans)
# does one of the 4 operations above on all 4 ranges of vlans
# but tries to be a little smart about not visting ranges not needed.

sub vlanTrunkUtil($$$$) {
    my ($self, $op, $ifIndex, @vlans) = @_;

    my ($bitfield, %vranges, @result);

    if ($op == $VOP_CLEARALL)
        { @result = @vlans = (1, 1025, 2049, 3073); }
    foreach my $vlan (@vlans)
	{ push @{$vranges{($vlan >> 10) & 3}}, $vlan; }

    while (my ($bank, $banklist) = each %vranges) {
	my ($bankbase, $bankOID) = (($bank << 10), $vtrunkOIDS{$bank});
	$self->lock() unless ($op == $VOP_CHECK);
	if ($op == $VOP_CLEARALL) {
	    $bitfield = $p1k;
	} else {
	    $bitfield = snmpitGetFatal($self->{SESS}, [$bankOID,$ifIndex]);
	    # the cisco 650x sometimes returns only a few bytes...
	    my @bits = split //, unpack("B1024", $bitfield . $p1k);

	    foreach my $vlan (@$banklist) {
		 if ($op == $VOP_CHECK) {
		     if ($bits[$vlan - $bankbase]) { push @result, $vlan; }
		 } else {
		     $bits[$vlan - $bankbase] = $op;
		     push @result, $vlan;
		 }
	    }
	    $bitfield = pack("B*", join('',@bits)) unless ($op == $VOP_CHECK);
	}
	next if ($op == $VOP_CHECK);

	# don't need to check result because it dies if it doesn't work.
	snmpitSetFatal($self->{SESS},
	    [$bankOID,$ifIndex,$bitfield,"OCTETSTR"]);
	$self->unlock();
    }
    return sort @result;
}

#
# List all VLANs on the device
#
# usage: listVlans($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listVlans($) {
    my $self = shift;

    $self->debug("Getting VLAN info...\n");
    # We don't need VlanIndex really...
    my $VlanName = ["vtpVlanName"]; # index by 1.vlan #

    my $VlanPortVlan;
    if ($self->{OSTYPE} eq "CatOS") {
	$VlanPortVlan = ["vlanPortVlan"]; #index is ifIndex
    } elsif ($self->{OSTYPE} eq "IOS") {
	$VlanPortVlan = ["vmVlan"]; #index is ifIndex
    }

    #
    # Walk the tree to find the VLAN names
    #
    my ($rows) = snmpitBulkwalkFatal($self->{SESS},$VlanName);
    my %Names = ();
    my %Members = ();
    foreach my $rowref (@$rows) {
	my ($name,$vlan_number,$vlan_name) = @$rowref;
	#
	# We get the VLAN number in the form 1.number - we need to strip
	# off the '1.' to make it useful
	#
	$vlan_number =~ s/^1\.//;

	$self->debug("Got $name $vlan_number $vlan_name\n");
	if (!$Names{$vlan_number}) {
	    $Names{$vlan_number} = $vlan_name;
	    @{$Members{$vlan_number}} = ();
	}
    }

    #
    # Walk the tree for the VLAN members
    #
    ($rows) = snmpitBulkwalkFatal($self->{SESS},$VlanPortVlan);
    $self->debug("Vlan members walk returned " . scalar(@$rows) . " rows\n");
    foreach my $rowref (@$rows) {
	my ($name,$modport,$vlan_number) = @$rowref;
	$self->debug("Got $name $modport $vlan_number\n",3);
	my ($node) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$modport);
	if (!$node) {
	    $modport =~ s/\./\//;
	    $node = $self->{NAME} . ".$modport";
	}
	push @{$Members{$vlan_number}}, $node;
	if (!$Names{$vlan_number}) {
	    $self->debug("listVlans: WARNING: port $self->{NAME}.$modport in non-existant " .
		"VLAN $vlan_number\n");
	}
    }

    #
    # Walk trunks for the VLAN members
    #
    ($rows) = snmpitBulkwalkFatal($self->{SESS},["vlanTrunkPortDynamicStatus"]);
    $self->debug("Trunk members walk returned " . scalar(@$rows) . " rows\n");
    foreach my $rowref (@$rows) {
	my ($name,$ifIndex,$status) = @$rowref;
	$self->debug("Got $name $ifIndex $status\n",3);
	if ($status ne "trunking") { next;}
        # XXX: This should really print out something more useful, like the
        # other end of the trunk
        my $node = $self->{NAME} . ".trunk$ifIndex";
        #my ($node) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$ifIndex);
        #if (!$node) {
        #    my ($modport) = $self->convertPortFormat($PORT_FORMAT_MODPORT,$ifIndex);
        #    $modport =~ s/\./\//;
        #    $node = $self->{NAME} . ".$modport";
        #}

	# Get the allowed VLANs on this trunk
	my @trunklans = $self->vlanTrunkUtil($VOP_CHECK, $ifIndex, keys %Names);

	foreach my $vlan_number (@trunklans) {
	    $self->debug("got vlan $vlan_number on trunk $node\n",3);
	    push @{$Members{$vlan_number}}, $node;
	}
    }

    #
    # Build a list from the name and membership lists
    #
    my @list = ();
    foreach my $vlan_id (sort {$a <=> $b} keys %Names) {
    	if ($vlan_id != 1) {
	    #
    	    # Special case for Cisco - VLAN 1 is special and should not
    	    # be reported
	    #
    	    push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
    	}
    }
    $self->debug(join("\n",(map {join ",", @$_} @list))."\n");

    return @list;
}

#
# Walk a table that's indexed by ifindex. Convert the ifindex to a port, and
# stuff the value into the given hash
#
# usage: walkTableIfIndex($self,$tableID,$hash,$procfun)
#        $tableID is the name of the table to walk
#        $hash is a reference to the hash we will be updating
#        $procfun is a function run on the data for pre-processing
# returns: nothing
# Internal-only function
#
sub walkTableIfIndex($$$;$) {
    my $self = shift;
    my ($table,$hash,$fun) = @_;
    if (!$fun) {
        $fun = sub { $_[0]; }
    }

    #
    # Grab the whole table in one fell swoop
    #
    my @table = snmpitBulkwalkFatal($self->{SESS},[$table]);

    foreach my $table (@table) {
        foreach my $row (@$table) {
            my ($oid,$index,$data) = @$row;

	    #
	    # Some generic MIBs return just a port number on the 2900.
	    # Convert those to module.port format
	    #
	    $self->debug("  $oid: $index: $data\n", 2);
	    if ($self->{SWITCHCLASS} == 2900 &&
		$index =~ /^\d+$/) {
		$index = "0.$index";
		$self->debug("    index rewritten to $index\n", 2);
	    }

            #
            # Convert the ifindex we got into a port
            # XXX - Should use convertPortFormat(), right? I've preserved the
            # historical code in case it has some special behavior we depend on
            #
            if (! defined $self->{IFINDEX}{$index}) { next; }
            my $port = portnum("$self->{NAME}:$index")
                || portnum("$self->{NAME}:".$self->{IFINDEX}{$index});
            if (! defined $port) { next; } # Skip if we don't know about it
            
            #
            # Apply the user's processing function
            #
            my $pdata = &$fun($data);
            ${$hash}{$port} = $pdata;
        }
    }
}


#
# List all ports on the device
#
# usage: listPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
#
sub listPorts($) {
    my $self = shift;

    my %Able = ();
    my %Link = ();
    my %speed = ();
    my %duplex = ();

    $self->debug("Getting port information...\n");
    $self->walkTableIfIndex('ifAdminStatus',\%Able,
        sub { if ($_[0] =~ /up/) { "yes" } else { "no" } });
    $self->walkTableIfIndex('ifOperStatus',\%Link);

    #
    # For some silly reason, these next two things are in a different MIB on
    # 2900s
    #
    if ($self->{SWITCHCLASS} == 2900) {
        $self->walkTableIfIndex('c2900PortAdminSpeed',\%speed);
        $self->walkTableIfIndex('c2900PortDuplexState',\%duplex,
            # Have to translate some values that differ from the other MIB
            sub { if    ($_[0] =~ /auto/) { "auto" }
                  elsif ($_[0] =~ /full/) { "full" }
                  elsif ($_[0] =~ /half/) { "half" }
                  else                    { $_[0] } });
    } else {
        $self->walkTableIfIndex('portAdminSpeed',\%speed);
        $self->walkTableIfIndex('portDuplex',\%duplex);
    }

    # Insert stuff here to get ifSpeed if necessary... AdminSpeed is the
    # _desired_ speed, and ifSpeed is the _real_ speed it is using

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %Able ) {
	my $vlan;
	if (! defined ($speed{$id}) ) { $speed{$id} = " "; }
	if (! defined ($duplex{$id}) ) { $duplex{$id} = " "; }
	$speed{$id} =~ s/s([10]+)000000/${1}Mbps/;
	push @rv, [$id,$Able{$id},$Link{$id},$speed{$id},$duplex{$id}];
    }
    return @rv;
}

# 
# Get statistics for ports on the switch
#
# usage: getPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
# TODO: Clean up undefined variable warnings
#
sub getStats ($) {
    my $self = shift;

    #
    # Walk the tree for the VLAN members
    #
    my $vars = new SNMP::VarList(['ifInOctets'],['ifInUcastPkts'],
    				 ['ifInNUcastPkts'],['ifInDiscards'],
				 ['ifInErrors'],['ifInUnknownProtos'],
				 ['ifOutOctets'],['ifOutUcastPkts'],
				 ['ifOutNUcastPkts'],['ifOutDiscards'],
				 ['ifOutErrors'],['ifOutQLen']);
    my @stats = $self->{SESS}->bulkwalk(0,32,$vars);

    #
    # We need to flip the two-dimentional array we got from bulkwalk on
    # its side, and convert ifindexes into node:port
    #
    my $i = 0;
    my %stats;
    foreach my $array (@stats) {
	while (@$array) {
	    my ($name,$ifindex,$value) = @{shift @$array};
	    my ($port) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$ifindex);
	    if ($port) {
		${$stats{$port}}[$i] = $value;
	    }
	}
	$i++;
    }

    return map [$_,@{$stats{$_}}], sort {tbsort($a,$b)} keys %stats;

}

#
# Get the ifindex for an EtherChannel (trunk given as a list of ports)
#
# usage: getChannelIfIndex(self, ports)
#        Returns: undef if more than one port is given, and no channel is found
#           an ifindex if a channel is found and/or only one port is given
#
#
sub getChannelIfIndex($@) {
    my $self = shift;
    my @ports = @_;
    my @ifIndexes = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@ports);

    my $ifindex = undef;

    #
    # Try to get a channel number for each one of the ports in turn - we'll
    # take the first one we get
    #
    foreach my $port (@ifIndexes) {
        my $channel = snmpitGetFatal($self->{SESS},["pagpGroupIfIndex",$port]);
        if (($channel =~ /^\d+$/) && ($channel != 0)) {
            $ifindex = $channel;
            last;
        }
    }
    
    #
    # If we didn't get a channel number, and we were only given a single port,
    # we can return the ifIndex for that port. Note that we tried to get a
    # channel number first, in case someone did something silly like give us a
    # single port channel.
    #
    if (!$ifindex) {
        if (@ifIndexes == 1) {
            $ifindex = $ifIndexes[0];
        }
    }

    return $ifindex;
}

#
# Enable, or disable,  port on a trunk
#
# usage: setVlansOnTrunk(self, ifindex, value, vlan_numbers)
#        ifindex: ifindex of the trunk to operate on
#        value: 0 to disallow the VLAN on the trunk, 1 to allow it
#	 vlan_numbers: An array of cisco-native VLAN numbers operate on
#        Returns 1 on success, 0 otherwise
#
sub setVlansOnTrunk($$$$) {
    my $self = shift;
    my ($port, $value, @vlan_numbers) = @_;

    #
    # Some error checking
    #
    if (($value != 1) && ($value != 0)) {
	die "Invalid value $value passed to setVlanOnTrunk\n";
    }
    if (grep(/^1$/,@vlan_numbers)) {
	die "VLAN 1 passed to setVlanOnTrunk\n"
    }

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    @vlan_numbers = $self->vlanTrunkUtil($value, $ifIndex, @vlan_numbers);
    return (scalar(@vlan_numbers) != 0);
}

#
# Clear the list of allowed VLANs from a trunk
#
# usage: clearAllVlansOnTrunk(self, modport)
#        modport: module.port of the trunk to operate on
#        Returns 1 on success, 0 otherwise
#
sub clearAllVlansOnTrunk($$) {
    my $self = shift;
    my ($modport) = @_;

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);

    #
    # If this is part of an EtherChannel, we have to find the ifIndex for the
    # channel.
    # TODO: Perhaps this should be general - ie. $self{IFINDEX} should have
    # the channel ifIndex the the port is in a channel. Not sure that
    # this is _always_ beneficial, though
    #
    my $channel = snmpitGetFatal($self->{SESS},["pagpGroupIfIndex",$ifIndex]);
    if (($channel =~ /^\d+$/) && ($channel != 0)) {
	$ifIndex = $channel;
    }
    my @vlan_numbers = $self->vlanTrunkUtil($VOP_CLEARALL, $ifIndex);
    return (scalar(@vlan_numbers) != 0);
}

#
# Easy flush of FDB for a (vlan, trunk port) if port is on vlan
# by removing it and adding it back
#
# usage: resetVlanIfOnTrunk(self, modport, vlan_number)
#        modport: module.port of the trunk to operate on
#	 vlan_number: A cisco-native VLAN number to check
#        return value currently ignored.

sub resetVlanIfOnTrunk($$$) {
    my ($self, $modport, $vlan_number) = @_;


    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);

    #
    # If this is part of an EtherChannel, we have to find the ifIndex for the
    # channel.
    # TODO: Perhaps this should be general - ie. $self{IFINDEX} should have
    # the channel ifIndex the the port is in a channel. Not sure that
    # this is _always_ beneficial, though
    # NOTE: This 'conversion' is no longer needed, since we call
    # getChannelIfIndex on the port before passing it into this function
    #
    #my $channel = snmpitGetFatal($self->{SESS},["pagpGroupIfIndex",$ifIndex]);
    #if (!($channel =~ /^\d+$/) || ($channel == 0)) {
    #	print "WARNING: resetVlanIfOnTrunk got bad channel ($channel) for $self->{NAME}.$modport\n";
    #	return 0;
    #}
    #if (($channel =~ /^\d+$/) && ($channel != 0)) {
    #	$ifIndex = $channel;
    #}

    my @vlan_numbers = $self->vlanTrunkUtil($VOP_CHECK, $ifIndex, $vlan_number);
    if (@vlan_numbers) {
	$self->vlanTrunkUtil($VOP_CLEAR, $ifIndex, $vlan_number);
	$self->vlanTrunkUtil($VOP_SET, $ifIndex, $vlan_number);
    }
    return 0;
}

#
# Enable trunking on a port
#
# usage: enablePortTrunking2(self, modport, nativevlan, equaltrunking)
#        modport: module.port of the trunk to operate on
#        nativevlan: VLAN number of the native VLAN for this trunk
#        Returns 1 on success, 0 otherwise
#
sub enablePortTrunking2($$$$) {
    my ($self,$port,$native_vlan,$equaltrunking) = @_;
    my $trunking_vlan = ($equaltrunking ? 1 : $native_vlan);

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);

    #
    # Clear out the list of allowed VLANs for this trunk port, so that when it
    # comes up, there is not some race condition
    #
    my $rv = $self->clearAllVlansOnTrunk($port);
    if (!$rv) {
	warn "ERROR: Unable to clear VLANs on trunk\n";
	return 0;
    } 

    #
    # Set the type of the trunk - we only do dot1q for now
    #
    my $trunkType = ["vlanTrunkPortEncapsulationType",$ifIndex,"dot1Q","INTEGER"];
    $rv = snmpitSetWarn($self->{SESS},$trunkType);
    if (!$rv) {
	warn "ERROR: Unable to set encapsulation type\n";
	return 0;
    }

    #
    # Set the native VLAN for this trunk
    #
    my $nativeVlan = ["vlanTrunkPortNativeVlan",$ifIndex,$trunking_vlan,"INTEGER"];
    $rv = snmpitSetWarn($self->{SESS},$nativeVlan);
    if (!$rv) {
	warn "ERROR: Unable to set native VLAN on trunk\n";
	return 0;
    }

    #
    # Finally, enable trunking!
    #
    my $trunkEnable = ["vlanTrunkPortDynamicState",$ifIndex,"onNoNegotiate","INTEGER"];
    $rv = snmpitSetWarn($self->{SESS},$trunkEnable);
    if (!$rv) {
	warn "ERROR: Unable to enable trunking\n";
	return 0;
    }

    if ($equaltrunking) { return 1; }

    #
    # Allow the native VLAN to cross the trunk
    #
    $rv = $self->setVlansOnTrunk($port,1,$native_vlan);
    if (!$rv) {
	warn "ERROR: Unable to enable native VLAN on trunk\n";
	return 0;
    }

    return 1;
    
}

#
# Disable trunking on a port
#
# usage: disablePortTrunking(self, modport)
#        modport: module.port of the trunk to operate on
#        Returns 1 on success, 0 otherwise
#
sub disablePortTrunking($$) {
    my $self = shift;
    my ($port) = @_;

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);


    #
    # Clear out the list of allowed VLANs for this trunk port
    #
    my $rv = $self->clearAllVlansOnTrunk($port);
    if (!$rv) {
	warn "ERROR: Unable to clear VLANs on trunk\n";
	return 0;
    } 

    #
    # Disable trunking!
    #
    my $trunkDisable = ["vlanTrunkPortDynamicState",$ifIndex,"off","INTEGER"];
    $rv = snmpitSetWarn($self->{SESS},$trunkDisable);
    if (!$rv) {
	warn "ERROR: Unable to enable trunking\n";
	return 0;
    }

    return 1;
    
}

#
# Reads the IfIndex table from the switch, for SNMP functions that use 
# IfIndex rather than the module.port style. Fills out the objects IFINDEX
# members,
#
# usage: readifIndex(self)
#        returns nothing
#
sub readifIndex($) {
    my $self = shift;

    #
    # How we fill this table is highly dependant on which OS the switch
    # is running - CatOS provides a convenient table to convert from
    # node/port to ifindex, but under IOS, we have to infer it from the
    # port description
    #

    if ($self->{OSTYPE} eq "CatOS") {
	my ($rows) = snmpitBulkwalkFatal($self->{SESS},["portIfIndex"]);

	foreach my $rowref (@$rows) {
	    my ($name,$modport,$ifindex) = @$rowref;

	    $self->{IFINDEX}{$modport} = $ifindex;
	    $self->{IFINDEX}{$ifindex} = $modport;
	}
    } elsif ($self->{OSTYPE} eq "IOS") {
	my ($rows) = snmpitBulkwalkFatal($self->{SESS},["ifDescr"]);
   
	foreach my $rowref (@$rows) {
	    my ($name,$iid,$descr) = @$rowref;
	    if ($descr =~ /(\D*)(\d+)\/(\d+)$/) {
                my $type = $1;
                my $module = $2;
                my $port = $3;

		$self->debug("IFINDEX: $descr ($type,$module,$port)\n", 2);

                if ($self->{NON_MODULAR_HACK}) {
                    #
                    # Hack for non-modular switches with both 100Mbps and
                    # gigabit ports - consider gigabit ports to be on module 1
                    #
                    if (($module == 0) && ($type =~ /^gi/i)) {
                        $module = 1;
                        $self->debug("NON_MODULAR_HACK: Moving $descr to mod $module\n");
                    }
                }
		my $modport = "$module.$port";
		my $ifindex;
		if (defined($iid) && ($iid ne "")) {
		    $ifindex = $iid;
		} else {
		    $name =~ /(\d+)$/;
		    $ifindex = $1;
		}

		$self->{IFINDEX}{$modport} = $ifindex;
		$self->{IFINDEX}{$ifindex} = $modport;

		$self->debug("IFINDEX: $modport,$ifindex\n", 2);
	    }
	}
    }
}

#
# Read a set of values for all given ports.
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

    my @ifindicies = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@$ports);
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
        # TODO: Convert this to snmpitGet*(), but can't yet because it doesn't
        # support VarLists
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
# Tell the switch to dump its configuration file to the specified file
# on the specified server, via TFTP
#
# Usage: writeConfigTFTP($server, $filename). The server can be either a
#        hostname or an IP address. The destination filename must exist and be
#        world-writable, or TFTP will refuse to write to it
# Returns: 1 on success, 0 otherwise
#
sub writeConfigTFTP($$$) {
    my ($self,$server,$filename) = @_;
    
    #
    # TODO - convert from Fatal() to Warn() calls
    #

    #
    # Make sure we've loaded in the proper MIBs to do this
    #
    if (!$self->{TFTPWRITE}) {
	warn "No support for copying config via TFTP - possible missing MIB " .
             "file\n";
	return 0;
    }

    #
    # Start off by resolving the server's name into an IP address
    #
    my $ip = inet_aton($server);
    if (!$ip) {
	warn "Unable to lookup hostname $server\n";
	return 0;
    }

    my $ipstr = join(".",unpack('C4',$ip));

    #
    # CatOS switches use the CISCO-STACK-MIB for this, IOS switches use
    # CISCO-CONFIG-COPY-MIB (which is much more powerful)
    #
    if ($self->{OSTYPE} eq "CatOS") {
        #
        # Set up a few values on the switch to tell it where to stick the config
        # file
        #
        my $setHost = ["tftpHost",0,$ipstr,"STRING"];
        my $setFilename = ["tftpFile",0,$filename,"STRING"];

        snmpitSetFatal($self->{SESS},$setHost);
        snmpitSetFatal($self->{SESS},$setFilename);

        #
        # Okay, go!
        #
        my $tftpGo = ["tftpAction","0","uploadConfig","INTEGER"];
        snmpitSetFatal($self->{SESS},$tftpGo);

        #
        # Poll to see if it suceeded - wait for a while, but not forever!
        #
        my $tftpResult = ["tftpResult",0];
        my $iters = 0;
        my $rv;
        while (($rv = snmpitGetFatal($self->{SESS},$tftpResult))
            eq "inProgress" && ($iters < 30)) {
            $iters++;
            sleep(1);
        }
        if ($iters == 30) {
            warn "TFTP write took longer than 30 seconds!";
            return 0;
        } else {
            if ($rv ne "success") {
                warn "TFTP write failed with error $rv\n";
                return 0;
            } else {
                return 1;
            }
        }

    } else {
        #
        # We generate a random number that we'll use to identify this session.
        #
        my $sessid = int(rand(65536));

        #
        # Create an entry in the ccCopyTable. createAndWait means we'll be
        # sending more data in subsequent packets
        #
        snmpitSetFatal($self->{SESS},["ccCopyEntryRowStatus",$sessid,
            'createAndWait','INTEGER']);

        #
        # We'll be uploading to a TFTP server
        #
        snmpitSetFatal($self->{SESS},["ccCopyDestFileType",$sessid,
            'networkFile','INTEGER']);
        snmpitSetFatal($self->{SESS},["ccCopyServerAddress",$sessid,
            $ipstr,'STRING']);
        snmpitSetFatal($self->{SESS},["ccCopyFileName",$sessid,
            $filename,'STRING']);
        snmpitSetFatal($self->{SESS},["ccCopyProtocol",$sessid,
            'tftp','INTEGER']);
        
        #
        # We want the running-config file (ie. the current configuration)
        #
        snmpitSetFatal($self->{SESS},["ccCopySourceFileType",$sessid,
            'runningConfig','INTEGER']);

        #
        # Engage!
        #
        snmpitSetFatal($self->{SESS},["ccCopyEntryRowStatus",$sessid,
            'active','INTEGER']);

        #
        # Wait for it to finish
        #
        my $ccCopyResult = ["ccCopyState",$sessid];
        my $iters = 0;
        my $rv;
        while ($rv = snmpitGetFatal($self->{SESS},$ccCopyResult)) {
            # We finished, one way or the other...
            if ($rv eq "successful" || $rv eq "failed") {
                last;
            }
            # Give up, it's taken too long
            if ($iters++ == 30) {
                last;
            }

            sleep(1);
        }

        #
        # If it failed, find out why
        #
        my $cause = snmpitGetWarn($self->{SESS},["ccCopyFailCause",$sessid]);

        #
        # Remove our ccCopyTable entry
        #
        snmpitSetFatal($self->{SESS},["ccCopyEntryRowStatus",$sessid,
            'destroy','INTEGER']);

        if ($iters == 30) {
            warn "TFTP write took longer than 30 seconds!";
            return 0;
        } else {
            if ($rv ne "successful") {
                warn "TFTP write failed with error $rv ($cause)\n";
                return 0;
            } else {
                return 1;
            }
        }

    }
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
    $lock_held = 1
}

sub unlock($) {
	if ($lock_held == 1) { TBScriptUnlock();}
	$lock_held = 0;
}

#
# Enable Openflow
#
sub enableOpenflow($$) {
    my $self = shift;
    my $vlan = shift;
    my $RetVal;
    
    #
    # Cisco switch doesn't support Openflow yet.
    #
    warn "ERROR: Cisco swith doesn't support Openflow now";
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
    # Cisco switch doesn't support Openflow yet.
    #
    warn "ERROR: Cisco swith doesn't support Openflow now";
    return 0;
}

#
# Set controller
#
sub setOpenflowController($$$) {
    my $self = shift;
    my $vlan = shift;
    my $controller = shift;
    my $RetVal;
    
    #
    # Cisco switch doesn't support Openflow yet.
    #
    warn "ERROR: Cisco swith doesn't support Openflow now";
    return 0;
}

#
# Set listener
#
sub setOpenflowListener($$$) {
    my $self = shift;
    my $vlan = shift;
    my $listener = shift;
    my $RetVal;
    
    #
    # Cisco switch doesn't support Openflow yet.
    #
    warn "ERROR: Cisco swith doesn't support Openflow now";
    return 0;
}

#
# Get used listener ports
#
sub getUsedOpenflowListenerPorts($$) {
    warn "ERROR: Cisco swith doesn't support Openflow now\n";
}

#
# Check if Openflow is supported on this switch
#
sub isOpenflowSupported($) {
    #
    # Cisco switch doesn't support Openflow yet.
    #
    return 0;
}

# End with true
1;
