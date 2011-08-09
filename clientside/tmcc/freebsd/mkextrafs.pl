#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
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
    print("Usage: mkextrafs.pl [-2fl] [-s slice] [-r disk] <mountpoint>\n");
    print("       [-r disk]  disk device to use (default: ad0)\n");
    print("       [-s slice] DOS partition to use or 0 for entire disk (default: 4)\n");
    print("       <mount>    where the new filesystem will be mounted\n");
    print("       -f         DANGEROUS! force creation of the filesystem\n");
    print("                   even if partition is in use\n");
    print("       -2         hack: break DOS partition into two unequal (70/30)\n");
    print("                   BSD partitions and mount the first\n");
    print("Usage: mkextrafs.pl -l [-f]\n");
    print("       -l  list available partitions\n");
    print("       -f  also list inactive partitions that have non-zero type\n");
    exit(-1);
}
my $optlist    = "2fls:r:cn";
my $diskopt;
my $checkit    = 0;
my $forceit    = 0;
my $noinit     = 0;
my $showonly   = 0;
my $twoparts   = 0;
my $debug      = 0;

#
# Yep, hardwired for now.  Should be options or queried via TMCC.
#
my $disk       = "ad0";
my $slice      = "4";
my $partition  = "e";

sub mysystem($);
sub showspace($);
sub os_find_freedisk($$); # XXX from liblocsetup.pm

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
# Determine if we should use the "geom" tools to make everything happen.
# Empirically, this seems to only be needed for FreeBSD 8 and above.
#
my $usegeom = 0;
if (`uname -r` =~ /^(\d+)\./ && $1 > 7) {
    $usegeom = $1;
}

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
if (defined($options{"r"})) {
    $diskopt = $options{"r"};
}
if (defined($options{"n"})) {
    $noinit = $options{"n"};
}
if (defined($options{"l"})) {
    $showonly = 1;
}
if (defined($options{"2"})) {
    $twoparts = 1;
}

