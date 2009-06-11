#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008-2009 University of Utah and the Flux Group.
# All rights reserved.
#
# Implements the libvnode API for OpenVZ support in Emulab.
#
package libvnode;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( VNODE_STATUS_RUNNING VNODE_STATUS_STOPPED VNODE_STATUS_BOOTING 
              VNODE_STATUS_INIT VNODE_STATUS_STOPPING VNODE_STATUS_UNKNOWN
              ipToMac macAddSep fatal mysystem
            );

sub VNODE_STATUS_RUNNING() { return "running"; }
sub VNODE_STATUS_STOPPED() { return "stopped"; }
sub VNODE_STATUS_BOOTING() { return "booting"; }
sub VNODE_STATUS_INIT() { return "init"; }
sub VNODE_STATUS_STOPPING() { return "stopping"; }
sub VNODE_STATUS_UNKNOWN() { return "unknown"; }

sub ipToMac($) {
    my $ip = shift;

    return sprintf("0000%02x%02x%02x%02x",$1,$2,$3,$4)
	if ($ip =~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/);

    return undef;
}

sub macAddSep($;$) {
    my ($mac,$sep) = @_;
    if (!defined($sep)) {
	$sep = ":";
    }

    return "$1$sep$2$sep$3$sep$4$sep$5$sep$6"
	if ($mac =~ /^([0-9a-zA-Z]{2})([0-9a-zA-Z]{2})([0-9a-zA-Z]{2})([0-9a-zA-Z]{2})([0-9a-zA-Z]{2})([0-9a-zA-Z]{2})$/);

    return undef;
}

#
# Print error and exit.
#
sub fatal($)
{
    my ($msg) = @_;

    die("*** $0:\n".
	"    $msg\n");
}

#
# Run a command string, redirecting output to a logfile.
#
sub mysystem($)
{
    my ($command) = @_;

    if (1) {
	print STDERR "mysystem: '$command'\n";
    }

    system($command);
    if ($?) {
	fatal("Command failed: $? - $command");
    }
}

#
# Life's a rich picnic.  And all that.
1;
