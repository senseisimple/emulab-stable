#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008-2010 University of Utah and the Flux Group.
# All rights reserved.
#
# Implements the libvnode API for Xen support in Emulab.
#
# Note that there is no distinguished first or last call of this library
# in the current implementation.  Every vnode creation (through mkvnode.pl)
# will invoke all the root* and vnode* functions.  It is up to us to make
# sure that "one time" operations really are executed only once.
#
# TODO:
# + Clear out old, incorrect state in /var/lib/xend.
#   Maybe have to do this when tearing down (killing) vnodes.
#
# + Make more robust, little turds of state still get left around
#   that wreak havoc on reboot.
#
# + Support image loading.
#
package libvnode_xen;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( init setDebug rootPreConfig
              rootPreConfigNetwork rootPostConfig
	      vnodeCreate vnodeDestroy vnodeState
	      vnodeBoot vnodePreBoot vnodeHalt vnodeReboot
	      vnodeUnmount
	      vnodePreConfig vnodePreConfigControlNetwork
              vnodePreConfigExpNetwork vnodeConfigResources
              vnodeConfigDevices vnodePostConfig vnodeExec
	    );

%ops = ( 'init' => \&init,
         'setDebug' => \&setDebug,
         'rootPreConfig' => \&rootPreConfig,
         'rootPreConfigNetwork' => \&rootPreConfigNetwork,
         'rootPostConfig' => \&rootPostConfig,
         'vnodeCreate' => \&vnodeCreate,
         'vnodeDestroy' => \&vnodeDestroy,
         'vnodeState' => \&vnodeState,
         'vnodeBoot' => \&vnodeBoot,
         'vnodeHalt' => \&vnodeHalt,
# XXX needs to be implemented
         'vnodeUnmount' => \&vnodeUnmount,
         'vnodeReboot' => \&vnodeReboot,
# XXX needs to be implemented
         'vnodeExec' => \&vnodeExec,
         'vnodePreConfig' => \&vnodePreConfig,
         'vnodePreConfigControlNetwork' => \&vnodePreConfigControlNetwork,
         'vnodePreConfigExpNetwork' => \&vnodePreConfigExpNetwork,
         'vnodeConfigResources' => \&vnodeConfigResources,
         'vnodeConfigDevices' => \&vnodeConfigDevices,
         'vnodePostConfig' => \&vnodePostConfig,
       );


use strict;
use English;
use Data::Dumper;
use Socket;
use File::Basename;
use File::Path;
use File::Copy;

# Pull in libvnode
require "/etc/emulab/paths.pm"; import emulabpaths;
use libvnode;
use libtestbed;

#
# Turn off line buffering on output
#
$| = 1;

#
# Load the OS independent support library. It will load the OS dependent
# library and initialize itself. 
# 

##
## Standard utilities and files section
##

my $BRCTL = "/usr/sbin/brctl";
my $IFCONFIG = "/sbin/ifconfig";
my $ROUTE = "/sbin/route";
my $SYSCTL = "/sbin/sysctl";
my $VLANCONFIG = "/sbin/vconfig";
my $MODPROBE = "/sbin/modprobe";
my $DHCPCONF_FILE = "/etc/dhcpd.conf";

my $debug = 0;

##
## Randomly chosen convention section
##

# global lock
my $GLOBAL_CONF_LOCK = "xenconf";

# default image to load on logical disks
my %defaultImage = (
 'name'    => "emulab-ops-emulab-ops-XEN-GUEST-F8-XXX",
 'kernel'  => "/boot/vmlinuz-2.6.18.8-xenU",
 'ramdisk' => "/boot/initrd-2.6.18.8-xenU.img"
);

# where all our config files go
my $VMDIR = "/var/emulab/vms";
my $XENDIR = "/var/xen";

# Xen LVM volume group name
my $VGNAME = "xen-vg";

##
## Indefensible, arbitrary constant section
##

# Maximum vnodes per physical host, used to size memory and disks
my $MAX_VNODES = 8;

# Minimum GB of disk per vnode
my $MIN_GB_DISK = 8;

# Minimum MB of memory per vnode
my $MIN_MB_VNMEM = 64;

# Minimum memory for dom0
my $MIN_MB_DOM0MEM = 256;

# Minimum acceptible size (in GB) of LVM VG for domUs.
my $XEN_MIN_VGSIZE = ($MAX_VNODES * $MIN_GB_DISK);

# XXX fixed-for-now LV size for all logical disks
my $XEN_LDSIZE = $MIN_GB_DISK;

#
# Information about a virtual node accumulated over the various vnode* calls.
# Gets output in a persistent form right before we boot and we know that
# everything succeeded.
#
my %vninfo = ();

# Local functions
sub findRoot();
sub copyRoot($$);
sub createRootDisk($);
sub replace_hacks($);
sub disk_hacks($);
sub configFile($);
sub domain0Memory();
sub totalMemory();
sub memoryPerVnode();
sub hostIP($);
sub createDHCP($);
sub addDHCP($$$$);
sub subDHCP($$);
sub formatDHCP($$$);
sub fixupMac($);
sub createControlNetworkScript($$$);
sub createExpNetworkScript($$$$);
sub createExpBridges($$);
sub destroyExpBridges($$);
sub domainStatus($);
sub domainExists($);
sub addConfig($$$);
sub createXenConfig($$);
sub readXenConfig($);

sub init($)
{
    my ($pnode_id,) = @_;

    makeIfaceMaps();
    makeBridgeMaps();

    return 0;
}

sub setDebug($)
{
    $debug = shift;
    libvnode::setDebug($debug);
    print "libvnode_xen: debug=$debug\n"
	if ($debug);
}

