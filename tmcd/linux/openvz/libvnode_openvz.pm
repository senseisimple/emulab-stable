#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008-2009 University of Utah and the Flux Group.
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
              vz_vnodePreConfig 
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
use Data::Dumper;

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

my $defaultImage = "emulab-default";

sub VZSTAT_RUNNING() { return "running"; }
sub VZSTAT_STOPPED() { return "stopped"; }

my $VZCTL  = "/usr/sbin/vzctl";
my $VZLIST = "/usr/sbin/vzlist";
my $IFCONFIG = "/sbin/ifconfig";
my $ROUTE = "/sbin/route";
my $BRCTL = "/usr/sbin/brctl";
my $IPTABLES = "/sbin/iptables";
my $MODPROBE = "/sbin/modprobe";
my $RMMOD = "/sbin/rmmod";

my $VZRC   = "/etc/init.d/vz";
my $MKEXTRAFS = "/usr/local/etc/emulab/mkextrafs.pl";

my $VZROOT = "/vz/root";

my $CTRLIPFILE = "/var/emulab/boot/myip";
my $IMQDB      = "/var/emulab/db/imqdb";
my $MAXIMQ     = 127;

my $CONTROL_IFNUM  = 999;
my $CONTROL_IFDEV  = "eth${CONTROL_IFNUM}";
my $EXP_BASE_IFNUM = 0;

my $debug = 0;

my %if2mac = ();
my %mac2if = ();
my %ip2if = ();
my %bridges = ();
my %if2br = ();

