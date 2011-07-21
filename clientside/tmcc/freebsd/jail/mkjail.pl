#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
# Kernel, jail, netstat, route, ifconfig, ipfw, header files.
# 
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;
use Fcntl ':flock';

# Drag in path stuff so we can find emulab stuff. Also untaints path.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }

use libsetup qw(REMOTE SHAREDHOST
		LOCALROOTFS TMTOPOMAP TMLTMAP TMLTPMAP TBDebugTimeStamp);
use libtmcc;

#
# Questions:
#
# * Whats the hostname for the jail? Perhaps vnodename.emulab.net
# * What should /etc/resolv.conf look like?
# 

#
# Create a jailed environment. There are some stub files stored in
# /etc/jail that copied into the jail.
#
sub usage()
{
    print("Usage: mkjail.pl [-V] [-s] [-i <ipaddr>] [-p <pid>] ".
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
system("sysctl jail.set_hostname_allowed=0 >/dev/null 2>&1");

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
    fatal("Caught a SIG${signame}! Killing the jail ...");
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
# XXX hack: use loopback NFS mounts rather than nullfs loopback mounts
# which are still buggy.  Can do either for one or both of local
# filesystems (ROOTMNTDIRS) or remote filesystems (/proj, /users).
#
my $NFSMOUNT_LOCAL  = 1;
my $NFSMOUNT_REMOTE = 0;	# too much load on NFS server

#
# Functions
#

sub mkrootfs($);
sub restorerootfs($);
sub cleanmess($);
sub startproxy($);
sub cleanup();
sub fatal($);
sub mysystem($);
sub getjailconfig($);
sub setjailoptions();
sub addcontrolroutes();
sub removevnodedir($);
sub getnextrtabid();
sub jailcnetaliases();
sub setcnethostalias($);
sub clearcnethostalias($);
sub getcnetrouter($);
sub getcnetmask($);
sub CheckFreeVN($$);

#
# Locals
#
my $JAILPATH	= "/var/emulab/jails";
my $ETCJAIL     = "/etc/jail";
my $DEVCACHEMEM = "$JAILPATH/devmem";
my $DEVCACHENMEM= "$JAILPATH/devnomem";
my $LOCALFS	= LOCALROOTFS();
my $LOCALMNTPNT = "/local";
my $TMCC	= "$BINDIR/tmcc";
my $JAILCONFIG  = "jailconfig";
my @ROOTCPDIRS	= ("etc", "root");
my @ROOTMKDIRS  = ("dev", "tmp", "var", "usr", "proc", "users", 
		   "bin", "sbin", "home", $LOCALMNTPNT);
my @ROOTMNTDIRS = ("bin", "sbin", "usr");
my @EMUVARDIRS	= ("logs", "db", "jails", "boot", "lock");
my $VNFILESECT  = 512 * ((1024 * 1024) / 512); # 64MB in 512b sectors.
my $MAXVNDEVS	= 100;
my $IP;
my $IPALIAS;
my $IPMASK;
my $PID;
my $idnumber;
my $jailhostname;
my $debug	= 1;
my $cleaning	= 0;
my $vndevice;
my @mntpoints   = ();
my $jailpid;
my $tmccpid;
my $interactive = 0;
my $USEVCNETROUTES = 0;
my @controlroutes = ();

# This stuff is passed from tmcd, which we parse into a config string
# and an option set.
my %jailconfig  = ();
my $jailoptions = "";
my $sshdport    = 50000;	# Bogus default, good for testing.
my $routetabid;			# Default to main routing table.
my $jailflags   = 0;
my @jailips     = ();		# List of jail IPs (for routing table).
my $JAIL_DEVMEM = 0x01;		# We need to know if these options given.
my $JAIL_ROUTING= 0x02;

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
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

if (defined($options{'V'})) {
    $USEVCNETROUTES = 1;
}

#
# If the vnodeid looks like what we expect (pcvmXXX-YYY) then pull out
# the YYY part and use that for the rtabid and the vn device number.
# Saves on searching and allocation. Need a better solution at some point.
#
if ($vnodeid =~ /^[\w]*-(\d+)$/) {
    $idnumber = $1;
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

#
# If no IP set in the jail config, then use hostip.
#
if (!defined($IP)) {
    $IP = $hostip;
}

print("Setting up jail for $vnodeid using $IP\n")
    if ($debug);

#
# Create the "disk";
#
if (-e "$vnodeid/root") {
    #
    # Try to pick up where we left off.
    #
    TBDebugTimeStamp("mkjail restoring root fs");
    restorerootfs("$JAILPATH/$vnodeid");
}
else {
    #
    # Create the root filesystem.
    #
    TBDebugTimeStamp("mkjail creating root fs");
    mkrootfs("$JAILPATH/$vnodeid");
}
TBDebugTimeStamp("mkjail done with root fs");

#
# Lets tell the caller it can proceed. 
#
system("touch $JAILPATH/$vnodeid/proceed");

#
# If the jail has its own IP, must insert the control network alias.
# We use a 255.255.255.255 netmask since there might be multiple
# virtual nodes from the same subnet on this node. 
#
if (defined($IPALIAS)) {
    setcnethostalias($IPALIAS);
}

#
# Add control routes if jail gets its own route table.
#
if ($jailflags & $JAIL_ROUTING) {
    addcontrolroutes();
}

#
# Set up jail interfaces. We pass it the rtabid as well, and the script
# does all the right stuff. 
#
TBDebugTimeStamp("mkjail doing ifsetup for jail");
mysystem("ifsetup -j $vnodeid ".
	 (($jailflags & $JAIL_ROUTING) ? "-r $routetabid " : "") .
	 "boot");

#
# Start the tmcc proxy. This path will be valid in both the outer
# environment and in the jail!
#
TBDebugTimeStamp("mkjail starting the tmcc proxy");
startproxy("$JAILPATH/$vnodeid");
TBDebugTimeStamp("mkjail tmcc proxy is running");

#
# Start the jail. We do it in a child so we can send a signal to the
# jailed process to force it to shutdown. The jail has to shut itself
# down.
#
$jailpid = fork();
if ($jailpid) {
    #
    # We do not really care about the exit status of the jail. We want to catch
    # any children, which includes the tmcc proxy. If it dies, something has
    # gone wrong, and that appears to happen a lot. Lets restart it until we
    # figure out what is going wrong.
    #
    while (1) {
	my $kidpid = waitpid(-1, 0);

	if ($kidpid == $jailpid) {
	    undef($jailpid);
	    last;
	}
	if ($kidpid == $tmccpid) {
	    print("TMCC proxy exited with status $?. Restarting ...\n");
	    startproxy("$JAILPATH/$vnodeid");
	    next;
	}
	print("Unknown child $kidpid exited with status $?!\n");
    }
}
else {
    $SIG{TERM} = 'DEFAULT';
    $ENV{'TMCCVNODEID'} = $vnodeid;

    TBDebugTimeStamp("mkjail starting the jail");
    
    my $cmd = "jail $jailoptions $JAILPATH/$vnodeid/root $jailhostname $IP ".
	"/etc/jail/injail -v $vnodeid";
    if ($interactive) {
	$cmd .= " /bin/csh";
    }
    exec($cmd);
    die("*** $0:\n".
	"    exec failed to start the jail!\n");
}
	 
#
# Once we exit, cleanup the mess.
#
cleanup();
exit(0);

#
# Create a file for a vnode device, vnconfig it, newfs, and then
# mount it on the "root" directory.
#
sub mkrootfs($)
{
    my ($path) = @_;
    my $vnsize = $VNFILESECT;

    chdir($path) or
	fatal("Could not chdir to $path: $!");

    mkdir("root", 0770) or
	fatal("Could not mkdir 'root' in $path: $!");
    
    #
    # Lots of zeros. Note we oseek to create a big hole.
    #
    TBDebugTimeStamp("mkjail doing the dd");
    mysystem("dd if=/dev/zero of=root.vnode bs=512 oseek=$vnsize count=1");
    TBDebugTimeStamp("mkjail done with the dd");

    #
    # Find a free vndevice. We start with the obvious choice, and if that
    # fails, fall back to searching.
    #
    if (defined($idnumber) && CheckFreeVN($idnumber, "root.vnode")) {
	$vndevice = $idnumber;
    }
    else {
	for (my $i = 0; $i < $MAXVNDEVS; $i++) {
	    if (CheckFreeVN($i, "root.vnode")) {
		$vndevice = $i;
		last;
	    }
	}
    }
    fatal("Could not find a free vn device!") 
	if (!defined($vndevice));
    print("Using vn${vndevice}\n")
 	if ($debug);

    TBDebugTimeStamp("mkjail vnconfig, label, newfs, tunefs, mount");
    mysystem("vnconfig -s labels vn${vndevice} root.vnode");
    mysystem("disklabel -r -w vn${vndevice} auto");
    mysystem("newfs -U -b 8192 -f 1024 -i 16384 -c 15 /dev/vn${vndevice}c");
    mysystem("tunefs -m 5 -o time /dev/vn${vndevice}c");
    mysystem("mount /dev/vn${vndevice}c root");
    TBDebugTimeStamp("mkjail vnconfig, label, newfs, tunefs, mount done!");
    push(@mntpoints, "$path/root");

    #
    # Okay, copy in the top level directories. 
    #
    foreach my $dir (@ROOTCPDIRS) {
	mysystem("hier -f -FN cp /$dir root/$dir");
    }
    TBDebugTimeStamp("mkjail copying root cp dirs done!");

    #
    # Make some other directories that are nice to have!
    #
    foreach my $dir (@ROOTMKDIRS) {
	mkdir("root/$dir", 0755) or
	    fatal("Could not mkdir '$dir' in $path/root: $!");
    }
    TBDebugTimeStamp("mkjail creating root mkdir dirs done!");

    #
    # Okay, mount some other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	if ($NFSMOUNT_LOCAL) {
	    mysystem("mount -r localhost:/$dir $path/root/$dir");
	} else {
	    mysystem("mount -r -t null /$dir $path/root/$dir");
	}
	push(@mntpoints, "$path/root/$dir");
    }
    TBDebugTimeStamp("mkjail mounting root NFS dirs done!");

    #
    # The proc FS in the jail is per-jail of course.
    # 
    mysystem("mount -t procfs proc $path/root/proc");
    TBDebugTimeStamp("mkjail mounting /proc done!");
    push(@mntpoints, "$path/root/proc");

    #
    # /tmp is special of course
    #
    mysystem("chmod 1777 root/tmp");

    #
    # /dev is also special. It gets a very restricted set of entries.
    # Note that we create some BPF devices since they work in our jails.
    #
    # Set up the dev directory cache; running MAKEDEV is incredibly slow,
    # but copying a directory device entries is really fast!
    #
    if (! -e $DEVCACHEMEM) {
	TBDebugTimeStamp("mkjail creating /dev cached copy");
	
	mkdir($DEVCACHEMEM, 0755) or
	    fatal("Could not mkdir $DEVCACHEMEM: $!");
	mkdir($DEVCACHENMEM, 0755) or
	    fatal("Could not mkdir $DEVCACHENMEM: $!");

	# Jails with JAIL_DEVMEM get more devices.
	mysystem("cd $DEVCACHEMEM; cp -p /dev/MAKEDEV .; ".
		 " ./MAKEDEV bpf31 std pty0; " .
		 # Remove some extraneous devices.
		 "rm -f pci io klog console; " .
		 #
		 # "Link" /dev/console to /dev/null We actually create a
		 # separate special file rather than just linking in case
		 # anyone mucks with the permissions on /dev/console.
		 #
		 "hier cp null console; chmod 600 console");

	# Otherwise no devmem.
	mysystem("cd $DEVCACHENMEM; cp -p /dev/MAKEDEV .; ".
		 " ./MAKEDEV bpf31 jail; " .
		 # Remove some extraneous devices.
		 "rm -f pci io klog console; " .
		 #
		 # "Link" /dev/console to /dev/null We actually create a
		 # separate special file rather than just linking in case
		 # anyone mucks with the permissions on /dev/console.
		 #
		 "hier cp null console; chmod 600 console");
    }

    #
    # Now copy contents of cached device directory.
    # 
    TBDebugTimeStamp("mkjail copying cached /dev directory");
    
    if ($jailflags & $JAIL_DEVMEM) {
	mysystem("hier cp $DEVCACHEMEM $path/root/dev");
    }
    else {
	mysystem("hier cp $DEVCACHENMEM $path/root/dev");
    }
    
    #
    # Create stub /var and create the necessary log files.
    #
    # NOTE: I stole this little diddy from /etc/rc.diskless2.
    #
    TBDebugTimeStamp("mkjail creating /var");
    
    mysystem("mtree -nqdeU -f /etc/mtree/BSD.var.dist ".
	     "-p $path/root/var >/dev/null 2>&1");
    mysystem("mkdir -p $path/root/$path");

    #
    # Make the emulab directories since they are not in the mtree file.
    #
    if (! -e "$path/root/var/emulab") {
	mkdir("$path/root/var/emulab", 0755) or
	    fatal("Could not mkdir 'emulab' in $path/root/var: $!");
    }
    foreach my $dir (@EMUVARDIRS) {
	if (! -e "$path/root/var/emulab/$dir") {
	    mkdir("$path/root/var/emulab/$dir", 0755) or
		fatal("Could not mkdir 'dir' in $path/root/var/emulab: $!");
	}
    }
    TBDebugTimeStamp("mkjail copying the tmcc cache");
    
    tmcccopycache($vnodeid, "$path/root");
    if (-e TMTOPOMAP()) {
	mysystem("cp -fp " . TMTOPOMAP() . " $path/root/var/emulab/boot");
    }
    if (-e TMLTMAP()) {
	mysystem("cp -fp " . TMLTMAP() . " $path/root/var/emulab/boot");
    }
    if (-e TMLTPMAP()) {
	mysystem("cp -fp " . TMLTPMAP() . " $path/root/var/emulab/boot");
    }

    #
    # Stash the control net IP if not the same as the host IP
    #
    if ($IP ne $hostip) {
	mysystem("echo $IP > $path/root/var/emulab/boot/myip");
    }

    TBDebugTimeStamp("mkjail creating /var/log files");

    #
    # Get a list of all the plain files and create zero length versions
    # in the new var.
    #
    opendir(DIR, "/var/log") or
	fatal("Cannot opendir /var/log: $!");
    my @logs = grep { -f "/var/log/$_" } readdir(DIR);
    closedir(DIR);

    foreach my $log (@logs) {
	mysystem("touch $path/root/var/log/$log");
    }

    TBDebugTimeStamp("mkjail configuring etc files");

    #
    # Now a bunch of stuff to set up a nice environment in the jail.
    #
    mysystem("rm -f $path/root/etc/rc.conf.local");
    mysystem("cp -p $ETCJAIL/rc.conf $ETCJAIL/rc.local $ETCJAIL/crontab ".
	     "$path/root/etc");
    mysystem("cp /dev/null $path/root/etc/fstab");
    mysystem("ln -s /usr/compat $path/root/compat");

    #
    # Premunge the sshd config: no X11 forwarding and get rid of old
    # ListenAddresses.
    #
    mysystem("sed -i .bak ".
	     "-e 's/^X11Forwarding.*yes/X11Forwarding no/' ".
	     "-e '/^ListenAddress/d' ".
	     "$path/root/etc/ssh/sshd_config");
    
    # Port/Address for sshd.
    if ($IP ne $hostip) {
	mysystem("echo 'ListenAddress $IP' >> ".
		 "$path/root/etc/ssh/sshd_config");
	foreach my $ip (@jailips) {
	    mysystem("echo 'ListenAddress $ip' >> ".
		     "$path/root/etc/ssh/sshd_config");
	}
    }
    else {
	mysystem("echo 'sshd_flags=\"\$sshd_flags -p $sshdport\"' >> ".
		 "$path/root/etc/rc.conf");
    }

    # In the jail, 127.0.0.1 refers to the jail, but we want to use the
    # nameserver running *outside* the jail.
    mysystem("cat /etc/resolv.conf | ".
	     "sed -e 's/127\.0\.0\.1/$hostip/' > ".
	     "$path/root/etc/resolv.conf");

    TBDebugTimeStamp("mkjail setting up password/group files");

    #
    # Password/group file stuff. If remote the jail picks them up. Locally
    # we start with current set of accounts on the physnode, since this is
    # more efficient (less tmcd work), and besides, the physnode starts with
    # exactly the same accounts.
    #
    if (REMOTE() || SHAREDHOST()) {
	mysystem("cp -p $ETCJAIL/group $path/root/etc");
	mysystem("cp -p $ETCJAIL/master.passwd $path/root/etc");
	mysystem("pwd_mkdb -p -d $path/root/etc $path/root/etc/master.passwd");
    }
    else {
	mysystem("cp -p /etc/group /etc/master.passwd $path/root/etc");
	mysystem("pwd_mkdb -p -d $path/root/etc $path/root/etc/master.passwd");
	# XXX
	mysystem("cp -p $DBDIR/passdb.db $DBDIR/groupdb.db $path/root/$DBDIR");
    }
    
    TBDebugTimeStamp("mkjail mounting NFS filesystems");
    
    #
    # Give the jail an NFS mount of the local project directory. This one
    # is read-write.
    #
    if (defined($PID) && -e $LOCALFS && -e "$LOCALFS/$PID") {
	mysystem("mkdir -p $path/root/$LOCALMNTPNT/$PID");
	if ($NFSMOUNT_REMOTE) {
	    mysystem("mount localhost:$LOCALFS/$PID ".
		     "      $path/root/$LOCALMNTPNT/$PID");
	} else {
	    mysystem("mount -t null $LOCALFS/$PID ".
		     "      $path/root/$LOCALMNTPNT/$PID");
	}
	push(@mntpoints, "$path/root/$LOCALMNTPNT/$PID");

#	mysystem("ln -s $LOCALMNTPNT/$PID/$vnodeid $path/root/opt");
	mkdir("$path/root/opt", 0775);
    }
    else {
	mkdir("$path/root/opt", 0775);
    }

    #
    # Mount up a local logs directory. 
    #
    mysystem("mkdir -p $path/logs")
	if (! -e "$path/logs");
    mkdir("$path/root/$LOCALMNTPNT/logs", 0775);
    
    if ($NFSMOUNT_REMOTE) {
	mysystem("mount localhost:$path/logs ".
		 "      $path/root/$LOCALMNTPNT/logs");
    }
    else {
	mysystem("mount -t null $path/logs ".
		 "      $path/root/$LOCALMNTPNT/logs");
    }
    push(@mntpoints, "$path/root/$LOCALMNTPNT/logs");

    #
    # Ug. Until we have SFS working the way I want it, NFS mount the
    # usual directories inside the jail. This duplicates a lot of mounts,
    # but not sure what to do about that. 
    #
    if (! REMOTE()) {
	mysystem("$BINDIR/rc/rc.mounts ".
		 "-j $vnodeid $path/root $NFSMOUNT_REMOTE boot");
    }
    TBDebugTimeStamp("mkjail mounting NFS filesystems done");

    cleanmess($path);
    return 0;
}

#
# Restore a jail after a crash.
#
sub restorerootfs($)
{
    my ($path) = @_;

    chdir($path) or
	fatal("Could not chdir to $path: $!");

    #
    # Find a free vndevice. We start with the obvious choice, and if that
    # fails, fall back to searching.
    #
    if (defined($idnumber) && CheckFreeVN($idnumber, "root.vnode")) {
	$vndevice = $idnumber;
    }
    else {
	for (my $i = 0; $i < $MAXVNDEVS; $i++) {
	    if (CheckFreeVN($i, "root.vnode")) {
		$vndevice = $i;
		last;
	    }
	}
    }
    fatal("Could not find a free vn device!") 
	if (!defined($vndevice));
    print("Using vn${vndevice}\n")
 	if ($debug);

    mysystem("vnconfig -s labels vn${vndevice} root.vnode");
    mysystem("fsck -p -y /dev/vn${vndevice}c");
    mysystem("mount /dev/vn${vndevice}c root");
    push(@mntpoints, "$path/root");

    #
    # Okay, mount some other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	if ($NFSMOUNT_LOCAL) {
	    mysystem("mount -r localhost:/$dir $path/root/$dir");
	} else {
	    mysystem("mount -r -t null /$dir $path/root/$dir");
	}
	push(@mntpoints, "$path/root/$dir");
    }

    tmcccopycache($vnodeid, "$path/root");
    if (-e TMTOPOMAP()) {
	mysystem("cp -fp " . TMTOPOMAP() . " $path/root/var/emulab/boot");
    }
    if (-e TMLTMAP()) {
	mysystem("cp -fp " . TMLTMAP() . " $path/root/var/emulab/boot");
    }
    if (-e TMLTPMAP()) {
	mysystem("cp -fp " . TMLTPMAP() . " $path/root/var/emulab/boot");
    }

    #
    # The proc FS in the jail is per-jail of course.
    # 
    mysystem("mount -t procfs proc $path/root/proc");
    push(@mntpoints, "$path/root/proc");

    #
    # Stash the control net IP if not the same as the host IP
    #
    if ($IP ne $hostip) {
	mysystem("echo $IP > $path/root/var/emulab/boot/myip");
    }

    #
    # Premunge the sshd config: no X11 forwarding and get rid of old
    # ListenAddresses.
    #
    mysystem("sed -i .bak ".
	     "-e 's/^X11Forwarding.*yes/X11Forwarding no/' ".
	     "-e '/^ListenAddress/d' ".
	     "$path/root/etc/ssh/sshd_config");
    
    # Port/Address for sshd.
    if ($IP ne $hostip) {
	mysystem("echo 'ListenAddress $IP' >> ".
		 "$path/root/etc/ssh/sshd_config");
	foreach my $ip (@jailips) {
	    mysystem("echo 'ListenAddress $ip' >> ".
		     "$path/root/etc/ssh/sshd_config");
	}
    }
    else {
	mysystem("echo 'sshd_flags=\"\$sshd_flags -p $sshdport\"' >> ".
		 "$path/root/etc/rc.conf");
    }

    #
    # Give the jail an NFS mount of the local project directory. This one
    # is read-write.
    #
    if (defined($PID) && -e $LOCALFS && -e "$LOCALFS/$PID") {
	mysystem("mkdir -p $path/root/$LOCALMNTPNT/$PID");
	if ($NFSMOUNT_REMOTE) {
	    mysystem("mount localhost:$LOCALFS/$PID ".
		     "      $path/root/$LOCALMNTPNT/$PID");
	} else {
	    mysystem("mount -t null $LOCALFS/$PID ".
		     "      $path/root/$LOCALMNTPNT/$PID");
	}
	push(@mntpoints, "$path/root/$LOCALMNTPNT/$PID");
    }

    #
    # Mount up a local logs directory. 
    #
    mysystem("mkdir -p $path/logs")
	if (! -e "$path/logs");
    
    if ($NFSMOUNT_REMOTE) {
	mysystem("mount localhost:$path/logs ".
		 "      $path/root/$LOCALMNTPNT/logs");
    }
    else {
	mysystem("mount -t null $path/logs ".
		 "      $path/root/$LOCALMNTPNT/logs");
    }
    push(@mntpoints, "$path/root/$LOCALMNTPNT/logs");

    #
    # Ug. Until we have SFS working the way I want it, NFS mount the
    # usual directories inside the jail. This duplicates a lot of mounts,
    # but not sure what to do about that. 
    #
    if (! REMOTE()) {
	mysystem("$BINDIR/rc/rc.mounts ".
		 "-j $vnodeid $path/root $NFSMOUNT_REMOTE boot");
    }
    return 0;
}

