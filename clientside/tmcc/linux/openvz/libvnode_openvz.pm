#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008-2011 University of Utah and the Flux Group.
# All rights reserved.
#
# Implements the libvnode API for OpenVZ support in Emulab.
#
package libvnode_openvz;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( vz_init vz_setDebug
              vz_rootPreConfig vz_rootPreConfigNetwork vz_rootPostConfig 
              vz_vnodeCreate vz_vnodeDestroy vz_vnodeState 
              vz_vnodeBoot vz_vnodeHalt vz_vnodeReboot 
              vz_vnodePreConfig vz_vnodeUnmount
              vz_vnodePreConfigControlNetwork vz_vnodePreConfigExpNetwork 
              vz_vnodeConfigResources vz_vnodeConfigDevices
              vz_vnodePostConfig vz_vnodeExec
            );

%ops = ( 'init' => \&vz_init,
	 'setDebug' => \&vz_setDebug,
	 'rootPreConfig' => \&vz_rootPreConfig,
	 'rootPreConfigNetwork' => \&vz_rootPreConfigNetwork,
	 'rootPostConfig' => \&vz_rootPostConfig,
	 'vnodeCreate' => \&vz_vnodeCreate,
	 'vnodeDestroy' => \&vz_vnodeDestroy,
	 'vnodeState' => \&vz_vnodeState,
	 'vnodeBoot' => \&vz_vnodeBoot,
	 'vnodeHalt' => \&vz_vnodeHalt,
	 'vnodeUnmount' => \&vz_vnodeUnmount,
	 'vnodeReboot' => \&vz_vnodeReboot,
	 'vnodeExec' => \&vz_vnodeExec,
	 'vnodePreConfig' => \&vz_vnodePreConfig,
	 'vnodePreConfigControlNetwork' => \&vz_vnodePreConfigControlNetwork,
	 'vnodePreConfigExpNetwork' => \&vz_vnodePreConfigExpNetwork,
	 'vnodeConfigResources' => \&vz_vnodeConfigResources,
	 'vnodeConfigDevices' => \&vz_vnodeConfigDevices,
	 'vnodePostConfig' => \&vz_vnodePostConfig,
    );


use strict;
use English;
BEGIN { @AnyDBM_File::ISA = qw(DB_File GDBM_File NDBM_File) }
use AnyDBM_File;
use Data::Dumper;
use Socket;

# Pull in libvnode
require "/etc/emulab/paths.pm"; import emulabpaths;
use libvnode;
use libtestbed;
use libsetup;

#
# Turn off line buffering on output
#
$| = 1;

#
# Load the OS independent support library. It will load the OS dependent
# library and initialize itself. 
# 

my $defaultImage = "emulab-default";

my $DOLVM = 1;
my $DOLVMDEBUG = 0;
my $LVMDEBUGOPTS = "-vvv -dddddd";

my $DOVZDEBUG = 0;
my $VZDEBUGOPTS = "--verbose";

my $GLOBAL_CONF_LOCK = "vzconf";

sub VZSTAT_RUNNING() { return "running"; }
sub VZSTAT_STOPPED() { return "stopped"; }
sub VZSTAT_MOUNTED() { return "mounted"; }

my $VZCTL  = "/usr/sbin/vzctl";
my $VZLIST = "/usr/sbin/vzlist";
my $IFCONFIG = "/sbin/ifconfig";
my $ROUTE = "/sbin/route";
my $BRCTL = "/usr/sbin/brctl";
my $IPTABLES = "/sbin/iptables";
my $MODPROBE = "/sbin/modprobe";
my $RMMOD = "/sbin/rmmod";
my $VLANCONFIG = "/sbin/vconfig";
my $IP = "/sbin/ip";

my $VZRC   = "/etc/init.d/vz";
my $MKEXTRAFS = "/usr/local/etc/emulab/mkextrafs.pl";

my $CTRLIPFILE = "/var/emulab/boot/myip";
my $IMQDB      = "/var/emulab/db/imqdb";
my $MAXIMQ     = 64;

my $CONTROL_IFNUM  = 999;
my $CONTROL_IFDEV  = "eth${CONTROL_IFNUM}";
my $EXP_BASE_IFNUM = 0;

my $RTDB           = "/var/emulab/db/rtdb";
my $RTTABLES       = "/etc/iproute2/rt_tables";
# Temporary; later kernel version increases this.
my $MAXROUTETTABLE = 255;

my $debug = 0;

# XXX needs lifting up
my $JAILCTRLNET = "172.16.0.0";
my $JAILCTRLNETMASK = "255.240.0.0";

my $USE_NETEM = 0;
my $USE_MACVLAN = 0;

#
# If we are using a modern kernel, use netem instead of our own plr/delay
# qdiscs (which are no longer maintained as of 11/2011).
#
my ($kmaj,$kmin,$kpatch) = libvnode::getKernelVersion();
print STDERR "Got Linux kernel version numbers $kmaj $kmin $kpatch\n";
if ($kmaj >= 2 && $kmin >= 6 && $kpatch >= 32) {
    print STDERR "Using Linux netem instead of custom qdiscs.\n";
    $USE_NETEM = 1;
    print STDERR "Using Linux macvlan instead of OpenVZ veths.\n";
    $USE_MACVLAN = 1;
}

#
# Helpers.
#
sub findControlNet();
sub makeIfaceMaps();
sub makeBridgeMaps();
sub findIface($);
sub findMac($);
sub editContainerConfigFile($$);
sub InitializeRouteTable();
sub AllocateRouteTable($);
sub LookupRouteTable($);
sub FreeRouteTable($);
sub vmexists($);
sub vmstatus($);
sub vmrunning($);
sub vmstopped($);

#
# Initialize the lib (and don't use BEGIN so we can do reinit).
#
sub vz_init {
    my ($pnode_id,) = @_;

    makeIfaceMaps();
    makeBridgeMaps();

    #
    # Turn off LVM if already using a /vz mount.
    #
    if (-e "/vz/.nolvm" || -e "/vz.save/.nolvm" || -e "/.nolvm") {
	$DOLVM = 0;
	mysystem("/sbin/dmsetup remove_all");
    }

    #
    # Enable/disable LVM debug options.
    #
    if (-e "/vz/.lvmdebug" || -e "/vz.save/.lvmdebug" || -e "/.lvmdebug") {
	$DOLVMDEBUG = 1;
    }
    if (!$DOLVMDEBUG) {
	$LVMDEBUGOPTS = "";
    }

    #
    # Enable/disable VZ debug options.
    #
    if (-e "/vz/.vzdebug" || -e "/vz.save/.vzdebug" || -e "/.vzdebug") {
	$DOVZDEBUG = 1;
    }
    if (!$DOVZDEBUG) {
	$VZDEBUGOPTS = "";
    }

    return 0;
}

