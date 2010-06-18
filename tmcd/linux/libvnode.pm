#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008-2010 University of Utah and the Flux Group.
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
	      makeIfaceMaps makeBridgeMaps
	      findControlNet findIface findMac
	      existsBridge findBridge findBridgeIfaces
              findVirtControlNet findDNS downloadImage setState
            );

use Data::Dumper;
use libtmcc;

sub VNODE_STATUS_RUNNING() { return "running"; }
sub VNODE_STATUS_STOPPED() { return "stopped"; }
sub VNODE_STATUS_MOUNTED() { return "mounted"; }
sub VNODE_STATUS_BOOTING() { return "booting"; }
sub VNODE_STATUS_INIT()    { return "init"; }
sub VNODE_STATUS_STOPPING(){ return "stopping"; }
sub VNODE_STATUS_UNKNOWN() { return "unknown"; }

#
# Magic control network config parameters.
#
my $PCNET_IP_FILE   = "/var/emulab/boot/myip";
my $PCNET_MASK_FILE = "/var/emulab/boot/mynetmask";
my $PCNET_GW_FILE   = "/var/emulab/boot/routerip";
my $VCNET_NET	    = "172.16.0.0";
my $VCNET_MASK      = "255.240.0.0";
my $VCNET_GW	    = "172.16.0.1";

my $debug = 0;

sub mysystem($);

sub setDebug($) {
    $debug = shift;
    print "libvnode: debug=$debug\n"
	if ($debug);
}

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

my %if2mac = ();
my %mac2if = ();
my %ip2if = ();
my %ip2mask = ();
my %ip2net = ();
my %ip2maskbits = ();

#
# Grab iface, mac, IP info from /sys and /sbin/ip.
#
sub makeIfaceMaps()
{
    # clean out anything
    %if2mac = ();
    %mac2if = ();
    %ip2if = ();
    %ip2net = ();
    %ip2mask = ();
    %ip2maskbits = ();

    my $devdir = '/sys/class/net';
    opendir(SD,$devdir) 
	or die "could not find $devdir!";
    my @ifs = grep { /^[^\.]/ && -f "$devdir/$_/address" } readdir(SD);
    closedir(SD);

    foreach my $iface (@ifs) {
	if ($iface =~ /^([\w\d\-_]+)$/) {
	    $iface = $1;
	}
	else {
	    next;
	}

	open(FD,"/sys/class/net/$iface/address") 
	    or die "could not open /sys/class/net/$iface/address!";
	my $mac = <FD>;
	close(FD);
	next if (!defined($mac) || $mac eq '');

	$mac =~ s/://g;
	chomp($mac);
	$if2mac{$iface} = $mac;
	$mac2if{$mac} = $iface;

	# also find ip, ugh
	my $pip = `ip addr show dev $iface | grep 'inet '`;
	chomp($pip);
	if ($pip =~ /^\s+inet\s+(\d+\.\d+\.\d+\.\d+)\/(\d+)/) {
	    my $ip = $1;
	    $ip2if{$ip} = $iface;
	    my @ip = split(/\./,$ip);
	    my $bits = int($2);
	    my @netmask = (0,0,0,0);
	    my ($idx,$counter) = (0,8);
	    for (my $i = $bits; $i > 0; --$i) {
		--$counter;
		$netmask[$idx] += 2 ** $counter;
		if ($counter == 0) {
		    $counter = 8;
		    ++$idx;
		}
	    }
	    my @network = ($ip[0] & $netmask[0],$ip[1] & $netmask[1],
			   $ip[2] & $netmask[2],$ip[3] & $netmask[3]);
	    $ip2net{$ip} = join('.',@network);
	    $ip2mask{$ip} = join('.',@netmask);
	    $ip2maskbits{$ip} = $bits;
	}
    }

    if ($debug > 1) {
	print STDERR "makeIfaceMaps:\n";
	print STDERR "if2mac:\n";
	print STDERR Dumper(%if2mac) . "\n";
	#print STDERR "mac2if:\n";
	#print STDERR Dumper(%mac2if) . "\n";
	print STDERR "ip2if:\n";
	print STDERR Dumper(%ip2if) . "\n";
	print STDERR "\n";
    }

    return 0;
}