#
# Called on each vnode, but should only be executed once per boot.
# We use a file in /var/run (cleared on reboots) to ensure this.
#
sub rootPreConfig($)
{
    #
    # Haven't been called yet, grab the lock and double check that someone
    # didn't do it while we were waiting.
    #
    if (! -e "/var/run/xen.ready") {
	my $locked = TBScriptLock($GLOBAL_CONF_LOCK,
				  TBSCRIPTLOCK_GLOBALWAIT(), 900);
	if ($locked != TBSCRIPTLOCK_OKAY()) {
	    return 0
		if ($locked == TBSCRIPTLOCK_IGNORE());
	    print STDERR "Could not get the xeninit lock after a long time!\n";
	    return -1;
	}
    }
    if (-e "/var/run/xen.ready") {
        TBScriptUnlock();
        return 0;
    }
    
    print "Configuring root vnode context\n";

    #
    # Start the Xen daemon if not running.
    # There doesn't seem to be a sure fire way to tell this.
    # However, one of the important things xend should do for us is
    # set up a bridge device for the control network, so we look for this.
    # The bridge should have the same name as the control network interface.
    #
    my ($cnet_iface,undef,undef,undef,undef,undef,$cnet_gw) = findControlNet();
    if (!existsBridge($cnet_iface)) {
	print "Starting xend and configuring cnet bridge...\n"
	    if ($debug);
	mysystem("/usr/sbin/xend start");

	#
	# xend tends to lose the default route, so make sure it exists.
	#
	system("route del default >/dev/null 2>&1");
	mysystem("route add default gw $cnet_gw");
    }

    #
    # Ensure that LVM is loaded in the kernel and ready.
    #
    print "Enabling LVM...\n"
	if ($debug);

    # be ready to snapshot later on...
    open(FD, "gunzip -c /proc/config.gz |");
    my $snapshot = "n";
    while (my $line = <FD>) {
	if ($line =~ /^CONFIG_DM_SNAPSHOT=([yYmM])/) {
	    $snapshot = $1;
	    last;
	}
    }
    close(FD);
    if ($snapshot eq 'n' || $snapshot eq 'N') {
	print STDERR "ERROR: this kernel does not support LVM snapshots!\n";
	TBScriptUnlock();
	return -1;
    }
    elsif ($snapshot eq 'm' || $snapshot eq 'M') {
	mysystem("$MODPROBE dm-snapshot");
    }

    #
    # See if our LVM volume group for VMs exists and create it if not.
    #
    my $vg = `vgs | grep $VGNAME`;
    if ($vg !~ /^\s+${VGNAME}\s/) {
	print "Creating volume group...\n"
	    if ($debug);

	#
	# Find available devices of sufficient size, prepare them,
	# and incorporate them into a volume group.
	#
	my $blockdevs = "";
	my %devs = libvnode::findSpareDisks();
	my $totalSize = 0;
	foreach my $dev (keys(%devs)) {
	    if (defined($devs{$dev}{"size"})) {
		$blockdevs .= " /dev/$dev";
		$totalSize += $devs{$dev}{"size"};
	    }
	    else {
		foreach my $part (keys(%{$devs{$dev}})) {
		    $blockdevs .= " /dev/${dev}${part}";
		    $totalSize += $devs{$dev}{$part}{"size"};
		}
	    }
	}
	if ($blockdevs eq '') {
	    print STDERR "ERROR: findSpareDisks found no disks for LVM!\n";
	    TBScriptUnlock();
	    return -1;
	}
		    
	mysystem("pvcreate $blockdevs");
	mysystem("vgcreate $VGNAME $blockdevs");

	my $size = lvmVGSize($VGNAME);
	if ($size < $XEN_MIN_VGSIZE) {
	    print STDERR "ERROR: physical disks not big enough to support".
		" VMs ($size < $XEN_MIN_VGSIZE)\n";
	    TBScriptUnlock();
	    return -1;
	}
    }

    #
    # Make sure our volumes are active -- they seem to become inactive
    # across reboots
    #
    mysystem("vgchange -a y $VGNAME");

    #
    # For compatibility with existing (physical host) Emulab images,
    # the physical host provides DHCP info for the vnodes.  So we create
    # a skeleton dhcpd.conf file here.
    #
    # Note that we must first add an alias to the control net bridge so
    # that we (the physical host) are in the same subnet as the vnodes,
    # otherwise dhcpd will fail.
    #
    # XXX Note also that createDHCP takes an array of hashes describing hosts.
    # We could use this in the future to bulk-initialize the file.
    #
    if (system("ifconfig $cnet_iface:1 | grep -q 'inet addr'")) {
	print "Creating $cnet_iface:1 alias...\n"
	    if ($debug);
	my ($vip,$vmask) = domain0ControlNet();
	mysystem("ifconfig $cnet_iface:1 $vip netmask $vmask");
    }

    print "Creating dhcp.conf skeleton...\n"
        if ($debug);
    createDHCP(undef);

    mysystem("touch /var/run/xen.ready");
    TBScriptUnlock();
    return 0;
}

sub rootPreConfigNetwork($$$)
{
    my ($node_ifs,$node_ifsets,$node_lds) = @_;

    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the xennetwork lock after a long time!\n";
	return -1;
    }

    #
    # If we blocked, it would be because vnodes have come or gone,
    # so we need to rebuild the maps.
    #
    makeIfaceMaps();
    makeBridgeMaps();

    TBScriptUnlock();
    return 0;
}

sub rootPostConfig($)
{
    return 0;
}

#
# Create the basic context for the VM and give it a unique ID for identifying
# "internal" state.
#
sub vnodeCreate($$$)
{
    my ($vnode_id,$imagename,$raref) = @_;
    my %image = %defaultImage;

    my $vmid;
    if ($vnode_id =~ /^\w+\d+\-(\d+)$/) {
	$vmid = $1;
    }
    else {
	fatal("xen_vnodeCreate: bad vnode_id $vnode_id!");
    }

    my $lvname = $image{'name'};

    #
    # No image specified, use a default based on the dom0 OS.
    #
    if (!defined($imagename)) {
	#
	# Setup the default image now.
	# XXX right now this is a hack where we just copy the dom0
	# filesystem and clone (snapshot) that.
	#
	$imagename = $defaultImage{'name'};
	print STDERR "xen_vnodeCreate: no image specified, using default ('$imagename')\n";

	my $vname = $imagename . ".0";
	if (!findLVMLogicalVolume($vname)) {
	    createRootDisk($vname);
	}
	$raref = { "IMAGETIME" => 0 };
    }

    print STDERR "xen_vnodeCreate: loading image '$imagename'\n";
    if (!defined($raref)) {
	fatal("xen_vnodeCreate: cannot load image without loadinfo\n");
    }

    # Tell stated we are getting ready for a reload
    libvnode::setState("RELOADSETUP");

    $lvname = createImageDisk($imagename, $raref);
    if (!$lvname) {
	fatal("xen_vnodeCreate: cannot create logical volume for $imagename");
    }

    # Tell stated via tmcd
    libvnode::setState("RELOADING");

    #
    # Since we may have (re)loaded a new image for this vnode, check
    # and make sure the vnode snapshot disk is associated with the
    # correct image.  Otherwise destroy the current vnode LVM so it
    # will get correctly associated below.
    #
    if (findLVMLogicalVolume($vnode_id)) {
	my $olvname = findLVMOrigin($vnode_id);
	if ($olvname ne $lvname) {
	    if (system("lvremove -f $VGNAME/$vnode_id")) {
		fatal("xen_vnodeCreate: could not destroy old disk for $vnode_id");
	    }
	}
    }

    #
    # Create the snapshot LVM.
    #
    if (!findLVMLogicalVolume($vnode_id)) {
	my $basedisk = lvmVolumePath($lvname);
	if (system("lvcreate -s -L ${XEN_LDSIZE}G -n $vnode_id $basedisk")) {
	    print STDERR "libvnode_xen: could not create disk for $vnode_id\n";
	    return -1;
	}
    }

    #
    # XXX need a better way to determine this!
    #
    my $os;
    if ($imagename =~ /BSD/) {
	$os = "FreeBSD";
	$image{'kernel'} = "/boot/freebsd8/kernel";
	undef $image{'ramdisk'};
    } else {
	$os = "Linux";
    }

    #
    # Create the config file and fill in the disk/filesystem related info.
    # Since we don't want to leave a partial config file in the event of
    # a failure down the road, we just accumulate the config info in a string
    # and write it out right before we boot.
    #
    # BSD stuff inspired by:
    # http://wiki.freebsd.org/AdrianChadd/XenHackery
    #
    $vninfo{$vmid}{'cffile'} = [];

    my $vndisk = lvmVolumePath($vnode_id);
    my $kernel = $image{'kernel'};
    my $ramdisk = $image{'ramdisk'};
    my $vdisk = "sda";	# yes, this is right for FBSD too

    addConfig($vmid, "# Xen configuration script for $os vnode $vnode_id", 2);
    addConfig($vmid, "name = '$vnode_id'", 2);
    addConfig($vmid, "kernel = '$kernel'", 2);
    addConfig($vmid, "ramdisk = '$ramdisk'", 2)
	if (defined($ramdisk));
    addConfig($vmid, "disk = ['phy:$vndisk,$vdisk,w']", 2);

    if ($os eq "FreeBSD") {
	addConfig($vmid, "extra = 'boot_verbose=1'", 2);
	addConfig($vmid, "extra += ',vfs.root.mountfrom=ufs:/dev/da0a'", 2);
	addConfig($vmid, "extra += ',kern.bootfile=/boot/kernel/kernel'", 2);
    } else {
	addConfig($vmid, "root = '/dev/$vdisk ro'", 2);
	addConfig($vmid, "extra = '3 xencons=tty'", 2);
    }

    #
    # Finish off the state transitions started by createImageDisk.
    #
    if (defined($imagename)) {
	# Tell stated via tmcd
	libvnode::setState("RELOADDONE");
	sleep(4);
	libvnode::setState("SHUTDOWN");
    }

    return $vmid;
}