#
# Prepare the root context.  Run once at boot.
#
sub vz_rootPreConfig {
    #
    # Only want to do this once, so use file in /var/run, which
    # is cleared at boot.
    #
    return 0
	if (-e "/var/run/openvz.ready");

    if ((my $locked = TBScriptLock($GLOBAL_CONF_LOCK,
				   TBSCRIPTLOCK_GLOBALWAIT(), 900)) 
	!= TBSCRIPTLOCK_OKAY()) {
	return 0
	    if ($locked == TBSCRIPTLOCK_IGNORE());
	print STDERR "Could not get the vzinit lock after a long time!\n";
	return -1;
    }
    # we must have the lock, so if we need to return right away, unlock
    if (-e "/var/run/openvz.ready") {
        TBScriptUnlock();
        return 0;
    }

    # make sure filesystem is setup 
    if ($DOLVM) {
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

	if (system('vgs '.$LVMDEBUGOPTS.' | grep -E -q '."'".'^[ ]+openvz.*$'."'")) {
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
		die "findSpareDisks found no disks, can't use LVM!\n";
	    }
		    
	    mysystem("pvcreate $LVMDEBUGOPTS $blockdevs");
	    mysystem("vgcreate $LVMDEBUGOPTS openvz $blockdevs");

	    # XXX eventually could move this into its own logical volume, but
	    # we don't ever know how many images we'll have to store.
	    mysystem("$VZRC stop");
	    mysystem("rm -rf /vz")
		if (-e "/vz");
	    mysystem("mkdir /vz");
	    mysystem("cp -pR /vz.save/* /vz/");
	}

	# make sure our volumes are active -- they seem to become inactive
	# across reboots
	mysystem("vgchange $LVMDEBUGOPTS -a y openvz");
    }
    else {
	#
	# We need to create a local filesystem.
	# First see if the "extra" filesystem has already been created,
	# Emulab often mounts it as /local for various purposes.
	#
	# about the funny quoting: don't ask... emacs perl mode foo.
	if (!system('grep -q '."'".'^/dev/.*/local.*$'."'".' /etc/fstab')) {
	    # local filesystem already exists, just create a subdir
	    if (! -d "/local/vz") {
		mysystem("$VZRC stop");
		mysystem("mkdir /local/vz");
		mysystem("cp -pR /vz.save/* /local/vz/");
		mysystem("touch /local/vz/.nolvm");
	    }
	    if (-e "/vz") {
		mysystem("rm -rf /vz");
		mysystem("ln -s /local/vz /vz");
	    }
	}
	else {
	    # about the funny quoting: don't ask... emacs perl mode foo.
	    if (system('grep -q '."'".'^/dev/.*/vz.*$'."'".' /etc/fstab')) {
		mysystem("$VZRC stop");
		mysystem("rm -rf /vz")
		    if (-e "/vz");
		mysystem("mkdir /vz");
		mysystem("$MKEXTRAFS -f /vz");
		mysystem("cp -pR /vz.save/* /vz/");
		mysystem("touch /vz/.nolvm");
	    }
	    if (system('mount | grep -q \'on /vz\'')) {
		mysystem("mount /vz");
	    }
	}
    }

    # We need to increase the size of the net.core.netdev_max_backlog 
    # sysctl var in the root context; not sure to what amount, or exactly 
    # why though.  Perhaps there is too much contention when handling enqueued
    # packets on the veths?
    mysystem("sysctl -w net.core.netdev_max_backlog=2048");

    # make sure the initscript is going...
    if (system("$VZRC status 2&>1 > /dev/null")) {
	mysystem("$VZRC start");
    }

    # get rid of this simple container device support
    if (!system('lsmod | grep -q vznetdev')) {
	system("$RMMOD vznetdev");
    }

    if ($USE_MACVLAN) {
	#
	# If we build dummy shortbridge nets atop either a physical
	# device, or atop a dummy device, load these!
	#
	mysystem("$MODPROBE macvlan");
	mysystem("$MODPROBE dummy");
    }
    else {
	# this is what we need for veths
	mysystem("$MODPROBE vzethdev");
    }

    # For tunnels
    mysystem("$MODPROBE ip_gre");

    # For VLANs
    mysystem("$MODPROBE 8021q");

    # we need this stuff for traffic shaping -- only root context can
    # modprobe, for now.
    if (!$USE_NETEM) {
	mysystem("$MODPROBE sch_plr");
	mysystem("$MODPROBE sch_delay");
    }
    else {
	mysystem("$MODPROBE sch_netem");
    }
    mysystem("$MODPROBE sch_htb");

    # make sure our network hooks are called
    if (system('grep -q -e EXTERNAL_SCRIPT /etc/vz/vznet.conf')) {
	if (! -e '/etc/vz/vznet.conf') {
	    open(FD,">/etc/vz/vznet.conf") 
		or die "could not open /etc/vz/vznet.conf: $!";
	    print FD "#!/bin/bash\n";
	    print FD "\n";
	    close(FD);
	}
	mysystem("echo 'EXTERNAL_SCRIPT=\"/usr/local/etc/emulab/vznetinit-elab.sh\"' >> /etc/vz/vznet.conf");
    }

    #
    # XXX all this network config stuff should be done in PreConfigNetwork,
    # but we can't rmmod the IMQ module to change the config, so no point.
    #

    # Ug, pre-create a bunch of imq devices, since adding new ones
    # does not work right yet.
    mysystem("$MODPROBE imq numdevs=$MAXIMQ");
    mysystem("$MODPROBE ipt_IMQ");

    # Create a DB to manage them.
    my %MDB;
    if (!dbmopen(%MDB, $IMQDB, 0660)) {
	print STDERR "*** Could not create $IMQDB\n";
	TBScriptUnlock();
	return -1;
    }
    for (my $i = 0; $i < $MAXIMQ; $i++) {
	$MDB{"$i"} = ""
	    if (!exists($MDB{"$i"}));
    }
    dbmclose(%MDB);

    if (InitializeRouteTables()) {
	print STDERR "*** Could not initialize routing table DB\n";
	TBScriptUnlock();
	return -1;
    }
    mysystem("touch /var/run/openvz.ready");
    TBScriptUnlock();
    return 0;
}

