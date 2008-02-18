#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2008 University of Utah and the Flux Group.
# All rights reserved.
#
# Kernel, jail, netstat, route, ifconfig, ipfw, header files.
#
use strict;
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;
use Fcntl ':flock';

# Drag in path stuff so we can find emulab stuff. Also untaints path.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }

use libsetup qw(REMOTE LOCALROOTFS TMTOPOMAP TMLTMAP TMLTPMAP
		TBDebugTimeStamp);
use libtmcc;

#
# Questions:
#
# How to setup the virtual networks?
#
# * vservers provides localhost virtualization
# * use etun devices to give vif inside and outside of vserver
# * bridge outside etuns together as appropriate, possibly including
#   phys interfaces
#
# Optimizations:
# * if link is p2p and all "on node", just use etun pair w/no bridge
#

#
# Create a jailed environment. There are some stub files stored in
# /etc/jail that copied into the jail.
#
sub usage()
{
    print("Usage: mkvserver.pl [-V] [-s] [-i <ipaddr>] [-p <pid>] ".
	  "[-h <hostname>] <vnodeid>\n");
    exit(-1);
}
my  $optlist = "Vi:p:e:sh:";

#
# Only real root can run this script.
#
if ($UID) {
    die("Must be root to run this script!\n");
}

#
# Catch ^C and exit with error. 
#
my $leaveme = 0;
sub handler ($) {
    my ($signame) = @_;
    
    $SIG{INT}  = 'IGNORE';
    $SIG{USR1} = 'IGNORE';
    $SIG{TERM} = 'IGNORE';
    $SIG{HUP}  = 'IGNORE';

    if ($signame eq 'USR1') {
	$leaveme = 1;
    }
    fatal("Caught a SIG${signame}! Killing the vserver ...");
}
$SIG{INT}  = \&handler;
$SIG{USR1} = \&handler;
$SIG{HUP}  = \&handler;
$SIG{TERM} = 'IGNORE';

#
# Turn off line buffering on output
#
STDOUT->autoflush(1);
STDERR->autoflush(1);

# XXX
my $JAILCNET	   = "172.16.0.0";
my $JAILCNETMASK   = "255.240.0.0";

my $USEPROXY	   = 1;
my $USECHPID	   = 0;
if ($USECHPID) {
    $USEPROXY = 0;
}

#
# Locals
#
my $JAILPATH	   = "/var/emulab/jails";
my $ETCVSERVER     = "/usr/local/etc/emulab/vserver";
my $VSERVER	   = "/usr/sbin/vserver";
my $VSERVERUTILS   = "/usr/lib/util-vserver";
my $TMCC	   = "$BINDIR/tmcc";
my $VSERVERDIR     = "/vservers";
my $JAILCONFIG     = "jailconfig";
my @ROOTCPDIRS     = ("etc", "root");
my @ROOTMKDIRS     = ("dev", "tmp", "var", "usr", "proc", "users", "lib",
		      "bin", "sbin", "home", "local");
my @ROOTMNTDIRS    = ("bin", "sbin", "usr", "lib");
my @EMUVARDIRS	   = ("logs", "db", "jails", "boot", "lock");
my $IP;
my $IPMASK;
my $PID;
my $VDIR;
my $CDIR;
my $idnumber;
my $jailhostname;
my $jailpid;
my $tmccpid;
my $debug	   = 1;
my @mntpoints      = ();
my $USEVCNETROUTES = 0;
my $USEHASHIFIED   = 0;
my @controlroutes  = ();
my $interactive    = 0;
my $cleaning	   = 0;

# This stuff is passed from tmcd, which we parse into a config string
# and an option set.
my %jailconfig  = ();
my $sshdport    = 50000;	# Bogus default, good for testing.
my $jailflags   = 0;
my @jailips     = ();		# List of jail IPs (for routing table).

#
# Protos
#
sub mkvserver($);
sub upvserver($);
sub LoopMount($$);
sub PreparePhysNode();
sub PrepareFilesystems();
sub fatal($);
sub getjailconfig($);
sub setjailoptions();
sub startproxy($);
sub mysystem($);
sub cleanup();
sub startproxy($);

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
my %options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV != 1) {
    usage();
}
my $vnodeid = $ARGV[0];

