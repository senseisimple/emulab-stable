#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;
use Socket;

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
    print("Usage: mkjail.pl [-s] [-i <ipaddr>] [-p <pid>] <hostname>\n");
    exit(-1);
}
my  $optlist = "i:p:e:s";

#
# Only real root can run this script.
#
if ($UID) {
    die("Must be root to run this script!\n");
}
system("sysctl jail.set_hostname_allowed=0");

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

#
# Untaint the environment.
# 
$ENV{'PATH'} = "/tmp:/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:".
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Deal with the screwy path mess that I created!
#
my $EMULABPATH;
if (-e "/usr/local/etc/emulab/tmcc") {
    $EMULABPATH = "/usr/local/etc/emulab";
}
elsif (-e "/etc/testbed/tmcc") {
    $EMULABPATH = "/etc/testbed";
}
else {
    die("*** $0:\n".
	"    Could not locate the testbed directory!\n");
}

#
# Locals
#
my $JAILPATH	= "/var/emulab/jails";
my $JAILCONFIG  = "/etc/jail";
my $LOCALROOTFS = "/local";
my $TMCC	= "$EMULABPATH/tmcc";
my @ROOTCPDIRS	= ("etc", "root");
my @ROOTMKDIRS  = ("dev", "tmp", "var", "usr", "proc", "users", "opt",
		   "bin", "sbin", "home", $LOCALROOTFS);
my @ROOTMNTDIRS = ("bin", "sbin", "usr");
my $VNFILEMBS   = 64;
my $MAXVNDEVS	= 10;
my $IP;
my $PID;
my $debug	= 1;
my $cleaning	= 0;
my $vndevice;
my @mntpoints   = ();
my $jailpid;
my $tmccpid;
my $interactive = 0;

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
my $HOST = $ARGV[0];

#
# Untaint the arguments.
#
if ($HOST =~ /^([-\w\/]+)$/) {
    $HOST = $1;
}
else {
    die("Tainted argument $HOST!\n");
}

if (defined($options{'s'})) {
    $interactive = 1;
}

#
# If no IP, then it defaults to our hostname's IP.
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
else {
    my $hostname = `hostname`;

    # Untaint and strip newline.
    if ($hostname =~ /^([-\w\.]+)$/) {
	$hostname = $1;

	my (undef,undef,undef,undef,@ipaddrs) = gethostbyname($hostname);
	$IP = inet_ntoa($ipaddrs[0]);
    }
}
if (!defined($IP)) {
    usage();
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

print("Setting up jail for HOST:$HOST using IP:$IP\n")
    if ($debug);

#
# First create the directory tree and such.
# 
chdir($JAILPATH) or
    die("Could not chdir to $JAILPATH: $!\n");

if (! -e $HOST) {
    mkdir($HOST, 0770) or
	fatal("Could not mkdir $HOST in $JAILPATH: $!");
}

if (-e "$HOST/root") {
    #
    # Try to pick up where we left off.
    # 
    restorerootfs("$JAILPATH/$HOST");
}
else {
    #
    # Create the root filesystem.
    #
    mkrootfs("$JAILPATH/$HOST");
}

#
# Start the tmcc proxy. This path will be valid in both the outer
# environment and in the jail!
#
startproxy("$JAILPATH/$HOST");

#
# Start the jail. We do it in a child so we can send a signal to the
# jailed process to force it to shutdown. The jail has to shut itself
# down.
#
$jailpid = fork();
if ($jailpid) {
    # We do not really care about the exit status of the jail.
    waitpid($jailpid, 0);
    undef($jailpid);
}
else {
    $SIG{TERM} = 'DEFAULT';
    $ENV{'TMCCVNODEID'} = $HOST;

    my $cmd = "jail $JAILPATH/$HOST/root $HOST $IP $JAILCONFIG/injail.pl";
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

    chdir($path) or
	fatal("Could not chdir to $path: $!");

    mkdir("root", 0770) or
	fatal("Could not mkdir 'root' in $path: $!");
    
    #
    # Big file of zeros.
    # 
    mysystem("dd if=/dev/zero of=root.vnode bs=1m count=$VNFILEMBS");

    #
    # Find a free vndevice.
    #
    for (my $i = 0; $i < $MAXVNDEVS; $i++) {
	system("vnconfig -e -s labels vn${i} root.vnode");
	if (! $?) {
	    $vndevice = $i;
	    last;
	}
    }
    fatal("Could not find a free vn device!") 
	if (!defined($vndevice));

    mysystem("disklabel -r -w vn${vndevice} auto");
    mysystem("newfs -b 8192 -f 1024 -i 4096 -c 15 /dev/vn${vndevice}c");
    mysystem("mount /dev/vn${vndevice}c root");
    push(@mntpoints, "$path/root");

    #
    # Okay, copy in the top level directories. 
    #
    foreach my $dir (@ROOTCPDIRS) {
	mysystem("hier -f -FN cp /$dir root/$dir");
    }

    #
    # Make some other directories that are nice to have!
    #
    foreach my $dir (@ROOTMKDIRS) {
	mkdir("root/$dir", 0755) or
	    fatal("Could not mkdir '$dir' in $path/root: $!");
    }

    #
    # Okay, mount some other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	mysystem("mount -r localhost:/$dir $path/root/$dir");
	push(@mntpoints, "$path/root/$dir");
    }

    #
    # The proc FS in the jail is per-jail of course.
    # 
    mysystem("mount -t procfs proc $path/root/proc");
    push(@mntpoints, "$path/root/proc");

    #
    # /tmp is special of course
    #
    mysystem("chmod 1777 root/tmp");

    #
    # /dev is also special. It gets a very restricted set of entries.
    # Note that we create some BPF devices since they work in our jails.
    #
    mysystem("cd $path/root/dev; cp -p /dev/MAKEDEV .; ./MAKEDEV jail bpf31");
    
    #
    # Create stub /var and create the necessary log files.
    #
    # NOTE: I stole this little diddy from /etc/rc.diskless2.
    #
    mysystem("mtree -nqdeU -f /etc/mtree/BSD.var.dist ".
	     "-p $path/root/var >/dev/null 2>&1");
    mysystem("mkdir -p $path/root/$path");

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

    #
    # Now a bunch of stuff to set up a nice environment in the jail.
    #
    mysystem("cp -p $JAILCONFIG/rc.conf $path/root/etc");
    mysystem("cp -p $JAILCONFIG/rc.local $path/root/etc");
    mysystem("cp -p $JAILCONFIG/group $path/root/etc");
    mysystem("cp -p $JAILCONFIG/master.passwd $path/root/etc");
    mysystem("cp /dev/null $path/root/etc/fstab");
    mysystem("pwd_mkdb -p -d $path/root/etc $path/root/etc/master.passwd");
    mysystem("echo '$IP		$HOST' >> $path/root/etc/hosts");

    #
    # Give the jail an NFS mount of the local project directory. This one
    # is read-write.
    #
    if (defined($PID) && -e $LOCALROOTFS && -e "$LOCALROOTFS/$PID") {
	mysystem("mkdir -p $path/root/$LOCALROOTFS/$PID");
	mysystem("mount localhost:$LOCALROOTFS/$PID ".
		 "$path/root/$LOCALROOTFS/$PID");
	push(@mntpoints, "$path/root/$LOCALROOTFS/$PID");
    }

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
    # Find a free vndevice.
    #
    for (my $i = 0; $i < $MAXVNDEVS; $i++) {
	system("vnconfig -e -s labels vn${i} root.vnode");
	if (! $?) {
	    $vndevice = $i;
	    last;
	}
    }
    fatal("Could not find a free vn device!") 
	if (!defined($vndevice));

    mysystem("fsck -y /dev/vn${vndevice}c");
    mysystem("mount /dev/vn${vndevice}c root");
    push(@mntpoints, "$path/root");

    #
    # Okay, mount some other directories to save space.
    #
    foreach my $dir (@ROOTMNTDIRS) {
	mysystem("mount -r localhost:/$dir $path/root/$dir");
	push(@mntpoints, "$path/root/$dir");
    }

    #
    # The proc FS in the jail is per-jail of course.
    # 
    mysystem("mount -t procfs proc $path/root/proc");
    push(@mntpoints, "$path/root/proc");

    #
    # Give the jail an NFS mount of the local project directory. This one
    # is read-write.
    #
    if (defined($PID) && -e $LOCALROOTFS && -e "$LOCALROOTFS/$PID") {
	mysystem("mount localhost:$LOCALROOTFS/$PID ".
		 "$path/root/$LOCALROOTFS/$PID");
	push(@mntpoints, "$path/root/$LOCALROOTFS/$PID");
    }

    cleanmess($path);
    return 0;
}

