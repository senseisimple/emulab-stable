#!/usr/bin/perl -wT

#
# FreeBSD specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $EGREP $MOUNT $UMOUNT $TMPASSWD
	 os_cleanup_node os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod
	 os_rpminstall_line update_delays
       );

# Must come after package declaration!
use English;

#
# This is the FreeBSD dependent part of the setup library. 
# 
my $SETUPDIR = "/etc/testbed";
libsetup::libsetup_init($SETUPDIR);

#
# Various programs and things specific to FreeBSD and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/usr/bin/egrep -s -q";
$MOUNT		= "/sbin/mount";
$UMOUNT		= "/sbin/umount";
$TMGROUP	= "$SETUPDIR/group";
$TMPASSWD	= "$SETUPDIR/master.passwd";

#
# These are not exported
#
my $USERADD     = "/usr/sbin/pw useradd";
my $USERDEL     = "/usr/sbin/pw userdel";
my $USERMOD     = "/usr/sbin/pw usermod";
my $GROUPADD	= "/usr/sbin/pw groupadd";
my $CHPASS	= "/usr/bin/chpass -p";
my $MKDB	= "/usr/sbin/pwd_mkdb -p";
my $IFCONFIG    = "/sbin/ifconfig %s inet %s netmask %s %s %s";
my $IFC_100MBS  = "media 100baseTX";
my $IFC_10MBS   = "media 10baseT/UTP";
my $IFC_FDUPLEX = "mediaopt full-duplex";
my $RPMINSTALL  = "/usr/local/bin/rpm -i %s";

#
# Delay node configuration goop.
#
my $KERNEL100   = "/kernel.100HZ";
my $KERNEL1000  = "/kernel.1000HZ";
my $KERNEL10000 = "/kernel.10000HZ";
my @KERNELS     = ($KERNEL100, $KERNEL1000, $KERNEL10000); 
my $kernel      = $KERNEL100;
my $IFACE	= "fxp";
my $CTLIFACENUM = "4";
my $CTLIFACE    = "${IFACE}${CTLIFACENUM}";
my $TMDELAY	= "$SETUPDIR/rc.delay";
my $TMCCCMD_DELAY = "delay";

#
# OS dependent part of cleanup node state.
# 
sub os_cleanup_node () {
    unlink $TMDELAY;

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
sub os_ifconfig_line($$$$$)
{
    my ($iface, $inet, $mask, $speed, $duplex) = @_;
    my $media    = "";
    my $mediaopt = "";

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
	$mediaopt = $IFC_FDUPLEX;
    }
   
    return sprintf($IFCONFIG, $iface, $inet, $mask, $media, $mediaopt);
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

    return system("$GROUPADD $group -g $gid");
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
	$glist = join(',', split(/,/, $glist), "wheel");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    return system("$USERMOD $login -g $gid $glist");
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "wheel");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    if (system("$USERADD $login -u $uid -g $gid $glist ".
	       "-d $homedir -s /bin/tcsh -c \"$gcos\"") != 0) {
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
    print STDOUT "Checking Testbed delay configuration ... \n";
    dodelays();
}

#
# Special update.
#
sub update_delays()
{
    dodelays();
    system($TMDELAY);
}
    
