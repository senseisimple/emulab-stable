#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Linux specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $SFSSD $SFSCD
	 os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_groupdel os_cleanup_node os_homedirdel
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual
       );

# Must come after package declaration!
use English;

#
# This is the Linux dependent part of the setup library. 
# 
my $SETUPDIR = "/usr/local/etc/emulab";
libsetup::libsetup_init($SETUPDIR);

#
# Various programs and things specific to FreeBSD and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/bin/egrep -q";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";

#
# These are not exported
#
my $USERADD     = "/usr/sbin/useradd";
my $USERDEL     = "/usr/sbin/userdel";
my $USERMOD     = "/usr/sbin/usermod";
my $GROUPADD	= "/usr/sbin/groupadd";
my $GROUPDEL	= "/usr/sbin/groupdel";
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
sub os_usermod($$$$$)
{
    my($login, $gid, $glist, $pswd, $root) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "root");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    return system("$USERMOD -g $gid $glist -p '$pswd' $login");

}

#
# Remove a homedir. Might someday archive and ship back.
#
sub os_homedirdel($$)
{
    my ($login, $homedir) = @_;
    
    return 0;
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

    if (system("$USERADD -M -u $uid -g $gid $glist -p '$pswd' ".
	       "-d $homedir -s /bin/tcsh -c \"$gcos\" $login") != 0) {
	warn "*** WARNING: $USERADD $login error.\n";
	return -1;
    }
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
