#!/usr/pkg/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

use English;

#
# Initialize at boot time.
#
my $TMCC	= "/etc/testbed/tmcc";
my $TMIFC       = "/etc/testbed/rc.ifc";
my $TMRPM       = "/etc/testbed/rc.rpm";
my $TMSTARTUPCMD= "/etc/testbed/startupcmd";
my $TMGROUP     = "/etc/testbed/group";
my $TMPASSWD    = "/etc/testbed/master.passwd";
my $TMHOSTS     = "/etc/testbed/hosts";
my $HOSTSFILE   = "/etc/hosts";
my $TMNICKNAME  = "/etc/testbed/nickname";
my @CONFIGS	= ($TMIFC, $TMRPM, $TMSTARTUPCMD, $TMNICKNAME);
my $REBOOTCMD   = "reboot";
my $STATCMD     = "status";
my $IFCCMD      = "ifconfig";
my $ACCTCMD     = "accounts";
my $HOSTSCMD    = "hostnames";
my $RPMCMD      = "rpms";
my $STARTUPCMD  = "startupcmd";
my $IFCONFIG    = "/sbin/ifconfig cs%d alias %s netmask %s ".
		  "media 10baseT mediaopt full-duplex\n";
my $CP		= "/bin/cp -f";
my $MKDB	= "/usr/sbin/pwd_mkdb -p";
my $USERADD	= "/usr/sbin/useradd";
my $USERMOD	= "/usr/sbin/usermod";
my $GROUPADD	= "/usr/sbin/groupadd";
my $IFACE	= "cs";
my $CTLIFACENUM = "0";
my $CTLIFACE    = "${IFACE}${CTLIFACENUM}";
my $project     = "";
my $eid         = "";
my $vname       = "";
my $PROJDIR     = "/proj";
my $USERDIR     = "/users";
my $PROJMOUNTCMD= "/sbin/mount fs.emulab.net:/q/$PROJDIR/%s $PROJDIR/%s";
my $USERMOUNTCMD= "/sbin/mount fs.emulab.net:$USERDIR/%s $USERDIR/%s";
my $HOSTNAME    = "%s\t%s-%s %s\n";

#
# This is a debugging thing for my home network.
# 
my $NODE	= "MYIP=155.99.214.136";
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

1;
