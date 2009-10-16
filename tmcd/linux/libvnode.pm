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
	      VNODE_STATUS_MOUNTED
              ipToMac macAddSep fatal mysystem mysystem2
              downloadImage setState
            );

use libtmcc;

sub VNODE_STATUS_RUNNING() { return "running"; }
sub VNODE_STATUS_STOPPED() { return "stopped"; }
sub VNODE_STATUS_MOUNTED() { return "mounted"; }
sub VNODE_STATUS_BOOTING() { return "booting"; }
sub VNODE_STATUS_INIT()    { return "init"; }
sub VNODE_STATUS_STOPPING(){ return "stopping"; }
sub VNODE_STATUS_UNKNOWN() { return "unknown"; }

sub mysystem($);

sub setState($) {
    my ($state) = @_;

    libtmcc::tmcc(TMCCCMD_STATE(),"$state");
}

#
# A spare disk or disk partition is one whose partition ID is 0 and is not
# mounted and is not in /etc/fstab AND is larger than 8GB.  Yes, this means 
# it's possible that we might steal partitions that are in /etc/fstab by 
# UUID -- oh well.
#
# This function returns a hash of device name => part number => size (bytes);
# note that a device name => size entry is filled IF the device has no
# partitions.
#
sub findSpareDisks() {
    my %retval = ();
    my %mounts = ();
    my %ftents = ();

    # /proc/partitions prints sizes in 1K phys blocks
    my $BLKSIZE = 1024;

    open (MFD,"/proc/mounts") 
	or die "open(/proc/mounts): $!";
    while (my $line = <MFD>) {
	chomp($line);
	if ($line =~ /^([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+([^\s]+)\s+/) {
	    $mounts{$1} = $2;
	}
    }
    close(MFD);

    open (FFD,"/etc/fstab") 
	or die "open(/etc/fstab): $!";
    while (my $line = <FFD>) {
	chomp($line);
	if ($line =~ /^([^\s]+)\s+([^\s]+)/) {
	    $ftents{$1} = $2;
	}
    }
    close(FFD);

    open (PFD,"/proc/partitions") 
	or die "open(/proc/partitions): $!";
    while (my $line = <PFD>) {
	chomp($line);
	if ($line =~ /^\s*\d+\s+\d+\s+(\d+)\s+([a-zA-Z]+)$/) {
	    if (!defined($mounts{"/dev/$2"}) && !defined($ftents{"/dev/$2"})) {
		$retval{$2}{"size"} = $BLKSIZE * $1;
	    }
	}
	elsif ($line =~ /^\s*\d+\s+\d+\s+(\d+)\s+([a-zA-Z]+)(\d+)$/) {
	    my ($dev,$part) = ($2,$3);

	    # XXX don't include extended partitions (the reason is to filter
	    # out pseudo partitions that linux creates for bsd disklabel 
	    # slices -- we don't want to use those!
	    # 
	    # (of course, a much better approach would be to check if a 
	    # partition is contained within another and not use it.)
	    next 
		if ($part > 4);

	    if (exists($retval{$dev}{"size"})) {
		delete $retval{$dev}{"size"};
		if (scalar(keys(%{$retval{$dev}})) == 0) {
		    delete $retval{$dev};
		}
	    }
	    if (!defined($mounts{"/dev/$dev$part"}) 
		&& !defined($ftents{"/dev/$dev$part"})) {

		# try checking its ext2 label
		my @outlines = `dumpe2fs -h /dev/$dev$part 2>&1`;
		if (!$?) {
		    my ($uuid,$label);
		    foreach my $line (@outlines) {
			if ($line =~ /^Filesystem UUID:\s+([-\w\d]+)/) {
			    $uuid = $1;
			}
			elsif ($line =~ /^Filesystem volume name:\s+([-\/\w\d]+)/) {
			    $label = $1;
			}
		    }
		    if ((defined($uuid) && defined($ftents{"UUID=$uuid"}))
			|| (defined($label) && defined($ftents{"LABEL=$label"})
			    && $ftents{"LABEL=$label"} eq $label)) {
			next;
		    }
		}

		# one final check: partition id
		my $output = `sfdisk --print-id /dev/$dev $part`;
		chomp($output);
		if ($?) {
		    print STDERR "WARNING: findSpareDisks: error running 'sfdisk --print-id /dev/$dev $part': $! ... ignoring /dev/$dev$part\n";
		}
		elsif ($output eq "0") {
		    $retval{$dev}{"$part"}{"size"} = $BLKSIZE * $1;
		}
	    }
	}
    }
    foreach my $k (keys(%retval)) {
	if (scalar(keys(%{$retval{$k}})) == 0) {
	    delete $retval{$k};
	}
    }
    close(PFD);

    return %retval;
}

#
# Since (some) vnodes are imageable now, we provide an image fetch
# mechanism.  Caller provides an imagepath for frisbee, and a hash of args that
# comes directly from loadinfo.
#
sub downloadImage($$) {
    my ($imagepath,$reload_args_ref) = @_;

    return -1 
	if (!defined($imagepath) || !defined($reload_args_ref));

    my $addr = $reload_args_ref->{"ADDR"};
    my $FRISBEE = "/usr/local/etc/emulab/frisbee";

    if ($addr =~/^(\d+\.\d+\.\d+\.\d+):(\d+)$/) {
	my $mcastaddr = $1;
	my $mcastport = $2;

	mysystem("$FRISBEE -m $mcastaddr -p $mcastport  $imagepath");
    }
    elsif ($addr =~ /^http/) {
	mysystem("wget -nv -N -P -O $imagepath '$addr'");
    }

    return 0;
}

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
sub mysystem2($)
{
    my ($command) = @_;

    if (1) {
	print STDERR "mysystem: '$command'\n";
    }

    system($command);
    if ($?) {
	print STDERR "Command failed: $? - '$command'\n";
    }
}

#
# Life's a rich picnic.  And all that.
1;