#
# Okay, we clean up some of what is in /etc and /etc/emulab so that the
# jail cannot see that stuff. 
#
sub cleanmess($) {
    my ($path) = @_;

    #
    # And, some security stuff. We want to remove bits of stuff from the
    # jail that would enable it to talk to tmcd directly. 
    #
    mysystem("rm -f $path/root/etc/emulab.cdkey $path/root/etc/emulab.pkey");

    # This is /etc/emulab inside the jail.
    mysystem("rm -f $path/root/$ETCDIR/*.pem $path/root/$ETCDIR/master.passwd");
    mysystem("rm -rf $path/root/$ETCDIR/cvsup.auth $path/root/$ETCDIR/.cvsup");

    #
    # Copy in emulabman if it exists.
    #
    if (my (undef,undef,undef,undef,undef,undef,undef,$dir) =
	getpwnam("emulabman")) {
	mysystem("hier cp $dir $path/root/home/emulabman");
    }
}

#
# Start the tmcc proxy and insert the unix path into the environment
# for the jail to pick up.
#
sub startproxy($)
{
    my ($dir) = @_;
    my $log   = "$dir/tmcc.log";

    #
    # The point of these paths is so that there is a comman path to
    # the socket both outside and inside the jail. Yuck!
    #
    my $insidepath  = "$dir/tmcc";
    my $outsidepath = "$dir/root/$insidepath";

    $tmccpid = fork();
    if ($tmccpid) {
	#
	# Create a proxypath file so that libtmcc in the jail will know to
	# use a proxy for all calls; saves having all clients explicitly
	# use -l for every tmcc call.
	#
	mysystem("echo $insidepath > $dir/root/${BOOTDIR}/proxypath");
	
	select(undef, undef, undef, 0.2);
	return 0;
    }
    $SIG{TERM} = 'DEFAULT';

    # The -o option will cause the proxy to detach but not fork!
    # Eventually change this to standard pid file kill.
    exec("$TMCC -d -x $outsidepath -n $vnodeid -o $log");
    die("Exec of $TMCC failed! $!\n");
}

