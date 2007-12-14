#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;

# Drag in path stuff so we can find emulab stuff.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }
my $DOSTYPE = "$BINDIR/dostype";

#
# This file goes in boss:/z/testbed/distributions on boss so that is can
# be passed over via wget to the CDROM on widearea nodes.
#
sub usage()
{
    print("Usage: mkextrafs.pl [-f] [-s slice] [-r disk] <mountpoint>\n");
    exit(-1);
}
my $optlist    = "fs:r:cn";
my $diskopt;
my $checkit    = 0;
my $forceit    = 0;
my $noinit     = 0;

#
# Yep, hardwired for now.  Should be options or queried via TMCC.
#
my $disk       = "ad0";
my $slice      = "4";
my $partition  = "e";

sub mysystem($);

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
if (defined($options{"s"})) {
    $slice = $options{"s"};
}
if (defined($options{"c"})) {
    $checkit = 1;
}
if (defined($options{"c"})) {
    $checkit = 1;
}
if (defined($options{"r"})) {
    $diskopt = $options{"r"};
}
if (defined($options{"n"})) {
    $noinit = $options{"n"};
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
# XXX determine the disk based on the root fs if not provided.
#
if (defined($diskopt)) {
    $disk = $diskopt;
}
else {
    my $rootdev = `df | egrep '/\$'`;
    if ($rootdev =~ /^\/dev\/([a-z]+\d+)s[1-4][a-h]/) {
	$disk = $1;
    }
}

my $slicedev   = "${disk}s${slice}";
my $fsdevice   = "/dev/${slicedev}${partition}";

#
# An existing fstab entry indicates we have already done this
# XXX override with forceit?  Would require unmounting and removing from fstab.
#
if (!system("egrep -q -s '^${fsdevice}' /etc/fstab")) {
    if ($checkit) {
	exit(0);
    }
    die("*** $0:\n".
	"    There is already an entry in /etc/fstab for $fsdevice\n");
}
elsif ($checkit) {
    exit(1);
}

#
# Likewise, if already mounted somewhere, fail
#
my $mounted = `mount | egrep '^${fsdevice}'`;
if ($mounted =~ /^${fsdevice} on (\S*)/) {
    die("*** $0:\n".
	"    $fsdevice is already mounted on $1\n");
}

my $slicesetup = `fdisk -s ${disk} | grep '^[ ]*${slice}:'`;
if ($slicesetup =~ /^[ ]*${slice}:\s*(\d*)\s*(\d*)\s*(0x\S\S)\s*/) {
    $stype = hex($3);
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
	    "    non-zero partition type ($stype) for /dev/${slicedev}, ".
	    "use -f to override\n");
    }
} elsif ($stype && $stype != 165) {
    warn("*** $0: WARNING: changing partition type from $stype to 165\n");
}

#
# Set the partition type to BSD if not already set.
#
if ($stype != 165) {
    die("*** $0:\n".
	"    No $DOSTYPE program, cannot set type of DOS partition\n")
	if (! -e "$DOSTYPE");
    mysystem("$DOSTYPE -f /dev/$disk $slice 165");
}
elsif ($noinit) {
    mysystem("mount $fsdevice $mountpoint");
    mysystem("echo \"$fsdevice $mountpoint ufs rw 0 2\" >> /etc/fstab");
    exit(0);
}

#
# Now create the disklabel
#
my $tmpfile = "/tmp/disklabel";
mysystem("disklabel -w -r $slicedev auto");
mysystem("disklabel -r $slicedev > $tmpfile");

#
# Tweak the disk label to ensure there is an 'e' partition
# In FreeBSD 4.x, we just add one.
# In FreeBSD 5.x, we replace the 'a' partition which is created by default.
#
open(DL, "+<$tmpfile") or
    die("*** $0:\n".
	"    Could not edit temporary label in $tmpfile!\n");
my @dl = <DL>;
if (scalar(grep(/^  ${partition}: /, @dl)) != 0) {
    die("*** $0:\n".
	"    Already an \'$partition\' partition?!\n");
}
seek(DL, 0, 0);
truncate(DL, 0);

my $done = 0;
foreach my $line (@dl) {
    # we assume that if the 'a' paritition exists, it is the correct one
    my $pat = q(^  a: );
    if (!$done && $line =~ /$pat/) {
	$line =~ s/$pat/  e: /;
	$done = 1;
    }
    $pat = q(^  c: );
    if (!$done && $line =~ /$pat/) {
	print DL $line;
	$line =~ s/$pat/  e: /;
	$done = 1;
    }
    print DL $line;
}
close(DL);
mysystem("disklabel -R -r $slicedev $tmpfile");
unlink($tmpfile);

# FreeBSD 5 doesn't have MAKEDEV
mysystem("cd /dev; ./MAKEDEV ${slicedev}c")
    if (-e "/dev/MAKEDEV");

mysystem("newfs -U -i 25000 $fsdevice");
mysystem("echo \"$fsdevice $mountpoint ufs rw 0 2\" >> /etc/fstab");

mysystem("mount $mountpoint");
mysystem("mkdir $mountpoint/local");
exit(0);

sub mysystem($)
{
    my ($command) = @_;

    if (0) {
	print "'$command'\n";
    }
    else {
	print "'$command'\n";
	my $rv = system($command);
	if ($rv) {
	    die("*** $0:\n".
		"    Failed ($rv): '$command'\n");
	}
    }
    return 0
}
