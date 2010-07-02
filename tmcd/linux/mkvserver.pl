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
use POSIX ':sys_wait_h';

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
my $JAILROUTER	   = "172.16.0.1";

my $MOUNTELABFS	   = 1;
my $USEPROXY	   = 1;
my $USENEWNET	   = 1;
if ($USENEWNET) {
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
my $JDIR;
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

my $cnet_bridge_dev = "cbr0";

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
sub SetupCnetBridge();
sub CreateCnet($);
sub DestroyCnet($);
sub InjectIFs($);
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

TBDebugTimeStamp("mkvserver starting to do real work");

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
    TBDebugTimeStamp("mkvserver getting jail config");
    getjailconfig("$JAILPATH/$vnodeid");
}

my $phys_cnet_if = `control_interface`;
chomp($phys_cnet_if);

if (! -r "$BOOTDIR/myip" ||
    ! -r "$BOOTDIR/mynetmask" ||
    ! -r "$BOOTDIR/routerip") {
    fatal("Need control net IP/mask/router for setting up vnode cnet");
}
my $cnet_ip = `cat $BOOTDIR/myip`;
chomp($cnet_ip);
my $cnet_mask = `cat $BOOTDIR/mynetmask`;
chomp($cnet_mask);
my $cnet_router = `cat $BOOTDIR/routerip`;
chomp($cnet_router);

#
# See if special options supported, and if so setup args as directed.
#
TBDebugTimeStamp("mkvserver setting jail options");
setjailoptions();

#
# Physical host's address on virtual control net.
# Must be called after setjailoptions.
#
my $cnet_alias = inet_aton($IP);
if ((inet_ntoa($cnet_alias & inet_aton($JAILCNETMASK)) eq $JAILCNET)) {
    $cnet_alias = inet_ntoa($cnet_alias & inet_aton("255.255.255.0"));
} else {
    undef $cnet_alias;
}

# Do some prep stuff on the physical node if this is the first vserver.
PreparePhysNode();
PrepareFilesystems()
    if ($USEHASHIFIED);

print("Setting up jail for $vnodeid using $IP\n")
    if ($debug && defined($IP));

$VDIR = "$VSERVERDIR/$vnodeid";
$CDIR = "/etc/vservers/$vnodeid";

# XXX should really be the same as $VDIR
$JDIR = "$JAILPATH/$vnodeid";

#
# Create and configure a control net interface
#
# XXX right now this is done before configuring the vserver since
# it appears that the interface needs to be passed in.  Perhaps the
# device doesn't actually need to exist during config and can be added
# directly to the /etc/vservers config hierarchy.
#
my $cnetdev = CreateCnet($vnodeid);

#
# Set up experimental interfaces.
#
TBDebugTimeStamp("mkvserver doing ifsetup for vserver");
mysystem("ifsetup -j $vnodeid boot");

#
# Create the vserver.
#
if (-e $VDIR) {
    #
    # Try to pick up where we left off.
    #
    TBDebugTimeStamp("mkvserver restoring root fs");
    upvserver("$vnodeid");
}
else {
    #
    # Create the root filesystem.
    #
    TBDebugTimeStamp("Creating vserver");
    mkvserver("$vnodeid");
}
TBDebugTimeStamp("mkvserver done with root fs");

# Create and configure experimental interfaces

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
    local $SIG{ALRM} = sub { kill("TERM", $childpid); };
    alarm 10;
    waitpid($childpid, 0);
    alarm 0;

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

    unlink("$VDIR/$BOOTDIR/vserver.pid");
    exec("$VSERVER $vnodeid start --rescue --rescue-init $BINDIR/vserver-init start");
    die("*** $0:\n".
	"    exec failed to start the vserver!\n");
}