#
# Cleanup at exit.
#
sub cleanup()
{
    if ($cleaning) {
	die("*** $0:\n".
	    "    Oops, already cleaning!\n");
    }
    $cleaning = 1;
    
    #
    # Note that the unmounts will fail unless all the processes inside
    # the jail exit!
    # 
    if (defined($tmccpid)) {
	kill('TERM', $tmccpid);
	waitpid($tmccpid, 0);
    }

    if (defined($jailpid)) {
	kill('USR1', $jailpid);
	waitpid($jailpid, 0);
    }

    # Clean up interfaces we might have setup
    system("ifsetup -j $vnodeid shutdown");

    # If the jail has its own IP, clean the alias.
    if (defined($IPALIAS)) {
	clearcnethostalias($IPALIAS);
    }

    # Clean control routes
    foreach my $route (reverse(@controlroutes)) {
	print("deleting route: '$route'\n");
	system("route delete -rtabid $routetabid $route");
    }

    # Unmounts.
    if (! REMOTE()) {
	# If this fails, fail, we do not want to continue. Dangerous.
	system("$BINDIR/rc/rc.mounts ".
	       "-j $vnodeid foo $NFSMOUNT_REMOTE shutdown");

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
    if (defined($vndevice)) {
	system("vnconfig -u vn${vndevice}");
    }
    if (!$leaveme) {
        #
        # Ug, with NFS mounts inside the jail, we need to be really careful.
        #
	if (-d "$JAILPATH/$vnodeid/root" &&
	    !rmdir("$JAILPATH/$vnodeid/root")) {
	    die("*** $0:\n".
		"    $JAILPATH/$vnodeid/root is not empty! This is bad!\n");
	}
	system("rm -rf $JAILPATH/$vnodeid");
    }
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
# Run a command string, redirecting output to a logfile.
#
sub mysystem($)
{
    my ($command) = @_;

    system($command);
    if ($?) {
	fatal("Command failed: $? - $command");
    }
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
			$IPALIAS = $1;
			$IPMASK  = $2;
		    }
		}
		last SWITCH;
	    };
	    /^PORTRANGE$/ && do {
		if ($val =~ /(\d+),(\d+)/) {
		    $portrange = "$1:$2";
		}
		last SWITCH;
	    };
	    /^SSHDPORT$/ && do {
		if ($val =~ /(\d+)/) {
		    $sshdport     = $1;
		}
		last SWITCH;
	    };
	    /^SYSVIPC$/ && do {
		if ($val) {
		    $jailoptions .= " -o sysvipc";
		}
		else {
		    $jailoptions .= " -o nosysvipc";
		}
		last SWITCH;
	    };
	    /^INETRAW$/ && do {
		if ($val) {
		    $jailoptions .= " -o inetraw";
		}
		else {
		    $jailoptions .= " -o noinetraw";
		}
		last SWITCH;
	    };
	    /^BPFRO$/ && do {
		if ($val) {
		    $jailoptions .= " -o bpfro";
		}
		else {
		    $jailoptions .= " -o nobpfro";
		}
		last SWITCH;
	    };
	    /^INADDRANY$/ && do {
		if ($val) {
		    $jailoptions .= " -o inaddrany";
		}
		else {
		    $jailoptions .= " -o noinaddrany";
		}
		last SWITCH;
	    };
	    /^ROUTING$/ && do {
		if ($val) {
		    $jailoptions .= " -o routing -i 127.0.0.1 ";

		    $jailflags |= $JAIL_ROUTING;
		    #
		    # If the jail gets routing privs, then it must get
		    # its own routing table. 
		    #
		    $routetabid   = getnextrtabid();
		    $jailoptions .= " -r $routetabid";
		}
		last SWITCH;
	    };
	    /^IPFW$/ && do {
		if ($val) {
		    $jailoptions .= " -o ipfw";
		}
		else {
		    $jailoptions .= " -o noipfw";
		}
		last SWITCH;
	    };
	    /^IPDIVERT$/ && do {
		if ($val) {
		    $jailoptions .= " -o ipdivert";
		}
		else {
		    $jailoptions .= " -o noipdivert";
		}
		last SWITCH;
	    };
	    /^DEVMEM$/ && do {
		$jailflags |= $JAIL_DEVMEM;
		last SWITCH;
	    };
 	    /^IPADDRS$/ && do {
		# Comma separated list of IPs
		my @iplist = split(",", $val);

		foreach my $ip (@iplist) {
		    if ($ip =~ /(\d+\.\d+\.\d+\.\d+)/) {
			$jailoptions .= " -i $1";
			push(@jailips, $1);
		    }
		}
 		last SWITCH;
 	    };
	}
    }
    #
    # If there is no IP for the jail, must restrict the port range of
    # it. Otherwise it has it own IP, and there is no need to.
    # 
    if (! defined($IPALIAS)) {
	if (defined($portrange)) {
	    $jailoptions .= " -p $portrange";
	}
	else {
	    fatal("Must have a port range if jail ip equals host ip!");
	}
	print("SSHD port is $sshdport\n");
    }

    system("sysctl jail.inetraw_allowed=1 >/dev/null 2>&1");
    system("sysctl jail.bpf_allowed=1 >/dev/null 2>&1");
    system("sysctl jail.inaddrany_allowed=1 >/dev/null 2>&1");
    system("sysctl jail.multiip_allowed=1 >/dev/null 2>&1");
    system("sysctl jail.ipfw_allowed=1 >/dev/null 2>&1");
    system("sysctl jail.ipdivert_allowed=1 >/dev/null 2>&1");
    system("sysctl net.link.ether.inet.useloopback=0 >/dev/null 2>&1");

    if ($?) {
	print("Special jail options are NOT supported!\n");
	$jailoptions = "";
	$jailflags   = 0;
	return 0;
    }
    print("Special jail options are supported: '$jailoptions'\n");

    return 0;
}

