#!/usr/bin/perl -wT
use English;

#
# Initialize at boot time.
#
my $TMCC	= "/etc/testbed/tmcc";
my $TMIFC       = "/etc/testbed/rc.ifc";
my $TMDELAY	= "/etc/testbed/rc.delay";
my $TMRPM       = "/etc/testbed/rc.rpm";
my $TMTARBALLS  = "/etc/testbed/rc.tarballs";
my $TMSTARTUPCMD= "/etc/testbed/startupcmd";
my $TMGROUP     = "/etc/testbed/group";
my $TMPASSWD    = "/etc/testbed/master.passwd";
my $TMHOSTS     = "/etc/testbed/hosts";
my $HOSTSFILE   = "/etc/hosts";
my $TMNICKNAME  = "/etc/testbed/nickname";
my @CONFIGS	= ($TMIFC, $TMDELAY, $TMRPM, $TMSTARTUPCMD, $TMNICKNAME,
		   $TMTARBALLS);
my $REBOOTCMD   = "reboot";
my $STATCMD     = "status";
my $IFCCMD      = "ifconfig";
my $ACCTCMD     = "accounts";
my $DELAYCMD    = "delay";
my $HOSTSCMD    = "hostnames";
my $RPMCMD      = "rpms";
my $TARBALLCMD  = "tarballs";
my $STARTUPCMD  = "startupcmd";
my $DELTACMD    = "deltas";
my $STARTSTATCMD= "startstatus";
my $READYCMD    = "ready";
my $IFCONFIG    = "/sbin/ifconfig fxp%d inet %s netmask %s ".
		  "media 100baseTX mediaopt full-duplex\n";
my $RPMINSTALL  = "/usr/local/bin/rpm -i %s\n";
my $CP		= "/bin/cp -f";
my $MKDB	= "/usr/sbin/pwd_mkdb -p";
my $PW		= "/usr/sbin/pw";
my $CHPASS	= "/usr/bin/chpass";
my $DELTAINSTALL= "/usr/local/bin/install-delta";
my $TARINSTALL  = "/usr/local/bin/install-tarfile";
my $IFACE	= "fxp";
my $CTLIFACENUM = "4";
my $CTLIFACE    = "${IFACE}${CTLIFACENUM}";
my $project     = "";
my $eid         = "";
my $vname       = "";
my $PROJDIR     = "/proj";
my $USERDIR     = "/users";
my $PROJMOUNTCMD= "/sbin/mount fs.emulab.net:/q/$PROJDIR/%s $PROJDIR/%s";
my $USERMOUNTCMD= "/sbin/mount fs.emulab.net:$USERDIR/%s $USERDIR/%s";
my $HOSTNAME    = "%s\t%s-%s %s\n";
my $KERNEL100   = "/kernel.100HZ";
my $KERNEL1000  = "/kernel.1000HZ";
my $KERNEL10000 = "/kernel.10000HZ";
my @KERNELS     = ($KERNEL100, $KERNEL1000, $KERNEL10000); 
my $kernel      = $KERNEL100;

#
# This is a debugging thing for my home network.
# 
my $NODE	= "MYIP=155.101.132.101";
$NODE		= "";

#
# Inform the master we have rebooted.
#
sub inform_reboot()
{
    open(TM, "$TMCC $NODE $REBOOTCMD |")
	or die "Cannot start $TMCC: $!";
    close(TM)
	or die $? ? "$TMCC exited with status $?" : "Error closing pipe: $!";

    return 0;
}

#
# Check node allocation. Returns 0/1 for free/allocated status.
#
sub check_status ()
{
    print STDOUT "Checking Testbed reservation status ... \n";

    open(TM, "$TMCC $NODE $STATCMD |")
	or die "Cannot start $TMCC: $!";
    $_ = <TM>;
    close(TM)
	or die $? ? "$TMCC exited with status $?" : "Error closing pipe: $!";

    if ($_ =~ /^FREE/) {
	print STDOUT "  Free!\n";
	return 0;
    }
    
    if ($_ =~ /ALLOCATED=([-\@\w.]*)\/([-\@\w.]*) NICKNAME=([-\@\w.]*)/) {
	$project = $1;
	$eid     = $2;
	$vname   = $3;
	$nickname= "$vname.$eid.$project";
	print STDOUT "  Allocated! PID: $1, EID: $2, NickName: $nickname\n";
    }
    else {
	die("Error getting reservation status\n");
    }
    return ($project, $eid, $vname);
}

