#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
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
    print("Usage: mkextrafs.pl [-f] [-s slice] <mountpoint>\n");
    exit(-1);
}
my  $optlist = "fs:";

#
# Yep, hardwired for now.  Should be options or queried via TMCC.
#
my $disk       = "ad0";
my $slice      = "4";
my $partition  = "e";
my $forceit    = 0;

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
if (@ARGV != 1) {
    usage();
}
my $mountpoint  = $ARGV[0];

if (! -d $mountpoint) {
    die("*** $0:\n".
	"    $mountpoint does not exist!\n");
}

#
# XXX determine the disk based on the root fs
#
my $rootdev = `df | egrep '/\$'`;
if ($rootdev =~ /^\/dev\/([a-z]+\d+)s[1-4][a-h]/) {
    $disk = $1;
}

my $slicedev   = "${disk}s${slice}";
my $fsdevice   = "/dev/${slicedev}${partition}";

#
# An existing fstab entry indicates we have already done this
# XXX override with forceit?  Would require unmounting and removing from fstab.
#
if (!system("egrep -q -s '^${fsdevice}' /etc/fstab")) {
    if ($checkit) {
	exit(-1);
    }
    die("*** $0:\n".
	"    There is already an entry in /etc/fstab for $fsdevice\n");
}
elsif ($checkit) {
    exit(0);
}

#
# Likewise, if already mounted somewhere, fail
#
my $mounted = `mount | egrep '^${fsdevice}'`;
if ($mounted =~ /^${fsdevice} on (\S*)/) {
    die("*** $0:\n".
	"    $fsdevice is already mounted on $1\n");
}

my ($sstart, $ssize, $stype) = getslicestuff($disk, $slice);

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
# Set the partition type to BSD
#
my $tmpfile = "/tmp/disklabel";
mysystem("echo \"p $slice 165 $sstart $ssize\" | fdisk -f - $disk");

#
# Hmm...fdisk may choose to quietly modify the size downward from what we
# gave it, to ensure "cylinder alignment".  (It may also change the start,
# but we are going to ignore that case as we are completely screwed if
# that happens...)  Unfortunately, the modified size doesn't show up
# "in kernel" til we reboot, so the "disklabel auto" command below will
# create a label based on the incorrect larger size.  We of course, create
# our filesystem based on the disklabel size, so the result is a filesystem
# that works until you reboot, at which point some of it is outside the
# slice and partition boundary.  Bad news.
#
# We use fdisk to reread the partition size.  At least fdisk is internally
# consistant and will report the size that it truncated the partition to.
# If fdisk reports a different value than it did originally, we record the
# smaller size which is used later when we edit the disklabel.
#
my ($nsstart, $nssize, $nstype) = getslicestuff($disk, $slice);
if ($nsstart != $sstart || $nstype != 165) {
    die("*** $0:\n".
	"    fdisk changed type or start?!\n");
}
if ($nssize < $ssize) {
    warn("*** $0: WARNING: lowered partition size from $ssize to $nssize\n");
} else {
    $nssize = $ssize;
}

#
# Now create the disklabel
#
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
	if ($ssize != $nssize) {
	    $line =~ s/$ssize/$nssize/;
	}
	$done = 1;
    }
    $pat = q(^  c: );
    if (!$done && $line =~ /$pat/) {
	if ($ssize != $nssize) {
	    $line =~ s/$ssize/$nssize/;
	}
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

sub getslicestuff($$) {
    my ($_disk, $_slice) = @_;

    my $slicesetup = `fdisk -s ${_disk} | grep '^[ ]*${_slice}:'`;
    my $sstart;
    my $ssize;
    my $stype;

    if ($slicesetup =~ /^[ ]*${_slice}:\s*(\d*)\s*(\d*)\s*(0x\S\S)\s*/) {
	$sstart = $1;
	$ssize = $2;
	$stype = hex($3);
    }
    else {
	die("*** $0:\n".
	    "    Could not parse slice $slice fdisk entry!\n");
    }
    return ($sstart, $ssize, $stype);
}