#
# Append a list of static routes to the rc.conf file. This basically
# augments the list of static routes that tmcd tells us to install. 
#
sub addcontrolroutes()
{
    my ($rc)   = @_;
    my $count  = 0;

    #
    # XXX use address for localhost.  For some reason the route command is
    # going to the DNS before using the hosts file, despite the host.conf
    # file.
    # 
    my $routerip  = getcnetrouter($USEVCNETROUTES);
    my $localhost = "127.0.0.1";
    my $hostip    = `cat $BOOTDIR/myip`;
    chomp($hostip);

    push(@controlroutes, "default $routerip");
    push(@controlroutes, "$localhost -interface lo0");
    push(@controlroutes, "$hostip $localhost");

    if ($IP ne $hostip) {
	my $net;

	#
	# Setup a route for all jails on this node, to the loopback.
	#
	$net = inet_ntoa(inet_aton($IP) & inet_aton("255.255.255.0"));
	push(@controlroutes, "-net $net -interface lo0 255.255.255.0");

	#
	# All other jails are reachable via the control net interface.
	#
	# N.B. privnet is setup first even before the default route.
	# This is because the gateway for the default route is on the
	# privnet, but our control net interface is not (because it has
	# a 255.255.255.255 netmask by virtue of being an alias on the
	# real control interface).  Thus there is no way to use the
	# default route until the privnet route is up (ARP complains
	# about "host is not on local network").
	#
	$net = inet_ntoa(inet_aton($IP) & inet_aton($IPMASK));
	push(@controlroutes, "-net $net -interface $phys_cnet_if $IPMASK");
	
	#
	# If using the virtual control net for routes, also make sure that
	# nodes are reachable with their real control net addresses directly.
	#
	if ($USEVCNETROUTES) {
	    my $mask = getcnetmask(0);
	    
	    $net = inet_ntoa(inet_aton($hostip) & inet_aton($mask));
	    push(@controlroutes,
		 "-net $net -netmask $mask -interface $phys_cnet_if");
	}
    }

    #
    # Now a list of routes for each of the IPs the jail has access
    # to. The idea here is to override the interface route such that
    # traffic to the local interface goes through lo0 instead. This
    # avoids going through traffic shaping when, say, pinging your own
    # interface!
    # 
    foreach my $ip (@jailips) {
	push(@controlroutes, "$ip -interface lo0");
    }

    #
    # Now add them:
    #
    foreach my $route (@controlroutes) {
	print("Adding route: '$route'\n");
	mysystem("route add -rtabid $routetabid $route");
    }
    return 0;
}