#
# Prepare any network stuff in the root context on a global basis.  Run once
# at boot, or at reconfigure.  For openvz, this consists of creating bridges
# and configuring them as necessary.
#
sub vz_rootPreConfigNetwork {
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the vznetwork lock after a long time!\n";
	return -1;
    }

    # Do this again after lock.
    makeIfaceMaps();
    makeBridgeMaps();
    
    my ($node_ifs,$node_ifsets,$node_lds) = @_;

    # setup forwarding on ctrl net -- NOTE that iptables setup to do NAT
    # actually happens per vnode now.
    my ($iface,$ip,$netmask,$maskbits,$network,$mac) = findControlNet();
    mysystem("echo 1 > /proc/sys/net/ipv4/conf/$iface/forwarding");
    # XXX only needed for fake mac hack, which should go away someday
    mysystem("echo 1 > /proc/sys/net/ipv4/conf/$iface/proxy_arp");

    #
    # If we're using veths, figure out what bridges we need to make:
    # we need a bridge for each physical iface that is a multiplex pipe,
    # and one for each VTAG given PMAC=none (i.e., host containing both sides
    # of a link, or an entire lan).
    #
    my %brs = ();
    my $prefix = "br.";
    if ($USE_MACVLAN) {
	$prefix = "mvsw.";
    }
    foreach my $node (keys(%$node_ifs)) {
	foreach my $ifc (@{$node_ifs->{$node}}) {
	    next if (!$ifc->{ISVIRT});

	    if ($ifc->{ITYPE} eq "loop") {
		my $vtag  = $ifc->{VTAG};

		#
		# No physical device. Its a loopback (trivial) link/lan
		# All we need is a common bridge to put the veth ifaces into.
		#
		my $brname = "${prefix}$vtag";
		$brs{$brname}{ENCAP} = 0;
		$brs{$brname}{SHORT} = 0;
	    }
	    elsif ($ifc->{ITYPE} eq "vlan") {
		my $iface = $ifc->{IFACE};
		my $vtag  = $ifc->{VTAG};
		my $vdev  = "${iface}.${vtag}";

		system("$VLANCONFIG set_name_type DEV_PLUS_VID_NO_PAD");
		system("$VLANCONFIG add $iface $vtag");
		system("$VLANCONFIG set_name_type VLAN_PLUS_VID_NO_PAD");
		system("$IFCONFIG $vdev up");

		my $brname = "${prefix}$vdev";
		$brs{$brname}{ENCAP} = 1;
		$brs{$brname}{SHORT} = 0;
		$brs{$brname}{PHYSDEV} = $vdev;
	    }
	    elsif ($ifc->{PMAC} eq "none") {
		my $brname = "${prefix}" . $ifc->{VTAG};
		# if no PMAC, we don't need encap on the bridge
		$brs{$brname}{ENCAP} = 0;
		# count up the members so we can figure out if this is a shorty
		if (!exists($brs{$brname}{MEMBERS})) {
		    $brs{$brname}{MEMBERS} = 0;
		}
		else {
		    $brs{$brname}{MEMBERS}++;
		}
	    }
	    else {
		my $iface = findIface($ifc->{PMAC});
		my $brname = "${prefix}$iface";
		$brs{$brname}{ENCAP} = 1;
		$brs{$brname}{SHORT} = 0;
		$brs{$brname}{PHYSDEV} = $iface;
	    }
	}
    }

    #
    # Make bridges and add phys ifaces.
    #
    # Or, in the macvlan case, create a dummy device if there is no
    # underlying physdev to "host" the macvlan.
    #
    foreach my $k (keys(%brs)) {
	# postpass to setup SHORT if only two members and no PMAC
	if (exists($brs{$k}{MEMBERS})) {
	    if ($brs{$k}{MEMBERS} == 2) {
		$brs{$k}{SHORT} = 1;
	    }
	    else {
		$brs{$k}{SHORT} = 0;
	    }
	    $brs{$k}{MEMBERS} = undef;
	}

	if (!$USE_MACVLAN) {
	    # building bridges is an important activity
	    if (! -d "/sys/class/net/$k/bridge") {
		mysystem("$BRCTL addbr $k");
	    }
	    # repetitions of this should not hurt anything
	    mysystem("$IFCONFIG $k 0 up");
	}

	if (exists($brs{$k}{PHYSDEV})) {
	    if (!$USE_MACVLAN) {
		# make sure this iface isn't already part of another bridge; if it
		# it is, remove it from there first and add to this bridge.
		my $obr = findBridge($brs{$k}{PHYSDEV});
		if (defined($obr)) {
		    mysystem("$BRCTL delif " . $obr . " " .$brs{$k}{PHYSDEV});
		    # rebuild hashes
		    makeBridgeMaps();
		}
		mysystem("$BRCTL addif $k $brs{$k}{PHYSDEV}");
	    }
	}
	elsif ($USE_MACVLAN
	       && ! -d "/sys/class/net/$k") {
	    # need to create a dummy device to "host" the macvlan ports
	    mysystem("$IP link add name $k type dummy");
	}
    }

    # Use the IMQDB to reserve the devices to the container. We have the lock.
    my %MDB;
    if (!dbmopen(%MDB, $IMQDB, 0660)) {
	print STDERR "*** Could not create $IMQDB\n";
	TBScriptUnlock();
	return -1;
    }
    my $i = 0;
    foreach my $node (keys(%$node_lds)) {
        foreach my $ldc (@{$node_lds->{$node}}) {
	    if ($ldc->{"TYPE"} eq 'duplex') {
		while ($i < $MAXIMQ) {
		    my $current = $MDB{"$i"};

		    if (!defined($current) ||
			$current eq "" || $current eq $node) {
			$MDB{"$i"} = $node;
			$i++;
			last;
		    }
		    $i++;
		}
		if ($i == $MAXIMQ) {
		    print STDERR "*** No more IMQs\n";
		    TBScriptUnlock();
		    return -1;
		}
	    }
	}
	# Clear anything else this node is using; no longer needed.
	for (my $j = $i; $j < $MAXIMQ; $j++) {
	    my $current = $MDB{"$j"};

	    if (!defined($current)) {
		$MDB{"$j"} = $current = "";
	    }
	    if ($current eq $node) {
		$MDB{"$j"} = "";
	    }
	}
    }
    dbmclose(%MDB);

    TBScriptUnlock();
    return 0;
}

sub vz_rootPostConfig {
    # Locking, if this ever does something?
    return 0;
}

