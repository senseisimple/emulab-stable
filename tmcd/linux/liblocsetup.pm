#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Linux specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $NFSMOUNT $UMOUNT $TMPASSWD $SFSSD $SFSCD $RPMCMD
	 $HOSTSFILE $LOOPBACKMOUNT $TMGROUP $TMSHADOW $TMGSHADOW
	 os_account_cleanup os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_ifconfig_veth os_viface_name os_modpasswd
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual os_homedirdel
	 os_groupdel os_getnfsmounts os_islocaldir
	 os_fwconfig_line os_fwrouteconfig_line os_config_gre
	 os_get_disks os_get_disk_size os_get_partition_info os_nfsmount
       );

# Must come after package declaration!
use English;
use Fcntl;

# Load up the paths. Its conditionalized to be compatabile with older images.
# Note this file has probably already been loaded by the caller.
BEGIN
{
    if (-e "/etc/emulab/paths.pm") {
	require "/etc/emulab/paths.pm";
	import emulabpaths;
    }
    else {
	my $ETCDIR  = "/etc/rc.d/testbed";
	my $BINDIR  = "/etc/rc.d/testbed";
	my $VARDIR  = "/etc/rc.d/testbed";
	my $BOOTDIR = "/etc/rc.d/testbed";
    }
}

# Convenience.
sub REMOTE()	{ return libsetup::REMOTE(); }
sub REMOTEDED()	{ return libsetup::REMOTEDED(); }
sub PLAB()	{ return libsetup::PLAB(); }
sub LINUXJAILED()  { return libsetup::LINUXJAILED(); }
sub GENVNODE()     { return libsetup::GENVNODE(); }
sub GENVNODETYPE() { return libsetup::GENVNODETYPE(); }

#
# Various programs and things specific to Linux and that we want to export.
# 
$CP		= "/bin/cp";
$DF		= "/bin/df";
$EGREP		= "/bin/egrep -q";
# Note that we try multiple versions in os_nfsmount below; this is for legacy
# code, or code where the mount is best done in the caller itself... or code
# I didn't want to convert!
$NFSMOUNT	= "/bin/mount -o vers=2,udp"; # Force NFS Version 2 over UDP
$LOOPBACKMOUNT	= "/bin/mount -n -o bind ";
$UMOUNT		= "/bin/umount";
$TMPASSWD	= "$ETCDIR/passwd";
$TMGROUP	= "$ETCDIR/group";
$TMSHADOW       = "$ETCDIR/shadow";
$TMGSHADOW      = "$ETCDIR/gshadow";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";
$RPMCMD		= "/bin/rpm";
$HOSTSFILE	= "/etc/hosts";
$WGET		= "/usr/bin/wget";

#
# These are not exported
#
my $TMGROUP	= "$ETCDIR/group";
my $TMSHADOW    = "$ETCDIR/shadow";
my $TMGSHADOW   = "$ETCDIR/gshadow";
my $USERADD     = "/usr/sbin/useradd";
my $USERDEL     = "/usr/sbin/userdel";
my $USERMOD     = "/usr/sbin/usermod";
my $GROUPADD	= "/usr/sbin/groupadd";
my $GROUPDEL	= "/usr/sbin/groupdel";
my $IFCONFIGBIN = "/sbin/ifconfig";
my $IFCONFIG    = "$IFCONFIGBIN %s inet %s netmask %s";
my $VLANCONFIG  = "/sbin/vconfig";
my $IFC_1000MBS  = "1000baseTx";
my $IFC_100MBS  = "100baseTx";
my $IFC_10MBS   = "10baseT";
my $IFC_FDUPLEX = "FD";
my $IFC_HDUPLEX = "HD";
my @LOCKFILES   = ("/etc/group.lock", "/etc/gshadow.lock");
my $MKDIR	= "/bin/mkdir";
my $GATED	= "/usr/sbin/gated";
my $ROUTE	= "/sbin/route";
my $SHELLS	= "/etc/shells";
my $DEFSHELL	= "/bin/tcsh";
my $IWCONFIG    = '/usr/local/sbin/iwconfig';
my $WLANCONFIG  = '/usr/local/bin/wlanconfig';
my $RMMOD       = '/sbin/rmmod';
my $MODPROBE    = '/sbin/modprobe';
my $IWPRIV      = '/usr/local/sbin/iwpriv';
my $BRCTL       = "/usr/sbin/brctl";

my $PASSDB   = "$VARDIR/db/passdb";
my $GROUPDB  = "$VARDIR/db/groupdb";
my $SYSETCDIR = "/etc";

my $debug = 0;