#
# The logical disk has been created.
# Here we just mount it and invoke the callback.
#
# XXX not that the callback only works when we can mount the VM OS's
# filesystems!  Since all we do right now is Linux, this is easy.
#
sub vnodePreConfig($$$){
    my ($vnode_id,$vmid,$callback) = @_;

    #
    # XXX vnodeCreate is not called when a vnode was halted or is rebooting.
    # In that case, we read in any existing config file.
    #
    if (!exists($vninfo{$vmid}) || !exists($vninfo{$vmid}{'cffile'})) {
	my $aref = readXenConfig(configFile($vnode_id));
	if (!$aref) {
	    die("libvnode_xen: vnodePreConfig: no Xen config for $vnode_id!?");
	}
	$vninfo{$vmid}{'cffile'} = $aref;
    }

    #
    # XXX can only do the rest for Linux vnodes
    #
    if ($vninfo{$vmid}{'cffile'}->[0] !~ /Linux/) {
	print "libvnode_xen: vnodePreConfig: short-circuit non-Linux vnode\n";
	return 0;
    }

    mkpath(["/mnt/xen/$vnode_id"]);
    my $dev = lvmVolumePath($vnode_id);
    my $vnoderoot = "/mnt/xen/$vnode_id";
    mysystem("mount $dev $vnoderoot");

    # XXX this should no longer be needed, but just in case
    if (! -e "$vnoderoot/var/emulab/boot/vmname" ) {
	print STDERR
	    "libvnode_xen: WARNING: vmname not set by dhclient-exit-hook\n";
	open(FD,">$vnoderoot/var/emulab/boot/vmname") 
	    or die "vnodePreConfig: could not open vmname for $vnode_id: $!";
	print FD "$vnode_id\n";
	close(FD);
    }

    # Use the physical host pubsub daemon
    my (undef, $ctrlip) = findControlNet();
    if (!$ctrlip || $ctrlip !~ /^(\d+\.\d+\.\d+\.\d+)$/) {
	die "vnodePreConfig: could not get control net IP for $vnode_id";
    }

    # Should be handled in libsetup.pm, but just in case
    if (! -e "$vnoderoot/var/emulab/boot/localevserver" ) {
	open(FD,">$vnoderoot/var/emulab/boot/localevserver") 
	    or die "vnodePreConfig: could not open localevserver for $vnode_id: $!";
	print FD "$ctrlip\n";
	close(FD);
    }

    my $ret = &$callback($vnoderoot);

    mysystem("umount $dev");
    return $ret;
}

#
# Configure the control network for a vnode.
#
# XXX for now, I just perform all the actions here til everything is working.
# This means they cannot easily be undone if something fails later on.
#
sub vnodePreConfigControlNetwork($$$$$$$$$$)
{
    my ($vnode_id,$vmid,$ip,$mask,$mac,$gw,
	$vname,$longdomain,$shortdomain,$bossip) = @_;

    if (!exists($vninfo{$vmid}) || !exists($vninfo{$vmid}{'cffile'})) {
	die("libvnode_xen: vnodePreConfig: no state for $vnode_id!?");
    }

    my $fmac = fixupMac($mac);
    # Note physical host control net IF is really a bridge
    my ($cbridge) = findControlNet();
    my $cscript = "$VMDIR/$vnode_id/cnet-$mac";

    # Save info for the control net interface for config file.
    $vninfo{$vmid}{'cnet'}{'mac'} = $fmac;
    $vninfo{$vmid}{'cnet'}{'bridge'} = $cbridge;
    $vninfo{$vmid}{'cnet'}{'script'} = $cscript;

    # Create a network config script for the interface
    my $stuff = {'name' => $vnode_id,
		 'ip' => $ip,
		 'hip' => $gw,
		 'fqdn', => $longdomain,
		 'mac' => $fmac};
    createControlNetworkScript($vmid, $stuff, $cscript);

    # Create a DHCP entry
    $vninfo{$vmid}{'dhcp'}{'name'} = $vnode_id;
    $vninfo{$vmid}{'dhcp'}{'ip'} = $ip;
    $vninfo{$vmid}{'dhcp'}{'mac'} = $fmac;

    # a route to reach the vnode
    system("$ROUTE add $ip dev $cbridge");

    return 0;
}