#
# Untaint the arguments.
#
if ($vnodeid =~ /^([-\w\/\.]+)$/) {
    $vnodeid = $1;
    $jailhostname = $1;
}
else {
    die("Tainted argument $vnodeid!\n");
}

if (defined($options{'s'})) {
    $interactive = 1;
}

TBDebugTimeStamp("mkjail starting to do real work");

#
# Get the parent IP.
# 
my $hostname = `hostname`;
my $hostip;

# Untaint and strip newline.
if ($hostname =~ /^([-\w\.]+)$/) {
    $hostname = $1;

    my (undef,undef,undef,undef,@ipaddrs) = gethostbyname($hostname);
    $hostip = inet_ntoa($ipaddrs[0]);
}

#
# If no IP, then it defaults to our hostname's IP, *if* none is provided
# by the jail configuration file. That check is later. 
# 
if (defined($options{'i'})) {
    $IP = $options{'i'};

    if ($IP =~ /^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})$/) {
	$IP = $1;
    }
    else {
	die("Tainted argument $IP!\n");
    }
}

if (defined($options{'p'})) {
    $PID = $options{'p'};

    if ($PID =~ /^([-\@\w]+)$/) {
	$PID = $1;
    }
    else {
	die("Tainted argument $PID.");
    }
}

if (defined($options{'h'})) {
    $jailhostname = $options{'h'};

    if ($jailhostname =~ /^([-\w\.]+)$/) {
	$jailhostname = $1;
    }
    else {
	die("Tainted argument $jailhostname.");
    }
}

#
# In most cases, the $vnodeid directory will have been created by the caller,
# and a config file possibly dropped in.
# When debugging, we have to create it here. 
# 
chdir($JAILPATH) or
    die("Could not chdir to $JAILPATH: $!\n");

if (! -e $vnodeid) {
    mkdir($vnodeid, 0770) or
	fatal("Could not mkdir $vnodeid in $JAILPATH: $!");
}
else {
    TBDebugTimeStamp("mkjail getting jail config");
    getjailconfig("$JAILPATH/$vnodeid");
}

my $phys_cnet_if = `control_interface`;
chomp($phys_cnet_if);

#
# See if special options supported, and if so setup args as directed.
#
TBDebugTimeStamp("mkjail setting jail options");
setjailoptions();

# Do some prep stuff on the physical node if this is the first vserver.
PreparePhysNode();
PrepareFilesystems()
    if ($USEHASHIFIED);

print("Setting up jail for $vnodeid using $IP\n")
    if ($debug && defined($IP));

$VDIR = "$VSERVERDIR/$vnodeid";
$CDIR = "/etc/vservers/$vnodeid";

#
# Create the vserver.
#
if (-e $VDIR) {
    #
    # Try to pick up where we left off.
    #
    TBDebugTimeStamp("mkjail restoring root fs");
    upvserver("$vnodeid");
}
else {
    #
    # Create the root filesystem.
    #
    TBDebugTimeStamp("Creating vserver");
    mkvserver("$vnodeid");
}
TBDebugTimeStamp("mkjail done with root fs");
if ($USEPROXY) {
    startproxy("$vnodeid");
}

#
# Start the vserver. If all goes well, this will exit cleanly, with the
# vserver running in its new context. Still, lets protect it with a timer
# since it might get hung up inside and we do not want to get stuck here.
#
my $childpid = fork();
if ($childpid) {
    #local $SIG{ALRM} = sub { kill("TERM", $childpid); };
    #alarm 30;
    waitpid($childpid, 0);
    #alarm 0;

    #
    # If failure then cleanup.
    #
    if ($?) {
	fatal("mkvserver: $vnodeid vserver startup exited with $?");
    }
}
else {
    $SIG{TERM} = 'DEFAULT';

    TBDebugTimeStamp("mkvserver: starting the vserver");

    exec("$VSERVER $vnodeid start");
    die("*** $0:\n".
	"    exec failed to start the vserver!\n");
}