#
# OS dependent part of cleanup node state.
# 
sub os_account_cleanup()
{
    #
    # Don't just splat the master passwd/group files into place from $ETCDIR.
    # Instead, grab the current Emulab uids/gids, grab the current group/passwd
    # files and their shadow counterparts, remove any emulab u/gids from the 
    # loaded instance of the current files, then push any new/changed uid/gids
    # into the master files in $ETCDIR.  Also, we remove accounts from the
    # master files if they no longer appear in the current files.  Finally, we
    # strip deleted uids from any groups they might appear in (!).
    #
    my %PDB;
    my %GDB;

    dbmopen(%PDB, $PASSDB, undef) or
	fatal("Cannot open $PASSDB: $!");
    dbmopen(%GDB, $GROUPDB, undef) or
	fatal("Cannot open $GROUPDB: $!");

    if ($debug) {
	use Data::Dumper;
	print Dumper(%PDB) . "\n\n";
	print Dumper(%GDB) . "\n\n";
    }

    my %lineHash = ();
    my %lineList = ();

    foreach my $file ("$SYSETCDIR/passwd","$SYSETCDIR/group",
		      "$SYSETCDIR/shadow","$SYSETCDIR/gshadow",
		      "$ETCDIR/passwd","$ETCDIR/group",
		      "$ETCDIR/shadow","$ETCDIR/gshadow") {
	open(FD,$file)
	    or die "open($file): $!";
	my $i = 0;
	my $lineCounter = 1;
	while (my $line = <FD>) {
	    chomp($line);

	    # store the line in the list
	    if (!defined($lineList{$file})) {
		$lineList{$file} = [];
	    }
	    $lineList{$file}->[$i] = $line;

	    # fill the hash for fast lookups, and place the array idx of the
	    # element in the lineList as the hash value so that we can undef
	    # it if deleting it, while still preserving the line orderings in
	    # the original files.  whoo!
	    if ($line ne '' && $line =~ /^([^:]+):.*$/) {
		if (!defined($lineHash{$file})) {
		    $lineHash{$file} = {};
		}
		$lineHash{$file}->{$1} = $i++;
	    }
	    else {
		print STDERR "malformed line $lineCounter in $file, ignoring!\n";
	    }
	    ++$lineCounter;
	}
    }

    print Dumper(%lineHash) . "\n\n\n"
	if ($debug);

    # remove emulab groups first (save a bit of work):
    while (my ($group,$gid) = each(%GDB)) {
	print "DEBUG: $group/$gid\n";
	foreach my $file ("$SYSETCDIR/group","$SYSETCDIR/gshadow") {
	    if (defined($lineHash{$file}->{$group})) {
		# undef its line
		$lineList{$file}->[$lineHash{$file}->{$group}] = undef;
		delete $lineHash{$file}->{$group};
		print "DEBUG: deleted group $group from $file\n"
		    if ($debug);
	    }
	}
    }

    # now remove emulab users from users files, AND from the group list
    # in any groups :-)
    while (my ($user,$uid) = each(%PDB)) {
	foreach my $file ("$SYSETCDIR/passwd","$SYSETCDIR/shadow") {
	    if (defined($lineHash{$file}->{$user})) {
		# undef its line
		$lineList{$file}->[$lineHash{$file}->{$user}] = undef;
		delete $lineHash{$file}->{$user};
		print "DEBUG: deleted user  $user from $file\n"
		    if ($debug);
	    }
	}

	# this is indeed a lot of extra text processing, but whatever
	foreach my $file ("$SYSETCDIR/group","$SYSETCDIR/gshadow") {
	    foreach my $group (keys(%{$lineHash{$file}})) {
		my $groupLine = $lineList{$file}->[$lineHash{$file}->{$group}];
		# grab the fields
		# split using -1 to make sure our empty trailing fields are
		# added!
		my @elms = split(/\s*:\s*/,$groupLine,-1);
		# grab the user list
		my @ulist = split(/\s*,\s*/,$elms[scalar(@elms)-1]);
		# build a new list
		my @newulist = ();
		my $j = 0;
		my $k = 0;
		for ($j = 0; $j < scalar(@ulist); ++$j) {
		    # only add to the new user list if it's not the user we're
		    # removing.
		    my $suser = $ulist[$j];
		    if ($suser ne $user) {
			$newulist[$k++] = $suser;
		    }
		}
		# rebuild the user list
		$elms[scalar(@elms)-1] = join(',',@newulist);
		# rebuild the line from the fields
		$groupLine = join(':',@elms);
		# stick the line back into the "file"
		$lineList{$file}->[$lineHash{$file}->{$group}] = $groupLine;
	    }
	}
    }

    # now, merge current files into masters.
    foreach my $pairRef (["$SYSETCDIR/passwd","$ETCDIR/passwd"],
			 ["$SYSETCDIR/group","$ETCDIR/group"],
			 ["$SYSETCDIR/shadow","$ETCDIR/shadow"],
			 ["$SYSETCDIR/gshadow","$ETCDIR/gshadow"]) {
	my ($real,$master) = @$pairRef;

	foreach my $ent (keys(%{$lineHash{$real}})) {
	    # skip root and toor!
	    next 
		if ($ent eq 'root' || $ent eq 'toor');

	    # push new entities into master
	    if (!defined($lineHash{$master}->{$ent})) {
		# append new "line"
		$lineHash{$master}->{$ent} = scalar(@{$lineList{$master}});
		$lineList{$master}->[$lineHash{$master}->{$ent}] = 
		    $lineList{$real}->[$lineHash{$real}->{$ent}];
		print "DEBUG: adding $ent to $master\n"
		    if ($debug);
	    }
	    # or replace modified entities
	    elsif ($lineList{$real}->[$lineHash{$real}->{$ent}] 
		   ne $lineList{$master}->[$lineHash{$master}->{$ent}]) {
		$lineList{$master}->[$lineHash{$master}->{$ent}] = 
		    $lineList{$real}->[$lineHash{$real}->{$ent}];
		print "DEBUG: updating $ent in $master\n"
		    if ($debug);
	    }
	}

	# now remove stale lines from the master
	my @todelete = ();
	foreach my $ent (keys(%{$lineHash{$master}})) {
	    if (!defined($lineHash{$real}->{$ent})) {
		# undef its line
		$lineList{$master}->[$lineHash{$master}->{$ent}] = undef;
		push @todelete, $ent;
	    }
	}
	foreach my $delent (@todelete) {
	    delete $lineHash{$master}->{$delent};
	}
    }

    # now write the masters to .new files so we can diff, do the diff for 
    # files that are world-readable, then mv into place over the masters.
    my %modes = ( "$ETCDIR/passwd" => 0644,"$ETCDIR/group" => 0644,
		  "$ETCDIR/shadow" => 0600,"$ETCDIR/gshadow" => 0600 );
    foreach my $file (keys(%modes)) {
	sysopen(FD,"${file}.new",O_CREAT | O_WRONLY | O_TRUNC,$modes{$file})
	    # safe to die here cause we haven't moved any .new files into
	    # place
	    or die "sysopen(${file}.new): $!";
	for (my $i = 0; $i < scalar(@{$lineList{$file}}); ++$i) {
	    # remember, some lines may be undef cause we deleted them
	    if (defined($lineList{$file}->[$i])) {
		print FD $lineList{$file}->[$i] . "\n";
	    }
	}
	close(FD);
    }
    foreach my $file (keys(%modes)) {
	my $retval;
	if ($modes{$file} == 0644) {
	    print STDERR "Running 'diff -u $file ${file}.new'\n";
	    $retval = system("diff -u $file ${file}.new");
	    if ($retval) {
		print STDERR "Files ${file}.new and $file differ; updating $file.\n";
		system("mv ${file}.new $file");
	    }
	}
	else {
	    print STDERR "Running 'diff -q -u $file ${file}.new'\n";
	    $retval = system("diff -q -u $file ${file}.new");
	    if ($retval) {
		print STDERR "Files ${file}.new and $file differ, but we can't show the changes!  Updating $file anyway!\n";
		system("mv ${file}.new $file");
	    }
	}
    }

    dbmclose(%PDB);
    dbmclose(%GDB);

    #
    # Splat the new files into place, doh
    #
    unlink @LOCKFILES;

    printf STDOUT "Resetting passwd and group files\n";
    if (system("$CP -f $TMGROUP $TMPASSWD /etc") != 0) {
        print STDERR "Could not copy default group file into place: $!\n";
        return -1;
    }

    if (system("$CP -f $TMSHADOW $TMGSHADOW /etc") != 0) {
        print STDERR "Could not copy default passwd file into place: $!\n";
        return -1;
    }

    return 0;
}