#
# This is where new interfaces get added to the experimental network.
# For each vnode we need to:
#  - possibly create (or arrange to have created) a bridge device
#  - create config file lines for each interface
#  - arrange for the correct routing
#
sub vnodePreConfigExpNetwork($)
{
    my ($vnode_id,$vmid,$ifconfigs,$ldconfigs,$tunconfigs) = @_;

    # Keep track of links (and implicitly, bridges) that need to be created
    my @links = ();

    # Build up a config file line for all interfaces, starting with cnet
    my $vifstr = "vif = ['" .
	"mac=" . $vninfo{$vmid}{'cnet'}{'mac'} . ", " .
        "bridge=" . $vninfo{$vmid}{'cnet'}{'bridge'} . ", " .
        "script=" . $vninfo{$vmid}{'cnet'}{'script'} . "'";

    foreach my $interface (@$ifconfigs){
        print "interface " . Dumper($interface) . "\n"
	    if ($debug > 1);
        my $mac = "";
        my $physical_mac = "";
        my $tag = 0;
        my $brname = $interface->{'LAN'};
	my $ifname = $interface->{'ITYPE'} . $interface->{'ID'};
        if ($interface->{'ITYPE'} eq "veth"){
            $mac = $interface->{'MAC'};
            if ($interface->{'PMAC'} ne "none"){
                $physical_mac = $interface->{'PMAC'};
            }
        } elsif ($interface->{'ITYPE'} eq "vlan"){
            $mac = $interface->{'MAC'};
            $tag = $interface->{'VTAG'};
            $physical_mac = $interface->{'PMAC'};
        } else {
            $mac = $interface->{'MAC'};
        }

	#
	# In the era of shared nodes, we cannot name the bridges
	# using experiment local names (e.g., the link name).
	# Bridges are now named after either the physical interface
	# they are associated with or the "tag" if there is no physical
	# interface.
	#
	if ($interface->{'PMAC'} ne "none") {
	    $brname = "pbr" . findIface($interface->{'PMAC'});
	} elsif ($interface->{'VTAG'} ne "0") {
	    $brname = "lbr" . $interface->{'VTAG'};
	}

	#
	# If there is shaping info associated with the interface
	# then we need a custom script.
	#
	my $script = "";
	foreach my $ldinfo (@$ldconfigs) {
	    if ($ldinfo->{'IFACE'} eq $mac) {
		$script = "$VMDIR/$vnode_id/enet-$mac";
		my $log = "/var/emulab/logs/enet-$vnode_id.log";
		createExpNetworkScript($vmid, $ldinfo, $script, $log);
	    }
	}

	# add interface to config file line
	$vifstr .= ", 'vifname=$ifname, mac=" .
	    fixupMac($mac) . ", bridge=$brname";
	if ($script ne "") {
	    $vifstr .= ", script=$script";
	}
	$vifstr .= "'";

	# Push vif info
        my $link = {'mac' => fixupMac($mac),
		    'ifname' => $ifname,
                    'brname' => $brname,
		    'script' => $script,
                    'physical_mac' => $physical_mac,
                    'tag' => $tag
                    };
        push @links, $link;
    }

    # push out config file line for all interfaces
    # XXX note that we overwrite since a modify might add/sub IFs
    $vifstr .= "]";
    addConfig($vmid, $vifstr, 1);

    $vninfo{$vmid}{'links'} = \@links;
    return 0;
}

sub vnodeConfigResources($){
    my ($vnode_id,$vmid) = @_;

    #
    # Give the vnode some memory.
    # XXX no way to specify this right now, so we give each vnode
    # the same amount based on an arbitrary maximum vnode limit.
    #
    my $memory = memoryPerVnode();
    if ($memory < 48){
        die("48MB is the minimum amount of memory for a Xen VM; ".
	    "could only get {$memory}MB.".
	    "Adjust libvnode_xen::MAX_VNODES");
    }
    addConfig($vmid, "memory = $memory", 0);

    return 0;
}

sub vnodeConfigDevices($$)
{
    my ($vnode_id,$vmid) = @_;
    return 0;
}

sub vnodeState($$)
{
    my ($vnode_id,$vmid) = @_;

    my $err = 0;
    my $out = VNODE_STATUS_UNKNOWN();

    # right now, if it shows up in the list, consider it running
    if (domainExists($vnode_id)) {
	$out = VNODE_STATUS_RUNNING();
    }
    # otherwise, if the logical disk exists, consider it stopped
    elsif (findLVMLogicalVolume($vnode_id)) {
	$out = VNODE_STATUS_STOPPED();
    }
    return ($err, $out);
}

sub vnodeBoot($)
{
    my ($vnode_id,$vmid) = @_;

    if (!exists($vninfo{$vmid}) ||
	!exists($vninfo{$vmid}{'cffile'})) {
	die("libvnode_xen: vnodeBoot $vnode_id: no essential state!?");
    }

    #
    # We made it here without error, so create persistent state.
    # Xen config file...
    #
    my $config = configFile($vnode_id);
    if ($vninfo{$vmid}{'cfchanged'}) {
	if (createXenConfig($config, $vmid)) {
	    die("libvnode_xen: vnodeBoot $vnode_id: could not create $config");
	}
    } elsif (! -e $config) {
	die("libvnode_xen: vnodeBoot $vnode_id: $config file does not exist!");
    }

    # DHCP entry...
    if (exists($vninfo{$vmid}{'dhcp'})) {
	my $name = $vninfo{$vmid}{'dhcp'}{'name'};
	my $ip = $vninfo{$vmid}{'dhcp'}{'ip'};
	my $mac = $vninfo{$vmid}{'dhcp'}{'mac'};
	addDHCP($name, $ip, $mac, 1);
    }

    # physical bridge devices...
    if (createExpBridges($vnode_id, $vninfo{$vmid}{'links'})) {
	die("libvnode_xen: vnodeBoot $vnode_id: could not create bridges");
    }

    libvnode::setState("BOOTING");
    # and finally, create the VM
    mysystem("xm create $config");
    print "Created virtual machine $vnode_id\n";
    return 0;
}

sub vnodePostConfig($)
{
    return 0;
}

sub vnodeReboot($)
{
    my ($id) = @_;
    if ($id =~ m/(.*)/){
        $id = $1;
    }
    mysystem("/usr/sbin/xm reboot $id");
}

sub vnodeDestroy($)
{
    my ($vnode_id,$vmid) = @_;
    if ($vnode_id =~ m/(.*)/){
        $vnode_id = $1;
    }
    if (domainExists($vnode_id)) {
	mysystem("/usr/sbin/xm destroy $vnode_id");
	# XXX hang out awhile waiting for domain to disappear
	domainGone($vnode_id, 15);
    }

    #
    # We do these whether or not the domain existed
    #
    destroyExpBridges($vnode_id, $vninfo{$vmid}{'links'});
    if (findLVMLogicalVolume($vnode_id)) {
	if (system("lvremove -f $VGNAME/$vnode_id")) {
	    print STDERR
		"libvnode_xen: could not destroy disk for $vnode_id\n";
	}
    }

    return 0;
}

sub vnodeHalt($)
{
    my ($vnode_id) = @_;

    if ($vnode_id =~ m/(.*)/) {
        $vnode_id = $1;
    }
    mysystem("/usr/sbin/xm shutdown $vnode_id");
    # XXX hang out awhile waiting for domain to disappear
    domainGone($vnode_id, 15);

    return 0;
}