#
# Ug, with NFS mounts inside the jail, we need to be really careful.
#
sub removevnodedir($)
{
    my ($dir) = @_;

    if (-d "$dir/root" && !rmdir("$dir/root")) {
	die("*** $0:\n".
	    "    $dir/root is not empty! This is very bad!\n");
    }
    system("rm -rf $dir");
}

#
# Get a free routing table ID. This should eventually come from the
# kernel, but for now just use a file with a number in it. 
#
sub getnextrtabid()
{
    my $nextrtabid = 1;

    #
    # If the vnodeid is what we think (pcvmXXX-YYY) then just use the YYY
    # part. At some point we need better control of this from Emulab Central.
    #
    if (defined($idnumber)) {
	$nextrtabid = $idnumber;
	goto gotit;
    }

    #
    # The chances of a race are low, but need to deal with it anyway.
    #
    my $lockfile  = "/var/emulab/lock/rtabid";
    my $rtabidfile= "/var/emulab/db/rtabid";

    open(LOCK, ">>$lockfile") ||
	fatal("Could not open $lockfile\n");
    
    while (flock(LOCK, LOCK_EX|LOCK_NB) == 0) {
	print "Waiting to get lock for $lockfile\n";
	sleep(1);
    }
    if (-e $rtabidfile) {
	my $rtabid = `cat $rtabidfile`;
	if ($rtabid =~ /^(\d*)$/) {
	    $nextrtabid = $1 + 1;
	}
	else {
	    close(LOCK);
	    fatal("Bad data in $rtabidfile!");
	}
    }
    system("echo $nextrtabid > $rtabidfile");
    close(LOCK);

  gotit:
    #
    # Flush that routing table so its pristine. Again, this should be
    # handled in the kernel.
    #
    system("route flush -rtabid $nextrtabid");
    
    return $nextrtabid;
}

