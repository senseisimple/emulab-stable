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
    qw ( $CP $EGREP $MOUNT $UMOUNT $TMPASSWD $SFSSD $SFSCD $TMDELMAP
	 os_cleanup_node os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_useradd os_userdel os_usermod os_mkdir
	 os_rpminstall_line update_delays
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

#
# Various programs and things specific to FreeBSD and that we want to export.
# 
$CP		= "/bin/cp";
$EGREP		= "/usr/bin/egrep -s -q";
$MOUNT		= "/sbin/mount";
$UMOUNT		= "/sbin/umount";
$TMPASSWD	= "$ETCDIR/master.passwd";
$SFSSD		= "/usr/local/sbin/sfssd";
$SFSCD		= "/usr/local/sbin/sfscd";
$TMDELMAP	= "$BOOTDIR/delay_mapping";

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
my $IFCONFIG    = "/sbin/ifconfig %s inet %s netmask %s %s %s";
my $IFC_100MBS  = "media 100baseTX";
my $IFC_10MBS   = "media 10baseT/UTP";
my $IFC_FDUPLEX = "mediaopt full-duplex";
my $RPMINSTALL  = "/usr/local/bin/rpm -i %s";
my $MKDIR	= "/bin/mkdir";
my $GATED	= "/usr/local/sbin/gated";
my $ROUTE	= "/sbin/route";

#
# Delay node configuration goop.
#
my $KERNEL100   = "/kernel.100HZ";
my $KERNEL1000  = "/kernel.1000HZ";
my $KERNEL10000 = "/kernel.10000HZ";
my $KERNELDELAY = "/kernel.delay";	# New images. Linked to kernel.10000HZ
my @KERNELS     = ($KERNEL100, $KERNEL1000, $KERNEL10000, $KERNELDELAY); 
my $kernel      = $KERNEL100;
my $TMDELAY	= "$BOOTDIR/rc.delay";
my $TMCCCMD_DELAY = "delay";

#
# OS dependent part of cleanup node state.
# 
sub os_cleanup_node ($) {
    my ($scrub) = @_;

    if (REMOTE()) {
	return 0;
    }

    unlink $TMDELAY;
    unlink $TMDELMAP;

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
sub os_usermod($$$$$)
{
    my($login, $gid, $glist, $pswd, $root) = @_;

    if ($root) {
	$glist = join(',', split(/,/, $glist), "wheel");
    }
    if ($glist ne "") {
	$glist = "-G $glist";
    }

    if (system("$CHPASS '$pswd' $login") != 0) {
	warn "*** WARNING: $CHPASS $login error.\n";
	return -1;
    }
    return system("$USERMOD $login -g $gid $glist");
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root) = @_;
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
    }

    if (system("$USERADD $login -u $uid -g $gid $args ".
	       "-m -s /bin/tcsh -c \"$gcos\"") != 0) {
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
    print STDOUT "Checking Testbed delay configuration ... \n";
    dodelays();
}

#
# Special update.
#
sub update_delays()
{
    # This should never happen!
    if (REMOTE() || MFS()) {
	print "Ignoring update delay on remote/MFS node!\n";
	return 0;
    }
    dodelays();
    system($TMDELAY);
    return 0;
}
    
sub dodelays ()
{
    my @delays;
    my $checkreplace;

    my $TM = libsetup::OPENTMCC($TMCCCMD_DELAY);
    while (<$TM>) {
	push(@delays, $_);
    }
    libsetup::CLOSETMCC($TM);

    if (@delays) {
	$count    = 69;
	$mindelay = 10000;

	open(MAP, ">$TMDELMAP")
	    or die("Could not open $TMDELMAP");
    
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

 	#
 	# If we are in a new-style image that uses a polling-based kernel
 	# turn on polling.
 	#
 	if (-e $KERNELDELAY) {
 	    print DEL "sysctl -w kern.polling.enable=1\n";
 	}

	$count = 69;
	foreach $delay (@delays) {
	    $pat  = q(DELAY INT0=([\d\w]+) INT1=([\d\w]+) );
	    $pat .= q(PIPE0=(\d+) DELAY0=([\d\.]+) BW0=(\d+) PLR0=([\d\.]+) );
	    $pat .= q(PIPE1=(\d+) DELAY1=([\d\.]+) BW1=(\d+) PLR1=([\d\.]+) );
	    $pat .= q(LINKNAME=([-\d\w]+) );
	    $pat .= q(RED0=(\d) RED1=(\d) );
	    $pat .= q(LIMIT0=(\d+) );
	    $pat .= q(MAXTHRESH0=(\d+) MINTHRESH0=(\d+) WEIGHT0=([\d\.]+) );
	    $pat .= q(LINTERM0=(\d+) QINBYTES0=(\d+) BYTES0=(\d+) );
	    $pat .= q(MEANPSIZE0=(\d+) WAIT0=(\d+) SETBIT0=(\d+) );
	    $pat .= q(DROPTAIL0=(\d+) GENTLE0=(\d+) );
	    $pat .= q(LIMIT1=(\d+) );
	    $pat .= q(MAXTHRESH1=(\d+) MINTHRESH1=(\d+) WEIGHT1=([\d\.]+) );
	    $pat .= q(LINTERM1=(\d+) QINBYTES1=(\d+) BYTES1=(\d+) );
	    $pat .= q(MEANPSIZE1=(\d+) WAIT1=(\d+) SETBIT1=(\d+) );
	    $pat .= q(DROPTAIL1=(\d+) GENTLE1=(\d+));
	    
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
	    $linkname  = $11;
	    $red1      = $12;
	    $red2      = $13;
		
	    #
	    # Only a few of these NS RED params make sense for dummynet,
	    # but they all come through; someday they might be used.
	    #
	    $limit1     = $14;
	    $maxthresh1 = $15;
	    $minthresh1 = $16;
	    $weight1    = $17;
	    $linterm1   = $18;
	    $qinbytes1  = $19;
	    $bytes1     = $20;
	    $meanpsize1 = $21;
	    $wait1      = $22;
	    $setbit1    = $23;
	    $droptail1  = $24;
	    $gentle1    = $25;
	    $limit2     = $26;
	    $maxthresh2 = $27;
	    $minthresh2 = $28;
	    $weight2    = $29;
	    $linterm2   = $30;
	    $qinbytes2  = $31;
	    $bytes2     = $32;
	    $meanpsize2 = $33;
	    $wait2      = $34;
	    $setbit2    = $35;
	    $droptail2  = $36;
	    $gentle2    = $37;

	    #
	    # Delays are floating point numbers (unit is ms). ipfw does not
	    # support floats, so apply a cheesy rounding function to convert
            # to an integer (since perl does not have a builtin way to
	    # properly round a floating point number to an integer).
	    #
	    $delay1 = int($delay1 + 0.5);
	    $delay2 = int($delay2 + 0.5);

	    #
	    # Qsizes are in slots or packets. My perusal of the 4.3 code
	    # shows the limits are 50 < slots <= 100 or 0 <= bytes <= 1MB.
	    #
	    my $queue1 = "";
	    my $queue2 = "";
	    if ($qinbytes1) {
		if ($limit1 <= 0 || $limit1 > (1024 * 1024)) {
		    print "Q limit $limit1 for pipe $p1 is bogus.\n";
		}
		else {
		    $queue1 = "queue ${limit1}bytes";
		}
	    }
	    elsif ($limit1 != 0) {
		if ($limit1 < 0 || $limit1 > 100) {
		    print "Q limit $limit1 for pipe $p1 is bogus.\n";
		}
		else {
		    $queue1 = "queue $limit1";
		}
	    }
	    if ($qinbytes2) {
		if ($limit2 <= 0 || $limit2 > (1024 * 1024)) {
		    print "Q limit $limit2 for pipe $p2 is bogus.\n";
		}
		else {
		    $queue2 = "queue ${limit2}bytes";
		}
	    }
	    elsif ($limit2 != 0) {
		if ($limit2 < 0 || $limit2 > 100) {
		    print "Q limit $limit2 for pipe $p2 is bogus.\n";
		}
		else {
		    $queue2 = "queue $limit2";
		}
	    }

	    #
	    # RED/GRED stuff
	    #
	    my $redparams1 = "";
	    my $redparams2 = "";
	    if ($red1) {
		if ($gentle1) {
		    $redparams1 = "gred ";
		}
		else {
		    $redparams1 = "red ";
		}
		my $max_p = 1 / $linterm1;
		$redparams1 .= "$weight1/$minthresh1/$maxthresh1/$max_p";
	    }
	    if ($red2) {
		if ($gentle2) {
		    $redparams2 = "gred ";
		}
		else {
		    $redparams2 = "red ";
		}
		my $max_p = 1 / $linterm2;
		$redparams2 .= "$weight2/$minthresh2/$maxthresh2/$max_p";
	    }

	    print DEL "ifconfig $iface1 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ifconfig $iface2 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ipfw add pipe $p1 ip from any to any in recv $iface1\n";
	    print DEL "ipfw add pipe $p2 ip from any to any in recv $iface2\n";
	    print DEL "ipfw pipe $p1 config delay ${delay1}ms ";
	    print DEL "bw ${bandw1}Kbit/s plr $plr1 $queue1 $redparams1\n";
	    print DEL "ipfw pipe $p2 config delay ${delay2}ms ";
	    print DEL "bw ${bandw2}Kbit/s plr $plr2 $queue2 $redparams2\n";

	    print STDOUT "  $iface1/$iface2 pipe $p1 config delay ";
	    print STDOUT "${delay1}ms bw ${bandw1}Kbit/s plr $plr1 ";
	    print STDOUT "$queue1 $redparams1\n";
	    print STDOUT "  $iface1/$iface2 pipe $p2 config delay ";
	    print STDOUT "${delay2}ms bw ${bandw2}Kbit/s plr $plr2 ";
	    print STDOUT "$queue2 $redparams2\n";

	    print MAP "$linkname duplex $iface1 $iface2 $p1 $p2\n";

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
	close(MAP);
    
	#
	# Now do kernel configuration. All of the above work is wasted,
	# but such is life.
	#
 	if (-e $KERNELDELAY) {
 	    $kernel = $KERNELDELAY;
 	} else {
 	    $kernel = $KERNEL1000;
 	}
	$checkreplace = 1;
    }
    else {
	#
	# Make sure we are running the correct non delay kernel. Thi
	# is really only necessary in the absence of disk reloading.
	# Further, we don't want to replace a user supplied kernel.
	#
	$kernel = $KERNEL100;
	$checkreplace = 0;
	foreach $kern (@KERNELS) {
	    if (-e $kern && system("cmp -s /kernel $kern") == 0) {
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
			#
			# Make sure that, even if the reboot command returns
			# before the node is totally down, this process doesn't
			# exit (otherwise, we would proceed with testbed setup)
			#
			sleep(10000);
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
	$cmd = "sysctl -w net.inet.ip.forwarding=1\n";
	$cmd .= "sysctl -w net.inet.ip.fastforwarding=1";
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