#
# Stick our nickname in a file in case someone wants it.
#
sub create_nicknames()
{
    open(NICK, ">$TMNICKNAME")
	or die("Could not open $TMNICKNAME: $!");
    print NICK "$nickname\n";
    close(NICK);

    return 0;
}

#
# Mount the project directory.
#
sub mount_projdir()
{
    print STDOUT "Mounting the project directory on $PROJDIR/$project ... \n";

    if (! -e "$PROJDIR/$project") {
	if (! mkdir("$PROJDIR/$project", 0770)) {
	    print STDERR "Could not make directory $PROJDIR/$project: $!\n";
	}
    }

    if (system("mount | egrep -s -q ' $PROJDIR/$project '")) {
	if (system(sprintf($PROJMOUNTCMD, $project, $project)) != 0) {
	    print STDERR
		"Could not mount project directory on $PROJDIR/$project.\n";
	}
    }

    return 0;
}

#
# Do interface configuration.    
# Write a file of ifconfig lines, which will get executed.
#
sub doifconfig ()
{
    print STDOUT "Checking Testbed interface configuration ... \n";

    #
    # Open a connection to the TMCD, and then open a local file into which
    # we write ifconfig commands (as a shell script).
    # 
    open(TM,  "$TMCC $NODE $IFCCMD |")
	or die "Cannot start $TMCC: $!";
    open(IFC, ">$TMIFC")
	or die("Could not open $TMIFC: $!");
    print IFC "#!/bin/sh\n";
    
    while (<TM>) {
	$_ =~ /INTERFACE=(\d*) INET=([0-9.]*) MASK=([0-9.]*)/;
	printf STDOUT "  $IFCONFIG", $1, $2, $3;
	printf IFC $IFCONFIG, $1, $2, $3;
    }
    close(TM);
    close(IFC);
    chmod(0755, "$TMIFC");

    return 0;
}

#
# Host names configuration (/etc/hosts). 
#
sub dohostnames ()
{
    print STDOUT "Checking Testbed /etc/hosts configuration ... \n";

    open(TM,  "$TMCC $NODE $HOSTSCMD |")
	or die "Cannot start $TMCC: $!";
    open(HOSTS, ">>$HOSTSFILE")
	or die("Could not open $HOSTSFILE: $!");

    #
    # Now convert each hostname into hosts file representation and write
    # it to the hosts file.
    # 
    while (<TM>) {
	$_ =~ /NAME=([-\@\w.]+) LINK=([0-9]*) IP=([0-9.]*) ALIAS=([-\@\w.]*)/;
	printf STDOUT "  $1, $2, $3, $4\n";
	printf HOSTS  $HOSTNAME, $3, $1, $2, $4;
    }
    close(TM);
    close(HOSTS);

    return 0;
}

