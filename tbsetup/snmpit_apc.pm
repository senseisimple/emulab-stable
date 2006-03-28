#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for APC MasterSwitch power controllers
#
# supports new(ip), power(on|off|cyc[le],port), status
#

package snmpit_apc;

$| = 1; # Turn off line buffering on output

use SNMP;
use strict;

sub new($$;$) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $devicename = shift;
    my $debug = shift;

    if (!defined($debug)) {
	$debug = 0;
    }

    if ($debug) {
	print "snmpit_apm module initializing... debug level $debug\n";
    }

    $SNMP::debugging = ($debug - 5) if $debug > 5;
    my $mibpath = "/usr/local/share/snmp/mibs";
    &SNMP::addMibDirs($mibpath);
    &SNMP::addMibFiles("$mibpath/RFC1155-SMI.txt",
	"$mibpath/PowerNet-MIB.txt");

    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();              # parses default list of Mib modules
    $SNMP::use_enums = 1;         #use enum values instead of only ints
    print "Opening SNMP session to $devicename..." if $debug;
    my $sess =new SNMP::Session(DestHost => $devicename, Community => 'private');
    if (!defined($sess)) {
	warn("ERROR: Unable to connect to $devicename via SNMP\n");
	return undef;
    }

    my $self = {};

    $self->{SESS} = $sess;
    $self->{DEBUG} = $debug;
    $self->{DEVICENAME} = $devicename;

    bless($self,$class);
    return $self;
}

sub power {
    my $self = shift;
    my $op = shift;
    my @ports = @_;

    my $CtlOID = ".1.3.6.1.4.1.318.1.1.4.4.2.1.3";
    if    ($op eq "on")  { $op = "outletOn";    }
    elsif ($op eq "off") { $op = "outletOff";   }
    elsif ($op =~ /cyc/) { $op = "outletReboot";}

    my $errors = 0;

    foreach my $port (@ports) {
	print STDERR "**** Controlling port $port\n" if ($self->{DEBUG} > 1);
	if ($self->UpdateField($CtlOID,$port,$op)) {
	    print STDERR "Outlet #$port control failed.\n";
	    $errors++;
	}
    }

    return $errors;
}

sub status {
    my $self = shift;
    my $statusp = shift;
    my %status;

    my $StatOID = ".1.3.6.1.4.1.318.1.1.4.2.2";
    my $Status = 0;

    $Status = $self->{SESS}->get([[$StatOID,0]]);
    if (!defined $Status) {
	print STDERR $self->{DEVICENAME}, ": no answer from device\n";
	return 1;
    }
    print("Status is '$Status'\n") if $self->{DEBUG};

    if ($statusp) {
	my @stats = split '\s+', $Status;
	my $o = 1;
	foreach my $ostat (@stats) {
	    my $outlet = "outlet$o";
	    $status{$outlet} = $ostat;
	    $o++;
	}
	%$statusp = %status;
    }
    return 0;
}

sub UpdateField {
    my ($self,$OID,$port,$val) = @_;

    print "sess=$self->{SESS} $OID $port $val\n" if $self->{DEBUG} > 1;

    my $Status = 0;
    my $retval;

    print "Checking port $port of $self->{DEVICENAME} for $val..." if $self->{DEBUG};
    $Status = $self->{SESS}->get([[$OID,$port]]);
    if (!defined $Status) {
	print STDERR "Port $port, change to $val: No answer from device\n";
	return 1;
    } else {
	print "Okay.\nPort $port was $Status\n" if $self->{DEBUG};
	if ($Status ne $val) {
	    print "Setting $port to $val..." if $self->{DEBUG};
	    $retval = $self->{SESS}->set([[$OID,$port,$val,"INTEGER"]]);
	    print "Set returned '$retval'" if $self->{DEBUG};
	    if ($retval) { return 0; } else { return 1; }
	}
	return 0;
    }
}

# End with true
1;
