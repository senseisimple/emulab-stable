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
# Create a jailed environment. There are some stub files stored in
# /etc/jail that copied into the jail.
#
sub usage()
{
    print("Usage: mkjail.pl [-i <ipaddr>] <hostname>\n");
    exit(-1);
}
my  $optlist = "i:";

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
sub handler () {
    $SIG{INT}  = 'IGNORE';
    $SIG{TERM} = 'IGNORE';
    fatal("Caught a signal!");
}
$SIG{INT}  = \&handler;
$SIG{TERM} = \&handler;

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
# Locals
#
my $JAILPATH	= "/var/jails";
my $MOUNTDIRS   = ("/usr");
my @ROOTDIRS	= ("etc", "dev", "bin", "sbin", "root");
my $VNFILEMBS   = 64;
my $MAXVNDEVS	= 10;
my $IP;
my $debug	= 1;
my $vndevice;
my @mntpoints   = ();
my $jailpid;

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
    fatal("Tainted argument $HOST!");
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
	fatal("Tainted argument $IP!");
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

print("Setting up jail for HOST:$HOST using IP:$IP\n")
    if ($debug);

#
# First create the directory tree and such.
# 
chdir($JAILPATH) or
    die("Could not chdir to $JAILPATH: $!\n");

die("Jail for '$HOST' already exists!\n")
    if (-e $HOST);

mkdir($HOST, 0770) or
    fatal("Could not mkdir $HOST in $JAILPATH: $!");
    
chdir($HOST) or
    fatal("Could not chdir to $HOST: $!");

#
# Create the root filesystem.
#
mkrootfs("$JAILPATH/$HOST");

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
    $SIG{INT}  = 'DEFAULT';
    $SIG{TERM} = 'DEFAULT';

    exec("jail $JAILPATH/$HOST/root $HOST $IP /etc/jail/injail.pl");
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

    mysystem("disklabel -r -w vn${vndevice} auto");
    mysystem("newfs -b 8192 -f 1024 -i 4096 -c 15 /dev/vn${vndevice}c");
    mysystem("mount /dev/vn${vndevice}c root");
    push(@mntpoints, "$path/root");

    #
    # Okay, copy in the top level directories. This is not space efficient
    # of course, but its not too bad. 
    #
    foreach my $dir (@ROOTDIRS) {
	mysystem("hier -f -FN cp /$dir root/$dir");
    }

    #
    # Make some other directories that are nice to have!
    #
    foreach my $dir ("tmp", "var", "usr", "proc", "users") {
	mkdir("root/$dir", 0755) or
	    fatal("Could not mkdir '$dir' in $path/root: $!");
    }
    mysystem("mount -t procfs proc $path/root/proc");
    push(@mntpoints, "$path/root/proc");
    
    #
    # Create stub /var and create the necessary log files.
    #
    # NOTE: I stole this little diddy from /etc/rc.diskless2.
    #
    mysystem("mtree -nqdeU -f /etc/mtree/BSD.var.dist ".
	     "-p $path/root/var >/dev/null 2>&1");

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
    # Mount usr from localhost so that the user inside the jail sees
    # a read-only usr tree. 
    #
    mysystem("mount -r localhost:/usr $path/root/usr");
    push(@mntpoints, "$path/root/usr");

    #
    # Now a bunch of stuff to set up a nice environment in the jail.
    #
    mysystem("cp -p /etc/jail/rc.conf $path/root/etc");
    mysystem("cp -p /etc/jail/rc.local $path/root/etc");
    mysystem("cp -p /etc/jail/group $path/root/etc");
    mysystem("cp -p /etc/jail/master.passwd $path/root/etc");
    mysystem("cp /dev/null $path/root/etc/fstab");
    mysystem("pwd_mkdb -p -d $path/root/etc $path/root/etc/master.passwd");
    
    return 0;
}

#
# Cleanup at exit.
#
sub cleanup()
{
    if (defined($jailpid)) {
	kill('TERM', $jailpid);
	sleep(1);
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
    system("rm -rf $JAILPATH/$HOST");
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