sub dodelays ()
{
    my @delays;
    my $checkreplace;

    my $TM = libsetup::OPENTMCC($TMCCCMD_DELAY);
    while (<$TM>) {
	push(@delays, $_);
    }
    close($TM);

    if (@delays) {
	$count    = 69;
	$mindelay = 10000;
    
	open(DEL, ">$TMDELAY")
	    or die("Could not open $TMDELAY");
	print DEL "#!/bin/sh\n";
	print DEL "sysctl -w net.link.ether.bridge=0\n";
	print DEL "sysctl -w net.link.ether.bridge_ipfw=0\n";
	print DEL "sysctl -w net.link.ether.bridge_cfg=";

	foreach $delay (@delays) {
	    $delay =~ /DELAY INT0=([\d\w]+) INT1=([\d\w]+) /;
	    my $iface1 = libsetup::findiface($1);
	    my $iface2 = libsetup::findiface($2);

	    print DEL "$iface1:$count,$iface2:$count,";
	    $count++;
	}
	print DEL "\n";
	print DEL "sysctl -w net.link.ether.bridge=1\n";
	print DEL "sysctl -w net.link.ether.bridge_ipfw=1\n";
	print DEL "ipfw -f flush\n";

	$count = 69;
	foreach $delay (@delays) {
	    $pat  = q(DELAY INT0=([\d\w]+) INT1=([\d\w]+) );
	    $pat .= q(PIPE0=(\d+) DELAY0=([\d\.]+) BW0=(\d+) PLR0=([\d\.]+) );
	    $pat .= q(PIPE1=(\d+) DELAY1=([\d\.]+) BW1=(\d+) PLR1=([\d\.]+));
	    
	    $delay =~ /$pat/;

	    #
	    # tmcd returns the interfaces as MAC addrs.
	    # 
	    my $iface1 = libsetup::findiface($1);
	    my $iface2 = libsetup::findiface($2);
	    $p1        = $3;
	    $delay1    = $4;
	    $bandw1    = $5;
	    $plr1      = $6;
	    $p2        = $7;
	    $delay2    = $8;
	    $bandw2    = $9;
	    $plr2      = $10;

	    #
	    # Delays are floating point numbers (unit is ms). ipfw does not
	    # support floats, so apply a cheesy rounding function to convert
            # to an integer (since perl does not have a builtin way to
	    # properly round a floating point number to an integer).
	    #
	    $delay1 = int($delay1 + 0.5);
	    $delay2 = int($delay2 + 0.5);

	    print DEL "ifconfig $iface1 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ifconfig $iface2 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ipfw add pipe $p1 ip from any to any in recv $iface1\n";
	    print DEL "ipfw add pipe $p2 ip from any to any in recv $iface2\n";
	    print DEL "ipfw pipe $p1 config delay ${delay1}ms ";
	    print DEL "bw ${bandw1}Kbit/s plr $plr1\n";
	    print DEL "ipfw pipe $p2 config delay ${delay2}ms ";
	    print DEL "bw ${bandw2}Kbit/s plr $plr2\n";

	    print STDOUT "  $iface1/$iface2 pipe $p1 config delay ";
	    print STDOUT "${delay1}ms bw ${bandw1}Kbit/s plr $plr1\n";
	    print STDOUT "  $iface1/$iface2 pipe $p2 config delay ";
	    print STDOUT "${delay2}ms bw ${bandw2}Kbit/s plr $plr2\n";
    
	    $count++;
	}
	#
	# If a delay node, then lets report status and ready in so that batch
	# experiments do not become stuck.
	#
	printf DEL "%s %s\n", libsetup::TMCC(), libsetup::TMCCCMD_READY();
	printf DEL "%s %s 0\n", libsetup::TMCC(),libsetup::TMCCCMD_STARTSTAT();

	print DEL "echo \"Delay Configuration Complete\"\n";
	close(DEL);
	chmod(0755, "$TMDELAY");
    
	#
	# Now do kernel configuration. All of the above work is wasted, but
	# we needed to know the minimum delay. Eventually we will boot the
	# correct kernel to start with via PXE. 
	#
	$kernel = $KERNEL1000;
	$checkreplace = 1;
    }
    else {
	#
	# Make sure we are running the correct non delay kernel. This
	# is really only necessary in the absence of disk reloading.
	# Further, we don't want to replace a user supplied kernel.
	#
	$kernel = $KERNEL100;
	$checkreplace = 0;
	foreach $kern (@KERNELS) {
	    if (system("cmp -s /kernel $kern") == 0) {
		$checkreplace = 1;
	    }
	}
    }

    print STDOUT "Checking kernel configuration ... \n";
    if ($checkreplace) {
	#
	# Make sure we are running the correct kernel. 
	#
	if (-e $kernel) {
	    if (system("cmp -s /kernel $kernel") != 0) {
		if (system("cp -f /kernel /kernel.save")) {
		    print STDOUT
			"Could not backup /kernel! Aborting kernel change\n";
		}
		else {
		    if (system("cp -f $kernel /kernel")) {
			print STDOUT "Could not cp $kernel to /kernel! ".
			    "Aborting kernel change\n";
		    }
		    else {
			system("sync");
			system("reboot");
		    }
		}
	    }
	}
	else {
	    print STDOUT "Kernel $kernel does not exist!\n";
	}
    }

    return 0;
}

1;