#
# XXX right now vservers are not that cleanly intergrated with network
# namespaces.  To make an interface visible to a vserver, we have to
# associate it with the name space.  This is done by writing the pid of
# some process in the new namespace to a special file for the device in
# the /sys pseudo-filesystem.  To do this, we of course need to know the
# pid of some process in the namespace and we cannot know that until the
# vserver has been started.  However, just blindly starting the vserver
# will cause it to (among other things) start configuring and using
# interfaces that it does not yet have.  So we have to do a little dance
# with the vserver:
#
#  * wait for vserver to create $BOOTDIR/vserver.pid
#  * inject network interfaces into vserver using that pid
#  * tell vserver to continue by creating $BOOTDIR/ready
#  * wait for vserver to create $BOOTDIR/vrunning
#
# If vrunning gets created we pronounce the vnode "set up".
#
TBDebugTimeStamp("mkvserver: waiting for vserver to start...");
while (! -e "$VDIR/$BOOTDIR/vserver.pid") {
    sleep(2);
}
my $vspid = `cat $VDIR/$BOOTDIR/vserver.pid`;
chomp($vspid);
TBDebugTimeStamp("mkvserver: vserver started, pid $vspid...");

if (!InjectIFs($vspid)) {
    # XXX force vserver to exit
    unlink("$VDIR/$BOOTDIR/vserver.pid");
}
system("cp /dev/null $VDIR/$BOOTDIR/ready");
TBDebugTimeStamp("mkvserver: interfaces ready, vserver released...");

#
# Give the vserver time to finish booting.
# If the magic file does not get created, the inner setup failed somehow.
# Stop now.
#
for (my $i = 0; $i < 10; $i++) {
    last if (-e "$VDIR/$BOOTDIR/vrunning");
    print "  waiting for vserver to setup...\n";
    sleep(3);
}
fatal("vserver did not appear to set up properly. Exiting ...")
    if (! -e "$VDIR/$BOOTDIR/vrunning");