#
# If this file does not exist, the inner setup failed somehow. Stop now.
#
fatal("vserver did not appear to set up properly. Exiting ...")
    if (! -e "$VDIR/$BOOTDIR/vrunning");

print "vserver for $vnodeid started. Waiting ...\n";
$jailpid = fork();
if ($jailpid) {
    #
    # We do not really care about the exit status of the jail, we just want
    # to know when it dies inside.
    #
    while (1) {
	my $kidpid = waitpid(-1, 0);

	if ($kidpid == $jailpid) {
	    undef($jailpid);
	    last;
	}
	if ($USEPROXY && $kidpid == $tmccpid) {
	    print("TMCC proxy exited with status $?. Restarting ...\n");
	    startproxy("$vnodeid");
	    next;
	}
	print("Unknown child $kidpid exited with status $?!\n");
    }
}
else {
    $SIG{TERM} = 'DEFAULT';

    exec("$VSERVER $vnodeid exec sleep 1000000");
    die("*** $0:\n".
	"    exec failed to start the jail!\n");
}
print "vserver for $vnodeid has died. Cleaning up ...\n";
cleanup();
exit(0);

#
# Create a root filesystem for the vserver.
#
sub mkvserver($)
{
    my ($vnodeid) = @_;
    my $interface;

    # The filesystem for the vserver lands here.
    my $vdir = $VDIR;
    # The configuration directory is here.
    my $cdir = $CDIR;

    if (defined($IP)) {
	system("$VSERVERUTILS/mask2prefix $JAILCNETMASK");
	my $prefix = $? >> 8;
	$interface = "${vnodeid}=${phys_cnet_if}:${IP}/${prefix}";
    }
    else {
	$interface = "nodev:0.0.0.0/0";
    }

    my $enetifs = "";

    #
    # XXX still need code to create etun devices outside the vserver.
    # To create a pair you do:
    #    echo etun0,etun1 > /sys/module/etun/parameters/newif
    # To destroy do:
    #    echo etun0 > /sys/module/etun/parameters/delif
    # (just need to specify one end).  Apparently you can call these things
    # whatever you want (e.g., "veth").
    #
    # Then configure the IFs with appropriate IPs.
    #
if (0) {
    foreach my $ip (@jailips) {
	my $iface = `$BINDIR/findif -i $ip`;
	chomp($iface);
	if (!$iface) {
	    fatal("Could not find interface for jailIP $ip");
	}
	$enetifs .= " --interface ${iface}:${ip}/24";
    }
}

    # Create the skeleton vserver. It will be mostly empty.
    mysystem("$VSERVER $vnodeid build --force ".
	     "-m " . ($USEHASHIFIED ? "clone " : "skeleton ") .
	     "--hostname $jailhostname --interface $interface ".
	     "$enetifs --flags persistent ".
	     ($USEHASHIFIED ? "-- --source /vservers/root" : ""));

    #
    # Copy in the top level directories. 
    #
    foreach my $dir (@ROOTCPDIRS) {
	mysystem("rsync -a /$dir $vdir");
    }
    TBDebugTimeStamp("mkvserver: Copying root cp dirs done!");

    #
    # Set vserver capabilities and flags
    #
    # NET_BIND_SERVICE: Allows binding to TCP/UDP sockets below 1024
    # LBACK_REMAP:      Virtualize the loopback device
    # HIDE_LBACK:       Hide real address used for loopback
    # HIDE_NETIF:       Hide "foreign" network interfaces
    mysystem("echo 'NET_BIND_SERVICE' >> $cdir/bcapabilities");
    mysystem("echo 'LBACK_REMAP' >> $cdir/nflags");
    mysystem("echo 'HIDE_LBACK' >> $cdir/nflags");
    mysystem("echo 'HIDE_NETIF' >> $cdir/nflags");

    # XXX needed to do clone with CLONE_NEWNET
    mysystem("echo 'SYS_ADMIN' >> $cdir/bcapabilities");

    if (!$USEHASHIFIED) {
	#
	# Copy in the top level directories. 
	#
	foreach my $dir (@ROOTCPDIRS) {
	    mysystem("rsync -a /$dir $vdir");
	}
	TBDebugTimeStamp("mkvserver: Copying root cp dirs done!");

	#
	# Make some other directories that are need in /root.
	#
	foreach my $dir (@ROOTMKDIRS) {
	    if (! -e "$vdir/$dir") {
		mkdir("$vdir/$dir", 0755) or
		    fatal("Could not mkdir '$dir' in $vdir: $!");
	    }
	}

	#
	# Mount (read-only) these other directories to save space.
	#
	foreach my $dir (@ROOTMNTDIRS) {
	    LoopMount("/$dir", "$vdir/$dir");
	}

	#
	# Duplicate the /var hierarchy without the contents.
	#
	open(VARDIRS, "find /var -type d -print |")
	    or fatal("Could not start find on /var");
	while (<VARDIRS>) {
	    my $dir = $_;
	    chomp($dir);

	    mysystem("rsync -dlptgoD $dir $vdir$dir");
	}
	close(VARDIRS);

	#
	# Get a list of all the plain files and create zero length versions
	# in the new var.
	#
	opendir(DIR, "/var/log") or
	    fatal("Cannot opendir /var/log: $!");
	my @logs = grep { -f "/var/log/$_" } readdir(DIR);
	closedir(DIR);

	foreach my $log (@logs) {
	    mysystem("touch $vdir/var/log/$log");
	}
	TBDebugTimeStamp("mkvserver: creating root filesystem done.");
    }

    #
    # Clean out some stuff from /etc.
    #
    mysystem("/bin/rm -rf $vdir/etc/rc.d/rc*.d/*");

    # /tmp is special.
    mysystem("chmod 1777 $vdir/tmp");

    # Create a local logs directory. 
    mysystem("mkdir -p $vdir/local/logs")
	if (! -e "$vdir/local/logs");

    #
    # Some security stuff; remove files that would enable it to talk to
    # tmcd directly (only important on remote nodes). Must go through proxy.
    #
    if ($USEPROXY) {
	mysystem("rm -f $vdir/$ETCDIR/*.pem");
    }
    
    #
    # Now a bunch of stuff to set up a nice environment in the jail.
    #
    mysystem("ln -s ../../init.d/syslog $vdir/etc/rc3.d/S80syslog");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc3.d/S85sshd");
    mysystem("ln -s ../../init.d/syslog $vdir/etc/rc6.d/K85syslog");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc6.d/K80sshd");
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc3.d/S99invserver");
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc6.d/K99invserver");
    if ($USECHPID) {
	#
	# this script comes from tmcd/linux/vserver0.sh
	# assumes chpid and vserver1.sh have been installed in $BINDIR
	# (not part of makefile yet)
	#
        mysystem("ln -s ../../init.d/chpid $vdir/etc/rc3.d/S00chpid");
	mysystem("ln -s ../../init.d/chpid $vdir/etc/rc6.d/K00chpid");
    }

    # Kill anything that uses /dev/console in syslog; will not work.
    mysystem("sed -i.bak ".
	     "-e '/console/d' ".
	     "-e '/users/d' ".
	     "$vdir/etc/syslog.conf");
    
    # Premunge the sshd config: no X11 forwarding and get rid of old
    # ListenAddresses.
    #
    mysystem("sed -i.bak ".
	     "-e 's/^X11Forwarding.*yes/X11Forwarding no/' ".
	     "-e '/^ListenAddress/d' ".
	     "$vdir/etc/ssh/sshd_config");
    
    # Port/Address for sshd.
    if (defined($IP) && $IP ne $hostip) {
	# XXX This can come out (sshd bind to port 22) once we have
	# the network stack stuff in the kernel.
	mysystem("echo 'Port $sshdport' >> $vdir/etc/ssh/sshd_config");

	mysystem("echo 'ListenAddress $IP' >> ".
		 "$vdir/etc/ssh/sshd_config");
	foreach my $ip (@jailips) {
	    mysystem("echo 'ListenAddress $ip' >> ".
		     "$vdir/etc/ssh/sshd_config");
	}
    }
    else {
	mysystem("echo 'Port $sshdport' >> $vdir/etc/ssh/sshd_config");
    }

    tmcccopycache($vnodeid, "$vdir");
    if (-e TMTOPOMAP()) {
	mysystem("cp -fp " . TMTOPOMAP() . " $vdir/var/emulab/boot");
    }
    if (-e TMLTMAP()) {
	mysystem("cp -fp " . TMLTMAP() . " $vdir/var/emulab/boot");
    }
    if (-e TMLTPMAP()) {
	mysystem("cp -fp " . TMLTPMAP() . " $vdir/var/emulab/boot");
    }

    #
    # Stash the control net IP if not the same as the host IP
    #
    if ($IP ne $hostip) {
	mysystem("echo $IP > $vdir/var/emulab/boot/myip");
    }

    #
    # This is picked up by rc.invserver and put into the environment (tmcc).
    #
    mysystem("echo $vnodeid > $vdir/var/emulab/boot/realname");

    #
    # Password/group file stuff. If remote the jail picks them up. Locally
    # we start with current set of accounts on the physnode, since this is
    # more efficient (less tmcd work).
    #
    if (REMOTE()) {
	mysystem("cp -p $ETCDIR/group $ETCDIR/passwd $vdir/etc");
	mysystem("cp -p $ETCDIR/gshadow $ETCDIR/shadow $vdir/etc");
    }
    else {
	# Already copied /etc above, but also need the .db files so that
	# inner rc.accounts knows.
	mysystem("cp -p $DBDIR/passdb $DBDIR/groupdb $vdir/$DBDIR");
    }

    #
    # Mount the usual directories inside the jail. Use linux loopback
    # mounts which will remount an NFS filesystem.
    #
    if (! REMOTE()) {
	mysystem("$BINDIR/rc/rc.mounts -j $vnodeid $vdir 0 boot");
    }
    TBDebugTimeStamp("mkvserver: mounting NFS filesystems done");

    # Remove this file; rc.invserver will create it at the very end.
    # which indicates the inner environment is running reasonably well.
    unlink("$vdir/$BOOTDIR/vrunning")
	if (-e "$vdir/$BOOTDIR/vrunning");

}