#
# Return the number of alias on the jailhost control interface
#
sub jailcnetaliases()
{
    my $count = 0;

    if (open(IFC, "ifconfig $phys_cnet_if |")) {
	while (<IFC>) {
	    if ($_ =~ /inet ([0-9\.]*) netmask 0xffffffff/) {
		my $host = $1;
		if (inet_ntoa(inet_aton($host) & inet_aton($JAILCNETMASK)) eq
		    $JAILCNET) {
		    $count++;
		}
	    }
	}
	close(IFC);
    }
    return $count;
}

sub setcnethostalias($)
{
    my ($vnodeip) = @_;

    mysystem("ifconfig $phys_cnet_if alias $vnodeip netmask 255.255.255.255");

    if (!$USEVCNETROUTES) {
	return;
    }

    #
    # If the jail's IP is part of the local virtual control net,
    # make sure the physical host has an alias on it as well.
    #
    # The convention is that .0 is the physical host alias.
    #
    my $cnalias = inet_aton($vnodeip);
    if ((inet_ntoa($cnalias & inet_aton($JAILCNETMASK)) eq $JAILCNET) &&
	jailcnetaliases() == 1) {
	my $palias = inet_ntoa($cnalias & inet_aton("255.255.255.0"));
	mysystem("ifconfig $phys_cnet_if alias $palias netmask $JAILCNETMASK");
    }
}