#
# Generate and return an ifconfig line that is approriate for putting
# into a shell script (invoked at bootup).
#
sub os_ifconfig_line($$$$$$$$;$$$)
{
    my ($iface, $inet, $mask, $speed, $duplex, $aliases, $iface_type, $lan,
	$settings, $rtabid, $cookie) = @_;
    my ($miirest, $miisleep, $miisetspd, $media);
    my ($uplines, $downlines);

    #
    # Special handling for new style interfaces (which have settings).
    # This should all move into per-type modules at some point. 
    #
    if ($iface_type eq "ath" && defined($settings)) {

        # Get a handle on the "VAP" interface we will create when
        # setting up this interface.
        my ($ifnum) = $iface =~ /wifi(\d+)/;
        my $athiface = "ath" . $ifnum;

	#
	# Setting the protocol is special and appears to be card specific.
	# How stupid is that!
	#
	my $protocol = $settings->{"protocol"};
        my $privcmd = "/usr/local/sbin/iwpriv $athiface mode ";

        SWITCH1: for ($protocol) {
          /^80211a$/ && do {
              $privcmd .= "1";
              last SWITCH1;
          };
          /^80211b$/ && do {
              $privcmd .= "2";
              last SWITCH1;
          };
          /^80211g$/ && do {
              $privcmd .= "3";
              last SWITCH1;
          };
        }
	 
	#
	# At the moment, we expect just the various flavors of 80211, and
	# we treat them all the same, configuring with iwconfig and iwpriv.
	#
	my $iwcmd = "/usr/local/sbin/iwconfig $athiface ";
        my $wlccmd = "/usr/local/bin/wlanconfig $athiface create ".
            "wlandev $iface ";

	#
	# We demand to be given an ssid.
	#
	if (!exists($settings->{"ssid"})) {
	    warn("*** WARNING: No SSID provided for $iface!\n");
	    return undef;
	}
	$iwcmd .= "essid ". $settings->{"ssid"};

	# If we do not get a channel, pick one.
	if (exists($settings->{"channel"})) {
	    $iwcmd .= " channel " . $settings->{"channel"};
	}
	else {
	    $iwcmd .= " channel 3";
	}

	# txpower and rate default to auto if not specified.
	if (exists($settings->{"rate"})) {
	    $iwcmd .= " rate " . $settings->{"rate"};
	}
	else {
	    $iwcmd .= " rate auto";
	}
	if (exists($settings->{"txpower"})) {
	    $iwcmd .= " txpower " . $settings->{"txpower"};
	}
	else {
	    $iwcmd .= " txpower auto";
	}
	# Allow this too. 
	if (exists($settings->{"sens"})) {
	    $iwcmd .= " sens " . $settings->{"sens"};
	}

	# allow rts threshold and frag size
	if (exists($settings->{'rts'})) {
	    $iwcmd .= ' rts ' . $settings->{'rts'};
	}
	if (exists($settings->{'frag'})) {
	    $iwcmd .= ' frag ' . $settings->{'frag'};
	}

	#
	# We demand to be told if we are the master or a peon.
	# We might also be in another mode.  Thus, if accesspoint is specified,
	# we assume we are in either ap/sta (Master/Managed) mode.  If not,
	# we look for a 'mode' argument and assume adhoc if we don't get one.
	# The reason to assume adhoc is because we need accesspoint set to
	# know how to configure the device for ap/sta modes, and setting a
	# device to monitor mode by default sucks.
	# 
	# This needs to be last for some reason.
	#
	if (exists($settings->{'accesspoint'})) {
	    my $accesspoint = $settings->{"accesspoint"};
	    my $accesspointwdots;
	    
	    # Allow either dotted or undotted notation!
	    if ($accesspoint =~ /^(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})$/) {
		$accesspointwdots = "$1:$2:$3:$4:$5:$6";
	    }
	    elsif ($accesspoint =~
		   /^(\w{2}):(\w{2}):(\w{2}):(\w{2}):(\w{2}):(\w{2})$/) {
		$accesspointwdots = $accesspoint;
		$accesspoint      = "${1}${2}${3}${4}${5}${6}";
	    }
	    else {
		warn("*** WARNING: Improper format for MAC ($accesspoint) ".
		     "provided for $iface!\n");
		return undef;
	    }
	    
	    if (libsetup::findiface($accesspoint) eq $iface) {
		$wlccmd .= " wlanmode ap";
		$iwcmd .= " mode Master";
	    }
	    else {
		$wlccmd .= " wlanmode sta";
		$iwcmd .= " mode Managed ap $accesspointwdots";
	    }
	}
	elsif (exists($settings->{'mode'})) {
	    if ($settings->{'mode'} =~ /ad[\s\-]*hoc/i) {
		$wlccmd .= " wlanmode adhoc";
		$iwcmd .= " mode Ad-Hoc";
	    }
	    elsif ($settings->{'mode'} =~ /monitor/i) {
		$wlccmd .= " wlanmode monitor";
		$iwcmd .= " mode Monitor";
	    }
	    elsif ($settings->{'mode'} =~ /ap/i 
		   || $settings->{'mode'} =~ /access[\s\-]*point/i 
		   || $settings->{'mode'} =~ /master/i) {
		$wlccmd .= " wlanmode ap";
		$iwcmd .= " mode Master";
	    }
	    elsif ($settings->{'mode'} =~ /sta/i 
		   || $settings->{'mode'} =~ /managed/i) {
		$wlccmd .= " wlanmode sta";
		$iwcmd .= " mode Managed ap any";
	    }
	    else {
		warn("*** WARNING: Invalid mode provided for $iface!\n");
		return undef;
	    }
	}
	else {
	    warn("*** WARNING: No mode implied for $iface!\n");
	    return undef;
	}

        $uplines   = $wlccmd . "\n";
	$uplines  .= $privcmd . "\n";
	$uplines  .= $iwcmd . "\n";
	$uplines  .= sprintf($IFCONFIG, $athiface, $inet, $mask) . "\n";
	$downlines  = "$IFCONFIGBIN $athiface down\n";
	$downlines .= "$WLANCONFIG $athiface destroy\n";
	$downlines .= "$IFCONFIGBIN $iface down\n";
	return ($uplines, $downlines);
    }

    #
    # GNU Radio network interface on the flex900 daugherboard
    #
    if ($iface_type eq "flex900" && defined($settings)) {

        my $tuncmd = 
            "/bin/env PYTHONPATH=/usr/local/lib/python2.4/site-packages ".
            "$BINDIR/tunnel.py";

        if (!exists($settings->{"mac"})) {
            warn("*** WARNING: No mac address provided for gnuradio ".
                 "interface!\n");
            return undef;
        }

        my $mac = $settings->{"mac"};

        if (!exists($settings->{"protocol"}) || 
            $settings->{"protocol"} ne "flex900") {
            warn("*** WARNING: Unknown gnuradio protocol specified!\n");
            return undef;
        }

        if (!exists($settings->{"frequency"})) {
            warn("*** WARNING: No frequency specified for gnuradio ".
                 "interface!\n");
            return undef;
        }

        my $frequency = $settings->{"frequency"};
        $tuncmd .= " -f $frequency";

        if (!exists($settings->{"rate"})) {
            warn("*** WARNING: No rate specified for gnuradio interface!\n");
            return undef;
        }

        my $rate = $settings->{"rate"};
        $tuncmd .= " -r $rate";

	if (exists($settings->{'carrierthresh'})) {
	    $tuncmd .= " -c " . $settings->{'carrierthresh'};
	}
	if (exists($settings->{'rxgain'})) {
	    $tuncmd .= " --rx-gain=" . $settings->{'rxgain'};
	}

        $uplines = $tuncmd . " > /dev/null 2>&1 &\n";
        $uplines .= "sleep 5\n";
        $uplines .= "$IFCONFIGBIN $iface hw ether $mac\n";
        $uplines .= sprintf($IFCONFIG, $iface, $inet, $mask) . "\n";
        $downlines = "$IFCONFIGBIN $iface down";
        return ($uplines, $downlines);
    }

    #
    # Only do this stuff if we have a physical interface, otherwise it doesn't
    # mean anything.  We need this for virtnodes whose networks must be 
    # config'd from inside the container, vm, whatever.
    #
    if ($iface_type ne 'veth') {
        #
        # Need to check units on the speed. Just in case.
        #
	if ($speed =~ /(\d*)([A-Za-z]*)/) {
	    if ($2 eq "Mbps") {
		$speed = $1;
	    }
	    elsif ($2 eq "Kbps") {
		$speed = $1 / 1000;
	    }
	    else {
		warn("*** Bad speed units $2 in ifconfig, default to 100Mbps\n");
		$speed = 100;
	    }
	    if ($speed == 1000) {
		$media = $IFC_1000MBS;
	    }
	    elsif ($speed == 100) {
		$media = $IFC_100MBS;
	    }
	    elsif ($speed == 10) {
		$media = $IFC_10MBS;
	    }
	    else {
		warn("*** Bad Speed $speed in ifconfig, default to 100Mbps\n");
		$speed = 100;
		$media = $IFC_100MBS;
	    }
	}
	if ($duplex eq "full") {
	    $media = "$media-$IFC_FDUPLEX";
	}
	elsif ($duplex eq "half") {
	    $media = "$media-$IFC_HDUPLEX";
	}
	else {
	    warn("*** Bad duplex $duplex in ifconfig, default to full\n");
	    $duplex = "full";
	    $media = "$media-$IFC_FDUPLEX";
	}

        #
        # Linux is apparently changing from mii-tool to ethtool but some drivers
        # don't support the new interface (3c59x), some don't support the old
        # interface (e1000), and some (eepro100) support the new interface just
        # enough that they can report success but not actually do anything. Sweet!
        #
	my $ethtool;
	if (-e "/sbin/ethtool") {
	    $ethtool = "/sbin/ethtool";
	} elsif (-e "/usr/sbin/ethtool") {
	    $ethtool = "/usr/sbin/ethtool";
	}
	if (defined($ethtool)) {
	    # this seems to work for returning an error on eepro100
	    $uplines = 
		"if $ethtool $iface >/dev/null 2>&1; then\n    " .
		"  $ethtool -s $iface autoneg off speed $speed duplex $duplex\n    " .
		"  sleep 2 # needed due to likely bug in e100 driver on pc850s\n".
		"else\n    " .
		"  /sbin/mii-tool --force=$media $iface\n    " .
		"fi\n    ";
	} else {
	    $uplines = "/sbin/mii-tool --force=$media $iface\n    ";
	}
    }

    if ($inet eq "") {
	$uplines .= "$IFCONFIGBIN $iface up";
    }
    else {
	$uplines  .= sprintf($IFCONFIG, $iface, $inet, $mask);
	$downlines = "$IFCONFIGBIN $iface down";
    }
    
    return ($uplines, $downlines);
}