# XXX needs lifting up
my $JAILCTRLNET = "172.16.0.0";
my $JAILCTRLNETMASK = "255.240.0.0";
my $PUBCTRLNET = "155.98.36.0";
my $PUBCTRLNETMASK = "255.255.252.0";
my $PUBCTRLNETMASKBITS = 22;
#
# Helpers.
#
sub findControlNet();
sub makeIfaceMaps();
sub makeBridgeMaps();
sub findIface($);
sub findMac($);
sub editContainerConfigFile($$);

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

    if ((my $locked = TBScriptLock("vzconf", TBSCRIPTLOCK_GLOBALWAIT(), 90)) 
	!= TBSCRIPTLOCK_OKAY()) {
	return 0
	    if ($locked == TBSCRIPTLOCK_IGNORE());
	print STDERR "Could not get the vzinit lock after a long time!\n";
	return -1;
    }
    # 
    return 0
	if (-e "/var/run/openvz.ready");
    
    # make sure filesystem is setup 
    # about the funny quoting: don't ask... emacs perl mode foo.
    if (system('grep -q '."'".'^/dev/.*/vz.*$'."'".' /etc/fstab')) {
	mysystem("$VZRC stop");
	mysystem("mv /vz /vz.orig");
	mysystem("mkdir /vz");
	mysystem("$MKEXTRAFS -f /vz");
	mysystem("cp -pR /vz.orig/* /vz/");
	mysystem("rm -rf /vz.orig");
    }
    if (system('mount | grep -q \'on /vz\'')) {
	mysystem("mount /vz");
    }

    # make sure the initscript is going...
    if (system("$VZRC status 2&>1 > /dev/null")) {
	mysystem("$VZRC start");
    }

    # get rid of this simple container device support
    if (!system('lsmod | grep -q vznetdev')) {
	system("$RMMOD vznetdev");
    }
    # this is what we need for veths
    mysystem("$MODPROBE vzethdev");

    # we need this stuff for traffic shaping -- only root context can
    # modprobe, for now.
    mysystem("$MODPROBE sch_plr");
    mysystem("$MODPROBE sch_delay");
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

    # iptables/forwarding on ctrl net
    my ($iface,$ip,$mac) = findControlNet();

    mysystem("echo 1 > /proc/sys/net/ipv4/conf/$iface/forwarding");

    # If the SNAT rule is there, probably we're good.
    if (system('iptables -t nat -L POSTROUTING' . 
	       ' | grep -q -e \'^SNAT.* ' . $JAILCTRLNET . '\'')) {
	mysystem("$MODPROBE ip_nat");
	mysystem("$IPTABLES -t nat -A POSTROUTING" . 
		 " -s $JAILCTRLNET/$JAILCTRLNETMASK" . 
		 " -d $PUBCTRLNET/$PUBCTRLNETMASK -j ACCEPT");
	mysystem("$IPTABLES -t nat -A POSTROUTING" . 
		 " -s $JAILCTRLNET/$JAILCTRLNETMASK" . 
		 " -d $JAILCTRLNET/$JAILCTRLNETMASK -j ACCEPT");
	mysystem("$IPTABLES -t nat -A POSTROUTING" . 
		 " -s $JAILCTRLNET/$JAILCTRLNETMASK" . 
		 " -o $iface -j SNAT --to-source $ip");
    }

    # XXX only needed for fake mac hack, which should go away someday
    mysystem("echo 1 > /proc/sys/net/ipv4/conf/$iface/proxy_arp");

    # Ug, pre-create a bunch of imq devices, since adding news ones
    # does not work right yet.
    mysystem("$MODPROBE imq numdevs=$MAXIMQ");
    mysystem("$MODPROBE ipt_IMQ");

    # Create a DB to manage them.
    my %MDB;
    if (!dbmopen(%MDB, $IMQDB, 0660)) {
	print STDERR "*** Could not create $IMQDB\n";
	return -1;
    }
    for (my $i = 0; $i < $MAXIMQ; $i++) {
	$MDB{"$i"} = ""
	    if (!exists($MDB{"$i"}));
    }
    dbmclose(%MDB);

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
    if (TBScriptLock("vzconf", 0, 90) != TBSCRIPTLOCK_OKAY()) {
	print STDERR "Could not get the vznetwork lock after a long time!\n";
	return -1;
    }
    # Do this again after lock.
    makeIfaceMaps();
    makeBridgeMaps();
    
    my ($node_ifs,$node_ifsets,$node_lds) = @_;

    # figure out what bridges we need to make:
    # we need a bridge for each physical iface that is a multiplex pipe,
    # and one for each VTAG given PMAC=none (i.e., host containing both sides
    # of a link, or an entire lan).
    my %brs = ();
    foreach my $node (keys(%$node_ifs)) {
	foreach my $ifc (@{$node_ifs->{$node}}) {
	    next if (!$ifc->{ISVIRT});

	    if ($ifc->{PMAC} eq "none") {
		my $brname = "br" . $ifc->{VTAG};
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
		my $brname = "pbr$iface";
		$brs{$brname}{ENCAP} = 1;
		$brs{$brname}{SHORT} = 0;
		$brs{$brname}{PHYSDEV} = $iface;
	    }
	}
    }

    # actually make bridges and add phys ifaces
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

	# building bridges is an important activity
	if (system("brctl show | grep -q \'^$k\'")) {
	    mysystem("$BRCTL addbr $k");
	}
	# repetitions of this should not hurt anything
	mysystem("$IFCONFIG $k 0 up");

	# XXX here we would normally config the bridge to encapsulate or
	# act in short mode

	if (exists($brs{$k}{PHYSDEV})) {
	    # make sure this iface isn't already part of another bridge; if it
	    # it is, remove it from there first and add to this bridge.
	    if (exists($if2br{$brs{$k}{PHYSDEV}})) {
		mysystem("$BRCTL delif " . $if2br{$brs{$k}{PHYSDEV}} . 
			 " " .$brs{$k}{PHYSDEV});
		# rebuild hashes
		makeBridgeMaps();
	    }
	    mysystem("$BRCTL addif $k $brs{$k}{PHYSDEV}");
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
		    TBScriptUnLock();
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
    my ($vnode_id,$image) = @_;

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

    # build the container
    mysystem("$VZCTL create $vmid --ostemplate $image");

    # make sure bootvnodes actually starts things up on boot, not openvz
    mysystem("$VZCTL set $vmid --onboot no --name $vnode_id --save");

    return $vmid;
}