#
# Create an OpenVZ container to host a vnode.  Should be called only once.
#
sub vz_vnodeCreate {
    my ($vnode_id,$image,$reload_args_ref) = @_;

    my $vmid;
    if ($vnode_id =~ /^\w+\d+\-(\d+)$/) {
	$vmid = $1;
    }
    else {
	fatal("vz_vnodeCreate: bad vnode_id $vnode_id!");
    }

    if (!defined($image) || $image eq '') {
	$image = $defaultImage;
    }

    my $imagelockpath = "/var/emulab/run/openvz.image.$image.ready";
    my $imagelockname = "vzimage.$image";
    my $imagepath = "/vz/template/cache/${image}.tar.gz";

    my %reload_args;
    if (defined($reload_args_ref)) {
	%reload_args = %$reload_args_ref;

	# Tell stated via tmcd
	libvnode::setState("RELOADSETUP");

	#
	# So, we are reloading this vnode (and maybe others).  Need to grab
	# the global lock for this image, check if we really need to download
	# the image based on the mtime for the currently cached image (if there
	# is one), if there is old image state, move out of the way, then
	# download the new image.  State to move out of teh way for an old
	# image is the ready file, the image file, lvm "root" devices that we
	# previously had built still-live VMs out of (we need to rename them),
	# and finally, garbage collecting unused "root" devices.  
	#
	# Note that we need to be really careful with the last item -- we 
	# only GC if our create has happened successfully, and we take the 
	# global image GC lock to do so.  This may race due to the nature 
	# of global locks and result in not all old devices getting reaped, 
	# but oh well.  Best effort for now.
	#
	if ((my $locked = TBScriptLock($imagelockname,
				       TBSCRIPTLOCK_GLOBALWAIT(), 1800))
	    != TBSCRIPTLOCK_OKAY()) {
	    print STDERR
		"Could not get the $imagelockname lock after a long time!\n";
	    return -1;
	}

	# do we have the right image file already?
	my $incache = 0;
	if (-e $imagepath) {
	    my (undef,undef,undef,undef,undef,undef,undef,undef,undef,
		$mtime,undef,undef,undef) = stat($imagepath);
	    if ("$mtime" eq $reload_args{"IMAGEMTIME"}) {
		$incache = 1;
	    }
	    else {
		print "mtimes for $imagepath differ: local $mtime, server " . 
		    $reload_args{"IMAGEMTIME"} . "\n";
		unlink($imagepath);
	    }
	}

	if (!$incache && $DOLVM) {
	    # did we create an lvm device for the old image at some point?
	    # (i.e., does the image lock file exist?)
	    if (-e $imagelockpath) {
		# if there's already a logical device for this image...
		my $sysret = system("lvdisplay /dev/openvz/$image >& /dev/null");
		if (!$sysret) {
		    my $rand = int(rand(100000));
		    my @outlines = system("lvs --noheadings");
		    my $found = 0;
		    while (!$found) {
			foreach my $line (@outlines) {
			    if ($line =~ /^\s*([-_\d\w]+)\.(\d+)\s+openvz/) {
				if ($rand == $2) {
				    $found = 1;
				    last;
				}
			    }
			}
			if ($found) {
			    $found = 0;
			    $rand = int(rand(100000));
			    @outlines = system("lvs --noheadings");
			}
			else {
			    $found = 1;
			}
		    }

		    # rename nicely works even when snapshots exist
		    mysystem("lvrename $LVMDEBUGOPTS /dev/openvz/$image" . 
			     " /dev/openvz/$image.$rand");

		    # now we can remove the readyfile
		    unlink($imagelockpath);
		}
	    }
	}
	elsif (!$incache && -e $imagelockpath) {
	    # now we can remove the readyfile
	    unlink($imagelockpath);
	}

	# Tell stated via tmcd
	libvnode::setState("RELOADING");

	if (!$incache) {
	    # Now we just download the file, then let create do its normal thing
	    my $dret = libvnode::downloadImage($imagepath,0,$reload_args_ref);

	    # reload has finished, file is written... so let's set its mtime
	    utime(time(),$reload_args{"IMAGEMTIME"},$imagepath);
	}

	TBScriptUnlock();
    }

    my $createArg = "";
    if ((my $locked = TBScriptLock($imagelockname,
				   TBSCRIPTLOCK_GLOBALWAIT(), 1800))
	!= TBSCRIPTLOCK_OKAY()) {
	print STDERR
	    "Could not get the $imagelockname lock after a long time!\n";
	return -1;
    }
    if ($DOLVM) {
	my $MIN_ROOT_LVM_VOL_SIZE = 1024;
	my $MAX_ROOT_LVM_VOL_SIZE = 8 * 1024;
	my $MIN_SNAPSHOT_VOL_SIZE = 512;
	my $MAX_SNAPSHOT_VOL_SIZE = $MAX_ROOT_LVM_VOL_SIZE;

	# XXX size our snapshots to assume 50 VMs on the node.
	my $MAX_NUM_VMS = 50;

	# figure out how big our volumes should be based on the volume
	# group size
	my $vgSize;
	my $rootSize = $MAX_ROOT_LVM_VOL_SIZE;
	my $snapSize = $MAX_SNAPSHOT_VOL_SIZE;

	open (VFD,"vgdisplay openvz |")
	    or die "popen(vgdisplay openvz): $!";
	while (my $line = <VFD>) {
	    chomp($line);
	    if ($line =~ /^\s+VG Size\s+(\d+[\.\d]*)\s+(\w+)/) {
		# convert to MB
		if ($2 eq "GB") {    $vgSize = $1 * 1024; }
		elsif ($2 eq "TB") { $vgSize = $1 * 1024 * 1024; }
		elsif ($2 eq "PB") { $vgSize = $1 * 1024 * 1024 * 1024; }
		elsif ($2 eq "MB") { $vgSize = $1 + 0; }
		elsif ($2 eq "KB") { $vgSize = $1 / 1024; }
		last;
	    }
	}
	close(VFD);

	if (defined($vgSize)) {
	    $vgSize /= 50;

	    if ($vgSize < $MIN_ROOT_LVM_VOL_SIZE) {
		$rootSize = int($MIN_ROOT_LVM_VOL_SIZE);
	    }
	    elsif ($vgSize < $MAX_ROOT_LVM_VOL_SIZE) {
		$rootSize = int($vgSize);
	    }
	    if ($vgSize < $MIN_SNAPSHOT_VOL_SIZE) {
		$snapSize = int($MIN_SNAPSHOT_VOL_SIZE);
	    }
	    elsif ($vgSize < $MAX_SNAPSHOT_VOL_SIZE) {
		$snapSize = int($vgSize);
	    }
	}

	print STDERR "Using LVM with root size $rootSize MB, snapshot size $snapSize MB.\n";

	# we must have the lock, so if we need to return right away, unlock
	if (-e $imagelockpath) {
	    TBScriptUnlock();
	}
	else {
	    print "Creating LVM core logical device for image $image\n";

	    # ok, create the lvm logical volume for this image.
	    mysystem("lvcreate $LVMDEBUGOPTS -L${rootSize}M -n $image openvz");
	    mysystem("mkfs -t ext3 /dev/openvz/$image");
	    mysystem("mkdir -p /tmp/mnt/$image");
	    mysystem("mount /dev/openvz/$image /tmp/mnt/$image");
	    mysystem("mkdir -p /tmp/mnt/$image/root /tmp/mnt/$image/private");
	    mysystem("tar -xzf $imagepath -C /tmp/mnt/$image/private");
	    mysystem("umount /tmp/mnt/$image");

	    # ok, we're done
	    mysystem("mkdir -p /var/emulab/run");
	    mysystem("touch $imagelockpath");
	    TBScriptUnlock();
	}

	# Now take a snapshot of this image's logical device
	mysystem("lvcreate $LVMDEBUGOPTS -s -L${snapSize}M -n $vnode_id /dev/openvz/$image");
	mysystem("mkdir -p /mnt/$vnode_id");
	mysystem("mount /dev/openvz/$vnode_id /mnt/$vnode_id");

	$createArg = "--private /mnt/$vnode_id/private" . 
	    " --root /mnt/$vnode_id/root --nofs yes";
    }
    else {
	TBScriptUnlock();
    }

    if (defined($reload_args_ref)) {
	# Tell stated via tmcd
	libvnode::setState("RELOADDONE");
	sleep(4);
	libvnode::setState("SHUTDOWN");
    }

    # build the container
    mysystem("$VZCTL $VZDEBUGOPTS create $vmid --ostemplate $image $createArg");

    # make sure bootvnodes actually starts things up on boot, not openvz
    mysystem("$VZCTL $VZDEBUGOPTS set $vmid --onboot no --name $vnode_id --save");

    # set some resource limits:
    my %deflimits = ( "diskinodes" => "unlimited:unlimited",
		      "diskspace" => "unlimited:unlimited",
		      "numproc" => "unlimited:unlimited",
		      "numtcpsock" => "unlimited:unlimited",
		      "numothersock" => "unlimited:unlimited",
		      "vmguarpages" => "unlimited:unlimited",
		      "kmemsize" => "unlimited:unlimited",
		      "tcpsndbuf" => "unlimited:unlimited",
		      "tcprcvbuf" => "unlimited:unlimited",
		      "othersockbuf" => "unlimited:unlimited",
		      "dgramrcvbuf" => "unlimited:unlimited",
		      "oomguarpages" => "unlimited:unlimited",
		      "lockedpages" => "unlimited:unlimited",
		      "privvmpages" => "unlimited:unlimited",
		      "shmpages" => "unlimited:unlimited",
		      "numfile" => "unlimited:unlimited",
		      "numflock" => "unlimited:unlimited",
		      "numpty" => "unlimited:unlimited",
		      "numsiginfo" => "unlimited:unlimited",
		      #"dcachesize" => "unlimited:unlimited",
		      "numiptent" => "unlimited:unlimited",
		      "physpages" => "unlimited:unlimited",
		      #"cpuunits" => "unlimited",
		      "cpulimit" => "0",
		      "cpus" => "unlimited",
		      "meminfo" => "none",
	);
    my $savestr = "";
    foreach my $k (keys(%deflimits)) {
	$savestr .= " --$k $deflimits{$k}";
    }
    mysystem("$VZCTL $VZDEBUGOPTS set $vmid $savestr --save");

    # XXX give them cap_net_admin inside containers... necessary to set
    # txqueuelen on devices inside the container.  This may have other
    # undesireable side effects, but need it for now.
    mysystem("$VZCTL $VZDEBUGOPTS set $vmid --capability net_admin:on --save");

    #
    # Make some directories in case the guest doesn't have them -- the elab
    # mount and umount vz scripts need them to be there!
    #
    my $privroot = "/vz/private/$vnode_id";
    if ($DOLVM) {
	$privroot = "/mnt/$vnode_id/private";
    }
    mysystem("mkdir -p $privroot/var/emulab/boot/");

    # NOTE: we can't ever umount the LVM logical device because vzlist can't
    # return status appropriately if a VM's root and private areas don't
    # exist.
    if (0 && $DOLVM) {
	mysystem("umount /mnt/$vnode_id");
    }

    return $vmid;
}