#
# Reuse existing vserver.
#
sub upvserver($)
{
    my ($vnodeid) = @_;

    # The filesystem for the vserver lands here.
    my $vdir = $VDIR;

    tmcccopycache($vnodeid, "$vdir");
    if (-e TMTOPOMAP()) {
	mysystem("cp -fp " . TMTOPOMAP() . " $vdir/var/emulab/boot");
    }
    if (-e TMLTMAP()) {
	mysystem("cp -fp " . TMLTMAP() . " $vdir/var/emulab/boot");
    }
    if (-e TMLTPMAP()) {
	mysystem("cp -fp " . TMLTPMAP() . " $vdir/var/emulab/boot");
    }

    #
    # This is picked up by rc.invserver and put into the environment (tmcc).
    #
    mysystem("echo $vnodeid > $vdir/var/emulab/boot/realname");

    #
    # Mount (read-only) these other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	LoopMount("/$dir", "$vdir/$dir");
    }

    #
    # Mount the usual directories inside the jail. Use linux loopback
    # mounts which will remount an NFS filesystem.
    #
    if (! REMOTE()) {
	mysystem("$BINDIR/rc/rc.mounts -j $vnodeid $vdir 0 boot");
    }

    # Remove this file; rc.invserver will create it at the very end.
    # which indicates the inner environment is running reasonably well.
    unlink("$vdir/$BOOTDIR/vrunning")
	if (-e "$vdir/$BOOTDIR/vrunning");
}

