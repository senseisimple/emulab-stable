#!/usr/bin/perl -wT

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# FreeBSD specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $MOUNT $UMOUNT $TMPASSWD $SFSSD $SFSCD
	 os_cleanup_node os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_rpminstall_line os_ifconfig_veth
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
	my $ETCDIR  = "/etc/testbed";
	my $BINDIR  = "/etc/testbed";
	my $VARDIR  = "/etc/testbed";
	my $BOOTDIR = "/etc/testbed";
    }
}
# Convenience.
sub REMOTE()	{ return libsetup::REMOTE(); }
sub MFS()	{ return libsetup::MFS(); }
sub JAILED()	{ return libsetup::JAILED(); }

#
# Various programs and things specific to FreeBSD and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/usr/bin/egrep -s -q";
$MOUNT		= "/sbin/mount -o -b ";
$UMOUNT		= "/sbin/umount";
$TMPASSWD	= "$ETCDIR/master.passwd";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";

#
# These are not exported
#
my $TMGROUP	= "$ETCDIR/group";
my $USERADD     = "/usr/sbin/pw useradd";
my $USERDEL     = "/usr/sbin/pw userdel";
my $USERMOD     = "/usr/sbin/pw usermod";
my $GROUPADD	= "/usr/sbin/pw groupadd";
my $GROUPDEL	= "/usr/sbin/pw groupdel";
my $CHPASS	= "/usr/bin/chpass -p";
my $MKDB	= "/usr/sbin/pwd_mkdb -p";
my $IFCONFIGBIN = "/sbin/ifconfig";
my $IFCONFIG    = "$IFCONFIGBIN %s inet %s netmask %s %s %s";
my $IFALIAS     = "$IFCONFIGBIN %s alias %s netmask 0xffffff00";
my $IFC_1000MBS = "media 1000baseSX";
my $IFC_100MBS  = "media 100baseTX";
my $IFC_10MBS   = "media 10baseT/UTP";
my $IFC_FDUPLEX = "mediaopt full-duplex";
my $RPMINSTALL  = "/usr/local/bin/rpm -i %s";
my $MKDIR	= "/bin/mkdir";
my $GATED	= "/usr/local/sbin/gated";
my $ROUTE	= "/sbin/route";
my $SHELLS	= "/etc/shells";
my $DEFSHELL	= "/bin/tcsh";

#
# OS dependent part of cleanup node state. On a remote node, this will
# only be called from inside a JAIL, or from the prepare script. 
# 
sub os_cleanup_node ($) {
    my ($scrub) = @_;

    if (! $scrub) {
	return 0;
    }
    
    printf STDOUT "Resetting passwd and group files\n";
    if (system("$CP -f $TMGROUP /etc/group") != 0) {
	print STDERR "Could not copy default group file into place: $!\n";
	exit(1);
    }
    
    if (system("$CP -f $TMPASSWD /etc/master.passwd_testbed") != 0) {
	print STDERR "Could not copy default passwd file into place: $!\n";
	exit(1);
    }
    
    if (system("$MKDB /etc/master.passwd_testbed") != 0) {
	print STDERR "Failure running $MKDB on default password file: $!\n";
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
    my $media    = "";
    my $mediaopt = "";
    my ($uplines, $downlines);

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
	    warn("*** Bad Speed in ifconfig!\n");
	    $media = $IFC_100MBS;
	}
    }

    if ($duplex eq "full") {
	$mediaopt = $IFC_FDUPLEX;
    }

    $uplines   = sprintf($IFCONFIG, $iface, $inet, $mask, $media, $mediaopt);
    $downlines = "$IFCONFIGBIN $iface down\n";

    if ($aliases ne "") {
	# Must do this first to avoid lo0 routes.
	$uplines .= "\n".
	            "sysctl -w net.link.ether.inet.useloopback=0\n";

	foreach my $alias (split(',', $aliases)) {
	    my $ifalias = sprintf($IFALIAS, $iface, $alias);

	    $uplines   .= "$ifalias\n";
	    $downlines .= "$IFCONFIGBIN $iface -alias $alias\n";
	}
    }
    return ($uplines, $downlines);
}