# XXX implement these!
sub vnodeExec($$$)
{
    my ($vnode_id,$vmid,$command) = @_;

    if ($command eq "sleep 100000000") {
	while (1) {
	    my $stat = domainStatus($vnode_id);
	    # shutdown/destroyed
	    if (!$stat) {
		return 0;
	    }
	    # crashed
	    if ($stat =~ /c/) {
		return -1;
	    }
	    sleep(5);
	}
    }
    return -1;
}

sub vnodeUnmount($$)
{
    my ($vnode_id,$vmid) = @_;

    return 0;
}

#
# Local functions
#

sub findRoot()
{
    my $rootfs = `df / | grep /dev/`;
    if ($rootfs =~ /^(\/dev\/\S+)/) {
	my $dev = $1;
	return $dev;
    }
    die "libvnode_xen: cannot determine root filesystem";
}

sub copyRoot($$)
{
    my ($from, $to) = @_;
    my $disk_path = "/mnt/xen/disk";
    my $root_path = "/mnt/xen/root";
    print "Mount root\n";
    mysystem("mount $from $root_path");
    mysystem("mount -o loop $to $disk_path");
    mkpath([map{"$disk_path/$_"} qw(proc sys home tmp)]);
    print "Copying files\n";
    system("cp -a $root_path/{root,dev,var,etc,usr,bin,sbin,lib} $disk_path");

    # hacks to make things work!
    disk_hacks($disk_path);

    mysystem("umount $root_path");
    mysystem("umount $disk_path");
}

#
# Create the root "disk" (logical volume)
# XXX this is a temp hack til all vnode creations have an explicit image.
#
sub createRootDisk($)
{
    my ($lv) = @_;
    my $full_path = lvmVolumePath($lv);
    my $size = $XEN_LDSIZE;

    #
    # We only want to do this once.  Note that we wait up to 20 minutes since
    # it can take a long time to copy the filesystem.  If we really cared, we
    # could further trim what is copied; e.g., no share/doc.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 1200) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the xennetwork lock after a long time!\n";
	return -1;
    }

    system("/usr/sbin/lvcreate -n $lv -L ${size}G $VGNAME");
    system("echo y | mkfs -t ext3 $full_path");
    mysystem("e2label $full_path /");
    copyRoot(findRoot(), $full_path);

    TBScriptUnlock();
}

#
# Create a logical volume for the image if it doesn't already exist.
# We include the time stamp in the volume name so that we detect and
# automatically handle updated images.  Downloads an image as necessary.
#
# Returns the name of the logical volume, or zero on failure.
#
sub createImageDisk($$)
{
    my ($image,$raref) = @_;
    my $tstamp = $raref->{'IMAGEMTIME'};

    my $lvname = "$image.";
    if (defined($tstamp)) {
	$lvname .= $tstamp;
    } else {
	$lvname .= "0";
    }

    #
    # We might be creating multiple vnodes with the same image, so grab
    # the global lock for this image before doing anything.
    #
    my $imagelock = "xenimage.$lvname";
    my $locked = TBScriptLock($imagelock, TBSCRIPTLOCK_GLOBALWAIT(), 1800);
    if ($locked != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the $imagelock lock after a long time!\n";
	return 0;
    }

    if (findLVMLogicalVolume($lvname)) {
	if (!defined($tstamp)) {
	    print STDERR
		"libvnode_xen: WARNING: ",
		"requested image $image has no timestamp and already exists; ",
		"using existing (possibly outdated) version.\n";
	}
	TBScriptUnlock();
	return $lvname;
    }

    my $size = $XEN_LDSIZE;
    if (system("/usr/sbin/lvcreate -n $lvname -L ${size}G $VGNAME")) {
	print STDERR "libvnode_xen: could not create disk for $image\n";
	TBScriptUnlock();
	return 0;
    }

    # Now we just download the file, then let create do its normal thing
    my $imagepath = lvmVolumePath($lvname);
    my $dret = libvnode::downloadImage($imagepath, 1, $raref);

    #
    # XXX note that we don't declare RELOADDONE here since we haven't
    # actually created the vnode shadow disk yet.  That is the caller's
    # responsibility.
    #

    TBScriptUnlock();
    return $lvname;
}

sub replace_hack($)
{
    my ($q) = @_;
    if ($q =~ m/(.*)/){
        return $1;
    }
    return "";
}

sub disk_hacks($)
{
    my ($path) = @_;
    # erase cache from LABEL to devices
    my @files = <$path/etc/blkid/*>;
    unlink map{&replace_hack($_)} (grep{m/(.*blkid.*)/} @files);

    rmtree(["$path/var/emulab/boot/tmcc"]);

    # don't try to recursively boot vnodes!
    unlink("$path/usr/local/etc/emulab/bootvnodes");

    # don't start dhcpd in the VM
    unlink("$path/etc/dhcpd.conf");

    # enable the correct device for console
    system("sed -i.bak -e 's/xvc0/console/' $path/etc/inittab");
}

sub configFile($)
{
    my ($id) = @_;
    if ($id =~ m/(.*)/){
        return "$VMDIR/$1/xm.conf";
    }
    return "";
}

#
# Return MB of memory used by dom0
# Give it at least 256MB of memory.
#
sub domain0Memory()
{
    my $memtotal = `grep MemTotal /proc/meminfo`;
    if ($memtotal =~ /^MemTotal:\s*(\d+)\s(\w+)/) {
	my $num = $1;
	my $type = $2;
	if ($type eq "kB") {
	    $num /= 1024;
	}
	$num = int($num);
	return ($num >= $MIN_MB_DOM0MEM ? $num : $MIN_MB_DOM0MEM);
    }
    die("Could not find what the total memory for domain 0 is!");
}

#
# Return total MB of memory available to domUs
#
sub totalMemory()
{
    # returns amount in MB
    my $meminfo = `/usr/sbin/xm info | grep total_memory`;
    if ($meminfo =~ m/\s*total_memory\s*:\s*(\d+)/){
        my $mem = int($1);
        return $mem - domain0Memory();
    }
    die("Could not find what the total physical memory on this machine is!");
}

#
# Returns the control net IP of the physical host.
#
sub domain0ControlNet()
{
    #
    # XXX we use a woeful hack to get the virtual control net address,
    # just adding one to the GW address.  With our current hacky vnode IP
    # assignment, this address will always be available.
    #
    my (undef,$vmask,$vgw) = findVirtControlNet();
    if ($vgw =~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/) {
	my $vip = "$1.$2.$3." . ($4+1);
	return ($vip, $vmask);
    }
    die("domain0ControlNet: could not create control net virtual IP");
}

#
# Return the amount of memory to allocate per vnode.
# Round down to a multiple of 8 since...I say so.
#
sub memoryPerVnode()
{
    my $totmem = totalMemory();
    if ($totmem < ($MIN_MB_VNMEM * $MAX_VNODES)) {
	die("libvnode_xen: not enough memory to support maximum ($MAX_VNODES)".
	    " vnodes at ${MIN_MB_VNMEM}MB per (${totmem}MB available)");
    }
    my $memper = $totmem / $MAX_VNODES;
    return (int($memper / 8) * 8);
}

#
# Emulab image compatibility: the physical host acts as DHCP server for all
# the hosted vnodes since they expect to find out there identity, and identify
# their control net, via DHCP.
#
sub createDHCP($)
{
    my ($all) = @_;
    my ($vnode_net,$vnode_mask,$vnode_gw) = findVirtControlNet();
    my $vnode_dns = findDNS($vnode_gw);

    open(FILE, ">$DHCPCONF_FILE") or die("Cannot write $DHCPCONF_FILE");

    print FILE <<EOF;
#
# Do not edit!  Auto-generated by libvnode_xen.pm.
#
ddns-update-style  none;
default-lease-time 604800;
max-lease-time     704800;

subnet $vnode_net netmask $vnode_mask {
    option domain-name-servers $vnode_dns;
    option domain-name "emulab.net";
    option routers $vnode_gw;

    # INSERT VNODES AFTER
EOF
    ;

    if (defined($all)) {
	foreach (@$all) {
	    my ($data) = @_;
	    my $fqdn = $data->{'fqdn'};
	    my $host = $1 if ($fqdn =~ m/(\w+)\..*/);
	    my $ip = $data->{'ip'};
	    my $mac = $data->{'mac'};
	    print FILE formatDHCP($host, $ip, $mac), "\n";
	}
    }
    print FILE "    # INSERT VNODES BEFORE\n}\n";
    close(FILE);

    # make sure dhcpd is running
    system("/etc/init.d/dhcpd restart");
}

