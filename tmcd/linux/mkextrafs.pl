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

# Drag in path stuff so we can find emulab stuff.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }
my $DOSTYPE = "$BINDIR/dostype";

sub mysystem($);

sub usage()
{
    print("Usage: mkextrafs.pl [-f] [-lM] [-v <vglist>] <mountpoint>\n");
    exit(-1);
}
my  $optlist = "flv:M";

#
# Yep, hardwired for now.  Should be options or queried via TMCC.
#
my $disk       = "hda";
my $slice      = "4";
my $partition  = "";

my $forceit    = 0;

my $lvm        = 0;
my @vglist     = ();
my $lmonster   = 0;

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
if (defined($options{"l"})) {
    $lvm = 1;
}
if (defined($options{"v"})) {
    @vglist = split(/,/,$options{"v"});
}
if (defined($options{"M"})) {
    $lmonster = 1;
    if (scalar(@vglist)) {
	die("*** $0:\n".
	    "    If you want a single giant LVM-based filesystem, you can" . 
	    "    only specify a single volume group name!\n");
    }
}
if (scalar(@vglist) == 0) {
    @vglist = ('emulab',);
}
my $mountpoint;
if (!$lvm || ($lvm && $lmonster)) {
    if (@ARGV != 1) {
	usage();
    }
    $mountpoint  = $ARGV[0];
    
    if (! -d $mountpoint) {
	die("*** $0:\n".
	    "    $mountpoint does not exist!\n");
    }
}

#
# XXX determine the disk based on the root fs
#
my $rootdev = `df | egrep '/\$'`;
if ($rootdev =~ /^\/dev\/([a-z]+)\d+\s+/) {
    $disk = $1;
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
# Before we do anything, do lvm if necessary and do not make any filesystems
# inside the vgs unless they want a single monster fs.
#
if ($lvm) {
    my $retval = 0;

    my $blockdevs = "$fsdevice";
    if ($retval = system("pvcreate $blockdevs")) {
	die("*** $0:\n".
	    "    'pvcreate $blockdevs' failed!\n");
    }

    foreach my $vg (@vglist) {
	if (system("vgcreate $vg $blockdevs")) {
	    die("*** $0:\n".
		"    'vgcreate $vg $blockdevs' failed!\n");
	}
    }

    if ($lmonster) {
	if (system("lvcreate -n emulab -l 100\%VG $vglist[0]")) {
	    die("*** $0:\n".
		"    'lvcreate -n emulab -l 100\%VG $vglist[0]' failed!\n");
	}
	$fsdevice = "/dev/$vglist[0]/emulab";
    }
    else {
	exit(0);
    }
}

#
# Set the partition type to Linux if not already set.
#
# XXX sfdisk appears to stomp on partition one's bootblock, at least if it
# is BSD.  It zeros bytes in the block 0x200-0x400, I suspect it is attempting
# to invalidate any BSD disklabel.  While we could just use a scripted fdisk
# sequence here instead, sfdisk is so much more to-the-point.  So, we just
# save off the bootblock, run sfdisk and put the bootblock back.
#
# Would it seek out and destroy other BSD partitions?  Don't know.
# I cannot find the source for sfdisk.
#
if (!$lvm && $stype != 131) {
    die("*** $0:\n".
	"    No $DOSTYPE program, cannot set type of DOS partition\n")
	if (! -e "$DOSTYPE");
    mysystem("$DOSTYPE -f /dev/$disk $slice 131");
}

# eh, quick try for ext3 -- no way we can consistently check the kernel for 
# support, off the top of my head
if ( -e "/sbin/mkfs.ext3") {
    mysystem("mke2fs -j $fsdevice");
    mysystem("echo \"$fsdevice $mountpoint ext3 defaults 0 0\" >> /etc/fstab");
}
else {
    mysystem("mkfs $fsdevice");
    mysystem("echo \"$fsdevice $mountpoint ext2 defaults 0 0\" >> /etc/fstab");
}

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