#
# Specialized function for configing virtual ethernet devices:
#
#	'veth'	one end of an etun device embedded in a vserver
#	'vlan'	802.1q tagged vlan devices
#	'alias'	IP aliases on physical interfaces
#
sub os_ifconfig_veth($$$$$;$$$$%)
{
    my ($iface, $inet, $mask, $id, $vmac,
	$rtabid, $encap, $vtag, $itype, $cookie) = @_;
    my ($uplines, $downlines);

    if ($itype !~ /^(alias|vlan|veth)$/) {
	warn("Unknown virtual interface type $itype\n");
	return "";
    }

    #
    # Veth.
    #
    # Veths for Linux vservers mean vtun devices.  One end is outside
    # the vserver and is bridged with other veths and peths as appropriate
    # to form the topology.  The other end goes in the vserver and is
    # configured with an IP address.  This final step is not done here
    # as the vserver must be running first.
    #
    # In the current configuration, there is configuration that takes
    # place both inside and outside the vserver.
    #
    # Inside:
    # The inside case (LINUXJAILED() == 1) just configures the IP info on
    # the interface.
    #
    # Outside:
    # The outside actions are much more involved as described above.
    # The VTAG identifies a bridge device "ebrN" to be used.
    # The RTABID identifies the namespace, but we don't care here.
    #
    # To create a etun pair you do:
    #    echo etun0,etun1 > /sys/module/etun/parameters/newif
    # To destroy do:
    #    echo etun0 > /sys/module/etun/parameters/delif
    #
    if ($itype eq "veth") {
	#
	# We are inside a Linux jail.
	# We configure the interface pretty much like normal.
	#
	if (LINUXJAILED()) {
	    if ($inet eq "") {
		$uplines .= "$IFCONFIGBIN $iface up";
	    }
	    else {
		$uplines  .= sprintf($IFCONFIG, $iface, $inet, $mask);
		$downlines = "$IFCONFIGBIN $iface down";
	    }
	    
	    return ($uplines, $downlines);
	}

	#
	# Outside jail.
	# Create tunnels and bridge and plumb them all together.
	#
	my $brdev = "ebr$vtag";
	my $iniface = "veth$id";
	my $outiface = "peth$id";
	my $devdir = "/sys/module/etun/parameters";

	# UP
	$uplines = "";

	# modprobe (should be done already for cnet setup, but who cares)
	$uplines .= "modprobe etun\n";

	# make sure bridge device exists and is up
	$uplines .= "    $IFCONFIGBIN $brdev >/dev/null 2>&1 || {";
	$uplines .= "        $BRCTL addbr $brdev\n";
	$uplines .= "        $IFCONFIGBIN $brdev up\n";
	$uplines .= "    }\n";

	# create the tunnel device
	$uplines .= "    echo $outiface,$iniface > $devdir/newif || exit 1\n";

	# bring up outside IF, insert into bridge device
	$uplines .= "    $IFCONFIGBIN $outiface up || exit 2\n";
	$uplines .= "    $BRCTL addif $brdev $outiface || exit 3\n";

	# configure the MAC address for the inside device
	$uplines .= "    $IFCONFIGBIN $iniface hw ether $vmac || exit 4\n";

	# DOWN
	$downlines = "";

	# remove IF from bridge device, down it (remove bridge if empty?)
	$downlines .= "$BRCTL delif $brdev $outiface || exit 13\n";
	$downlines .= "    $IFCONFIGBIN $outiface down || exit 12\n";

	# destroy tunnel devices (this will fail if inside IF in vserver still)
	$downlines .= "    echo $iniface > $devdir/delif || exit 11\n";

	return ($uplines, $downlines);
    }

    #
    # IP aliases
    #
    if ($itype eq "alias") {
	my $aif = "$iface:$id";

	$uplines   = sprintf($IFCONFIG, $aif, $inet, $mask);
	$downlines = "$IFCONFIGBIN $aif down";

	return ($uplines, $downlines);
    }

    #
    # VLANs
    #   insmod 8021q (once only)
    #   vconfig set_name_type VLAN_PLUS_VID_NO_PAD (once only)
    #
    #	ifconfig eth0 up (should be done before we are ever called)
    #	vconfig add eth0 601 
    #   ifconfig vlan601 inet ...
    #
    #   ifconfig vlan601 down
    #	vconfig rem vlan601
    #
    if ($itype eq "vlan") {
	if (!defined($vtag)) {
	    warn("No vtag in veth config\n");
	    return "";
	}

	# one time stuff
	if (!exists($cookie->{"vlan"})) {
	    $uplines  = "/sbin/insmod 8021q >/dev/null 2>&1\n    ";
	    $uplines .= "$VLANCONFIG set_name_type VLAN_PLUS_VID_NO_PAD\n    ";
	    $cookie->{"vlan"} = 1;
	}

	my $vdev = "vlan$vtag";

	$uplines   .= "$VLANCONFIG add $iface $vtag\n    ";
	$uplines   .= sprintf($IFCONFIG, $vdev, $inet, $mask);
	$downlines .= "$IFCONFIGBIN $vdev down\n    ";
	$downlines .= "$VLANCONFIG rem $vdev";
    }

    return ($uplines, $downlines);
}

#
# Compute the name of a virtual interface device based on the
# information in ifconfig hash (as returned by getifconfig).
#
sub os_viface_name($)
{
    my ($ifconfig) = @_;
    my $piface = $ifconfig->{"IFACE"};

    #
    # Physical interfaces use their own name
    #
    if (!$ifconfig->{"ISVIRT"}) {
	return $piface;
    }

    #
    # Otherwise we have a virtual interface: alias, veth, vlan.
    #
    # alias: There is an alias device, but not sure what it is good for
    #        so for now we just return the phys device.
    # vlan:  vlan<VTAG>
    # veth:  veth<ID>
    #
    my $itype = $ifconfig->{"ITYPE"};
    if ($itype eq "alias") {
	return $piface;
    } elsif ($itype eq "vlan") {
	return $itype . $ifconfig->{"VTAG"};
    } elsif ($itype eq "veth") {
	return $itype . $ifconfig->{"ID"};
    }

    warn("Linux does not support virtual interface type '$itype'\n");
    return undef;
}

#
# Generate and return an string that is approriate for putting
# into /etc/hosts.
#
sub os_etchosts_line($$$)
{
    my ($name, $ip, $aliases) = @_;
    
    return sprintf("%s\t%s %s", $ip, $name, $aliases);
}

#
# Add a new group
# 
sub os_groupadd($$)
{
    my($group, $gid) = @_;

    return system("$GROUPADD -g $gid $group");
}

#
# Delete an old group
# 
sub os_groupdel($)
{
    my($group) = @_;

    return system("$GROUPDEL $group");
}