#
# Add or remove (host,IP,MAC) in the local dhcpd.conf
# If an entry already exists, replace it.
#
# XXX assume one line per entry
#
sub addDHCP($$$$) { return modDHCP(@_, 0); }
sub subDHCP($$) { return modDHCP("--", "--", @_, 1); }

sub modDHCP($$$$$)
{
    my ($host,$ip,$mac,$doHUP,$dorm) = @_;
    my $cur = "$DHCPCONF_FILE";
    my $bak = "$DHCPCONF_FILE.old";
    my $tmp = "$DHCPCONF_FILE.new";

    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 60) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the xennetwork lock after a long time!\n";
	return -1;
    }

    if (!open(NEW, ">$tmp")) {
	print STDERR "Could not create new DHCP file, ",
		     "$host/$ip/$mac not added\n";
	TBScriptUnlock();
	return -1;
    }
    if (!open(OLD, "<$cur")) {
	print STDERR "Could not open $cur, ",
		     "$host/$ip/$mac not added\n";
	close(NEW);
	unlink($tmp);
	TBScriptUnlock();
	return -1;
    }
    $host = lc($host);
    $mac = lc($mac);
    my $inrange = 0;
    my $found = 0;
    my $changed = 0;
    while (my $line = <OLD>) {
	if ($found) {
	    ;
	} elsif ($line =~ /INSERT VNODES AFTER/) {
	    $inrange = 1;
	} elsif ($line =~ /INSERT VNODES BEFORE/) {
	    $inrange = 0;
	    $found = 1;
	    if (!$dorm) {
		print NEW formatDHCP($host, $ip, $mac), "\n";
		$changed = 1;
	    }
	} elsif ($inrange &&
		 ($line =~ /ethernet ([\da-f:]+); fixed-address ([\d\.]+); option host-name ([^;]+);/i)) {
	    my $ohost = lc($3);
	    my $oip = $2;
	    my $omac = lc($1);
	    if ($mac eq $omac) {
		if ($dorm) {
		    # skip this entry; don't mark found so we find all
		    $changed = 1;
		    next;
		}
		$found = 1;
		if ($host ne $ohost || $ip ne $oip) {
		    print NEW formatDHCP($host, $ip, $omac), "\n";
		    $changed = 1;
		    next;
		}
	    }
	}
	print NEW $line;
    }
    close(OLD);
    close(NEW);

    #
    # Nothing changed, we are done.
    #
    if (!$changed) {
	unlink($tmp);
	TBScriptUnlock();
	return 0;
    }

    #
    # Move the new file in place, and optionally restart dhcpd
    #
    if (-e $bak) {
	if (!unlink($bak)) {
	    print STDERR "Could not remove $bak, ",
			 "$host/$ip/$mac not added\n";
	    unlink($tmp);
	    TBScriptUnlock();
	    return -1;
	}
    }
    if (!rename($cur, $bak)) {
	print STDERR "Could not rename $cur -> $bak, ",
		     "$host/$ip/$mac not added\n";
	unlink($tmp);
	TBScriptUnlock();
	return -1;
    }
    if (!rename($tmp, $cur)) {
	print STDERR "Could not rename $tmp -> $cur, ",
		     "$host/$ip/$mac not added\n";
	rename($bak, $cur);
	unlink($tmp);
	TBScriptUnlock();
	return -1;
    }
    if ($doHUP) {
	system("/etc/init.d/dhcpd restart");
    }

    TBScriptUnlock();
    return 0;
}

sub formatDHCP($$$)
{
    my ($host,$ip,$mac) = @_;
    my $xip = $ip;
    $xip =~ s/\.//g;

    return ("    host xen$xip { ".
	    "hardware ethernet $mac; ".
	    "fixed-address $ip; ".
	    "option host-name $host; }");
}

# convert 123456 into 12:34:56
sub fixupMac($)
{
    my ($x) = @_;
    $x =~ s/(\w\w)/$1:/g;
    chop($x);
    return $x;
}

#
# Write out the script that will be called when the control-net interface
# is instantiated by Xen.  This is just a stub which calls the common
# Emulab script in /etc/xen/scripts.
#
# XXX can we get rid of this stub by using environment variables?
#
sub createControlNetworkScript($$$)
{
    my ($vmid,$data,$file) = @_;
    my $host_ip = $data->{'hip'};
    my $name = $data->{'name'};
    my $ip = $data->{'ip'};

    open(FILE, ">$file") or die $!;
    print FILE "#!/bin/sh\n";
    print FILE "sh /etc/xen/scripts/emulab-cnet $vmid $host_ip $name $ip \$*\n";
    print FILE "exit \$?\n";
    close(FILE);
    chmod(0555, $file);
}