TBDebugTimeStamp("mkvserver: $vnodeid setup, waiting for termination...\n");
RUNNING: while (1) {
    $jailpid = fork();
    if ($jailpid) {
	#
	# We do not really care about the exit status of the jail, we just want
	# to know when it dies inside.
	#
	while (1) {
	    my $kidpid = waitpid(-1, 0);

	    #
	    # Vserver exec command returned.  See if vserver exited
	    # or if the sleep just ended.
	    #
	    if ($kidpid == $jailpid) {
		if (system("$VSERVER $vnodeid running")) {
		    # vserver is dead, cleanup
		    undef($jailpid);
		    last RUNNING;
		}
		# vserver sleep command returned, just restart it
		print "vserver watchdog died, restarting\n";
		last;
	    } elsif ($USEPROXY && $kidpid == $tmccpid) {
		print("TMCC proxy exited with status $?. Restarting ...\n");
		startproxy("$vnodeid");
	    } else {
		print("Unknown child $kidpid exited with status $?!\n");
	    }
	}
    } else {
	$SIG{TERM} = 'DEFAULT';

	exec("$VSERVER $vnodeid exec sleep 1000000");
	die("*** $0:\n".
	    "    failed to start vserver watchdog!\n");
    }
}
TBDebugTimeStamp("mkvserver: $vnodeid has died, cleaning up ...\n");
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

    my $enetifs = "";

    #
    # Add all the interfaces that the vserver will have access to
    #
    # XXX setting the IP here may or may not be needed.  It gets cleared
    # when we move the device into the vserver namespace later, but may
    # still be needed here for establishing access control.
    #
    if (defined($IP)) {
	system("$VSERVERUTILS/mask2prefix $JAILCNETMASK");
	my $prefix = $? >> 8;
	$interface = "${vnodeid}=${cnetdev}:${IP}/${prefix}";
    } else {
	$interface = "nodev:0.0.0.0/0";
    }

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
    # Set vserver capabilities and flags
    #
    # NET_BIND_SERVICE: Allows binding to TCP/UDP sockets below 1024
    # LBACK_REMAP:      Virtualize the loopback device
    # HIDE_LBACK:       Hide real address used for loopback
    # HIDE_NETIF:       Hide "foreign" network interfaces, this appears
    #			in both cflags and nflags, need both?
    #
    mysystem("echo 'NET_BIND_SERVICE' >> $cdir/bcapabilities");
    #mysystem("echo 'LBACK_ALLOW' >> $cdir/nflags");
    mysystem("echo 'LBACK_REMAP' >> $cdir/nflags");
    mysystem("echo 'HIDE_LBACK' >> $cdir/nflags");

    # XXX which is correct?
    mysystem("echo 'HIDE_NETIF' >> $cdir/nflags");
    mysystem("echo 'HIDE_NETIF' >> $cdir/cflags");

    mysystem("echo '~HIDE_CINFO' >> $cdir/cflags");
    mysystem("echo '~HIDE_MOUNT' >> $cdir/cflags");

    # XXX doesn't seem to get created
    mysystem("mkdir $cdir/spaces")
	if (!-d "$cdir/spaces");

    if ($USENEWNET) {
	# XXX create a network namespace, hack version
	# in theory, this...
	#mysystem("echo '0x4c000200' >> $cdir/spaces/mask");

	# ...or this should do something, but it doesn't
	# the vserver utils do not uniformly respect these
	# (vcontext at least, always uses a mask of 0)
	# so it is all about how the kernel is configured
	#mysystem("echo '' >> $cdir/spaces/net");

	# XXX needed to do clone with CLONE_NEWNET
	#mysystem("echo 'SYS_ADMIN' >> $cdir/bcapabilities");
    } else {
	#mysystem("echo '0xc000200' >> $cdir/spaces/mask");
    }
    mysystem("echo 'NET_ADMIN' >> $cdir/bcapabilities");
    mysystem("echo 'NET_RAW' >> $cdir/bcapabilities");

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
    mysystem("ln -s ../../init.d/syslog $vdir/etc/rc6.d/K85syslog");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc3.d/S85sshd");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc6.d/K80sshd");
    # XXX may be needed in the REMOTE case
    if (0) {
	mysystem("ln -s ../../init.d/pubsubd $vdir/etc/rc3.d/S95pubsubd");
	mysystem("ln -s ../../init.d/pubsubd $vdir/etc/rc6.d/K05pubsubd");
    }
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc3.d/S98invserver");
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc6.d/K98invserver");
    mysystem("cp -p $ETCVSERVER/vserver-init.sh $vdir/$BINDIR/vserver-init");
    mysystem("cp -p $ETCVSERVER/vserver-rc.sh $vdir/$BINDIR/vserver-rc");
    if ($USENEWNET) {
	# XXX this should be integrated with regular network startup
	mysystem("cp -p $ETCVSERVER/vserver-cnet.sh $vdir/etc/rc3.d/S00cnet");
	mysystem("cp -p $ETCVSERVER/vserver-cnet.sh $vdir/etc/rc6.d/K99cnet");
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
    # Stash the control net info for use by vserver startup script
    # to bring up control net.
    #
    if ($IP ne $hostip) {
	mysystem("echo $cnetdev > $vdir/var/emulab/boot/controlif");
	mysystem("echo $IP > $vdir/var/emulab/boot/myip");
	mysystem("echo $JAILCNETMASK > $vdir/var/emulab/boot/mynetmask");
	mysystem("echo $JAILROUTER > $vdir/var/emulab/boot/routerip");

	# XXX tell vnodes to attach to physhost event server
	mysystem("echo $cnet_alias > $vdir/var/emulab/boot/localevserver");
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
    if ($MOUNTELABFS && ! REMOTE()) {
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
    if ($MOUNTELABFS && ! REMOTE()) {
	mysystem("$BINDIR/rc/rc.mounts -j $vnodeid $vdir 0 boot");
    }

    #
    # Unlink all the subsystem lock files
    #
    mysystem("rm -f $vdir/var/lock/subsys/*");

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
    my $log         = "$JDIR/tmcc.log";

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
# * run some scripts to allow vserver setup,
# * make sure the etun device is loaded
# * create a control net bridge that virtnodes can be added to.
#
sub PreparePhysNode()
{
    mysystem("/etc/init.d/util-vserver start");
    mysystem("/etc/init.d/vprocunhide start");

    mysystem("modprobe etun >/dev/null 2>&1");
    SetupCnetBridge();
}

#
# Prepare cnet bridge:
#  * create bridge device
#  * take down real cnet interface
#  * assign cnet IP info to bridge
#  * bring real cnet IF up with no address
#  * add real cnet IF to bridge
#  * add an alias on the JAILNET for this host so we can talk to vnodes
#
sub SetupCnetBridge()
{
    # XXX hack check to see if bridge device is configured
    if (!system("ifconfig $cnet_bridge_dev >/dev/null 2>&1")) {
	return;
    }

    print "Configuring control net bridge $cnet_bridge_dev for phys host\n"
	if ($debug);

    mysystem("brctl addbr $cnet_bridge_dev");
    if (system("ifconfig $phys_cnet_if down")) {
	mysystem("brctl delbr $cnet_bridge_dev");
	die("*** $0:\n".
	    "    could not bring down cnet IF\n");
    }
    if (system("ifconfig $cnet_bridge_dev $cnet_ip netmask $cnet_mask")) {
	system("ifconfig $phys_cnet_if up");
	system("brctl delbr $cnet_bridge_dev");
	die("*** $0:\n".
	    "    could not ifconfig $cnet_bridge_dev\n");
    }
    if (system("ifconfig $phys_cnet_if 0.0.0.0") ||
	system("ifconfig $phys_cnet_if up")) {
	system("ifconfig $cnet_bridge_dev down");
	system("ifconfig $phys_cnet_if $cnet_ip netmask $cnet_mask");
	system("route add default gw $cnet_router");
	system("brctl delbr $cnet_bridge_dev");
	die("*** $0:\n".
	    "    could not re-ifconfig $phys_cnet_if\n");
    }
    if (system("brctl addif $cnet_bridge_dev $phys_cnet_if")) {
	system("ifconfig $phys_cnet_if down");
	system("ifconfig $cnet_bridge_dev down");
	system("ifconfig $phys_cnet_if $cnet_ip netmask $cnet_mask");
	system("route add default gw $cnet_router");
	system("brctl delbr $cnet_bridge_dev");
	die("*** $0:\n".
	    "    could not add $phys_cnet_if to $cnet_bridge_dev\n");
    }
    if (system("route add default gw $cnet_router")) {
	system("brctl delif $cnet_bridge_dev $phys_cnet_if");
	system("ifconfig $phys_cnet_if down");
	system("ifconfig $cnet_bridge_dev down");
	system("ifconfig $phys_cnet_if $cnet_ip netmask $cnet_mask");
	system("route add default gw $cnet_router");
	system("brctl delbr $cnet_bridge_dev");
	die("*** $0:\n".
	    "    could not add $phys_cnet_if to $cnet_bridge_dev\n");
    }
    if (defined($cnet_alias)) {
	mysystem("ifconfig $cnet_bridge_dev:1 $cnet_alias netmask $JAILCNETMASK");
    }
}

sub TeardownCnetBridge()
{
    system("brctl delif $cnet_bridge_dev $phys_cnet_if");
    system("ifconfig $phys_cnet_if down");
    system("ifconfig $cnet_bridge_dev down");
    system("ifconfig $phys_cnet_if $cnet_ip netmask $cnet_mask");
    system("route add default gw $cnet_router");
    system("brctl delbr $cnet_bridge_dev");
}

#
# Create an etun pair for the control net and insert the "parent" end
# into the control net bridge.  We do not bother to configure the child
# end and any config will get reset when the device is moved into the
# vnode namespace.
#
sub CreateCnet($)
{
    my ($vnodeid) = @_;
    my $vno;

    if ($vnodeid =~ /-(\d+)$/) {
	$vno = $1;
    } else {
	die("*** $0:\n".
	    "    $vnodeid: not in form name-#\n");
    }
    if (! -e "/sys/module/etun/parameters/newif") {
	die("*** $0:\n".
	    "    etun device not loader?!\n");
    }

    my $pend = "pnet$vno";
    my $vend = "cnet$vno";
    if (system("echo $pend,$vend > /sys/module/etun/parameters/newif")) {
	die("*** $0:\n".
	    "    cannot create etun device(s) for $vnodeid!?\n");
    }
    if (system("ifconfig $pend up") ||
	system("brctl addif $cnet_bridge_dev $pend")) {
	system("ifconfig $pend down");
	system("echo $vend > /sys/module/etun/parameters/delif");
	die("*** $0:\n".
	    "    cannot put vnode etun device $pend in cnet bridge\n");
    }

    return $vend;
}

sub DestroyCnet($)
{
    my ($vnodeid) = @_;
    my $vno;

    if ($vnodeid =~ /-(\d+)$/) {
	$vno = $1;
    } else {
	die("*** $0:\n".
	    "    $vnodeid: not in form name-#\n");
    }
    my $pend = "pnet$vno";
    my $vend = "cnet$vno";

    # Remove the outside end from the bridge
    system("ifconfig $pend down");
    system("brctl delif $cnet_bridge_dev $pend");

    # Take down the inside end and delete the tunnel
    system("ifconfig $vend down >/dev/null 2>&1");
    system("echo $pend > /sys/module/etun/parameters/delif");
}

sub InjectIFs($)
{
    my ($vspid) = @_;
    my $devdir = "/sys/class/net";
    my @eifs = ();

    if (!$USENEWNET) {
	return 1;
    }

    #
    # Figure out all the experimental interfaces
    #
    my $TMIFMAP = "$JDIR/ifmap";
    if ( -e "$TMIFMAP") {
	if (open(IFMAP, "<$TMIFMAP")) {
	    while (<IFMAP>) {
		if (/^(veth\d+).*/) {
		    push(@eifs, $1);
		}
	    }
	    close(IFMAP);
	}
    }

    #
    # Insert the control net
    #
    my $cmd = "chcontext --ctx 1 -- echo $vspid > $devdir/$cnetdev/new_ns_pid";
    print "system('$cmd')\n"
	if ($debug);
    if (system($cmd)) {
	print "Could not inject $cnetdev into vserver namespace\n";
	return 0;
    }

    #
    # Insert the experimental IFs
    #
    foreach my $eif (@eifs) {
	$cmd = "chcontext --ctx 1 -- echo $vspid > $devdir/$eif/new_ns_pid";
	print "system('$cmd')\n"
	    if ($debug);
	if (system($cmd)) {
	    print "Could not inject $eif into vserver namespace\n";
	    return 0;
	}
    }

    return 1;
}

sub DestroyIFs()
{
    my @eifs = ();

    #
    # Figure out all the experimental interfaces
    #
    my $TMIFMAP = "$JDIR/ifmap";
    if ( -e "$TMIFMAP") {
	if (open(IFMAP, "<$TMIFMAP")) {
	    while (<IFMAP>) {
		if (/^(veth\d+).*/) {
		    push(@eifs, $1);
		}
	    }
	    close(IFMAP);
	}
    }

    #
    # Destroy the experimental IFs.
    # We key on the "veth" side of each tunnel rather than the "peth".
    # This way if the interface is still "injected" in the vserver, we
    # won't see it and won't mess with it.
    #
    foreach my $eif (@eifs) {
	my $pif = $eif;
	$pif =~ s/veth/peth/;

	# XXX should remove the phys end from the bridge first
	system("ifconfig $pif down");

	system("ifconfig $eif down");
	system("echo $eif > /sys/module/etun/parameters/delif");
    }
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
    # If $jailpid is defined, we got here as the result of a signal to us.
    # If it is not defined, then in theory the vserver has exited, but it
    # may also be the case that just our inside watchdog process died.
    # To deal with the latter, we perform a check to see if the vserver
    # is running and kill it if it is.
    #
    if (defined($jailpid)) {
	system("$VSERVER $vnodeid exec $BINDIR/vserver-init stop");
	system("$VSERVER $vnodeid stop --rescue-init");
	waitpid($jailpid, 0);
	undef($jailpid);
    }
    elsif (system("$VSERVER $vnodeid running") == 0) {
	system("$VSERVER $vnodeid exec $BINDIR/vserver-init stop");
	system("$VSERVER $vnodeid stop --rescue-init");
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

    # Teardown experimental tunnel devices
    DestroyIFs();

    # Teardown the control net tunnel device we created
    DestroyCnet($vnodeid);

    #
    # XXX somehow we need to detect when nothing is left in the bridge so
    # we can teardown the cnet bridge device
    #
    #TeardownCnetBridge();
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
