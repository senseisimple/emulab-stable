#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;

#
# This file goes in boss:/z/testbed/distributions on boss so that is can
# be passed over via wget to the CDROM on widearea nodes.
#
sub usage()
{
    print("Usage: mkextrafs.pl <mountpoint>\n");
    exit(-1);
}
my  $optlist = "";

#
# Turn off line buffering on output
#
STDOUT->autoflush(1);
STDERR->autoflush(1);

#
# Untaint the environment.
# 
$ENV{'PATH'} = "/tmp:/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:".
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin:/usr/local/etc/testbed";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV != 1) {
    usage();
}
my $mountpoint  = $ARGV[0];

if (! -d $mountpoint) {
    die("*** $0:\n".
	"    $mountpoint does not exist!\n");
}

#
# Yep, hardwired for now.
#
my $rawdevice  = "ad0";
my $slice      = "4";
my $partition  = "e";
my $fsdevice   = "/dev/${rawdevice}s${slice}${partition}";
my $slice4setup= `fdisk -s $rawdevice | grep '^[ ]*4:'`;

if (!system("egrep -q -s '^${fsdevice}' /etc/fstab")) {
    die("*** $0:\n".
	"    There is already an entry in /ets/fstab for $fsdevice\n");
}
if ($slice4setup =~ /^[ ]*4:\s*(\d*)\s*(\d*)/) {
    mysystem("echo \"p 4 165 $1 $2\" | fdisk -f - $rawdevice")
}
else {
    die("*** $0:\n".
	"    Could not parse slice 4 fdisk entry!\n");
}
mysystem("disklabel -w -r ${rawdevice}s${slice} auto");

# Make sure the kernel really has the new label!
mysystem("disklabel -r ${rawdevice}s${slice} > /tmp/disklabel");
mysystem("disklabel -R -r ${rawdevice}s${slice} /tmp/disklabel");

$ENV{'EDITOR'} = "ed";

open(ED, "| disklabel -e -r ${rawdevice}s${slice}") or
    die("*** $0:\n".
	"    Oops, could not edit label on ${rawdevice}s${slice}!\n");

print ED "/^  c: /\n";
print ED "t\n";
print ED "s/c: /${partition}: /\n";
print ED "w\n";
print ED "q\n";
if (!close(ED)) {
    die("*** $0:\n".
	"    Oops, error editing label on ${rawdevice}s${slice}!\n");
}
mysystem("newfs -U ${rawdevice}s${slice}${partition}");
mysystem("echo \"$fsdevice $mountpoint ufs rw 0 2\" >> /etc/fstab");

mysystem("mount $mountpoint");

sub mysystem($)
{
    my ($command) = @_;

    if (0) {
	print "'$command'\n";
    }
    else {
	print "'$command'\n";
	system($command);
	if ($?) {
	    die("*** $0:\n".
		"    Failed: '$command'\n");
	}
    }
    return 0
}