#
# Remove a user account.
# 
sub os_userdel($)
{
    my($login) = @_;

    return system("$USERDEL $login");
}

#
# Modify user group membership.
# 
sub os_usermod($$$$$$)
{
    my($login, $gid, $glist, $pswd, $root, $shell) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }
    # Map the shell into a full path.
    $shell = MapShell($shell);

    return system("$USERMOD -s $shell -g $gid $glist -p '$pswd' $login");
}

#
# Modify user password.
# 
sub os_modpasswd($$)
{
    my($login, $pswd) = @_;

    if (system("$USERMOD -p '$pswd' $login") != 0) {
	warn "*** WARNING: resetting password for $login.\n";
	return -1;
    }
    if ($login eq "root" &&
	system("$USERMOD -p '$pswd' toor") != 0) {
	warn "*** WARNING: resetting password for toor.\n";
	return -1;
    }
    return 0;
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root, $shell) = @_;
    my $args = "";

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$args .= "-G $glist ";
    }
    # If remote, let it decide where to put the homedir.
    if (!REMOTE()) {
	$args .= "-d $homedir ";

	# Locally, if directory exists and is populated, skip -m
	# and make sure no attempt is made to create.
	if (! -d $homedir || ! -e "$homedir/.cshrc") {
	    $args .= "-m ";
	}
	else {
	    #
	    # -M is Redhat only option?  Overrides default CREATE_HOME.
	    # So we see if CREATE_HOME is set and if so, use -M.
	    #
	    if (!system("grep -q CREATE_HOME /etc/login.defs")) {
		$args .= "-M ";
	    }
	}
    }
    elsif (!PLAB()) {
	my $marg = "-m";

	#
	# XXX DP hack
	# Only force creation of the homdir if the default homedir base
	# is on a local FS.  On the DP, all nodes share a homedir base
	# which is hosted on one of the nodes, so we create the homedir
	# only on that node.
	#
	$defhome = `$USERADD -D 2>/dev/null`;
	if ($defhome =~ /HOME=(.*)/) {
	    if (!os_islocaldir($1)) {
		$marg = "";
	    }
	}

	# populate on remote nodes. At some point will tar files over.
	$args .= $marg;
    }

    # Map the shell into a full path.
    $shell = MapShell($shell);
    my $oldmask = umask(0022);

    if (system("$USERADD -u $uid -g $gid $args -p '$pswd' ".
	       "-s $shell -c \"$gcos\" $login") != 0) {
	warn "*** WARNING: $USERADD $login error.\n";
	umask($oldmask);
	return -1;
    }
    umask($oldmask);
    return 0;
}

#
# Remove a homedir. Might someday archive and ship back.
#
sub os_homedirdel($$)
{
    return 0;
}

#
# Create a directory including all intermediate directories.
#
sub os_mkdir($$)
{
    my ($dir, $mode) = @_;

    if (system("$MKDIR -p -m $mode $dir")) {
	return 0;
    }
    return 1;
}

#
# OS Dependent configuration. 
# 
sub os_setup()
{
    return 0;
}
    
#
# OS dependent, routing-related commands
#
sub os_routing_enable_forward()
{
    my $cmd;

    $cmd = "sysctl -w net.ipv4.conf.all.forwarding=1";
    return $cmd;
}

sub os_routing_enable_gated($)
{
    my ($conffile) = @_;
    my $cmd;

    #
    # XXX hack to avoid gated dying with TCP/616 already in use.
    #
    # Apparently the port is used by something contacting ops's
    # portmapper (i.e., NFS mounts) and probably only happens when
    # there are a bazillion NFS mounts (i.e., an experiment in the
    # testbed project).
    #
    $cmd  = "for try in 1 2 3 4 5 6; do\n";
    $cmd .= "\tif `cat /proc/net/tcp | ".
	"grep -E -e '[0-9A-Z]{8}:0268 ' >/dev/null`; then\n";
    $cmd .= "\t\techo 'gated GII port in use, sleeping...';\n";
    $cmd .= "\t\tsleep 10;\n";
    $cmd .= "\telse\n";
    $cmd .= "\t\tbreak;\n";
    $cmd .= "\tfi\n";
    $cmd .= "    done\n";
    $cmd .= "    $GATED -f $conffile";
    return $cmd;
}

sub os_routing_add_manual($$$$$;$)
{
    my ($routetype, $destip, $destmask, $gate, $cost, $rtabid) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE add -host $destip gw $gate";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE add -net $destip netmask $destmask gw $gate";
    } elsif ($routetype eq "default") {
	$cmd = "$ROUTE add default gw $gate";
    } else {
	warn "*** WARNING: bad routing entry type: $routetype\n";
	$cmd = "";
    }

    return $cmd;
}

sub os_routing_del_manual($$$$$;$)
{
    my ($routetype, $destip, $destmask, $gate, $cost, $rtabid) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE delete -host $destip";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE delete -net $destip netmask $destmask gw $gate";
    } elsif ($routetype eq "default") {
	$cmd = "$ROUTE delete default";
    } else {
	warn "*** WARNING: bad routing entry type: $routetype\n";
	$cmd = "";
    }

    return $cmd;
}

# Map a shell name to a full path using /etc/shells
sub MapShell($)
{
   my ($shell) = @_;

   if ($shell eq "") {
       return $DEFSHELL;
   }

   #
   # May be multiple lines (e.g., /bin/sh, /usr/bin/sh, etc.) in /etc/shells.
   # Just use the first entry.
   #
   my @paths = `grep '/${shell}\$' $SHELLS`;
   if ($?) {
       return $DEFSHELL;
   }
   my $fullpath = $paths[0];
   chomp($fullpath);

   # Sanity Checks
   if ($fullpath =~ /^([-\w\/]*)$/ && -x $fullpath) {
       $fullpath = $1;
   }
   else {
       $fullpath = $DEFSHELL;
   }
   return $fullpath;
}

# Return non-zero if given directory is on a "local" filesystem
sub os_islocaldir($)
{
    my ($dir) = @_;
    my $rv = 0; 

    my @dfoutput = `$DF -l $dir 2>/dev/null`;
    if (grep(!/^filesystem/i, @dfoutput) > 0) {
	$rv = 1;
    }
    return $rv;
}

sub os_getnfsmounts($)
{
    my ($rptr) = @_;
    my %mounted = ();

    #
    # Grab the output of the mount command and parse. 
    #
    if (! open(MOUNT, "/bin/mount|")) {
	print "os_getnfsmounts: Cannot run mount command\n";
	return -1;
    }
    while (<MOUNT>) {
	if ($_ =~ /^([-\w\.\/:\(\)]+) on ([-\w\.\/]+) type (\w+) .*$/) {
	    # Check type for nfs string.
	    if ($3 eq "nfs") {
		# Key is the remote NFS path, value is the mount point path.
		$mounted{$1} = $2;
	    }
	}
    }
    close(MOUNT);
    %$rptr = %mounted;
    return 0;
}

sub os_fwconfig_line($@)
{
    my ($fwinfo, @fwrules) = @_;
    my ($upline, $downline);
    my $errstr = "*** WARNING: Linux firewall not implemented\n";


    warn $errstr;
    $upline = "echo $errstr; exit 1";
    $downline = "echo $errstr; exit 1";

    return ($upline, $downline);
}

