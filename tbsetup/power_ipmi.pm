#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006-2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# module for controlling SuperMicro IPMI cards
# needs a working "/usr/local/bin/ipmitool" binary installed
# (try pkg_add -r ipmitool)
#
# supports new(ip), power(on|off|cyc[le]), status
#

package power_ipmi;

$| = 1; # Turn off line buffering on output

use strict;
$ENV{'PATH'} = '/bin:/usr/bin:/usr/local/bin'; # Required when using system() or backticks `` in combination with the perl -T taint checks

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
        print "power_ipmi module initializing... debug level $debug\n";
    }

    my $self = {};

    $self->{DEBUG} = $debug;

    $self->{DEVICENAME} = $devicename;
    $self->{un} = "ADMIN";              # default username
    $self->{pw} = "ADMIN";              # default password

    # Do a quick query, to see if it works
    system("ipmitool -H $self->{DEVICENAME} -U $self->{un} -P $self->{pw} power status >/dev/null 2>\&1");
    if ( $? != 0 ) {                    # system() sets $? to a non-zero value in case of failure
        warn "ERROR: Unable to connect to $devicename via IPMI\n";
        return undef;
    }
    else {
        bless($self,$class);
        return $self;
    }
}

sub power {
    my $self = shift;
    my $op = shift;
    my @ports = @_;   # Doesn't really matter, an IPMI card only has a single "outlet", which is for the node the card is installed in

    my $errors = 0;
    my ($retval, $output);

    if    ($op eq "on")  { $op = "power on";    }
    elsif ($op eq "off") { $op = "power off";   }
    elsif ($op =~ /cyc/) { $op = "power reset"; }

    ($retval,$output) = $self->_execipmicmd($op);
    if ( $retval != 0 ) {        # increment $errors if return value was not 0
        $errors++;
        print STDERR $self->{DEVICENAME}, ": could not control power status of device\n";
    }

    return $errors;
}

sub status {
    my $self = shift;
    my $statusp = shift; # pointer to an associative (hashed) array (i.o.w. passed by reference)
    my %status;          # local associative array which we'll pass back through $statusp

    my $errors = 0;
    my ($retval, $output);


    # Get power status (i.e. whether system is on/off)
    ($retval,$output) = $self->_execipmicmd("power status");
    if ( $retval != 0 ) {
        $errors++;
        print STDERR $self->{DEVICENAME}, ": could not get power status from device\n";
    }
    else { $status{'outlet1'} = $output; print("Power status is: $output\n") if $self->{DEBUG}; } # there's only 1 "outlet" on an IPMI card


    # Get Sensor Data Repository (sdr) entries and readings
    ($retval,$output) = $self->_execipmicmd("sdr");
    if ( $retval != 0 ) {
        $errors++;
        print STDERR $self->{DEVICENAME}, ": could not get sensor data from device\n";
    }
    else { $status{'sdr'} = $output; print("SDR data is:\n$output\n") if $self->{DEBUG}; }


    if ($statusp) { %$statusp = %status; } # update passed-by-reference array
    return $errors;
}

sub _execipmicmd {                         # _ indicates that this is a private method (Perl convention)
    my ($self,$op) = @_;
    my $ipmicmd = "ipmitool -H $self->{DEVICENAME} -U $self->{un} -P $self->{pw} "; # ipmitool command syntax, to be completed
                                                                                    # with an operation to perform
    my $cmd = $ipmicmd . $op;
    my $output;

    if ( $self->{DEBUG} > 1 ) {
        print STDERR "**** Executing $cmd\n";
        $output = `$cmd`;                  # get output of the $cmd, e.g. many lines of sensor readings or "Chassis power is on"
        chomp ( $output );                 # remove the \n at the end, if any
    }
    else { $output = `$cmd 2>/dev/null`; } # if there's any stderr output, silence it
                                           # (system() and backticks `` execute the command using the /bin/sh shell)

    return ($?, $output);
}

# End with true
1;