sub vz_vnodeDestroy {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL destroy $vnode_id");
    return -1
	if ($?);

    #
    # Clear the IMQ reservations. Must lock since IMQDB is a shared
    # resource.
    #
    if (TBScriptLock("vzconf", 0, 90) != TBSCRIPTLOCK_OKAY()) {
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

    my $status = vmstatus($vmid);
    if ($status eq 'running') {
	return VNODE_STATUS_RUNNING();
    }
    elsif ($status eq 'stopped') {
	return VNODE_STATUS_STOPPED();
    }

    return VNODE_STATUS_UNKNOWN();
}

sub vz_vnodeBoot {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL start $vnode_id");

    return 0;
}

sub vz_vnodeHalt {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL stop $vnode_id");

    return 0;
}

sub vz_vnodeReboot {
    my ($vnode_id,$vmid) = @_;

    mysystem("$VZCTL restart $vnode_id");

    return 0;
}

sub vz_vnodePreConfig {
    my ($vnode_id,$vmid,$callback) = @_;

    #
    # Look and see if this node already has imq devs mapped into it -- if
    # those match the ones in the IMQDB, do nothing, else fixup. Must lock
    # since IMQDB is a shared resource.
    #
    if (TBScriptLock("vzconf", 0, 90) != TBSCRIPTLOCK_OKAY()) {
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

	$devs{"$i"} = 1;
    }
    dbmclose(%MDB);
    TBScriptUnlock();
    
    my $existing = `sed -n -r -e 's/NETDEV="(.*)"/\1/p' /etc/vz/conf/$vmid.conf`;
    chomp($existing);
    foreach my $dev (split(/,/,$existing)) {
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
	    mysystem("$VZCTL set $vnode_id --netdev_add $dev --save");
	}
	elsif ($devs{$dev} == 0) {
	    mysystem("$VZCTL set $vnode_id --netdev_del $dev --save");
	}
    }
    #
    # Make sure container is mounted before calling the callback.
    #
    my $status = vmstatus($vmid);
    my $didmount = 0;
    if ($status ne 'running' && $status ne 'mounted') {
	mysystem("$VZCTL mount $vnode_id");
    }
    my $ret = &$callback("$VZROOT/$vmid");
    if ($didmount) {
	mysystem("$VZCTL umount $vnode_id");
    }
    return $ret;
}

#
# Preconfigure the control net interface; special case of vnodeConfigInterfaces.
#
sub vz_vnodePreConfigControlNetwork {
    my ($vnode_id,$vmid,$ip,$mask,$mac,$gw,
	$vname,$longdomain,$shortdomain,$bossip) = @_;

    my $myroot = "$VZROOT/$vmid";

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
    mysystem("$VZCTL set $vnode_id" . 
	     " --netif_add ${CONTROL_IFDEV},$cnet_mac,$cnet_veth,$ext_vethmac --save");

    #
    # Make sure container is mounted
    #
    my $status = vmstatus($vmid);
    my $didmount = 0;
    if ($status ne 'running' && $status ne 'mounted') {
	mysystem("$VZCTL mount $vnode_id");
    }

    #
    # Setup lo
    #
    open(FD,">$myroot/etc/sysconfig/network-scripts/ifcfg-lo") 
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
    unlink("$myroot/etc/sysconfig/network-scripts/ifcfg-eth99");

    #
    # setup the control net iface in the FS ...
    #
    open(FD,">$myroot/etc/sysconfig/network-scripts/ifcfg-${CONTROL_IFDEV}") 
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
    my ($ctrliface,$ctrlip,$ctrlmac) = findControlNet();
    open(FD,">$myroot/etc/sysconfig/network-scripts/route-${CONTROL_IFDEV}") 
	or die "vz_vnodePreConfigControlNetwork: could not open route-${CONTROL_IFDEV} for $vnode_id: $!";
    #
    # HUGE NOTE: we *have* to use the /<bits> form, not the /<netmask> form
    # for now, since our iproute version is old.
    #
    print FD "$PUBCTRLNET/$PUBCTRLNETMASKBITS dev ${CONTROL_IFDEV}\n";
    print FD "0.0.0.0/0 via $ctrlip\n";
    close(FD);

    #
    # ... and make sure it gets brought up on boot:
    # XXX: yes, this would blow away anybody's changes, but don't care now.
    #
    open(FD,">$myroot/etc/sysconfig/network") 
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
    my $mybootdir = "$myroot/var/emulab/boot/";

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
    print FD "$vnode_id\n";
    close(FD);

    #
    # Let's not hang ourselves before we start
    #
    open(FD,">$myroot/etc/resolv.conf") 
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
	mysystem("$VZCTL umount $vnode_id");
    }

    return 0;
}

