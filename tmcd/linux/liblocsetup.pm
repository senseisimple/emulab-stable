#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Linux specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $NFSMOUNT $UMOUNT $TMPASSWD $SFSSD $SFSCD $RPMCMD $HOSTSFILE
	 os_account_cleanup os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_ifconfig_veth
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual os_homedirdel
	 os_groupdel os_getnfsmounts
	 os_fwconfig_line os_fwrouteconfig_line
       );

# Must come after package declaration!
use English;

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

#
# Various programs and things specific to Linux and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/bin/egrep -q";
$NFSMOUNT	= "/bin/mount -o vers=2,udp"; # Force NFS Version 2 over UDP
$UMOUNT		= "/bin/umount";
$TMPASSWD	= "$ETCDIR/passwd";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";
$RPMCMD		= "/bin/rpm";
$HOSTSFILE	= "/etc/hosts";

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

#
# OS dependent part of cleanup node state.
# 
sub os_account_cleanup()
{
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
sub os_ifconfig_line($$$$$$$;$$)
{
    my ($iface, $inet, $mask, $speed, $duplex, $aliases,
	$iface_type, $settings, $rtabid) = @_;
    my ($miirest, $miisleep, $miisetspd, $media);
    my ($uplines, $downlines);

    #
    # Special handling for new style interfaces (which have settings).
    # This should all move into per-type modules at some point. 
    #
    if ($iface_type eq "ath" && defined($settings)) {

	#
	# Setting the protocol is special and appears to be card specific.
	# How stupid is that!
	#
	my $protocol = $settings->{"protocol"};
        $privcmd = "/sbin/iwpriv $iface mode ";

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
	my $iwcmd = "/sbin/iwconfig $iface ";

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
	if (exists($settings->{"sensitivity"})) {
	    $iwcmd .= " sens " . $settings->{"sensitivity"};
	}

	#
	# We demand to be told if we are the master or a peon.
	# This needs to be last for some reason.
	#
	if (!exists($settings->{"accesspoint"})) {
	    warn("*** WARNING: No accesspoint provided for $iface!\n");
	    return undef;
	}
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
	    $iwcmd .= " mode Master";
	}
	else {
	    $iwcmd .= " mode Managed ap $accesspointwdots";
	}

	$uplines   = sprintf($IFCONFIG, $iface, $inet, $mask) . "\n";
	$uplines  .= $privcmd . "\n";
	$uplines  .= $iwcmd;
	$downlines = "$IFCONFIGBIN $iface down";
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

        $uplines = $tuncmd . " > /dev/null 2>&1 &\n";
        $uplines .= "sleep 5\n";
        $uplines .= "$IFCONFIGBIN $iface hw ether $mac\n";
        $uplines .= sprintf($IFCONFIG, $iface, $inet, $mask) . "\n";
        $downlines = "$IFCONFIGBIN $iface down";
        return ($uplines, $downlines);
    }

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
	    "else\n    " .
	    "  /sbin/mii-tool --force=$media $iface\n    " .
	    "fi\n    ";
    } else {
	$uplines = "/sbin/mii-tool --force=$media $iface\n    ";
    }

    if ($inet ne "") {
	$uplines  .= sprintf($IFCONFIG, $iface, $inet, $mask);
	$downlines = "$IFCONFIGBIN $iface down";

	# handle aliases
	my $aix = 0;
	foreach my $alias (split(',', $aliases)) {
	    my $aif = "$iface:$aix";
	    $uplines   .= "\n    ";
	    $uplines   .= sprintf($IFCONFIG, $aif, $alias, $mask);
	    $downlines .= "\n    $IFCONFIGBIN $aif down";
	    $aix++;
	}
    }
    
    return ($uplines, $downlines);
}

#
# Specialized function for configing locally hacked veth devices.
#
sub os_ifconfig_veth($$$$$;$$$)
{
    return "";
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
# Add a user.
# 
sub os_useradd($$$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root, $shell) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }
    # Map the shell into a full path.
    $shell = MapShell($shell);

    if (system("$USERADD -M -u $uid -g $gid $glist -p '$pswd' ".
	       "-d $homedir -s $shell -c \"$gcos\" $login") != 0) {
	warn "*** WARNING: $USERADD $login error.\n";
	return -1;
    }
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

   my $fullpath = `grep '/${shell}\$' $SHELLS`;

   if ($?) {
       return $DEFSHELL;
   }

   # Sanity Check
   if ($fullpath =~ /^([-\w\/]*)$/) {
       $fullpath = $1;
   }
   else {
       $fullpath = $DEFSHELL;
   }
   return $fullpath;
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

1;