sub clearcnethostalias($)
{
    my ($vnodeip) = @_;

    system("ifconfig $phys_cnet_if -alias $vnodeip");

    if (!$USEVCNETROUTES) {
	return;
    }

    #
    # If the jail's IP is part of the local virtual control net,
    # and we were the last jail, remove the physical host's alias.
    #
    my $cnalias = inet_aton($vnodeip);
    if ((inet_ntoa($cnalias & inet_aton($JAILCNETMASK)) eq $JAILCNET) &&
	jailcnetaliases() == 0) {
	my $palias = inet_ntoa($cnalias & inet_aton("255.255.255.0"));
	system("ifconfig $phys_cnet_if -alias $palias");
    }
}

sub getcnetrouter($)
{
    my ($usevcnet) = @_;

    my $routerip;

    if (!$usevcnet) {
	$routerip = `cat $BOOTDIR/routerip`;
	chomp($routerip);
    } else {
	$routerip = inet_ntoa(inet_aton($JAILCNET) |
			      inet_aton("0.0.0.1"));
    }

    return $routerip;
}

sub getcnetmask($)
{
    my ($usevcnet) = @_;

    my $cnetmask = "255.255.255.0";

    if (!$usevcnet) {
	if (-e "$BOOTDIR/mynetmask") {
	    $cnetmask = `cat $BOOTDIR/mynetmask`;
	    chomp($cnetmask);
	}
    } else {
	$cnetmask = $JAILCNETMASK;
    }

    return $cnetmask;
}

#
# Check for free vn device.
#
sub CheckFreeVN($$)
{
    my ($vn, $file) = @_;
    
    # Make sure the dev entries exist!
    if (! -e "/dev/vn${vn}") {
	mysystem("(cd /dev; ./MAKEDEV vn${vn})");
    }
	
    system("vnconfig -c vn${vn} $file");
    return ($? ? 0 : 1);
}
