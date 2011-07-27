#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
package emulabpaths;
use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw( $BINDIR $ETCDIR $VARDIR $BOOTDIR $DBDIR $LOGDIR $LOCKDIR $BLOBDIR);

#
# This path stuff will go away when the world is consistent. Until then
# we need to be able to upgrade individual scripts to the various setups.
# I know, what a mess.
#
$BINDIR  = "";
$ETCDIR  = "";
$VARDIR  = "";
$BOOTDIR = "";
$DBDIR   = "";
$LOGDIR  = "/var/tmp";
$LOCKDIR = "/var/tmp";

if (-d "/usr/local/etc/emulab") {
    $BINDIR = "/usr/local/etc/emulab";
    unshift(@INC, "/usr/local/etc/emulab");
    if (-d "/etc/emulab") {
	$ETCDIR = "/etc/emulab";
    }
    else {
	$ETCDIR = "/usr/local/etc/emulab";
    }
    $VARDIR  = "/var/emulab";
    $BOOTDIR = "/var/emulab/boot";
    $LOGDIR  = "/var/emulab/logs";
    $LOCKDIR = "/var/emulab/lock";
    $DBDIR   = "/var/emulab/db";
}
elsif (-d "/etc/testbed") {
    unshift(@INC, "/etc/testbed");
    $ETCDIR  = "/etc/testbed";
    $BINDIR  = "/etc/testbed";
    $VARDIR  = "/etc/testbed";
    $BOOTDIR = "/etc/testbed";
    $DBDIR   = "/etc/testbed";
}
elsif (-d "/etc/rc.d/testbed") {
    unshift(@INC, "/etc/rc.d/testbed");
    $ETCDIR  = "/etc/rc.d/testbed";
    $BINDIR  = "/etc/rc.d/testbed";
    $VARDIR  = "/etc/rc.d/testbed";
    $BOOTDIR = "/etc/rc.d/testbed";
    $DBDIR   = "/etc/rc.d/testbed";
}
else {
    print "$0: Cannot find proper emulab paths!\n";
    exit 1;
}

$BLOBDIR = $BOOTDIR;

#
# Untaint path
#
$ENV{'PATH'} = "$BINDIR:/bin:/sbin:/usr/bin:/usr/sbin:".
    "/usr/local/bin:/usr/local/sbin:/usr/site/bin:/usr/site/sbin";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

1;

