#!/usr/bin/perl -wT

#
# Linux specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $MOUNT $UMOUNT $TMPASSWD
	 os_cleanup_node os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_rpminstall_line enable_ipod
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual os_homedirdel
       );

# Must come after package declaration!
use English;

#
# This is the Linux dependent part of the setup library. 
# 
my $SETUPDIR = "/etc/rc.d/testbed";
libsetup::libsetup_init($SETUPDIR);

#
# Various programs and things specific to Linux and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/bin/egrep -q";
$MOUNT		= "/bin/mount";
$UMOUNT		= "/bin/umount";
$TMPASSWD	= "$SETUPDIR/passwd";

#
# These are not exported
#
my $TMGROUP	= "$SETUPDIR/group";
my $TMSHADOW    = "/etc/rc.d/testbed/shadow";
my $TMGSHADOW   = "/etc/rc.d/testbed/gshadow";
my $USERADD     = "/usr/sbin/useradd";
my $USERDEL     = "/usr/sbin/userdel";
my $USERMOD     = "/usr/sbin/usermod";
my $GROUPADD	= "/usr/sbin/groupadd";
my $IFCONFIG    = "/sbin/ifconfig %s inet %s netmask %s";
my $IFC_100MBS  = "100baseTx";
my $IFC_10MBS   = "10baseT";
my $IFC_FDUPLEX = "FD";
my $IFC_HDUPLEX = "HD";
my $RPMINSTALL  = "/bin/rpm -i %s";
my @LOCKFILES   = ("/etc/group.lock", "/etc/gshadow.lock");
my $MKDIR	= "/bin/mkdir";
my $GATED	= "/usr/sbin/gated";
my $ROUTE	= "/sbin/route";

#
# OS dependent part of cleanup node state.
# 
sub os_cleanup_node ($) {
    my ($scrub) = @_;

    unlink @LOCKFILES;

    if (! $scrub) {
	return 0;
    }
    
    printf STDOUT "Resetting passwd and group files\n";
    if (system("$CP -f $TMGROUP $TMPASSWD /etc") != 0) {
	print STDERR "Could not copy default group file into place: $!\n";
	exit(1);
    }
    
    if (system("$CP -f $TMSHADOW $TMGSHADOW /etc") != 0) {
	print STDERR "Could not copy default passwd file into place: $!\n";
	exit(1);
    }

    return 0;
}

#
# Generate and return an ifconfig line that is approriate for putting
# into a shell script (invoked at bootup).
#
sub os_ifconfig_line($$$$$)
{
    my ($iface, $inet, $mask, $speed, $duplex) = @_;
    my ($ifc, $miirest, $miisleep, $miisetspd, $media);

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
	    warn("*** Bad speed units in ifconfig!\n");
	    $speed = 100;
	}
	if ($speed == 100) {
	    $media = $IFC_100MBS;
	}
	elsif ($speed == 10) {
	    $media = $IFC_10MBS;
	}
	else {
	    warn("*** Bad Speed in ifconfig!\n");
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
	warn("*** Bad duplex in ifconfig!\n");
	$media = "$media-$IFC_FDUPLEX";
    }

    $ifc = "/sbin/mii-tool --force=$media $iface\n" .
	   sprintf($IFCONFIG, $iface, $inet, $mask);
    
    return "$ifc";
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
sub os_usermod($$$$)
{
    my($login, $gid, $glist, $root) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    return system("$USERMOD -g $gid $glist $login");
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    if (system("$USERADD -M -u $uid -g $gid $glist -p $pswd ".
	       "-d $homedir -s /bin/tcsh -c \"$gcos\" $login") != 0) {
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
# Generate and return an RPM install line that is approriate for putting
# into a shell script (invoked at bootup).
#
sub os_rpminstall_line($)
{
    my ($rpm) = @_;
    
    return sprintf($RPMINSTALL, $rpm);
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
# OS dependent "ICMP Ping of Death" support
#

use Socket;

sub enable_ipod()
{
    if (system("sysctl net.ipv4.icmp_ipod_host")) {
	warn "*** WARNING: IPOD sysctls not supported in the kernel\n";
	return -1;
    }

    my ($bname, $bip) = split(/ /, `/etc/rc.d/testbed/tmcc bossinfo`);
    if (!defined($bip)) {
	warn "*** WARNING: could not determine boss node, IPOD not enabled\n";
	return -1;
    }
    my $ipuint = unpack("N", inet_aton($bip));

    # XXX arg to sysctl must be a signed 32-bit int, so we must "cast"
    my $sysctlcmd = sprintf("sysctl -w net.ipv4.icmp_ipod_host=%d", $ipuint);

    if (system($sysctlcmd)) {
	warn "*** WARNING: could not set IPOD host to $bip ($ipuint)\n";
	return -1;
    }
    if (system("sysctl -w net.ipv4.icmp_ipod_enabled=1")) {
	warn "*** WARNING: could not enable IPOD\n";
	return -1;
    }
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

sub os_routing_enable_gated()
{
    my $cmd;

    $cmd = "$GATED -f $SETUPDIR/gated_`$SETUPDIR/control_interface`.conf";
    return $cmd;
}

sub os_routing_add_manual($$$$$)
{
    my ($routetype, $destip, $destmask, $gate, $cost) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE add -host $destip gw $gate";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE add -net $destip netmask $destmask gw $gate";
    } else {
	warn "*** WARNING: bad routing entry type: $routetype\n";
	$cmd = "";
    }

    return $cmd;
}

sub os_routing_del_manual($$$$$)
{
    my ($routetype, $destip, $destmask, $gate, $cost) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE delete -host $destip";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE delete -net $destip netmask $destmask gw $gate";
    } else {
	warn "*** WARNING: bad routing entry type: $routetype\n";
	$cmd = "";
    }

    return $cmd;
}

1;
