#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;

sub mysystem($);

sub usage()
{
    print("Usage: mkextrafs.pl [-f] <mountpoint>\n");
    exit(-1);
}
my  $optlist = "f";

#
# Yep, hardwired for now.  Should be options or queried via TMCC.
#
my $disk       = "hda";
my $slice      = "4";
my $partition  = "";

my $forceit    = 0;

#
# Turn off line buffering on output
#
STDOUT->autoflush(1);
STDERR->autoflush(1);

#
# Untaint the environment.
# 
$ENV{'PATH'} = "/tmp:/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:".
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin:/usr/local/etc/emulab";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"f"})) {
    $forceit = 1;
}
if (@ARGV != 1) {
    usage();
}
my $mountpoint  = $ARGV[0];

if (! -d $mountpoint) {
    die("*** $0:\n".
	"    $mountpoint does not exist!\n");
}

my $diskdev    = "/dev/${disk}";
my $fsdevice   = "${diskdev}${slice}";

#
# An existing fstab entry indicates we have already done this
# XXX override with forceit?  Would require unmounting and removing from fstab.
#
if (!system("egrep -q -s '^${fsdevice}' /etc/fstab")) {
    die("*** $0:\n".
	"    There is already an entry in /etc/fstab for $fsdevice\n");
}

#
# Likewise, if already mounted somewhere, fail
#
my $mounted = `mount | egrep '^${fsdevice}'`;
if ($mounted =~ /^${fsdevice} on (\S*)/) {
    die("*** $0:\n".
	"    $fsdevice is already mounted on $1\n");
}

my $stype = `sfdisk $diskdev -c $slice`;
if ($stype ne "") {
    chomp($stype);
    $stype = hex($stype);
}
else {
    die("*** $0:\n".
	"    Could not parse slice $slice fdisk entry!\n");
}

#
# Fail if not forcing and the partition type is non-zero.
#
if (!$forceit) {
    if ($stype != 0) {
	die("*** $0:\n".
	    "    non-zero partition type ($stype) for ${disk}${slice}, ".
	    "use -f to override\n");
    }
} elsif ($stype && $stype != 131) {
    warn("*** $0: WARNING: changing partition type from $stype to 131\n");
}

#
# Set the partition type to Linux
#
mysystem("sfdisk $diskdev -c $slice 83");

mysystem("mkfs $fsdevice");
mysystem("echo \"$fsdevice $mountpoint ext2 defaults 0 0\" >> /etc/fstab");

mysystem("mount $mountpoint");
mysystem("mkdir $mountpoint/local");

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
