#!/usr/bin/perl -wT

#
# FreeBSD specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP
	 os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_groupdel os_cleanup_node os_homedirdel
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual
       );

# Must come after package declaration!
use English;

#
# This is the FreeBSD dependent part of the setup library. 
# 
my $SETUPDIR = "/usr/local/etc/testbed";
libsetup::libsetup_init($SETUPDIR);

#
# Various programs and things specific to FreeBSD and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/usr/bin/egrep -s -q";

#
# These are not exported
#
my $USERADD     = "/usr/sbin/pw useradd";
my $USERDEL     = "/usr/sbin/pw userdel";
my $USERMOD     = "/usr/sbin/pw usermod";
my $GROUPADD	= "/usr/sbin/pw groupadd";
my $GROUPDEL	= "/usr/sbin/pw groupdel";
my $CHPASS	= "/usr/bin/chpass -p";
my $MKDB	= "/usr/sbin/pwd_mkdb -p";
my $MKDIR	= "/bin/mkdir";
my $ROUTE	= "/sbin/route";

#
# OS dependent part of cleanup node state.
# 
sub os_cleanup_node ($) {
    my ($scrub) = @_;

    return 0;
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
# Remove a new group
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
sub os_usermod($$$$)
{
    my($login, $gid, $glist, $root) = @_;

#    if ($root) {
#	$glist = join(',', split(/,/, $glist), "wheel");
#    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    return system("$USERMOD $login -g $gid $glist");
}

#
# Remove a homedir. Might someday archive and ship back.
#
sub os_homedirdel($$)
{
    my ($login, $homedir) = @_;
    
    print "Removing home directory: $homedir\n";
    return(system("rm -rf $homedir"));
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root) = @_;

#    if ($root) {
#	$glist = join(',', split(/,/, $glist), "wheel");
#    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    if (system("$USERADD $login -u $uid -g $gid $glist ".
	       "-m -s /bin/tcsh -c \"$gcos\"") != 0) {
	warn "*** WARNING: $USERADD $login error.\n";
	return -1;
    }

    if (system("$CHPASS $pswd $login") != 0) {
	warn "*** WARNING: $CHPASS $login error.\n";
	return -1;
    }
    return 0;
}

#
# OS dependent, routing-related commands
#

sub os_routing_enable_forward()
{
    return "echo 'IP forwarding not turned on!'";
}

sub os_routing_enable_gated()
{
    return "echo 'GATED IS NOT ALLOWED!'";
}

sub os_routing_add_manual($$$$$)
{
    my ($routetype, $destip, $destmask, $gate, $cost) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE add -host $destip $gate";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE add -net $destip $gate $destmask";
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
	$cmd = "$ROUTE delete -net $destip $gate $destmask";
    } else {
	warn "*** WARNING: bad routing entry type: $routetype\n";
	$cmd = "";
    }

    return $cmd;
}

1;
