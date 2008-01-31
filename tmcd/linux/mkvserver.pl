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

#
# Locals
#
my $JAILPATH	   = "/var/emulab/jails";
my $ETCVSERVER     = "/usr/local/etc/emulab/vserver";
my $VSERVER	   = "/usr/sbin/vserver";
my $VSERVERDIR     = "/vservers";
my $JAILCONFIG     = "jailconfig";
my @ROOTCPDIRS     = ("etc", "root");
my @ROOTMKDIRS     = ("dev", "tmp", "var", "usr", "proc", "users", "lib",
		      "bin", "sbin", "home");
my @ROOTMNTDIRS    = ("bin", "sbin", "usr", "lib");
my @EMUVARDIRS	   = ("logs", "db", "jails", "boot", "lock");
my $IP;
my $IPMASK;
my $PID;
my $VDIR;
my $idnumber;
my $jailhostname;
my $jailpid;
my $debug	   = 1;
my @mntpoints      = ();
my $USEVCNETROUTES = 0;
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
sub fatal($);
sub getjailconfig($);
sub setjailoptions();
sub mysystem($);
sub cleanup();

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

print("Setting up jail for $vnodeid using $IP\n")
    if ($debug);

$VDIR = "$VSERVERDIR/$vnodeid";

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

#
# Start the vserver. If all goes well, this will exit cleanly, with the
# vserver running in its new context. Still, lets protect it with a timer
# since it might get hung up inside and we do not want to get stuck here.
#
my $childpid = fork();
if ($childpid) {
    local $SIG{ALRM} = sub { kill("TERM", $childpid); };
    alarm 30;
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

    if (defined($IP)) {
	system("/usr/lib/util-vserver/mask2prefix $JAILCNETMASK");
	my $prefix = $? >> 8;
	$interface = "${vnodeid}=${phys_cnet_if}:${IP}/${prefix}";
    }
    else {
	$interface = "nodev:0.0.0.0/0";
    }

    # Create the skeleton vserver. It will be mostly empty.
    mysystem("$VSERVER $vnodeid build --force -m skeleton ".
	     "--hostname $jailhostname --interface $interface ".
	     "--flags persistent");

    # The filesystem for the vserver lands here.
    my $vdir = $VDIR;
    # The configuration directory is here.
    my $cdir = "/etc/vservers/$vnodeid";

    #
    # Copy in the top level directories. 
    #
    foreach my $dir (@ROOTCPDIRS) {
	mysystem("rsync -a /$dir $vdir");
    }
    TBDebugTimeStamp("mkvserver: Copying root cp dirs done!");

    #
    # Set vserver "capabilities".
    #
    # Allows binding to TCP/UDP sockets below 1024
    mysystem("echo 'NET_BIND_SERVICE' > $cdir/bcapabilities");

    #
    # Clean out some stuff from /eatc.
    #
    mysystem("/bin/rm -rf $vdir/etc/rc.d/rc*.d/*");

    #
    # Make some other directories that are need in /root.
    #
    foreach my $dir (@ROOTMKDIRS) {
	if (! -e "$vdir/$dir") {
	    mkdir("$vdir/$dir", 0755) or
		fatal("Could not mkdir '$dir' in $vdir: $!");
	}
    }
    TBDebugTimeStamp("mkvserver: Creating root mkdir dirs done!");

    #
    # Mount (read-only) these other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	LoopMount("/$dir", "$vdir/$dir");
    }
    TBDebugTimeStamp("mkvserver: Mounting root dirs done!");

    # /tmp is special.
    mysystem("chmod 1777 $vdir/tmp");

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
    TBDebugTimeStamp("mkvserver: finished setting up /var");
    
    # Create a local logs directory. 
    mysystem("mkdir -p $vdir/local/logs")
	if (! -e "$vdir/local/logs");

    #
    # Now a bunch of stuff to set up a nice environment in the jail.
    #
#    mysystem("ln -s ../../init.d/syslog $vdir/etc/rc3.d/S80syslog");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc3.d/S85sshd");
#    mysystem("ln -s ../../init.d/syslog $vdir/etc/rc6.d/K85syslog");
    mysystem("ln -s ../../init.d/sshd $vdir/etc/rc6.d/K80sshd");
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc3.d/S99invserver");
    mysystem("cp -p $ETCVSERVER/rc.invserver $vdir/etc/rc6.d/K99invserver");
    
    # Premunge the sshd config: no X11 forwarding and get rid of old
    # ListenAddresses.
    #
    mysystem("sed -i.bak ".
	     "-e 's/^X11Forwarding.*yes/X11Forwarding no/' ".
	     "-e '/^ListenAddress/d' ".
	     "$vdir/etc/ssh/sshd_config");
    
    # Port/Address for sshd.
    if ($IP ne $hostip) {
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

    #
    # vserver 2.2 does not support loopback devices inside, so redirect
    # localhost so that testbed daemons can find the local pubsubd. This
    # will come out when we have the virtual network stuff.
    #
    mysystem("echo '$IP\t\tlocalhost loghost' > $vdir/etc/hosts");
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