sub vz_vnodeDestroy {
    my ($vnode_id,$vmid) = @_;

    if ($DOLVM) {
	mysystem("umount /mnt/$vnode_id");
	mysystem("lvremove $LVMDEBUGOPTS -f /dev/openvz/$vnode_id");
    }

    my @ifs  = ();
    open(CF,"/etc/vz/conf/$vmid.conf") or
	 die "could not open etc/vz/conf/$vmid.conf for read: $!";
    my @lines = grep { $_ =~ /^ELABROUTES/ } <CF>;
    seek(CF,0,0);
    my @ndlines = grep { $_ =~ /^NETDEV/ } <CF>;
    close(CF);
    if (@lines) {
	# Should only be one line.
	my $elabroutes = $lines[0];
	if ($elabroutes =~ /ELABROUTES="(.*)"/) {
	    $elabroutes = $1;
	}
	else {
	    print STDERR "Could not parse ELABROUTES in config file.\n";
	    return -1;
	}
	my @routes = split(/;/, $elabroutes);
	foreach my $route (@routes) {
	    my @tokens = split(/,/, $route);
	    # old format, skip to avoid errors.
	    next
		if (scalar(@tokens) == 2);
	    # The first and third tokens are the ones we have to delete.
	    push(@ifs, $tokens[0]);
	    push(@ifs, $tokens[2]);
	    FreeRouteTable($tokens[0]);
	}
    }
    foreach my $if (@ifs) {
	mysystem2("/sbin/ip rule del iif $if");
	if ($if =~ /^gre/) {
	    mysystem2("/sbin/ip tunnel del $if");
	}
    }
    FreeRouteTable("VZ$vmid");

    foreach my $ndline (@ndlines) {
	chomp($ndline);
	if ($ndline =~ /^NETDEV=\"(.+)\"$/) {
	    my @devs = split(/\s+/,$1);
	    foreach my $dev (@devs) {
		mysystem("$IP link del dev $dev");
	    }
	}
    }

    mysystem("$VZCTL $VZDEBUGOPTS destroy $vnode_id");
    return -1
	if ($?);

    #
    # Clear the IMQ reservations. Must lock since IMQDB is a shared
    # resource.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the vzpreconfig lock after a long time!\n";
	return -1;
    }
    my %MDB;
    if (!dbmopen(%MDB, $IMQDB, 0660)) {
	print STDERR "*** Could not open $IMQDB\n";
	TBScriptUnlock();
	return -1;
    }
    for (my $i = 0; $i < $MAXIMQ; $i++) {
	next
	    if ($MDB{"$i"} ne $vnode_id);
	$MDB{"$i"} = "";
    }
    dbmclose(%MDB);
    TBScriptUnlock();
    return 0;
}

sub vz_vnodeExec {
    my ($vnode_id,$vmid,$command) = @_;

    # Note: do not use mysystem here since that exits.
    system("$VZCTL exec2 $vnode_id $command");

    return $?;
}