#
# Start the tmcc proxy and insert the unix path into the environment
# for the jail to pick up.
#
sub startproxy($)
{
    my ($vnodeid) = @_;

    #
    # The point of these paths is so that there is a comman path to
    # the socket both outside and inside the jail.
    #
    my $insidepath  = "/var/emulab/tmcc";
    my $outsidepath = "${VDIR}${insidepath}";
    my $log         = "$JAILPATH/$vnodeid/tmcc.log";

    $tmccpid = fork();
    if ($tmccpid) {
	#
	# Create a proxypath file so that libtmcc in the jail will know to
	# use a proxy for all calls; saves having all clients explicitly
	# use -l for every tmcc call.
	#
	mysystem("echo $insidepath > $VDIR/${BOOTDIR}/proxypath");
	
	select(undef, undef, undef, 0.2);
	return 0;
    }
    $SIG{TERM} = 'DEFAULT';

    # The -o option will cause the proxy to detach but not fork.
    # Eventually change this to standard pid file kill.
    my $cmd = "$TMCC -d -x $outsidepath -n $vnodeid -o $log";
    print "$cmd\n"
	if ($debug);
    exec("$TMCC -d -x $outsidepath -n $vnodeid -o $log");
    die("Exec of $TMCC failed! $!\n");
}