sub os_fwrouteconfig_line($$$)
{
    my ($orouter, $fwrouter, $routestr) = @_;
    my ($upline, $downline);

    #
    # XXX assume the original default route should be used to reach servers.
    #
    # For setting up the firewall, this means we create explicit routes for
    # each host via the original default route.
    #
    # For tearing down the firewall, we just remove the explicit routes
    # and let them fall back on the now re-established original default route.
    #
    $upline  = "for vir in $routestr; do\n";
    $upline .= "        $ROUTE delete \$vir >/dev/null 2>&1\n";
    $upline .= "        $ROUTE add -host \$vir gw $orouter || {\n";
    $upline .= "            echo \"Could not establish route for \$vir\"\n";
    $upline .= "            exit 1\n";
    $upline .= "        }\n";
    $upline .= "    done";

    $downline  = "for vir in $routestr; do\n";
    $downline .= "        $ROUTE delete \$vir >/dev/null 2>&1\n";
    $downline .= "    done";

    return ($upline, $downline);
}

# proto for a function used in os_ifdynconfig_cmds
sub getCurrentIwconfig($;$);

#
# Returns a list of commands needed to change the current device state to 
# something matching the given configuration options.
#
sub os_ifdynconfig_cmds($$$$$) 
{
    my ($ret_aref,$iface,$action,$optref,$ifcfg) = @_;
    my %opts = %$optref;
    my %flags = ();
    # this is the hash returned from getifconfig, but only for this interface
    my %emifc = %$ifcfg;

    my @cmds = ();

    # only handle the atheros case for now, since it's the only one
    # that can be significantly parameterized
    if (exists($emifc{'TYPE'}) && $emifc{'TYPE'} eq 'ath') {
	my ($ifnum) = $iface =~ /wifi(\d+)/;
        my $ath = "ath${ifnum}";
	my $wifi = $iface;

	# check flags
	my ($reset_wlan,$reset_kmod,$remember) = (0,0,0);
	if (exists($opts{'resetkmod'}) && $opts{'resetkmod'} == 1) {
	    $reset_kmod = 1;
	    # note that this forces a wlan reset too!
	    $reset_wlan = 1;
	    delete $opts{'resetkmod'};
	}
	if (exists($flags{'resetwlan'}) && $opts{'resetwlan'} == 1) {
	    $reset_wlan = 1;
	    delete $opts{'resetwlan'};
	}
	# we only want to try to keep similar config options
	# if the user tells us to...
	if (exists($flags{'usecurrent'}) && $opts{'usecurrent'} == 1) {
	    $remember = 1;
	    delete $opts{'usecurrent'};
	}

	# handle the up/down case right away.
	if (($action eq 'up' || $action eq 'down') 
	    && scalar(keys(%opts)) == 0) {
	    push @cmds,"$IFCONFIGBIN $ath $action";
	    @$ret_aref = @cmds;
	    return 0;
	}

	# first grab as much current state as we can, so we don't destroy
	# previous state if we have to destroy the VAP (i.e., athX) interface
	# 
	# NOTE that we don't bother grabbing current ifconfig state --
	# we assume that the current state is just what Emulab configured!
	my $iwc_ref = getCurrentIwconfig($ath);
	my %iwc = %$iwc_ref;

	# hash containing new config:
	my %niwc;

	# first, whack the emulab and user-supplied configs
	# so that the iwconfig params match what we need to give iwconfig
	# i.e., emulab specifies ssid and we need essid.
	if (exists($emifc{'ssid'})) {
	    $emifc{'essid'} = $emifc{'ssid'};
	    delete $emifc{'ssid'};
	}
	if (exists($opts{'ssid'})) {
	    $opts{'essid'} = $opts{'ssid'};
	    delete $opts{'ssid'};
	}
	if (exists($opts{'ap'})) {
	    $opts{'accesspoint'} = $opts{'ap'};
	    delete $opts{'ap'};
	}
	# we want this to be determined by the keyword 'freq' to iwconfig, 
	# not channel
	if (exists($opts{'channel'}) && !exists($opts{'freq'})) {
	    $opts{'freq'} = $opts{'channel'};
	}

	for my $ok (keys(%opts)) {
	    print STDERR "opts kv $ok=".$opts{$ok}."\n";
	}
	for my $tk (keys(%iwc)) {
	    print STDERR "iwc kv $tk=".$iwc{$tk}."\n";
	}

	# here's how we set things up: we set niwc to emulab wireless data
	# (i.e., INTERFACE_SETTINGs), then add in any current state, then
	# add in any of the reconfig options.
	my $key;
	if ($remember) {
	    for $key (keys(%{$emifc{'SETTINGS'}})) {
		$niwc{$key} = $emifc{'SETTINGS'}->{$key};
	    }
	    for $key (keys(%iwc)) {
		$niwc{$key} = $iwc{$key};
	    }
	}
	for $key (keys(%opts)) {
	    $niwc{$key} = $opts{$key};
	}

	for my $nk (keys(%niwc)) {
	    print STDERR "niwc kv $nk=".$niwc{$nk}."\n";
	}

	# see what has changed and what we're going to have to do
	my ($mode_ch,$proto_ch) = (0,0);

	# first, change mode to a string matching those returned by iwconfig:
	if (exists($niwc{'mode'})) {
	    if ($niwc{'mode'} =~ /ad[\s\-]{0,1}hoc/i) {
		$niwc{'mode'} = 'Ad-Hoc';
	    }
	    elsif ($niwc{'mode'} =~ /monitor/i) {
		$niwc{'mode'} = "Monitor";
	    }
	    elsif ($niwc{'mode'} =~ /ap/i
		   || $niwc{'mode'} =~ /master/i) {
		$niwc{'mode'} = "Master";
	    }
	    elsif ($niwc{'mode'} =~ /sta/i 
		   || $niwc{'mode'} =~ /managed/i) {
		$niwc{'mode'} = 'Managed';
	    }
	    else {
		print STDERR "ERROR: invalid mode '" . $niwc{'mode'} . "'\n";
		return 10;
	    }
	}

	# also change protocol, sigh
	if (exists($niwc{'protocol'})) {
	    if ($niwc{'protocol'} =~ /(802){0,1}11a/) {
		$niwc{'protocol'} = '80211a';
	    }
	    elsif ($niwc{'protocol'} =~ /(802){0,1}11b/) {
		$niwc{'protocol'} = '80211b';
	    }
	    elsif ($niwc{'protocol'} =~ /(802){0,1}11g/) {
		$niwc{'protocol'} = '80211g';
	    }
	    else {
		print STDERR "ERROR: invalid protocol '" . $niwc{'protocol'} . 
		    "'\n";
		return 11;
	    }
	}

	# to be backwards compat:
	# If the user sets a mode, we will put the device in that mode.
	# If the user does not set a mode, but does set an accesspoint, 
	#   we force the mode to either Managed or Master.
	# If the user sets neither a mode nor accesspoint, but we are told to
	#   "remember" the current state, we use that mode and ap.
	if (exists($opts{'mode'})) {
	    if ($niwc{'mode'} eq 'Managed' && exists($niwc{'accesspoint'})) {
		# strip colons and lowercase to check if we are the accesspoint
		# or a station:
		my $tap = $niwc{'accesspoint'};
		$tap =~ s/://g;
		$tap = lc($tap);
		
		my $tmac = lc($emifc{'MAC'});
		
		if ($tap eq $tmac) {
		    # we are going to be the accesspoint; switch our mode to
		    # master
		    $niwc{'mode'} = 'Master';
		}
		else {
		    $niwc{'mode'} = 'Managed';
		    $niwc{'ap'} = $tap;
		}
	    }
	}
	elsif (exists($opts{'accesspoint'})) {
	    # strip colons and lowercase to check if we are the accesspoint
	    # or a station:
	    my $tap = $niwc{'accesspoint'};
	    $tap =~ s/://g;
	    $tap = lc($tap);
	    
	    my $tmac = lc($emifc{'MAC'});
	    
	    if ($tap eq $tmac) {
		# we are going to be the accesspoint; switch our mode to
		# master
		$niwc{'mode'} = 'Master';
	    }
	    else {
		$niwc{'mode'} = 'Managed';
		$niwc{'ap'} = $tap;
	    }
	}
	elsif ($remember) {
	    # swipe first the old emulab config state, then the current
	    # iwconfig state:
	    
	    # actually, this was already done above.
	}

	# get rid of ap option if we're the master:
	if (exists($niwc{'mode'}) && $niwc{'mode'} eq 'Master') {
	    delete $niwc{'ap'};
	}

	print STDERR "after whacking niwc into compliance:\n";
	for my $nk (keys(%niwc)) {
	    print STDERR "niwc kv $nk=".$niwc{$nk}."\n";
	}

	# assemble params to commands:
	my ($iwc_mode,$wlc_mode);
	my $iwp_mode;

	if (exists($niwc{'mode'}) && $niwc{'mode'} ne $iwc{'mode'}) {
	    $mode_ch = 1;
	}
	
	if (exists($niwc{'mode'})) {
	    $iwc_mode = $niwc{'mode'};
	    if ($niwc{'mode'} eq 'Ad-Hoc') {
		$wlc_mode = 'adhoc';
	    }
	    elsif ($niwc{'mode'} eq 'Managed') {
		$wlc_mode = 'sta';
	    }
	    elsif ($niwc{'mode'} eq 'Monitor') {
		$wlc_mode = 'monitor';
	    }
	    elsif ($niwc{'mode'} eq 'Master') {
		$wlc_mode = 'ap';
	    }
	}

	if (exists($niwc{'protocol'})) {
	    if ($niwc{'protocol'} ne $iwc{'protocol'}) {
		$proto_ch = 1;
	    }
	    
	    if ($niwc{'protocol'} eq '80211a') {
		$iwp_mode = 1;
	    }
	    elsif ($niwc{'protocol'} eq '80211b') {
		$iwp_mode = 2;
	    }
	    elsif ($niwc{'protocol'} eq '80211g') {
		$iwp_mode = 3;
	    }
	}

	# for atheros cards, if we have to change the mode, we have to 
	# first tear down the VAP and rerun wlanconfig, then reconstruct
	# and reconfig the VAP.
	if ($mode_ch == 1) {
	    $reset_wlan = 1;
	}

        # Log what we're going to do:
	if ($reset_wlan && defined($wlc_mode)) {
	    print STDERR "WLANCONFIG: iface=$wifi; mode=$wlc_mode\n";
	}
	if (($proto_ch || $reset_wlan) && defined($iwp_mode)) {
	    print STDERR "IWPRIV: proto=".$niwc{'protocol'}." ($iwp_mode)\n";
	}
	if ($reset_wlan) {
	    print STDERR "IFCONFIG: iface=$ath; ip=" . $emifc{'IPADDR'} . 
		"; netmask=" . $emifc{'IPMASK'} . "\n";
	}

	# assemble iwconfig params:
	my $iwcstr = '';
	if (exists($niwc{'essid'})) {
	    $iwcstr .= ' essid ' . $niwc{'essid'};
	}
	if (exists($niwc{'freq'})) {
	    $iwcstr .= ' freq ' . $niwc{'freq'};
	}
	if (exists($niwc{'rate'})) {
	    $iwcstr .= ' rate ' . $niwc{'rate'};
	}
	if (exists($niwc{'txpower'})) {
	    $iwcstr .= ' txpower ' . $niwc{'txpower'};
	}
	if (exists($niwc{'sens'})) {
	    $iwcstr .= ' sens ' . $niwc{'sens'};
	}
	if (exists($niwc{'rts'})) {
	    $iwcstr .= ' rts ' . $niwc{'rts'};
	}
	if (exists($niwc{'frag'})) {
	    $iwcstr .= ' frag ' . $niwc{'frag'};
	}
	if (defined($iwc_mode) && $iwc_mode ne '') {
	    $iwcstr .= " mode $iwc_mode";
	    
	    if ($iwc_mode eq 'Managed') {
		if (exists($niwc{'ap'})) {
		    if (!($niwc{'ap'} =~ /:/)) {
                        # I really dislike perl sometimes.
                        $iwcstr .= ' ap ' .
                            substr($niwc{'ap'},0,2) . ":" .
                            substr($niwc{'ap'},2,2) . ":" .
                            substr($niwc{'ap'},4,2) . ":" .
                            substr($niwc{'ap'},6,2) . ":" .
                            substr($niwc{'ap'},8,2) . ":" .
                            substr($niwc{'ap'},10,2);
                    }
                    else {
			$iwcstr .= ' ap ' . $niwc{'ap'};
		    }
		}
		else {
		    $iwcstr .= ' ap any';
		}
	    }
	}

	print STDERR "IWCONFIG: $iwcstr\n";

        #
        # Generate commands to reconfigure the device.
        #
	if ($action eq 'up') {
	    push @cmds,"$IFCONFIGBIN $ath $action";
	}

	if ($reset_wlan) {
	    push @cmds,"$IFCONFIGBIN $ath down";
	    push @cmds,"$IFCONFIGBIN $wifi down";
	    push @cmds,"$WLANCONFIG $ath destroy";
	    
	    if ($reset_kmod) {
		## also "reset" the kernel modules:
		push @cmds,"$RMMOD ath_pci ath_rate_sample ath_hal";
		push @cmds,"$RMMOD wlan_scan_ap wlan_scan_sta wlan";
		push @cmds,"$MODPROBE ath_pci autocreate=none";
	    }
	    
	    push @cmds,"$WLANCONFIG $ath create wlandev $wifi " . 
		"wlanmode $wlc_mode";
	}
	if (($proto_ch || $mode_ch || $reset_wlan) && defined($iwp_mode)) {
	    push @cmds,"$IWPRIV $ath mode $iwp_mode";
	}
	push @cmds,"$IWCONFIG $ath $iwcstr";
	if ($reset_wlan) {
	    push @cmds,"$IFCONFIGBIN $ath inet " . $emifc{'IPADDR'} . 
		" netmask " . $emifc{'IPMASK'} . " up";
	    # also make sure routing is up for this interface
	    push @cmds,"/var/emulab/boot/rc.route " . $emifc{'IPADDR'} . " up";
	}

	# We don't do this right now because when we have to reset
	# wlan state to force a new mode, we panic the kernel if we 
	# do a wlanconfig without first destroying any monitor mode VAPs.
	# What's more, I haven't found a way to see which VAP is attached to
	# which real atheros device.
	
	#if ($do_mon_vdev) {
	#    $athmon = "ath" . ($iface_num + 10);
	#    push @cmds,"$WLANCONFIG $athmon create wlandev $wifi wlanmode monitor";
	#    push @cmds,"$IFCONFIGBIN $athmon up";
	#}

	if ($action eq 'down') {
	    push @cmds,"$IFCONFIGBIN $ath $action";
	}
    }
    elsif (exists($emifc{'TYPE'}) && $emifc{'TYPE'} eq 'flex900') {
	# see if we have any flags...
	$resetkmod = 0;
	if (exists($opts{'resetkmod'}) && $opts{'resetkmod'} == 1) {
	    $resetkmod = 1;
	}

	# check args -- we MUST have freq and rate.
	my ($freq,$rate,$carrierthresh,$rxgain);

	if (!exists($opts{'protocol'})
	    || $opts{'protocol'} ne 'flex900') {
	    warn("*** WARNING: Unknown gnuradio protocol specified, " . 
		 "assuming flex900!\n");
        }

	if (exists($opts{'frequency'})) {
	    $freq = $opts{'frequency'};
	}
	elsif (exists($opts{'freq'})) {
            $freq = $opts{'freq'};
        }
	else {
	    warn("*** WARNING: No frequency specified for gnuradio ".
                 "interface!\n");
            return undef;
	}

	if (exists($opts{'rate'})) {
	    $rate = $opts{'rate'};
	}
	else {
	    warn("*** WARNING: No rate specified for gnuradio interface!\n");
            return undef;
        }

	if (exists($opts{'carrierthresh'})) {
	    $carrierthresh = $opts{'carrierthresh'};
	}
	if (exists($opts{'rxgain'})) {
	    $rxgain = $opts{'rxgain'};
	}

	#
	# Generate commands
	#
	push @cmds,"$IFCONFIGBIN $iface down";

	# find out if we have to kill the current tunnel process...
	my $tpid;
	if (!open(PSP, "ps axwww 2>&1 |")) {
	    print STDERR "ERROR: open: $!"; 
	    return 19;
	}
	while (my $psl = <PSP>) {
	    if ($psl =~ /\s*(\d+)\s*.*emulab\/tunnel\.py.*/) {
		$tpid = $1;
		last;
	    }
	}
	close(PSP);
	if (defined($tpid)) {
	    push @cmds,"kill $tpid";
	}

	if ($resetkmod) {
	    push @cmds,"/sbin/rmmod tun";
	    push @cmds,"/sbin/modprobe tun";
	}

	my $tuncmd = 
	    "/bin/env PYTHONPATH=/usr/local/lib/python2.4/site-packages " .
	    "$BINDIR/tunnel.py -f $freq -r $rate";
	if (defined($carrierthresh)) {
	    $tuncmd .= " -c $carrierthresh";
	}
	if (defined($rxgain)) {
	    $tuncmd .= " -rx-gain=$rxgain";
	}
	$tuncmd .= " > /dev/null 2>&1 &";
	push @cmds,$tuncmd;

	# Give the tun device time to come up
	push @cmds,"sleep 2";

	my $mac = $emifc{'MAC'};
	push @cmds,"$IFCONFIGBIN $iface hw ether $mac";
	push @cmds,"$IFCONFIGBIN $iface inet " . $emifc{'IPADDR'} .
	    " netmask " . $emifc{'IPMASK'} . " up";
	# also make sure routing is up for this interface
	push @cmds,"/var/emulab/boot/rc.route " . $emifc{'IPADDR'} . " up";
    }
    
    @$ret_aref = @cmds;
    
    return 0;
}

