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
	 os_setup os_groupadd os_useradd os_userdel os_usermod
	 os_rpminstall_line
       );

# Must come after package declaration!
use English;

#
# This is where the Linux dependent part of the setup library lives.
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
my $RPMINSTALL  = "/bin/rpm -i %s";
my @LOCKFILES   = ("/etc/group.lock", "/etc/gshadow.lock");

#
# OS dependent part of cleanup node state.
# 
sub os_cleanup_node () {
    unlink @LOCKFILES;

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
sub os_ifconfig_line($$$)
{
    my ($iface, $inet, $mask) = @_;
    
    return sprintf($IFCONFIG, $iface, $inet, $mask);
}

#
# Generate and return an string that is approriate for putting
# into /etc/hosts.
#
sub os_etchosts_line($$$$)
{
    my ($name, $link, $ip, $alias) = @_;
    
    return sprintf("%s\t%s-%s %s", $ip, $name, $link, $alias);
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
# Generate and return an RPM install line that is approriate for putting
# into a shell script (invoked at bootup).
#
sub os_rpminstall_line($)
{
    my ($rpm) = @_;
    
    return sprintf($RPMINSTALL, $rpm);
}

#
# OS Dependent configuration. 
# 
sub os_setup()
{
    return 0;
}
    
1;