sub vz_vnodeState {
    my ($vnode_id,$vmid) = @_;

    # Sometimes if the underlying filesystems are not mounted, we might get 
    # no status even though the vnode has been created (currently, this will
    # only happen with LVM)... since the openvz utils seem to need to see the
    # vnode filesystem in order to work properly, which is sensible).
    if ($DOLVM) {
	if (-e "/etc/vz/conf/$vmid.conf" && -e "/dev/openvz/$vnode_id"
	    && ! -e "/mnt/$vnode_id/private") {
	    print "Trying to mount LVM logical device for vnode $vnode_id: ";
	    mysystem("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
	    print "done.\n";
	}
    }

    my $status = vmstatus($vmid);
    return VNODE_STATUS_UNKNOWN()
	if (!defined($status));

    if ($status eq 'running') {
	return VNODE_STATUS_RUNNING();
    }
    elsif ($status eq 'stopped') {
	return VNODE_STATUS_STOPPED();
    }
    elsif ($status eq 'mounted') {
	return VNODE_STATUS_MOUNTED();
    }

    return VNODE_STATUS_UNKNOWN();
}

sub vz_vnodeBoot {
    my ($vnode_id,$vmid) = @_;

    if ($DOLVM) {
	system("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
    }

    mysystem("$VZCTL $VZDEBUGOPTS start $vnode_id");

    return 0;
}

sub vz_vnodeHalt {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL $VZDEBUGOPTS stop $vnode_id");
    return 0;
}

sub vz_vnodeUnmount {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL $VZDEBUGOPTS umount $vnode_id");

    return 0;
}

sub vz_vnodeReboot {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL $VZDEBUGOPTS restart $vnode_id");

    return 0;
}

sub vz_vnodePreConfig {
    my ($vnode_id,$vmid,$callback) = @_;

    # Make sure we're mounted so that vzlist and friends work; see NOTE about
    # mounting LVM logical devices above.
    if ($DOLVM) {
	system("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
    }

    #
    # Look and see if this node already has imq devs mapped into it -- if
    # those match the ones in the IMQDB, do nothing, else fixup. Must lock
    # since IMQDB is a shared resource.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the vzpreconfig lock after a long time!\n";
	return -1;
    }
    my %MDB;
    if (!dbmopen(%MDB, $IMQDB, 0660)) {
	print STDERR "*** Could not open $IMQDB\n";
	TBScriptUnlock();
	return -1;
    }
    my %devs = ();

    for (my $i = 0; $i < $MAXIMQ; $i++) {
	next
	    if ($MDB{"$i"} ne $vnode_id);

	$devs{"imq$i"} = 1;
    }
    dbmclose(%MDB);
    TBScriptUnlock();
    
    my $existing = `sed -n -r -e 's/NETDEV="(.*)"/\1/p' /etc/vz/conf/$vmid.conf`;
    chomp($existing);
    foreach my $dev (split(/,/,$existing)) {
	if (!($dev =~ /^imq/)) {
	    next;
	}

	if (!exists($devs{$dev})) {
	    # needs deleting
	    $devs{$dev} = 0;
	}
	else {
	    # was already mapped, leave alone
	    $devs{$dev} = undef;
	}
    }

    foreach my $dev (keys(%devs)) {
	if ($devs{$dev} == 1) {
	    mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id --netdev_add $dev --save");
	}
	elsif ($devs{$dev} == 0) {
	    mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id --netdev_del $dev --save");
	}
    }
    #
    # Make sure container is mounted before calling the callback.
    #
    my $status = vmstatus($vmid);
    my $didmount = 0;
    if ($status ne 'running' && $status ne 'mounted') {
	mysystem("$VZCTL $VZDEBUGOPTS mount $vnode_id");
	$didmount = 1;
    }
    my $privroot = "/vz/private/$vmid";
    if ($DOLVM) {
	$privroot = "/mnt/$vnode_id/private";
    }
    # Serialize the callback. Sucks. iptables.
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get callback lock after a long time!\n";
	return -1;
    }
    my $ret = &$callback("$privroot");
    TBScriptUnlock();
    if ($didmount) {
	mysystem("$VZCTL $VZDEBUGOPTS umount $vnode_id");
    }
    return $ret;
}

#
# Preconfigure the control net interface; special case of vnodeConfigInterfaces.
#
sub vz_vnodePreConfigControlNetwork {
    my ($vnode_id,$vmid,$ip,$mask,$mac,$gw,
	$vname,$longdomain,$shortdomain,$bossip) = @_;

    # setup iptables on real ctrl net
    my ($ciface,$cip,$cnetmask,$cmaskbits,$cnetwork,$cmac) = findControlNet();

    my @ipa = map { int($_); } split(/\./,$ip);
    my @maska = map { int($_); } split(/\./,$mask);
    my @neta = ($ipa[0] & $maska[0],$ipa[1] & $maska[1],
		$ipa[2] & $maska[2],$ipa[3] & $maska[3]);
    my $net = join('.',@neta);

    #
    # Have to serialize iptables access. Silly locking problem in the kernel.
    #
    if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "PreConfigControlNetwork: ".
	    "Could not get the lock after a long time!\n";
	return -1;
    }
    # If the SNAT rule is there, probably we're good.
    if (system('iptables -t nat -L POSTROUTING' . 
	       ' | grep -q -e \'^SNAT.* ' . $net . '\'')) {
	if (system("$MODPROBE ip_nat") ||
	    system("$IPTABLES -t nat -A POSTROUTING" . 
		   " -s $net/$mask" . 
		   " -d $cnetwork/$cnetmask -j ACCEPT") ||
	    system("$IPTABLES -t nat -A POSTROUTING" . 
		   " -s $net/$mask" . 
		   " -d $net/$mask -j ACCEPT") ||
	    system("$IPTABLES -t nat -A POSTROUTING" . 
		   " -s $net/$mask" . 
		   " -o $ciface -j SNAT --to-source $cip")) {
	    print STDERR "Could not PreConfigControlNetwork iptables\n";
	    TBScriptUnlock();
	    return -1;
	}
    }
    TBScriptUnlock();

    # Make sure we're mounted so that vzlist and friends work; see NOTE about
    # mounting LVM logical devices above.
    if ($DOLVM) {
	system("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
    }

    my $privroot = "/vz/private/$vmid";
    if ($DOLVM) {
	$privroot = "/mnt/$vnode_id/private";
    }

    # add the control net iface
    my $cnet_veth = "veth${vmid}.${CONTROL_IFNUM}";
    my $cnet_mac = macAddSep($mac);
    my $ext_vethmac = $cnet_mac;
    if ($ext_vethmac =~ /^(00:00)(.*)$/) {
	$ext_vethmac = "00:01$2";
    }

    #
    # we have to hack the VEID.conf file BEFORE calling --netif_add ... --save
    # below so that when the custom script is run against our changes, it does
    # the right thing!
    #
    my %lines = ( 'ELABCTRLIP' => '"' . $ip . '"',
		  'ELABCTRLDEV' => '"' . $cnet_veth . '"' );
    editContainerConfigFile($vmid,\%lines);

    # note that we don't assign a mac to the CT0 part of the veth pair -- 
    # openvz does that automagically
    mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id" . 
	     " --netif_add ${CONTROL_IFDEV},$cnet_mac,$cnet_veth,$ext_vethmac --save");

    #
    # Make sure container is mounted
    #
    my $status = vmstatus($vmid);
    my $didmount = 0;
    if ($status ne 'running' && $status ne 'mounted') {
	if ($DOLVM) {
	    system("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
	}
	mysystem("$VZCTL $VZDEBUGOPTS mount $vnode_id");
    }

    #
    # Setup lo
    #
    open(FD,">$privroot/etc/sysconfig/network-scripts/ifcfg-lo") 
	or die "vz_vnodePreConfigControlNetwork: could not open ifcfg-lo for $vnode_id: $!";
    print FD "DEVICE=lo\n";
    print FD "IPADDR=127.0.0.1\n";
    print FD "NETMASK=255.0.0.0\n";
    print FD "NETWORK=127.0.0.0\n";
    print FD "BROADCAST=127.255.255.255\n";
    print FD "ONBOOT=yes\n";
    print FD "NAME=loopback\n";
    close(FD);

    # remove any regular control net junk
    unlink("$privroot/etc/sysconfig/network-scripts/ifcfg-eth99");

    #
    # setup the control net iface in the FS ...
    #
    open(FD,">$privroot/etc/sysconfig/network-scripts/ifcfg-${CONTROL_IFDEV}") 
	or die "vz_vnodePreConfigControlNetwork: could not open ifcfg-${CONTROL_IFDEV} for $vnode_id: $!";
    print FD "DEVICE=${CONTROL_IFDEV}\n";
    print FD "IPADDR=$ip\n";
    print FD "NETMASK=$mask\n";
    
    my @ip;
    my @mask;
    if ($ip =~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/) {
	@ip = ($1,$2,$3,$4);
    }
    if ($mask =~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/) {
	@mask = ($1+0,$2+0,$3+0,$4+0);
    }
    my $network = ($ip[0] & $mask[0]) . "." . ($ip[1] & $mask[1]) . 
	"." . ($ip[2] & $mask[2]) . "." . ($ip[3] & $mask[3]);
    my $bcast = ($ip[0] | (~$mask[0] & 0xff)) . 
	"." . ($ip[1] | (~$mask[1] & 0xff)) . 
	"." . ($ip[2] | (~$mask[2] & 0xff)) . 
	"." . ($ip[3] | (~$mask[3] & 0xff));
    # grab number of network bits too, sigh
    my $maskbits = 0;
    foreach my $m (@mask) {
	for (my $i = 0; $i < 8; ++$i) {
	    $maskbits += (0x01 & ($m >> $i));
	}
    }

    print FD "NETWORK=$network\n";
    print FD "BROADCAST=$bcast\n";
    print FD "ONBOOT=yes\n";
    close(FD);

    # setup routes:
    my ($ctrliface,$ctrlip,$ctrlmask,$ctrlmaskbits,$ctrlnet,$ctrlmac) 
	= findControlNet();
    open(FD,">$privroot/etc/sysconfig/network-scripts/route-${CONTROL_IFDEV}") 
	or die "vz_vnodePreConfigControlNetwork: could not open route-${CONTROL_IFDEV} for $vnode_id: $!";
    #
    # HUGE NOTE: we *have* to use the /<bits> form, not the /<netmask> form
    # for now, since our iproute version is old.
    #
    print FD "$ctrlnet/$ctrlmaskbits dev ${CONTROL_IFDEV}\n";
    print FD "0.0.0.0/0 via $ctrlip\n";
    close(FD);

    #
    # ... and make sure it gets brought up on boot:
    # XXX: yes, this would blow away anybody's changes, but don't care now.
    #
    open(FD,">$privroot/etc/sysconfig/network") 
	or die "vz_vnodePreConfigControlNetwork: could not open sysconfig/networkfor $vnode_id: $!";
    print FD "NETWORKING=yes\n";
    print FD "HOSTNAME=$vname.$longdomain\n";
    print FD "DOMAIN=$longdomain\n";
    print FD "NOZEROCONF=yes\n";
    close(FD);

    #
    # dhclient-exit-hooks normally writes this stuff on linux, so we'd better
    # do it here.
    #
    my $mybootdir = "$privroot/var/emulab/boot/";

    # and before the dhclient stuff, do this first to tell bootsetup that we 
    # are a GENVNODE...
    open(FD,">$mybootdir/vmname") 
	or die "vz_vnodePreConfigControlNetwork: could not open vmname for $vnode_id: $!";
    print FD "$vnode_id\n";
    close(FD);
    # ...and that our event server is the proxy in the phys host
    open(FD,">$mybootdir/localevserver") 
	or die "vz_vnodePreConfigControlNetwork: could not open localevserver for $vnode_id: $!";
    print FD "$ctrlip\n";
    close(FD);

    open(FD,">$mybootdir/myip") 
	or die "vz_vnodePreConfigControlNetwork: could not open myip for $vnode_id: $!";
    print FD "$ip\n";
    close(FD);
    open(FD,">$mybootdir/mynetmask") 
	or die "vz_vnodePreConfigControlNetwork: could not open mynetmask for $vnode_id: $!";
    print FD "$mask\n";
    close(FD);
    open(FD,">$mybootdir/routerip") 
	or die "vz_vnodePreConfigControlNetwork: could not open routerip for $vnode_id: $!";
    print FD "$gw\n";
    close(FD);
    open(FD,">$mybootdir/controlif") 
	or die "vz_vnodePreConfigControlNetwork: could not open controlif for $vnode_id: $!";
    print FD "${CONTROL_IFDEV}\n";
    close(FD);
    open(FD,">$mybootdir/realname") 
	or die "vz_vnodePreConfigControlNetwork: could not open realname for $vnode_id: $!";
    print FD "$vnode_id\n";
    close(FD);
    open(FD,">$mybootdir/bossip") 
	or die "vz_vnodePreConfigControlNetwork: could not open bossip for $vnode_id: $!";
    print FD "$bossip\n";
    close(FD);

    #
    # Let's not hang ourselves before we start
    #
    open(FD,">$privroot/etc/resolv.conf") 
	or die "vz_vnodePreConfigControlNetwork: could not open resolv.conf for $vnode_id: $!";

    print FD "nameserver $bossip\n";
    print FD "search $shortdomain\n";
    close(FD);

    #
    # XXX Ugh, this is icky, but it avoids a second mount in PreConfig().
    # Want to copy all the tmcd config info from root context into the 
    # container.
    #
    mysystem("cp -R /var/emulab/boot/tmcc.$vnode_id $mybootdir/");

    if ($didmount) {
	mysystem("$VZCTL $VZDEBUGOPTS umount $vnode_id");
    }

    return 0;
}

#
# Preconfigures experimental interfaces in the vnode before its first boot.
#
sub vz_vnodePreConfigExpNetwork {
    my ($vnode_id,$vmid,$ifs,$lds,$tunnels) = @_;

    # Make sure we're mounted so that vzlist and friends work; see NOTE about
    # mounting LVM logical devices above.
    if ($DOLVM) {
	system("mount /dev/openvz/$vnode_id /mnt/$vnode_id");
    }

    my $basetable;
    my $elabifs = "";
    my $elabroutes = "";
    my %netif_strs = ();
    foreach my $ifc (@$ifs) {
	next if (!$ifc->{ISVIRT});

	my $br;
	my $prefix = "br.";
	if ($USE_MACVLAN) {
	    $prefix = "mvsw.";
	}

	my $physdev;
	if ($ifc->{ITYPE} eq "vlan") {
	    my $iface = $ifc->{IFACE};
	    my $vtag  = $ifc->{VTAG};
	    my $vdev  = "${iface}.${vtag}";
	    $br = "${prefix}$vdev";
	    $physdev = $vdev;
	}
	elsif ($ifc->{PMAC} eq "none" || $ifc->{ITYPE} eq "loop") {
	    $br = "${prefix}" . $ifc->{VTAG};
	    $physdev = $br;
	}
	else {
	    my $iface = findIface($ifc->{PMAC});
	    $br = "${prefix}$iface";
	    $physdev = $iface;
	}

	#
	# The ethX naming sucks, but hopefully it ensures unique, *easily 
	# reconfigurable* (i.e., without a local map file) naming for veths.
	#
	my $eth = "eth" . $ifc->{VTAG};
	my ($vethmac,$ethmac) = libsetup::build_fake_macs(libsetup::libsetup_getvnodeid(),
							  $ifc->{VMAC});

	($ethmac,$vethmac) = (macAddSep($ethmac),macAddSep($vethmac));

	if ($USE_MACVLAN) {
	    #
	    # Add the macvlan device atop the dummy devices created earlier,
	    # or atop the physical or vlan device.
	    #
	    my $vname = "mv$vmid.$ifc->{ID}";
	    if (! -d "/sys/class/net/$vname") {
		mysystem("$IP link add link $physdev name $vname address $vethmac type macvlan mode bridge ");
	    }
	    mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id --netdev_add $vname --save");
	}
	else {
	    #
	    # Add to ELABIFS for addition to conf file (for runtime config by 
	    # external custom script)
	    #
	    my $veth = "veth$vmid.$ifc->{ID}";
	    if ($elabifs ne '') {
		$elabifs .= ';';
	    }
	    $elabifs .= "$veth,$br";

	    #
	    # Save for later calling, since we need to hack the 
	    # config file BEFORE calling --netif_add so the custom postconfig 
	    # script does the right thing.
	    # Also store up the current set of netifs so we can delete any that
	    # might have been old!
	    #
	    $netif_strs{$eth} = "$eth,$ethmac,$veth,$vethmac";
	}
    }

    if (values(%{ $tunnels })) {
	#
	# gres and route tables are a global resource.
	#
	if (TBScriptLock($GLOBAL_CONF_LOCK, 0, 900) != TBSCRIPTLOCK_OKAY()) {
	    print STDERR "Could not get the tunne lock after a long time!\n";
	    return -1;
	}
	$basetable = AllocateRouteTable("VZ$vmid");
	if (!defined($basetable)) {
	    print STDERR "Could not allocate a routing table!\n";
	    TBScriptUnlock();
	    return -1;
	}

	#
	# Get current gre list.
	#
	if (! open(IP, "/sbin/ip tunnel show|")) {
	    print STDERR "Could not start /sbin/ip\n";
	    TBScriptUnlock();
	    return -1;
	}
	my %key2gre = ();
	my $maxgre  = 0;
	my $table   = $vmid + 100;

	while (<IP>) {
	    if ($_ =~ /^(gre\d*):.*key\s*([\d\.]*)/) {
		$key2gre{$2} = $1;
		if ($1 =~ /^gre(\d*)$/) {
		    $maxgre = $1
			if ($1 > $maxgre);
		}
	    }
	    elsif ($_ =~ /^(gre\d*):.*remote\s*([\d\.]*)\s*local\s*([\d\.]*)/) {
		#
		# This is just a temp fixup; delete tunnels with no key
		# since we no longer use non-keyed tunnels, and cause it
		# will cause the kernel to throw an error in the tunnel add
		# below. 
		#
		mysystem2("/sbin/ip tunnel del $1");
		if ($?) {
		    TBScriptUnlock();
		    return -1;
		}
	    }
	}
	if (!close(IP)) {
	    print STDERR "Could not get tunnel list\n";
	    TBScriptUnlock();
	    return -1;
	}

	foreach my $tunnel (values(%{ $tunnels })) {
	    next
		if ($tunnel->{"tunnel_style"} ne "gre");

	    my $name     = $tunnel->{"tunnel_lan"};
	    my $srchost  = $tunnel->{"tunnel_srcip"};
	    my $dsthost  = $tunnel->{"tunnel_dstip"};
	    my $inetip   = $tunnel->{"tunnel_ip"};
	    my $peerip   = $tunnel->{"tunnel_peerip"};
	    my $mask     = $tunnel->{"tunnel_ipmask"};
	    my $unit     = $tunnel->{"tunnel_unit"};
	    my $grekey   = inet_ntoa(pack("N", $tunnel->{"tunnel_tag"}));
	    my $gre;

	    if (exists($key2gre{$grekey})) {
		$gre = $key2gre{$grekey};
	    }
	    else {
		$gre = "gre" . ++$maxgre;
		mysystem2("/sbin/ip tunnel add $gre mode gre ".
			 "local $srchost remote $dsthost ttl 64 key $grekey");
		if ($?) {
		    TBScriptUnlock();
		    return -1;
		}
		mysystem2("/sbin/ifconfig $gre 0 up");
		if ($?) {
		    TBScriptUnlock();
		    return -1;
		}
		$key2gre{$grekey} = $gre;
	    }
	    #
	    # All packets arriving from gre devices will use the same table.
	    # The route will be a network route to the root context device.
	    # The route cannot be inserted until later, since the root 
	    # context device does not exists until the VM is running.
	    # See the route stuff in vznetinit-elab.sh.
	    #
	    mysystem2("/sbin/ip rule add unicast iif $gre table $basetable");
	    if ($?) {
		TBScriptUnlock();
		return -1;
	    }
	    # device name outside the container
	    my $veth = "veth$vmid.tun$unit";
	    # device name inside the container
	    my $eth  = "gre$unit";
	    
	    $netif_strs{$eth} = "$eth,,$veth";
	    if ($elabifs ne '') {
		$elabifs .= ';';
	    }
	    # Leave bridge blank; see vznetinit-elab.sh. It does stuff.
	    $elabifs .= "$veth,";
	    # Route.

	    if ($elabroutes ne '') {
		$elabroutes .= ';';
	    }
	    $elabroutes .= "$veth,$inetip,$gre";

	    #
	    # We need a routing table for each tunnel in the other direction.
	    # This makes sure that all packets coming out of the root context
	    # device (leaving the VM) got shoved into the real gre device.
	    # Need to use a default route so all packets ae matched, which is
	    # why we need a table per tunnel.
	    #
	    my $routetable = AllocateRouteTable($veth);
	    if (!defined($routetable)) {
		    print STDERR "No free route tables for $veth\n";
		    TBScriptUnlock();
		    return -1;
	    }
	    #
	    # Convenient, is that even though the root context device does
	    # not exist, we can insert the ip *rule* for it that directs
	    # the traffic through the iproute inserted below.
	    #
	    mysystem2("/sbin/ip rule add unicast iif $veth table $routetable");
	    if ($?) {
		TBScriptUnlock();
		return -1;
	    }
	    my $net = inet_ntoa(inet_aton($inetip) & inet_aton($mask));
	    mysystem2("/sbin/ip route replace ".
		      "  default dev $gre table $routetable");
	    if ($?) {
		TBScriptUnlock();
		return -1;
	    }
	}
	TBScriptUnlock();
    }

    #
    # Wait until end to do a single edit for all ifs, since they're all 
    # smashed into a single config file var
    #
    my %lines = ( 'ELABIFS'    => '"' . $elabifs . '"',
		  'ELABROUTES' => '"' . $elabroutes . '"');
    if (defined($basetable)) {
	$lines{'ROUTETABLE'} = '"' . $basetable . '"';
    }
    editContainerConfigFile($vmid,\%lines);

    #
    # Ok, add (and delete stale) veth devices!
    # Grab current ones first.
    #
    my @current = ();
    open(CF,"/etc/vz/conf/$vmid.conf") 
	or die "could not open etc/vz/conf/$vmid.conf for read: $!";
    my @lines = grep { $_ =~ /^NETIF/ } <CF>;
    close(CF);
    if (@lines) {
	# always take the last one :-)
	my $netifs = $lines[@lines-1];
	if ($netifs =~ /NETIF="(.*)"/) {
	    $netifs = $1;
	}
	my @nifs = split(/;/,$netifs);
	foreach my $nif (@nifs) {
	    if ($nif =~ /ifname=([\w\d\-]+)/) {
		# don't delete the control net device!
		next if ($1 eq $CONTROL_IFDEV);

		push @current, $1;
	    }
	}
    }

    # delete
    foreach my $eth (@current) {
	if (!exists($netif_strs{$eth})) {
	    mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id --netif_del $eth --save");
	}
    }
    # add/modify
    foreach my $eth (keys(%netif_strs)) {
	mysystem("$VZCTL $VZDEBUGOPTS set $vnode_id --netif_add $netif_strs{$eth} --save");
    }

    return 0;
}

sub vz_vnodeConfigResources {
    return 0;
}

sub vz_vnodeConfigDevices {
    return 0;
}

sub vz_vnodePostConfig {
    my ($vnode_id,$vmid) = @_;

    return 0;
}

sub vz_setDebug($) {
    $debug = shift;
    libvnode::setDebug($debug);
}

##
## Bunch of helper functions.
##

#
# Edit an openvz container config file -- add a little emulab header and some
# vars to signal customization.  After that, change/add any lines indicated by
# the key/val pairs in the hash (sensible since the config file is intended to
# be slurped up by shells or something).
#
sub editContainerConfigFile($$) {
    my ($vmid,$edlines) = @_;

    my $conffile = "/etc/vz/conf/$vmid.conf";

    open(FD,"$conffile") 
	or die "could not open $conffile: $!";
    my @lines = <FD>;
    close(FD);

    if (!grep(/^ELABCUSTOM/,@lines)) {
	$lines[@lines] = "\n";
	$lines[@lines] = "#\n";
	$lines[@lines] = "# Emulab hooks\n";
	$lines[@lines] = "#\n";
	$lines[@lines] = "CONFIG_CUSTOMIZED=\"yes\"\n";
	$lines[@lines] = "ELABCUSTOM=\"yes\"\n";
    }

    # make a copy so we can delete keys
    my %dedlines = ();
    foreach my $k (keys(%$edlines)) {
	$dedlines{$k} = $edlines->{$k};
    }

    for (my $i = 0; $i < @lines; ++$i) {
	# note that if the value is a string, the quotes have to be sent
	# in from caller!
	if ($lines[$i] =~ /^([^#][^=]+)=(.*)$/) {
	    my $k = $1;
	    if (exists($dedlines{$k}) && $2 ne $dedlines{$k}) {
		$lines[$i] = "$k=$dedlines{$k}\n";
		delete $dedlines{$k};
	    }
	}
    }
    foreach my $k (keys(%dedlines)) {
	$lines[@lines] = "$k=$dedlines{$k}\n";
    }

    open(FD,">$conffile") 
	or die "could not open $conffile for writing: $!";
    foreach my $line (@lines) {
	print FD $line;
    }
    close(FD);

    return 0;
}

sub vmexists($) {
    my $id = shift;

    return 1
	if (!system("$VZLIST $id"));
    return 0;
}

sub vmstatus($) {
    my $id = shift;

    open(PFD,"$VZLIST $id |") 
	or die "could not exec $VZLIST: $!";
    while (<PFD>) {
	if ($_ =~ /^\s+$id\s+[^\s]+\s+(\w+)/) {
	    close(PFD);
	    return $1;
	}
    }
    close(PFD);
    return undef;
}

sub vmrunning($) {
    my $id = shift;

    return 1 
	if (vmstatus($id) eq VZSTAT_RUNNING);
    return 0;
}

sub vmstopped($) {
    my $id = shift;

    return 1 
	if (vmstatus($id) eq VZSTAT_STOPPED);
    return 0;
}

#
# See if a route table already exists for the given tag, and if not,
# allocate it and return the table number.
#
sub AllocateRouteTable($)
{
    my ($token) = @_;
    my $rval = undef;

    if (! -e $RTDB && InitializeRouteTables()) {
	print STDERR "*** Could not initialize routing table DB\n";
	return undef;
    }
    my %RTDB;
    if (!dbmopen(%RTDB, $RTDB, 0660)) {
	print STDERR "*** Could not open $RTDB\n";
	return undef;
    }
    # Look for existing.
    for (my $i = 1; $i < $MAXROUTETTABLE; $i++) {
	if ($RTDB{"$i"} eq $token) {
	    $rval = $i;
	    print STDERR "Found routetable $i ($token)\n";
	    goto done;
	}
    }
    # Allocate a new one.
    for (my $i = 1; $i < $MAXROUTETTABLE; $i++) {
	if ($RTDB{"$i"} eq "") {
	    $RTDB{"$i"} = $token;
	    print STDERR "Allocate routetable $i ($token)\n";
	    $rval = $i;
	    goto done;
	}
    }
  done:
    dbmclose(%RTDB);
    return $rval;
}

sub LookupRouteTable($)
{
    my ($token) = @_;
    my $rval = undef;

    my %RTDB;
    if (!dbmopen(%RTDB, $RTDB, 0660)) {
	print STDERR "*** Could not open $RTDB\n";
	return undef;
    }
    # Look for existing.
    for (my $i = 1; $i < $MAXROUTETTABLE; $i++) {
	if ($RTDB{"$i"} eq $token) {
	    $rval = $i;
	    goto done;
	}
    }
  done:
    dbmclose(%RTDB);
    return $rval;
}

sub FreeRouteTable($)
{
    my ($token) = @_;
    
    my %RTDB;
    if (!dbmopen(%RTDB, $RTDB, 0660)) {
	print STDERR "*** Could not open $RTDB\n";
	return -1;
    }
    # Look for existing.
    for (my $i = 1; $i < $MAXROUTETTABLE; $i++) {
	if ($RTDB{"$i"} eq $token) {
	    $RTDB{"$i"} = "";
	    print STDERR "Free routetable $i ($token)\n";
	    last;
	}
    }
    dbmclose(%RTDB);
    return 0;
}

sub InitializeRouteTables()
{
    # Create clean route table DB and seed it with defaults.
    my %RTDB;
    if (!dbmopen(%RTDB, $RTDB, 0660)) {
	print STDERR "*** Could not create $RTDB\n";
	return -1;
    }
    # Clear all,
    for (my $i = 0; $i < $MAXROUTETTABLE; $i++) {
	$RTDB{"$i"} = ""
	    if (!exists($RTDB{"$i"}));
    }
    # Seed the reserved tables.
    if (! open(RT, $RTTABLES)) {
	print STDERR "*** Could not open $RTTABLES\n";
	return -1;
    }
    while (<RT>) {
	if ($_ =~ /^(\d*)\s*/) {
	    $RTDB{"$1"} = "$1";
	}
    }
    close(RT);
    dbmclose(%RTDB);
    return 0;
}

# what can I say?
1;