if ($showonly) {
    showspace($forceit);
    exit(0);
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
# For entire disk, we will use "fdisk -I" to initialize DOS partition 1.
# Note: we will create the BSD 'e' partition later.
#
if ($slice == 0) {
    $slicedev = "${disk}s1";
    $fsdevice = "/dev/${slicedev}e";
}

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

#
# See what the current type is for the partition
#
my $stype = -1;
if (!open(FD, "fdisk -s /dev/$disk 2>&1|")) {
    die("*** $0:\n".
        "    $disk: could not get partition info\n");
}
while (<FD>) {
    #
    # No MBR should mean that the entire disk is unused.
    # If they want to use the whole disk (slice==0), this is okay.
    #
    if (/invalid fdisk partition table found/) {
	if ($slice != 0) {
	    die("*** $0:\n".
		"    $disk: no partition table!\n");
	}
	$stype = 0;
    }
    #
    # Format of fdisk output is:
    #  /dev/da0: 30394 cyl 255 hd 63 sec
    #  Part        Start Size     Type Flags
    #  1:          63    12305790 0xa5 0x80
    #  ...
    #
    if (/^\s*([1-4]):\s+\d+\s+\d+\s+(0x\S\S)\s+/) {
	my $part = $1;
	my $type = hex($2);

	#
	# If there is a valid partition on the disk and they are
	# using the entire disk without forcing, stop them!
	#
	if ($slice == 0) {
	    if ($type != 0 && !$forceit) {
		die("*** $0:\n".
		    "    There are valid partitions on the disk;".
		    " need to specify -f to overwrite entire disk!\n");
	    }
	    $stype = 0;
	}
	if ($slice == $part) {
	    $stype = $type;
	    last;
	}
    }
}	    
close(FD);

if ($stype == -1) {
    die("*** $0:\n".
        "    Could not find partition $slice fdisk entry!\n");
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
# Dark magic to allow us to modify the open boot disk
#
if ($usegeom && $slice != 0) {
    if ($stype != 0) {
	mysystem("gpart delete -i $slice $disk");
    }
    mysystem("gpart add -i $slice -t freebsd $disk");
    $stype = 165;
}

#
# If they are whacking the whole disk, just use "fdisk -I' to initialize
# partition 1 of the disk.
#
if ($slice == 0 && !$noinit) {
    mysystem("fdisk -I /dev/$disk");
}
#
# Otherwise set the partition type to BSD if not already set.
#
elsif ($stype != 165) {
    die("*** $0:\n".
	"    No $DOSTYPE program, cannot set type of DOS partition\n")
	if (! -e "$DOSTYPE");
    mysystem("$DOSTYPE -f /dev/$disk $slice 165");
}
#
# If not recreating the filesystems, just try to mount it
#
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
# In "two part" mode, break into two partitions 'e' (70%) and 'f' (30%)
# and use the first.  This is primarily for elabinelab use, where we need
# a smaller /share that is not on the same FS as /proj and /users.
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
my $fsize = 0;
my $foff = 0;
my $bad = 0;
foreach my $line (@dl) {
    # we assume that if the 'a' paritition exists, it is the correct one
    if (!$done && $line =~ /^\s+a:\s+(\d+)\s+(\d+)(.*)$/) {
	my $size = $1;
	my $off = $2;
	my $rest = $3;
	if ($twoparts) {
	    my $esize = int($size / 16 * 0.7) * 16;
	    $foff = $off + $esize;
	    $fsize = $size - $esize;
	    $line = "  e: $esize $off $rest";
	} else {
	    $line = "  e: $size $off $rest";
	}
	$done = 1;
    }
    # otherwise we use the 'c' partition as our model
    if (!$done && $line =~ /^\s+c:\s+(\d+)\s+(\d+)(.*)$/) {
        print DL $line;
	my $size = $1;
	my $off = $2;
	my $rest = $3;
	if ($twoparts) {
	    my $esize = int($size / 16 * 0.7) * 16;
	    $foff = $off + $esize;
	    $fsize = $size - $esize;
	    $line = "  e: $esize $off $rest";
	} else {
	    $line = "  e: $size $off $rest";
	}
	$done = 1;
    }
    if ($line =~ /^  f:/) {
	$bad = 1;
    }
    print DL $line;
}
if (!$bad && $twoparts && $fsize > 0) {
    print DL "  f: $fsize $foff unused 0 0\n";
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

sub sizestr($)
{
    my ($bytes) = @_;
    my ($divisor, $units);

    if ($bytes > 1000 * 1000 * 1000 * 1000) {
	$divisor = 1000 * 1000 * 1000 * 1000;
	$units = "TB";
    } elsif ($bytes > 1000 * 1000 * 1000) {
	$divisor = 1000 * 1000 * 1000;
	$units = "GB";
    } else {
	$divisor = 1000 * 1000;
	$units = "MB";
    }
    return sprintf "%6.2f%s", $bytes / $divisor, $units;
}

sub showspace($)
{
    my ($force) = @_;

    # XXX we keep this dynamic since this script can operate standalone
    require liblocsetup;

    my %diskinfo = liblocsetup::os_find_freedisk(0, $force);
    if (!%diskinfo) {
	print "No space found on node\n";
    }
    print "Found space";
    print " ('*' = in use, but unmounted)"
	if ($force);
    print ":\nDisk\tSlice\tBytes\n";
    foreach my $disk (sort keys %diskinfo) {
	my $ff = "";
	my $iu = "";
	if (exists($diskinfo{$disk}{"size"})) {
	    my $size = sizestr($diskinfo{$disk}{"size"});
	    if ($diskinfo{$disk}{"type"} != 0) {
		$ff = "-f";
		$iu = "*";
	    }
	    print "$disk$iu\t \t$size # mkextrafs -r $disk -s 0 $ff\n";
	} else {
	    foreach my $part (keys %{$diskinfo{$disk}}) {
		my $pnum;
		($pnum = $part) =~ s/.*(\d+)$/$1/;
		if (exists($diskinfo{$disk}{$part}{"size"})) {
		    my $size = sizestr($diskinfo{$disk}{$part}{"size"});
		    if ($diskinfo{$disk}{$part}{"type"} != 0) {
			$ff = "-f";
			$iu = "*";
		    }
		    print "$disk$iu\t$pnum\t$size # mkextrafs -r $disk -s $pnum $ff\n";
		}
	    }
	}
    }
}