#
# Preconfigures experimental interfaces in the vnode before its first boot.
#
sub vz_vnodePreConfigExpNetwork {
    my ($vnode_id,$vmid,$ifs,$lds) = @_;

    my $elabifs = "";
    my %netif_strs = ();
    foreach my $ifc (@$ifs) {
	next if (!$ifc->{ISVIRT});

	#
	# Add to ELABIFS for addition to conf file (for runtime config by 
        # external custom script)
	#
	my $veth = "veth$vmid.$ifc->{ID}";
	my $br;
	if ($ifc->{PMAC} eq "none") {
	    $br = "br" . $ifc->{VTAG};
	}
	else {
	    my $iface = findIface($ifc->{PMAC});
	    $br = "pbr$iface";
	}
	if ($elabifs ne '') {
	    $elabifs .= ';';
	}
	$elabifs .= "$veth,$br";

	#
	# The ethX naming sucks, but hopefully it ensures unique, *easily 
	# reconfigurable* (i.e., without a local map file) naming for veths.
	#
	my $eth = "eth" . $ifc->{VTAG};
	my $ethmac = macAddSep($ifc->{VMAC});
	my $vethmac = $ethmac;
	if ($vethmac =~ /^(00:00)(.*)$/) {
	    $vethmac = "00:01$2";
	}

	#
	# Savefor later calling, since we need to hack the 
	# config file BEFORE calling --netif_add so the custom postconfig 
	# script does the right thing.
	# Also store up the current set of netifs so we can delete any that
	# might have been old!
	#
        $netif_strs{$eth} = "$eth,$ethmac,$veth,$vethmac";
    }

    #
    # Wait until end to do a single edit for all ifs, since they're all 
    # smashed into a single config file var
    #
    my %lines = ( 'ELABIFS' => '"' . $elabifs . '"');
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
	    mysystem("$VZCTL set $vnode_id --netif_del $eth --save");
	}
    }
    # add/modify
    foreach my $eth (keys(%netif_strs)) {
	mysystem("$VZCTL set $vnode_id --netif_add $netif_strs{$eth} --save");
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
		$dedlines{$k} = undef;
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

#
# Find control net iface info.  Returns (iface,ip,mac) tuple.
#
sub findControlNet() {
    my $ip = `cat $CTRLIPFILE`;
    chomp($ip);
    if ($ip =~ /^(\d+\.\d+\.\d+\.\d+)$/) {
	$ip = $1;
    }
    else {
	# can't/shouldn't really go on if this happens
	die "could not find valid ip in $CTRLIPFILE!";
    }

    return ($ip2if{$ip},$ip,$if2mac{$ip2if{$ip}});
}

#
# Grab iface, mac, IP info from /sys and /sbin/ip.
#
sub makeIfaceMaps() {
    # clean out anything
    %if2mac = ();
    %mac2if = ();
    %ip2if = ();

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
	if ($pip =~ /^\s+inet\s+(\d+\.\d+\.\d+\.\d+)/) {
	    $ip2if{$1} = $iface;
	}
    }

    if (1 && $debug) {
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

    if (1 && $debug) {
	print STDERR "makeBridgeMaps:\n";
	print STDERR "bridges:\n";
	print STDERR Dumper(%bridges) . "\n";
	print STDERR "if2br:\n";
	print STDERR Dumper(%if2br) . "\n";
	print STDERR "\n";
    }

    return 0;
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

# what can I say?
1;