#
# Read in the jail config file. 
#
sub getjailconfig($)
{
    my ($path) = @_;

    $path .= "/$JAILCONFIG";

    if (! -e $path) {
	return 0;
    }
    
    if (! open(CONFIG, $path)) {
	print("$path could not be opened for reading: $!\n");
	return -1;
    }
    while (<CONFIG>) {
	if ($_ =~ /^(.*)="(.*)"$/ ||
	    $_ =~ /^(.*)=(.+)$/) {
		$jailconfig{$1} = $2
		    if ($2 ne "");
	}
    }
    close(CONFIG);
    return 0;
}

#
# See if special jail opts supported.
#
sub setjailoptions() {
    my $portrange;
    
    #
    # Do this all the time, so that we can figure out the sshd port.
    # 
    foreach my $key (keys(%jailconfig)) {
	my $val = $jailconfig{$key};

        SWITCH: for ($key) {
	    /^JAILIP$/ && do {
		if ($val =~ /([\d\.]+),([\d\.]+)/) {
		    # Set the jail IP, but do not override one on the 
		    # command line.
		    if (!defined($IP)) {
			$IP      = $1;
			$IPMASK  = $2;
		    }
		}
		last SWITCH;
	    };
	    /^SSHDPORT$/ && do {
		if ($val =~ /(\d+)/) {
		    $sshdport     = $1;
		}
		last SWITCH;
	    };
 	    /^IPADDRS$/ && do {
		# Comma separated list of IPs
		my @iplist = split(",", $val);

		foreach my $ip (@iplist) {
		    if ($ip =~ /(\d+\.\d+\.\d+\.\d+)/) {
			push(@jailips, $1);
		    }
		}
 		last SWITCH;
 	    };
	}
    }
    return 0;
}

#
# Setup the physical node.
#
sub PreparePhysNode()
{
    mysystem("/etc/init.d/util-vserver start");
    mysystem("/etc/init.d/vprocunhide start");
}

#
# Loopback mount a directory.
#
sub LoopMount($$)
{
    my ($old, $new) = @_;
    
    mysystem("mount --bind $old $new");
    push(@mntpoints, $new);
}

#
# Print error and exit.
#
sub fatal($)
{
    my ($msg) = @_;

    cleanup();
    die("*** $0:\n".
	"    $msg\n");
}