sub createExpNetworkScript($$$$)
{
    my ($vmid,$info,$file,$lfile) = @_;
    my $TC = "/sbin/tc";

    open(FILE, ">$file") or die $!;
    print FILE "#!/bin/sh\n";
    print FILE "OP=\$1\n";
    print FILE "sh /etc/xen/scripts/vif-bridge \$*\n";
    print FILE "STAT=\$?\n";
    print FILE "if [ \$STAT -ne 0 -o \"\$OP\" != \"online\" ]; then\n";
    print FILE "    exit \$STAT\n";
    print FILE "fi\n";
    print FILE "# XXX redo what vif-bridge does to get named interface\n";
    print FILE "vifname=`xenstore read \$XENBUS_PATH/vifname`\n";
    print FILE "\${vifname:=\$vif}\n";
    print FILE "echo \"Configuring shaping for \$vifname (MAC ",
                     $info->{'IFACE'}, ")\" >>$lfile\n";

    my $iface     = $info->{'IFACE'};
    my $type      = $info->{'TYPE'};
    my $linkname  = $info->{'LINKNAME'};
    my $vnode     = $info->{'VNODE'};
    my $inet      = $info->{'INET'};
    my $mask      = $info->{'MASK'};
    my $pipeno    = $info->{'PIPE'};
    my $delay     = $info->{'DELAY'};
    my $bandw     = $info->{'BW'};
    my $plr       = $info->{'PLR'};
    my $rpipeno   = $info->{'RPIPE'};
    my $rdelay    = $info->{'RDELAY'};
    my $rbandw    = $info->{'RBW'};
    my $rplr      = $info->{'RPLR'};
    my $red       = $info->{'RED'};
    my $limit     = $info->{'LIMIT'};
    my $maxthresh = $info->{'MAXTHRESH'};
    my $minthresh = $info->{'MINTHRESH'};
    my $weight    = $info->{'WEIGHT'};
    my $linterm   = $info->{'LINTERM'};
    my $qinbytes  = $info->{'QINBYTES'};
    my $bytes     = $info->{'BYTES'};
    my $meanpsize = $info->{'MEANPSIZE'};
    my $wait      = $info->{'WAIT'};
    my $setbit    = $info->{'SETBIT'};
    my $droptail  = $info->{'DROPTAIL'};
    my $gentle    = $info->{'GENTLE'};

    $delay  = int($delay + 0.5) * 1000;
    $rdelay = int($rdelay + 0.5) * 1000;

    $bandw *= 1000;
    $rbandw *= 1000;

    my $queue = "";
    if ($qinbytes) {
	if ($limit <= 0 || $limit > (1024 * 1024)) {
	    print "Q limit $limit for pipe $pipeno is bogus, using default\n";
	}
	else {
	    $queue = int($limit/1500);
	    $queue = $queue > 0 ? $queue : 1;
	}
    }
    elsif ($limit != 0) {
	if ($limit < 0 || $limit > 100) {
	    print "Q limit $limit for pipe $pipeno is bogus, using default\n";
	}
	else {
	    $queue = $limit;
	}
    }

    my $pipe10 = $pipeno + 10;
    my $pipe20 = $pipeno + 20;
    $iface = "\$vifname";
    my $cmd;
    if ($queue ne "") {
	$cmd = "/sbin/ifconfig $iface txqueuelen $queue";
	print FILE "echo \"$cmd\" >>$lfile\n";
	print FILE "$cmd >>$lfile 2>&1\n";
    }
    $cmd = "$TC qdisc add dev $iface handle $pipeno root plr $plr";
    print FILE "echo \"$cmd\" >>$lfile\n";
    print FILE "$cmd >>$lfile 2>&1\n";

    $cmd = "$TC qdisc add dev $iface handle $pipe10 ".
	   "parent ${pipeno}:1 delay usecs $delay";
    print FILE "echo \"$cmd\" >>$lfile\n";
    print FILE "$cmd >>$lfile 2>&1\n";

    $cmd = "$TC qdisc add dev $iface handle $pipe20 ".
	   "parent ${pipe10}:1 htb default 1";
    print FILE "echo \"$cmd\" >>$lfile\n";
    print FILE "$cmd >>$lfile 2>&1\n";

    if ($bandw != 0) {
	$cmd = "$TC class add dev $iface classid $pipe20:1 ".
	       "parent $pipe20 htb rate ${bandw} ceil ${bandw}";
	print FILE "echo \"$cmd\" >>$lfile\n";
	print FILE "$cmd >>$lfile 2>&1\n";
    }

    close(FILE);
    chmod(0554, $file);
}

sub createExpBridges($$)
{
    my ($vnode_id,$linfo) = @_;

    if (@$linfo == 0) {
	return 0;
    }

    #
    # Since bridges and physical interfaces can be shared between vnodes,
    # we need to serialize this.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the xennetwork lock after a long time!\n";
	return -1;
    }

    # read the current state of affairs
    makeIfaceMaps();
    makeBridgeMaps();

    foreach my $link (@$linfo) {
	my $mac = $link->{'mac'};
	my $pmac = $link->{'physical_mac'};
	my $brname = $link->{'brname'};
	my $tag = $link->{'tag'};

	print "$vnode_id: looking up bridge $brname ".
	    "(mac=$mac, pmac=$pmac, tag=$tag)\n"
		if ($debug);

	#
	# Sanity checks (all fatal errors if incorrect right now):
	# Virtual interface should not exist at this point,
	# Any physical interfaces should exist,
	# If physical interface is in a bridge, it must be the right one,
	#
	my $vdev = findIface($mac);
	if ($vdev) {
	    print STDERR "createExpBridges: $vdev ($mac) should not exist!\n";
	    return 1;
	}
	my $pdev;
	my $pbridge;
	if ($pmac ne "") {
	    $pdev = findIface($pmac);
	    if (!$pdev) {
		print STDERR "createExpBridges: $pdev ($pmac) should exist!\n";
		return 2;
	    }
	    $pbridge = findBridge($pdev);
	    if ($pbridge && $pbridge ne $brname) {
		print STDERR "createExpBridges: $pdev ($pmac) in wrong bridge $pbridge!\n";
		return 3;
	    }
	}

	# Create bridge if it does not exist
	if (!existsBridge($brname)) {
	    if (system("$BRCTL addbr $brname")) {
		print STDERR "createExpBridges: could not create $brname\n";
		return 4;
	    }
	    if (system("$IFCONFIG $brname up")) {
		print STDERR "createExpBridges: could not ifconfig $brname\n";
		return 5;
	    }
	}

	# Add physical device to bridge if not there already
	if ($pdev && !$pbridge) {
	    if (system("$BRCTL addif $brname $pdev")) {
		print STDERR "createExpBridges: could not add $pdev to $brname\n";
		return 6;
	    }
	}
    }

    TBScriptUnlock();
    return 0;
}