#
# Find control net iface info.  Returns:
# (iface_name,IP,IPmask,IPmaskbits,IPnet,MAC,GW)
#
sub findControlNet()
{
    my $ip = (-r $PCNET_IP_FILE) ? `cat $PCNET_IP_FILE` : "0";
    chomp($ip);
    if ($ip =~ /^(\d+\.\d+\.\d+\.\d+)$/) {
	$ip = $1;
    } else {
	die "Could not find valid control net IP (no $PCNET_IP_FILE?)";
    }
    my $gw = (-r $PCNET_GW_FILE) ? `cat $PCNET_GW_FILE` : "0";
    chomp($gw);
    if ($gw =~ /^(\d+\.\d+\.\d+\.\d+)$/) {
	$gw = $1;
    } else {
	die "Could not find valid control net GW (no $PCNET_GW_FILE?)";
    }
    return ($ip2if{$ip}, $ip, $ip2mask{$ip}, $ip2maskbits{$ip}, $ip2net{$ip},
	    $if2mac{$ip2if{$ip}}, $gw);
}

#
# Find virtual control net iface info.  Returns:
# (net,mask,GW)
#
sub findVirtControlNet()
{
    return ($VCNET_NET, $VCNET_MASK, $VCNET_GW);
}

sub findIface($) {
    my $mac = shift;

    $mac =~ s/://g;
    return $mac2if{$mac}
        if (exists($mac2if{$mac}));

    return undef;
}

sub findMac($) {
    my $iface = shift;

    return $if2mac{$iface}
        if (exists($if2mac{$iface}));

    return undef;
}

my %bridges = ();
my %if2br = ();

sub makeBridgeMaps() {
    # clean out anything...
    %bridges = ();
    %if2br = ();

    my @lines = `brctl show`;
    # always get rid of the first line -- it's the column header 
    shift(@lines);
    my $curbr = '';
    foreach my $line (@lines) {
	if ($line =~ /^([\w\d\-]+)\s+/) {
	    $curbr = $1;
	    $bridges{$curbr} = [];
	}
	if ($line =~ /^[^\s]+\s+[^\s]+\s+[^\s]+\s+([\w\d\-\.]+)$/ 
	    || $line =~ /^\s+([\w\d\-\.]+)$/) {
	    push @{$bridges{$curbr}}, $1;
	    $if2br{$1} = $curbr;
	}
    }

    if ($debug > 1) {
	print STDERR "makeBridgeMaps:\n";
	print STDERR "bridges:\n";
	print STDERR Dumper(%bridges) . "\n";
	print STDERR "if2br:\n";
	print STDERR Dumper(%if2br) . "\n";
	print STDERR "\n";
    }

    return 0;
}

sub existsBridge($) {
    my $bname = shift;

    return 1
        if (exists($bridges{$bname}));

    return 0;
}

sub findBridge($) {
    my $iface = shift;

    return $if2br{$iface}
        if (exists($if2br{$iface}));

    return undef;
}

sub findBridgeIfaces($) {
    my $bname = shift;

    return @{$bridges{$bname}}
        if (exists($bridges{$bname}));

    return undef;
}

#
# Since (some) vnodes are imageable now, we provide an image fetch
# mechanism.  Caller provides an imagepath for frisbee, and a hash of args that
# comes directly from loadinfo.
#
sub downloadImage($$$) {
    my ($imagepath,$todisk,$reload_args_ref) = @_;

    return -1 
	if (!defined($imagepath) || !defined($reload_args_ref));

    my $addr = $reload_args_ref->{"ADDR"};
    my $FRISBEE = "/usr/local/etc/emulab/frisbee";
    my $IMAGEUNZIP = "/usr/local/bin/imageunzip";

    if ($addr =~/^(\d+\.\d+\.\d+\.\d+):(\d+)$/) {
	my $mcastaddr = $1;
	my $mcastport = $2;

	mysystem("$FRISBEE -m $mcastaddr -p $mcastport $imagepath");
    }
    elsif ($addr =~ /^http/) {
	if ($todisk) {
	    mysystem("wget -nv -N -P -O - '$addr' | $IMAGEUNZIP - $imagepath");
	} else {
	    mysystem("wget -nv -N -P -O $imagepath '$addr'");
	}
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
# XXX boss is the DNS server for everyone
#
sub findDNS($)
{
    my ($ip) = @_;

    my ($bossname,$bossip) = libtmcc::tmccbossinfo();
    if ($bossip =~ /^(\d+\.\d+\.\d+\.\d+)$/) {
	$bossip = $1;
    } else {
	die "Could not find boss IP address (tmccbossinfo failed?)";
    }

    return $bossip;
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