my %def_iwconfig_regex = ( 'protocol' => '.+(802.*11[abg]{1}).*',
			   'essid'    => '.+SSID:\s*"*([\w\d_\-\.]+)"*.*',
			   'mode'     => '.+Mode:([\w\-]+)\s+',
			   'freq'     => '.+Frequency:(\d+\.\d+\s*\w+).*',
			   'ap'       => '.+Access Point:\s*([0-9A-Za-z\:]+).*',
			   'rate'     => '.+Rate[:|=]\s*(\d+\s*[\w\/]*)\s*',
			   'txpower'  => '.+ower[:|=](\d+\s*[a-zA-Z]+).*',
			   'sens'     => '.+Sensitivity[:|=](\d+).*',
                           # can't set this on our atheros cards
                           #'retry'    => '.+Retry[:|=](\d+|off).*',
			   'rts'      => '.+RTS thr[:|=](\d+|off).*',
			   'frag'     => '.+Fragment thr[:|=](\d+|off).*',
                           # don't care about this on our cards
                           #'power'    => '.+Power Management[:|=](\d+|off).*',
			 );

#
# Grab current iwconfig data for a specific interface, based on the 
# specified regexps (which default to def_iwconfig_regex if unspecified).
# Postprocess the property values so that they can be stuck back into iwconfig.
#
sub getCurrentIwconfig($;$) {
    my ($dev,$regex_ref) = @_;
    my %regexps;

    if (!defined($dev) || $dev eq '') {
        return;
    }
    if (!defined($regex_ref)) {
	%regexps = %def_iwconfig_regex;
    }
    else {
	%regexps = %$regex_ref;
    }

    my %r = ();
    my @output = `$IWCONFIG`;

    my $foundit = 0;
    foreach my $line (@output) {
        if ($line =~ /^$dev/) {
            $foundit = 1;
        }
        elsif ($foundit && !($line =~ /^\s+/)) {
            last;
        }

        if ($foundit) {
            foreach my $iwprop (keys(%regexps)) {
                my $regexp = $regexps{$iwprop};
                if ($line =~ /$regexp/) {
                    $r{$iwprop} = $1;
                }
            }    
        }
    }
     
    # postprocessing.
    # We change the values back to valid args to the iwconfig command
    if (defined($r{'protocol'})) {
        $r{'protocol'} =~ s/\.//g;
    }
     
    if (defined($r{'rate'})) {
        if ($r{'rate'} =~ /^(\d+) Mb\/s/) {
            $r{'rate'} = "${1}M";
        }
        else {
            $r{'rate'} = $1;
        }
    }

    if (defined($r{'txpower'})) {
        if ($r{'txpower'} =~ /^(\d+)/) {
            $r{'txpower'} = $1;
        }
        else {
            $r{'txpower'} = 'auto';
        }
    }

    if (defined($r{'freq'})) {
        $r{'freq'} =~ s/\s//g;
    }

    foreach my $rk (keys(%r)) {
	print STDERR "gci $rk=".$r{$rk}."\n";
    }
     
    return \%r;
}