#
# Specialized function for configing locally hacked veth devices.
#
sub os_ifconfig_veth($$$$$;$)
{
    my ($iface, $inet, $mask, $id, $vmac, $rtabid) = @_;
    my ($uplines, $downlines);

    #
    # Do not try this on the MFS!
    #
    return ""
	if (MFS());

    require Socket;
    import Socket;

    # Need to derive a vlan tag. Just use the middle two octets.
    my $vtag = (unpack("I", inet_aton($inet)) >> 8) & 0xffff;

    if ($vmac =~ /^(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})(\w{2})$/) {
	$vmac = "$1:$2:$3:$4:$5:$6";
    }
    else {
	warn("Bad vmac in veth config: $vmac\n");
	return "";
    }
    $uplines = "";
    if (defined($iface)) {
	$uplines .= "$IFCONFIGBIN $iface up\n";
    }
    $uplines .= "$IFCONFIGBIN veth${id} create\n" .
	        "$IFCONFIGBIN veth${id} vethaddr $vmac/$vtag" .
		(defined($iface) ? " vethdev $iface\n" : "\n");

    if (defined($rtabid)) {
	$uplines .= "$IFCONFIGBIN veth${id} rtabid $rtabid\n";
    }
    $uplines  .= "$IFCONFIGBIN veth${id} inet $inet netmask $mask\n";
    $downlines = "$IFCONFIGBIN veth${id} down\n".
	         "$IFCONFIGBIN veth${id} destroy\n";
    return ($uplines, $downlines);
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

    return system("$GROUPADD $group -g $gid");
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
	$glist = join(',', split(/,/, $glist), "wheel");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }
    # Map the shell into a full path.
    $shell = MapShell($shell);

    if (system("$CHPASS '$pswd' $login") != 0) {
	warn "*** WARNING: $CHPASS $login error.\n";
	return -1;
    }
    return system("$USERMOD $login -s $shell -g $gid $glist");
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root, $shell) = @_;
    my $args = "";

    if ($root) {
	$glist = join(',', split(/,/, $glist), "wheel");
    }
    if ($glist ne "") {
	$args .= "-G $glist ";
    }
    # If remote, let it decide where to put the homedir.
    if (!REMOTE()) {
	$args .= "-d $homedir ";

	# Locally, if directory exists and is populated, skip -m
	# cause FreeBSD copies files in anyway!
	$args .= "-m "
	    if (! -e "$homedir/.cshrc");
    }
    else {
	# populate on remote nodes. At some point will tar files over.
	$args .= "-m ";
    }

    # Map the shell into a full path.
    $shell = MapShell($shell);

    if (system("$USERADD $login -u $uid -g $gid $args ".
	       "-s $shell -c \"$gcos\"") != 0) {
	warn "*** WARNING: $USERADD $login error.\n";
	return -1;
    }

    if (system("$CHPASS '$pswd' $login") != 0) {
	warn "*** WARNING: $CHPASS $login error.\n";
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
    # This should never happen!
    if (REMOTE() || MFS()) {
	print "Ignoring os setup on remote/MFS node!\n";
	return 0;
    }
}

#
# OS dependent, routing-related commands
#
sub os_routing_enable_forward()
{
    my $cmd;

    if (REMOTE()) {
	$cmd = "echo 'IP forwarding not turned on!'";
    }
    else {
	# No Fast Forwarding when operating with linkdelays. 
	$cmd  = "sysctl -w net.inet.ip.forwarding=1\n" .
	        "    if [ ! -e $BOOTDIR/rc.linkdelay ]; then\n" .
	        "        sysctl -w net.inet.ip.fastforwarding=1\n" .
		"    fi\n";
    }
    return $cmd;
}

sub os_routing_enable_gated()
{
    my $cmd;

    if (REMOTE()) {
	$cmd = "echo 'GATED IS NOT ALLOWED!'";
    }
    else {
	$cmd = "$GATED -f $BINDIR/gated_`$BINDIR/control_interface`.conf";
    }
    return $cmd;
}

sub os_routing_add_manual($$$$$;$)
{
    my ($routetype, $destip, $destmask, $gate, $cost, $rtabid) = @_;
    my $cmd;
    my $rtabopt = (defined($rtabid) ? "-rtabid $rtabid" : "");

    if ($routetype eq "host") {
	$cmd = "$ROUTE add $rtabopt -host $destip $gate";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE add $rtabopt -net $destip $gate $destmask";
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
    my $rtabopt = (defined($rtabid) ? "-rtabid $rtabid" : "");

    if ($routetype eq "host") {
	$cmd = "$ROUTE delete $rtabopt -host $destip";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE delete $rtabopt -net $destip $gate $destmask";
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