#
# Cleanup at exit.
#
sub cleanup()
{
    my $i = 0;
    
    if ($cleaning) {
	die("*** $0:\n".
	    "    Oops, already cleaning!\n");
    }
    $cleaning = 1;

    if (defined($tmccpid)) {
	kill('TERM', $tmccpid);
	waitpid($tmccpid, 0);
    }

    #
    # A vserver is not killed by sending it a signal.
    # Must use the stop button, which enters the vserver and takes it down.
    #
    if (defined($jailpid)) {
	system("$VSERVER $vnodeid stop");
	waitpid($jailpid, 0);
	undef($jailpid);
    }
    elsif (system("$VSERVER $vnodeid running") == 0) {
	system("$VSERVER $vnodeid stop");
    }
    # Wait for the status to actually change. Otherwise umounts will fail.
    for ($i = 0; $i < 30; $i++) {
	system("$VSERVER $vnodeid running");
	last
	    if ($?);
	sleep(1);
    }
    if ($i == 30) {
	die("*** $0:\n".
	    "    vserver $vnodeid status did not change to stopped\n");
    }

    if (!REMOTE()) {
	# If this fails, fail, we do want to continue. Dangerous!
	system("$BINDIR/rc/rc.mounts -j $vnodeid foo 0 shutdown");

	if ($?) {
	    # Avoid recursive calls to cleanup; do not use fatal.
	    die("*** $0:\n".
		"    rc.mounts failed!\n");
	}
    }
    while (@mntpoints) {
	my $mntpoint = pop(@mntpoints);

	# If the umounts fail, we do want to continue. Dangerous!
	system("umount $mntpoint");
	if ($?) {
	    # Avoid recursive calls to cleanup; do not use fatal.
	    die("*** $0:\n".
		"    umount '$mntpoint' failed: $!\n");
	}
    }
    if (!$leaveme) {
        #
        # Ug, with NFS mounts inside the jail, we need to be really careful.
        #
	system("/bin/rm -rf $VSERVERDIR/$vnodeid");
    }
}

#
# Run a command string, redirecting output to a logfile.
#
sub mysystem($)
{
    my ($command) = @_;

    print "system('$command')\n"
	if ($debug);
    
    system($command);
    if ($?) {
	fatal("Command failed: $? - $command");
    }
}

#
# Create the hashified filesystems that will be used in the vservers.
# This is done just once when the system boots and the hashified copy
# does not yet exist.
#
sub PrepareFilesystems()
{
    return 0
	if (-e "$VSERVERDIR/hashed");

    #
    # First create the copy of the root filesystems.
    #
    if (! -e "$VSERVERDIR/root") {
	mysystem("mkdir $VSERVERDIR/root") == 0
	    or return -1;

	foreach my $dir (@ROOTMKDIRS) {
	    if (! -e "$VSERVERDIR/root/$dir") {
		mkdir("$VSERVERDIR/root/$dir", 0755) or
		    fatal("Could not mkdir '$dir' in $VSERVERDIR/root: $!");
	    }
	}

	print "Making a copy of root directories to $VSERVERDIR/root\n";
	foreach my $dir (@ROOTMNTDIRS) {
	    mysystem("rsync -a /$dir $VSERVERDIR/root") == 0
		or return -1;
	}
    }

    #
    # Create the hashified copy of it.
    #
    mysystem("mkdir -p $VSERVERDIR/hashed/root") == 0
	or return -1;

    print "Making a hashied copy of $VSERVERDIR/root $VSERVERDIR/hashed\n";
    mysystem("$VSERVERUTILS/vhashify --manually ".
	     "--destination /vservers/hashed /vservers/root ''")
	== 0 or return -1;

    #
    # Now copy in the stuff that we do not want unified; each vserver
    # gets a copy.
    #
    # Copy in the top level directories. 
    #
    print "Copying the rest of the root directories to $VSERVERUTILS/root\n";
    foreach my $dir (@ROOTCPDIRS) {
	mysystem("rsync -a /$dir $VSERVERDIR/root") == 0
	    or return -1;
    }

    #
    # Duplicate the /var hierarchy without the contents.
    #
    open(VARDIRS, "find /var -type d -print |")
	or fatal("Could not start find on /var");
    while (<VARDIRS>) {
	my $dir = $_;
	chomp($dir);

	mysystem("rsync -dlptgoD $dir $VSERVERDIR/root${dir}");
    }
    close(VARDIRS);

    #
    # Get a list of all the plain files and create zero length versions
    # in the new var.
    #
    opendir(DIR, "/var/log") or
	fatal("Cannot opendir /var/log: $!");
    my @logs = grep { -f "/var/log/$_" } readdir(DIR);
    closedir(DIR);
    
    foreach my $log (@logs) {
	mysystem("touch $VSERVERDIR/root/var/log/$log");
    }
    return 0;
}