sub os_config_gre($$$$$$$)
{
    my ($name, $unit, $inetip, $peerip, $mask, $srchost, $dsthost) = @_;

    my $dev = "$name$unit";

    if (GENVNODE() && GENVNODETYPE() eq "openvz") {
	$dev = "gre$unit";
	
	if (system("$IFCONFIGBIN $dev $inetip netmask $mask up")) {
	    warn("Could not start tunnel $dev!\n");
	    return -1;
	}
    }
    elsif (system("ip tunnel add $dev mode gre ".
		  "remote $dsthost local $srchost") ||
	   system("ip link set $dev up") ||
	   system("ip addr add $inetip dev $dev") ||
	   system("$IFCONFIGBIN $dev netmask $mask")) {
	warn("Could not start tunnel $dev!\n");
	return -1;
    }
    return 0;
}

sub os_get_disks
{
	my @blockdevs;

	@blockdevs = map { s#/sys/block/##; $_ } glob('/sys/block/*');

	return @blockdevs;
}

sub os_get_disk_size($)
{
	my ($disk) = @_;
	my $size;

	$disk =~ s#^/dev/##;

	if (!open SIZE, "/sys/block/$disk/size") {
		warn "Couldn't open /sys/block/$disk/size: $!\n";
		return undef;
	}
	$size = <SIZE>;
	close SIZE;
	chomp $size;

	$size = $size * 512 / 1024 / 1024;

	return $size;
}

sub os_get_partition_info($$)
{
    my ($bootdev, $partition) = @_;

    $bootdev =~ s#^/dev/##;

    if (!open(FDISK, "fdisk -l /dev/$bootdev |")) {
	print("Failed to run fdisk on /dev/$bootdev!");
	return -1;
    }

    while (<FDISK>) {
	    next if (!m#^/dev/$bootdev$partition\s+#);

	    s/\*//;

	    my ($length, $ptype) = (split /\s+/)[3,4];

	    $length =~ s/\+$//;
	    $ptype = hex($ptype);

	    close FDISK;

	    return ($length, $ptype);
    }

    print "No such partition in fdisk summary info for MBR on /dev/$bootdev!\n";
    close FDISK;

    return -1;
}

sub os_nfsmount($$)
{
    my ($remote,$local) = @_;

    if (system("/bin/mount -o vers=2,udp $remote $local")
	&& system("/bin/mount -o udp $remote $local")) {
	return 1;
    }

    return 0;
}

1;