#
# Deal with the path mess I created! I should have split the emulab
# directory into a /etc/emulab part with keys and such, and a
# /usr/local/bin part that had the scripts. I do not want to mess with that
# now, so mount a tiny MFS over /usr/local/etc/emulab, and then remove the
# bits that we do not want the jail to see.
#
sub cleanmess($) {
    my ($path) = @_;

    #
    # And, some security stuff. We want to remove bits of stuff from the
    # jail that would enable it to talk to tmcd directly. 
    #
    mysystem("rm -f $path/root/etc/emulab.cdkey");
    mysystem("rm -f $path/root/etc/emulab.pkey");

    if (-e "/usr/local/etc/emulab/tmcc") {
	mysystem("mount_mfs -s 4096 -b 4096 -f 1024 -i 12000 -c 11 ".
		 "-T minimum dummy $path/root/usr/local/etc/emulab");
	push(@mntpoints, "$path/root/usr/local/etc/emulab");
	mysystem("hier cp /usr/local/etc/emulab ".
		 "        $path/root/usr/local/etc/emulab");

	#
	# And symlink /etc/testbed in. Ug, these paths are all a mess!
	# 
	mysystem("rm -rf $path/root/etc/testbed");
	mysystem("ln -s /usr/local/etc/emulab $path/root/etc/testbed");

	mysystem("rm -f  $path/root/usr/local/etc/emulab/*.pem");
	mysystem("rm -f  $path/root/usr/local/etc/emulab/cvsup.auth");
	mysystem("rm -rf $path/root/usr/local/etc/emulab/.cvsup");
    }
    else {
	mysystem("rm -f  $path/root/etc/testbed/*.pem");
	mysystem("rm -f  $path/root/etc/testbed/cvsup.auth");
	mysystem("rm -rf $path/root/etc/testbed/.cvsup");
    }

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
	# So tmcc will work nicely inside the jail without needing the
	# -l option specified all over.
	#
	$ENV{'TMCCUNIXPATH'} = $insidepath;
	
	select(undef, undef, undef, 0.2);
	return 0;
    }
    $SIG{TERM} = 'DEFAULT';

    # The -o option will cause the proxy to detach but not fork!
    # Eventually change this to standard pid file kill.
    exec("$TMCC -d -x $outsidepath -n $HOST -o $log");
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
	kill('TERM', $jailpid);
	waitpid($jailpid, 0);
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
	system("rm -rf $JAILPATH/$HOST");
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