#
# Delay node configuration
#
sub dodelays ()
{
    my @delays;
    my $checkreplace;
    
    print STDOUT "Checking Testbed delay configuration ... \n";

    open(TM,  "$TMCC $NODE $DELAYCMD |")
	or die "Cannot start $TMCC: $!";
    while (<TM>) {
	push(@delays, $_);
    }
    close(TM);

    if (@delays) {
	$count    = 69;
	$mindelay = 10000;
    
	open(DEL, ">$TMDELAY")
	    or die("Could not open $TMDELAY");
	print DEL "#!/bin/sh\n";
	print DEL "sysctl -w net.link.ether.bridge=0\n";
	print DEL "sysctl -w net.link.ether.bridge_ipfw=0\n";
	print DEL "sysctl -w net.link.ether.bridge_cfg=${CTLIFACE}:6,";

	foreach $delay (@delays) {
	    $delay =~ /DELAY INT0=(\w+) INT1=(\w+) DELAY/;
	    print DEL "$1:$count,$2:$count,";
	    $count++;
	}
	print DEL "\n";
	print DEL "sysctl -w net.link.ether.bridge=1\n";
	print DEL "sysctl -w net.link.ether.bridge_ipfw=1\n";
	print DEL "ipfw -f flush\n";

	$count = 69;
	$pipe  = 100;
	foreach $delay (@delays) {
	    $delay =~
          /DELAY INT0=(\w+) INT1=(\w+) DELAY=(\d+) BW=([\d\.]+) PLR=([\d\.]+)/;

	    #
	    # tmcd returns the INTs as fxpX. Nice, eh?
	    # 
	    $iface1 = $1;
	    $iface2 = $2;
	    $p1     = $pipe += 10;
	    $p2     = $pipe += 10;
	    $delay  = $3;
	    $bandw  = $4;
	    $plr    = $5;

	    #
	    # We want to know what the minimum delay is so we can boot the
	    # the correct kernel.
	    # 
	    if ($delay < $mindelay) {
		$mindelay = $delay;
	    }

	    print DEL "ifconfig $iface1 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ifconfig $iface2 media 100baseTX mediaopt full-duplex";
	    print DEL "\n";
	    print DEL "ipfw add pipe $p1 ip from any to any in recv $iface1\n";
	    print DEL "ipfw add pipe $p2 ip from any to any in recv $iface2\n";
	    print DEL "ipfw pipe $p1 config delay ${delay}ms ";
	    print DEL "bw ${bandw}Mbit/s plr $plr\n";
	    print DEL "ipfw pipe $p2 config delay ${delay}ms ";
	    print DEL "bw ${bandw}Mbit/s plr $plr\n";

	    print STDOUT "  $iface1/$iface2 pipe $p1/$p2 config delay ";
	    print STDOUT "${delay}ms bw ${bandw}Mbit/s plr $plr\n";
    
	    $count++;
	}
	#
	# If a delay node, then lets report status and ready in so that batch
	# experiments do not become stuck.
	#
	print DEL "$TMCC $READYCMD\n";
	print DEL "$TMCC $STARTSTATCMD 0\n";

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

#
# Account stuff. Again, open a connection to the TMCD, and receive
# ADDGROUP and ADDUSER commands. We turn those into "pw" commands.
#
sub doaccounts ()
{
    print STDOUT "Checking Testbed group/user configuration ... \n";

    open(TM, "$TMCC $NODE $ACCTCMD |")
	or die "Cannot start $TMCC: $!";

    while (<TM>) {
	if ($_ =~ /^ADDGROUP NAME=([-\@\w.]+) GID=([0-9]+)/) {
	    print STDOUT "  Group: $1/$2\n";

	    $group = $1;
	    $gid   = $2;

	    ($exists) = getgrgid($gid);
	    if ($exists) {
		next;
	    }
	
	    if (system("$PW groupadd $group -g $gid") != 0) {
		print STDERR "Error adding new group $1/$2: $!\n";
	    }
	    next;
	}
	if ($_ =~
	    /^ADDUSER LOGIN=([0-9a-z]+) PSWD=([^:]+) UID=(\d+) GID=(\d+) ROOT=(\d) NAME="(.*)"/)
	{
	    $login = $1;
	    $pswd  = $2;
	    $uid   = $3;
	    $gid   = $4;
	    $root  = $5;
	    $name  = $6;
	    if ( $name =~ /^(([^:]+$|^))$/ ) {
		$name = $1;
	    }
	    print STDOUT "  User: $login/$uid/$gid/$root/$name\n";

	    if (! -e "$USERDIR/$login") {
		if (! mkdir("$USERDIR/$login", 0770)) {
		    print STDERR "Could not mkdir $USERDIR/$login: $!\n";
		    next;
		}
	    }

	    if (system("mount | egrep -q -s ' $USERDIR/$login '")) {
		if (system(sprintf($USERMOUNTCMD, $login, $login)) != 0) {
		    print STDERR
			"Could not mount $USERDIR/$login.\n";
		    next;
		}
	    }

	    ($exists) = getpwuid($uid);
	    if ($exists) {
		if ($root) {
		    $GLIST = "-G wheel,$gid";
		}
		else {
		    $GLIST = "-G $gid";
		}
		system("$PW usermod $login $GLIST");
		next;
	    }

	    $GLIST = " ";
	    if ($root) {
		$GLIST = "-G wheel";
	    }

	    if (system("$PW useradd $login -u $uid -g $gid $GLIST ".
		       "-d $USERDIR/$login -s /bin/tcsh -c \"$name\"") != 0) {
		print STDERR "Error adding new user $login\n";
		next;
	    }
	    if (system("$CHPASS -p $pswd $login") != 0) {
		print STDERR "Error changing password for user $login\n";
	    }
	    next;
	}
    }
    close(TM);

    return 0;
}

#
# RPM configuration. Done after account stuff!
#
sub dorpms ()
{
    print STDOUT "Checking Testbed RPM configuration ... \n";

    open(TM,  "$TMCC $NODE $RPMCMD |")
	or die "Cannot start $TMCC: $!";
    open(RPM, ">$TMRPM")
	or die("Could not open $TMRPM: $!");
    print RPM "#!/bin/sh\n";
    
    while (<TM>) {
	if ($_ =~ /RPM=([-\@\w.\/]+)/) {
	    printf STDOUT "  $RPMINSTALL", $1;
	    printf RPM    "echo \"Installing RPM $1\"\n", $1;
	    printf RPM    "$RPMINSTALL", $1;
	}
    }
    close(TM);
    close(RPM);
    chmod(0755, "$TMRPM");

    return 0;
}

#
# TARBALL configuration. Done after account stuff!
#
sub dotarballs ()
{
    my @tarballs = ();

    print STDOUT "Checking Testbed Tarball configuration ... \n";

    open(TM,  "$TMCC $NODE $TARBALLCMD |")
	or die "Cannot start $TMCC: $!";
    while (<TM>) {
	push(@tarballs, $_);
    }
    close(TM);

    if (! @tarballs) {
	return 0;
    }
    
    open(TARBALL, ">$TMTARBALLS")
	or die("Could not open $TMTARBALLS: $!");
    print TARBALL "#!/bin/sh\n";
    
    foreach my $tarball (@tarballs) {
	if ($tarball =~ /DIR=([-\@\w.\/]+)\s+TARBALL=([-\@\w.\/]+)/) {
	    print  STDOUT "  $TARINSTALL $1 $2\n";
	    print  TARBALL  "echo \"Installing Tarball $2 in dir $1 \"\n";
	    print  TARBALL  "$TARINSTALL $1 $2\n";
	}
    }
    close(TARBALL);
    chmod(0755, "$TMTARBALLS");

    return 0;
}

#
# Experiment startup Command.
#
sub dostartupcmd ()
{
    print STDOUT "Checking Testbed Experiment Startup Command ... \n";

    open(TM,  "$TMCC $NODE $STARTUPCMD |")
	or die "Cannot start $TMCC: $!";
    open(RUN, ">$TMSTARTUPCMD")
	or die("Could not open $TMSTARTUPCMD: $!");
    
    while (<TM>) {
	if ($_ =~ /CMD=(\'[-\@\w.\/ ]+\') UID=([0-9a-z]+)/) {
	    print  STDOUT "  Will run $1 as $2\n";
	    print  RUN    "$_\n";
	}
    }
    close(TM);
    close(RUN);
    chmod(0755, "$TMSTARTUPCMD");

    return 0;
}

#
# Install deltas. Return 0 if nothing happened. Return -1 if there was
# an error. Return 1 if deltas installed, which tells the caller to reboot.
#
sub install_deltas ()
{
    my @deltas = ();
    my $reboot = 0;
    
    print STDOUT "Checking Testbed Delta configuration ... \n";

    open(TM,  "$TMCC $DELTACMD |")
	or die "Cannot start $TMCC: $!";
    while (<TM>) {
	push(@deltas, $_);
    }
    close(TM);

    #
    # No deltas. Just exit and let the boot continue.
    #
    if (! @deltas) {
	return 0;
    }

    #
    # Install all the deltas, and hope they all install okay. We reboot
    # if any one does an actual install (they may already be installed).
    # If any fails, then give up.
    # 
    foreach $delta (@deltas) {
	if ($delta =~ /DELTA=([-\@\w.\/]+)/) {
	    print STDOUT  "Installing DELTA $1 ...\n";

	    system("$DELTAINSTALL $1");
	    my $status = $? >> 8;
	    if ($status == 0) {
		$reboot = 1;
	    }
	    else {
		if ($status < 0) {
		    print STDOUT "Failed to install DELTA $1. Help!\n";
		    return -1;
		}
	    }
	}
    }
    return $reboot;
}

#
# If node is free, reset to a moderately clean state.
#
sub cleanup_node () {
    print STDOUT "Cleaning node; removing configuration files ...\n";
    unlink @CONFIGS;

    printf STDOUT "Resetting /etc/hosts file\n";
    if (system("$CP -f $TMHOSTS $HOSTSFILE") != 0) {
	print STDERR "Could not copy default /etc/hosts file into place: $!\n";
	exit(1);
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
	print STDERR "Failure running pwd_mkdb on default password file: $!\n";
	exit(1);
    }
}

1;
