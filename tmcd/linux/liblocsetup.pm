#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
	 os_cleanup_node os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_ifconfig_veth
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual os_homedirdel
	 os_groupdel
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
$NFSMOUNT	= "/bin/mount";
$UMOUNT		= "/bin/umount";
$TMPASSWD	= "$ETCDIR/passwd";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";
$RPMCMD		= "/bin/rpm";

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
my $IFCONFIG    = "/sbin/ifconfig %s inet %s netmask %s";
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
sub os_ifconfig_line($$$$$$;$)
{
    my ($iface, $inet, $mask, $speed, $duplex, $aliases, $rtabid) = @_;
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
    if (-e "/usr/sbin/ethtool") {
	# this seems to work for returning an error on eepro100
	$ifc = "if /usr/sbin/ethtool $iface >/dev/null 2>&1; then\n    " .
	       "  /usr/sbin/ethtool -s $iface autoneg off speed $speed duplex $duplex\n    " .
	       "else\n    " .
	       "  /sbin/mii-tool --force=$media $iface\n    " .
		   "fi\n    ";
    } else {
	$ifc = "/sbin/mii-tool --force=$media $iface\n    ";
    }

    $ifc .= sprintf($IFCONFIG, $iface, $inet, $mask);
    
    return "$ifc";
}

#
# Specialized function for configing locally hacked veth devices.
#
sub os_ifconfig_veth($$$$$;$)
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

1;