sub destroyExpBridges($$)
{
    my ($vnode_id,$linfo) = @_;

    #
    # XXX we may not be called in the same context as the bridge creation,
    # so the list of links may not be valid.
    #
    if (!defined($linfo) || @$linfo == 0) {
	print "destroyExpBridges: could not find link info\n"
	    if ($debug);
	return 0;
    }

    #
    # Since bridges and physical interfaces can be shared between vnodes,
    # we need to serialize this.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the xennetwork lock after a long time!\n";
	return -1;
    }

    my %bridges;

    #
    # Remove virtual devices from bridges.
    # Destroying the domain should have removed these, but just in case.
    #
    makeIfaceMaps();
    foreach my $link (@$linfo) {
	my $mac = $link->{'mac'};
	my $pmac = $link->{'physical_mac'};
	my $bridge = $link->{'brname'};
	my $tag = $link->{'tag'};

	print "$vnode_id: de-bridging $mac ".
	    "(mac=$mac, pmac=$pmac, bridge=$bridge, tag=$tag)\n"
		if ($debug);

	my $vdev = findIface($mac);
	if ($vdev) {
	    system("$IFCONFIG $vdev down");
	    if (existsBridge($bridge)) {
		system("$BRCTL delif $bridge $vdev");
	    }
	}
	$bridges{$bridge} = 1;
    }

    #
    # Remove any unused bridges.  This includes those that are now
    # empty (local bridges) and those with only a physical interface
    # in them (cross-node bridges).
    #
    makeBridgeMaps();
    foreach my $bridge (keys(%bridges)) {
	my @ifs = findBridgeIfaces($bridge);
	if (@ifs == 0) {
	    mysystem("$IFCONFIG $bridge down");
	    mysystem("$BRCTL delbr $bridge");
	}
	if (@ifs == 1) {
	    my $pbridge = "pbr" . $ifs[0];
	    if ($bridge eq $pbridge) {
		my $pdev = $ifs[0];
		mysystem("$BRCTL delif $bridge $pdev");
		mysystem("$IFCONFIG $bridge down");
		mysystem("$BRCTL delbr $bridge");
	    }
	}
    }

    TBScriptUnlock();
    return 0;
}

sub domainStatus($)
{
    my ($id) = @_;

    my $status = `xm list --long $id 2>/dev/null`;
    if ($status =~ /\(state ([\w-]+)\)/) {
	return $1;
    }
    return "";
}

sub domainExists($)
{
    my ($id) = @_;    
    return (domainStatus($id) ne "");
}

sub domainGone($$)
{
    my ($id,$wait) = @_;

    while ($wait--) {
	if (!domainExists($id)) {
	    return 1;
	}
	sleep(1);
    }
    return 0;
}

#
# Add a line 'str' to the XenConfig array for vnode 'vmid'.
#
# If overwrite is set, any existing line with the same key is overwritten,
# otherwise it is ignored.  If the line doesn't exist, it is always added.
#
# XXX overwrite is a hack.  Without a full parse of the config file lines
# we cannot say that two records are "the same" in particular because some
# records contains info for multiple instances (e.g., "vif").  In those
# cases, we would need to partially overwrite lines.  But we don't,
# we just overwrite the entire line.
#
sub addConfig($$$)
{
    my ($vmid,$str,$overwrite) = @_;

    if (!exists($vninfo{$vmid}) || !exists($vninfo{$vmid}{'cffile'})) {
	die("libvnode_xen: addConfig: no state for vnode $vmid!?");
    }
    my $aref = $vninfo{$vmid}{'cffile'};

    #
    # If appending (overwrite==2) or new line is a comment, tack it on.
    #
    if ($overwrite == 2 || $str =~ /^\s*#/) {
	push(@$aref, $str);
	return;
    }

    #
    # Other lines should be of the form key=value.
    # XXX if they are not, we just append them right now.
    #
    my ($key,$val);
    if ($str =~ /^\s*([^=\s]+)\s*=\s*(.*)$/) {
	$key = $1;
	$val = $2;
    } else {
	push(@$aref, $str);
	return;
    }

    #
    # For key=value lines, look for existing instance, replacing as required.
    #
    my $found = 0;
    for (my $i = 0; $i < scalar(@$aref); $i++) {
	if ($aref->[$i] =~ /^\s*#/) {
	    next;
	}
	if ($aref->[$i] =~ /^\s*([^=\s]+)\s*=\s*(.*)$/) {
	    my $ckey = $1;
	    my $cval = $2;
	    if ($ckey eq $key) {
		if ($overwrite && $cval ne $val) {
		    $aref->[$i] = $str;
		    $vninfo{$vmid}{'cfchanged'} = 1;
		}
		return;
	    }
	}
    }

    #
    # Not found, add it to the end
    #
    push(@$aref, $str);
    $vninfo{$vmid}{'cfchanged'} = 1;
}

sub readXenConfig($)
{
    my ($config) = @_;
    my @cflines = ();

    if (!open(CF, "<$config")) {
	return undef;
    }
    while (<CF>) {
	chomp;
	push(@cflines, "$_");
    }
    close(CF);

    return \@cflines;
}

sub createXenConfig($$)
{
    my ($config,$vmid) = @_;

    mkpath([dirname($config)]);
    if (!open(CF, ">$config")) {
	print STDERR "libvnode_xen: could not create $config\n";
	return -1;
    }
    foreach (@{$vninfo{$vmid}{'cffile'}}) {
	print CF "$_\n";
    }

    close(CF);
    return 0;
}

#
# Mike's replacements for Jon's Xen python-class-using code.
#
# Nothing personal, just that code used an external shell script which used
# an external python class which used an LVM shared library which comes from
# who knows where--all of which made me nervous.
#

#
# Return size of volume group in (decimal, aka disk-manufactuer) GB.
#
sub lvmVGSize($)
{
    my ($vg) = @_;

    my $size = `vgs --noheadings -o size $vg`;
    if ($size =~ /(\d+\.\d+)([mgt])/i) {
	$size = $1;
	my $u = lc($2);
	if ($u eq "m") {
	    $size /= 1000;
	} elsif ($u eq "t") {
	    $size *= 1000;
	}
	return $size;
    }
    die "libvnode_xen: cannot parse LVM volume group size";
}

sub lvmVolumePath($)
{
    my ($name) = @_;
    return "/dev/$VGNAME/$name";
}

sub findLVMLogicalVolume($)
{
    my ($lv) = @_;

    foreach (`lvs --noheadings -o name $VGNAME`) {
	if (/^\s*${lv}\s*$/) {
	    return 1;
	}
    }
    return 0;
}

#
# Return the LVM that the indicated one is a snapshot of, or a null
# string if none.
#
sub findLVMOrigin($)
{
    my ($lv) = @_;

    foreach (`lvs --noheadings -o name,origin $VGNAME`) {
	if (/^\s*${lv}\s+(\S+)\s*$/) {
	    return $1;	
	}
    }
    return "";
}

1
