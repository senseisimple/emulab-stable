#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
use strict;
use Getopt::Std;
use English;
use Errno;
use POSIX qw(strftime);
use Data::Dumper;

#
# The corollary to mkjail.pl in the freebsd directory ...
#
sub usage()
{
    print "Usage: vnodectl [-h] [-d] vnodeid\n" . 
          "  -d   Debug mode.\n" .
          "";
    exit(1);
}
my $optlist  = "d";
my $debug    = 1;
my $leaveme  = 0;
my $cleaning = 0;
my $rebooting= 0;
my $vnodeid;

#
# Turn off line buffering on output
#
$| = 1;

# Drag in path stuff so we can find emulab stuff.
BEGIN { require "/etc/emulab/paths.pm"; import emulabpaths; }

#
# Load the OS independent support library. It will load the OS dependent
# library and initialize itself. 
# 
use libsetup;
use libtmcc;
use libtestbed;
use liblocsetup;
    
# Pull in libvnode
use libvnode;

# Helpers
sub MyFatal($);
sub safeLibOp($$$$;@);

# Locals
my $CTRLIPFILE = "/var/emulab/boot/myip";
my $VMPATH     = "/var/emulab/vms";

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
my %options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
# hah, this is funny... emacs perl mode will not properly indent unless
# I throw this extra block in.  Woo!
{
    if (defined($options{"d"})) {
	$debug = 1;
    }
}
usage()
    if (@ARGV != 1);
$vnodeid = $ARGV[0];

#
# Must be root.
# 
if ($UID != 0) {
    die("*** $0:\n".
	"    Must be root to run this script!\n");
}

# Tell the library what vnode we are messing with.
libsetup_setvnodeid($vnodeid);

#
# Turn on debug timestamps if desired.
#
if ($debug) {
    TBDebugTimeStampsOn();
}

#
# XXX: for now, support only a single vnode type per phys node.  This is bad,
# but it's the current assumption.  For now, we also assume the nodetype since
# we only have pcvm.  Later, we need to get this info from tmcd so we know 
# lib to load.
#
my @nodetypes = ( GENVNODETYPE() );

#
# We go through this crap so that we can pull in multiple packages implementing
# the libvnode API so they (hopefully) won't step on our namespace too much.
#
my %libops = ();
foreach my $type (@nodetypes) {
    if ($type =~ /^([\w\d\-]+)$/) {
	$type = $1;
    }
    # load lib and initialize it
    my %ops;
    eval "use libvnode_$type; %ops = %libvnode_${type}::ops";
    if ($@) {
	die "while trying to load 'libvnode_$type': $@";
    }
    if (0 && $debug) {
	print "%ops($type):\n" . Dumper(%ops);
    }
    $libops{$type} = \%ops;
    if ($debug) {
	$libops{$type}{'setDebug'}->(1);
    }
    $libops{$type}{'init'}->()
}
if ($debug) {
    print "GENVNODETYPE " . GENVNODETYPE() . "\n";
    print "libops:\n" . Dumper(%libops);
}

#
# Need the domain, but no conistent way to do it. Ask tmcc for the
# boss node and parse out the domain. 
#
my $DOMAINNAME = tmccbossname();
die("Could not get bossname from tmcc!")
    if (!defined($DOMAINNAME));

if ($DOMAINNAME =~ /^[-\w]+\.(.*)$/) {
    $DOMAINNAME = $1;
}
else {
    die("Could not parse domain name!");
}

#
# Need the bossip, which we get from virthost emulab boot dir:
#
my $BOSSIP = `cat $BOOTDIR/bossip`;
chomp($BOSSIP);
if ($BOSSIP !~ /^\d+\.\d+\.\d+\.\d+$/) {
    die "Bad bossip '$BOSSIP' in $BOOTDIR/bossip!";
}

#
# In most cases, the vnodeid directory will have been created by the
# caller, and a config file possibly dropped in.  When debugging, we
# have to create it here.
# 
chdir($VMPATH) or
    die("Could not chdir to $VMPATH: $!\n");

if (! -e $vnodeid) {
    mkdir($vnodeid, 0770) or
	fatal("Could not mkdir $vnodeid in $VMPATH: $!");
}
my $VNDIR = "$VMPATH/$vnodeid";

my ($pid, $eid, $vname) = check_nickname();
my %vnconfig;
getgenvnodeconfig(\%vnconfig) == 0
    or fatal("Could not get vnode config for $vnodeid");

# XXX: need to do this for each type encountered! Only happens once.
my $vmtype = GENVNODETYPE();
TBDebugTimeStamp("starting $vmtype rootPreConfig()")
    if ($debug);
$libops{GENVNODETYPE()}{'rootPreConfig'}->();
TBDebugTimeStamp("finished $vmtype rootPreConfig()")
    if ($debug);

# Link and linkdelay stuff we need in the root context.
my %ifconfigs = ();
my %ldconfigs = ();
my @tmp1;
my @tmp2;

fatal("getifconfig($vnodeid): $!")
    if (getifconfig(\@tmp1));
$ifconfigs{$vnodeid} = \@tmp1;

fatal("getlinkdelayconfig($vnodeid): $!") 
    if (getlinkdelayconfig(\@tmp2));
$ldconfigs{$vnodeid} = \@tmp2;

if ($debug) {
    print "vnconfig:\n" . Dumper(%vnconfig);
    print "ifconfig:\n" . Dumper(%ifconfigs);
    print "ldconfig:\n" . Dumper(%ldconfigs);
}

TBDebugTimeStamp("starting $vmtype rootPreConfigNetwork")
    if ($debug);
$libops{GENVNODETYPE()}{'rootPreConfigNetwork'}->(\%ifconfigs,{},\%ldconfigs)
    == 0 or fatal("rootPreConfigNetwork failed!");
TBDebugTimeStamp("finished $vmtype rootPreConfigNetwork")
    if ($debug);

my ($vmid,$ret,$err);

#
# If this file exists, we are rebooting an existing container.
#
if (! -e "$VNDIR/vnode.info") {
    #
    # XXX XXX XXX: need to get this from tmcd!
    # NOTE: we first put the type into vndb so that the create call can go!
    #
    $vmtype = GENVNODETYPE();

    # OP: create
    ($ret,$err) = safeLibOp($vnodeid,'vnodeCreate',0,0,$vnodeid);
    if ($err) {
	MyFatal("vnodeCreate failed");
    }
    $vmid = $ret;

    mysystem("echo '$vmid $vmtype' > $VNDIR/vnode.info");
    mysystem("echo '$vnodeid' > $VMPATH/vnode.$vmid");
}
else {
    my $str = `cat $VNDIR/vnode.info`;
    chomp($str);
    ($vmid, $vmtype) = ($str =~ /^(\d*) (\w*)$/);

    MyFatal("Cannot determine vmid/from $VNDIR/vnode.info")
	if (!(defined($vmid) && defined($vmtype)));
    
    ($ret,$err) = safeLibOp($vnodeid,'vnodeState',1,0,$vnodeid,$vmid);
    if ($err) {
	MyFatal("Failed to get status for $vnodeid: $err");
    }
    if ($ret ne VNODE_STATUS_STOPPED()) {
	MyFatal("vnode $vnodeid not stopped, no booting!");
    }
    $rebooting = 1;
}

#
# Call back to do things to the container before it boots.
#
sub callback($)
{
    my ($path) = @_;

    if (SHAREDHOST()) {
	if (!$rebooting &&
	    system("/bin/cp -f ".
		   "$TMGROUP $TMPASSWD $TMSHADOW $TMGSHADOW $path/etc") != 0) {
	    return -1;
	}
    }
    return 0;
}

# OP: preconfig
if (safeLibOp($vnodeid,'vnodePreConfig',1,1,$vnodeid,$vmid,\&callback)) {
    MyFatal("vnodePreConfig failed");
}

# OP: control net preconfig
my $cnet_mac = ipToMac($vnconfig{'CTRLIP'});
my $ext_ctrlip = `cat $CTRLIPFILE`;
chomp($ext_ctrlip);
if ($ext_ctrlip !~ /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/) {
    # cannot/should not really go on if this happens.
    MyFatal("error prior to vnodePreConfigControlNetwork($vnodeid): " . 
	    " could not find valid ip in $CTRLIPFILE!");
}
my $longdomain = "${eid}.${pid}.${DOMAINNAME}";
    
if (safeLibOp($vnodeid,'vnodePreConfigControlNetwork',1,1,
	      $vnodeid,$vmid,$vnconfig{'CTRLIP'},
	      $vnconfig{'CTRLMASK'},$cnet_mac,
	      $ext_ctrlip,$vname,$longdomain,$DOMAINNAME,$BOSSIP)) {
    MyFatal("vnodePreConfigControlNetwork failed");
}

# OP: exp net preconfig
if (safeLibOp($vnodeid,'vnodePreConfigExpNetwork',1,1,$vnodeid,$vmid,
	      $ifconfigs{$vnodeid}, $ldconfigs{$vnodeid})) {
    MyFatal("vnodePreConfigExpNetwork failed");
}
if (safeLibOp($vnodeid,'vnodeConfigResources',1,1,$vnodeid,$vmid)) {
    MyFatal("vnodeConfigResources failed");
}
if (safeLibOp($vnodeid,'vnodeConfigDevices',1,1,$vnodeid,$vmid)) {
    MyFatal("vnodeConfigDevices failed");
}

#
# Start the container. If all goes well, this will exit cleanly, with the
# it running in its new context. Still, lets protect it with a timer
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
	MyFatal("$vnodeid container startup exited with $?");
    }
}
else {
    $SIG{TERM} = 'DEFAULT';

    TBDebugTimeStamp("Starting the container")
	if ($debug);

    if (safeLibOp($vnodeid,'vnodeBoot',1,1,$vnodeid,$vmid)) {
	print STDERR "*** ERROR: vnodeBoot failed\n";
	exit(1);
    }
    TBDebugTimeStamp("Container has been started")
	if ($debug);
    exit(0);
}
if (safeLibOp($vnodeid,'vnodePostConfig',1,1,$vnodeid,$vmid)) {
    MyFatal("vnodePostConfig failed");
}
# XXX: need to do this for each type encountered!
TBDebugTimeStamp("starting $vmtype rootPostConfig()")
    if ($debug);
$libops{$vmtype}{'rootPostConfig'}->();
TBDebugTimeStamp("finished $vmtype rootPostConfig()")
    if ($debug);

# This is for vnodesetup
mysystem("touch $VNDIR/running");

#
# Install a signal handler to catch signals from vnodesetup.
#
sub handler ($) {
    my ($signame) = @_;
    
    $SIG{INT}  = 'IGNORE';
    $SIG{USR1} = 'IGNORE';
    $SIG{TERM} = 'IGNORE';
    $SIG{HUP}  = 'IGNORE';

    if ($signame eq 'USR1') {
	$leaveme = 1;
    }
    MyFatal("Caught a SIG${signame}! Killing the container ...");
}
$SIG{INT}  = \&handler;
$SIG{USR1} = \&handler;
$SIG{HUP}  = \&handler;
$SIG{TERM} = 'IGNORE';

#
# This loop is to catch when the container halts itself.
#
RUNNING: while (1) {
    $childpid = fork();
    if ($childpid) {
	while (1) {
	    my $kidpid = waitpid(-1, 0);

	    #
	    # container exec command returned.  See if it exited
	    # or if the sleep just ended.
	    #
	    if ($kidpid == $childpid) {
		last RUNNING;
	    }
	    else {
		print("Unknown child $kidpid exited with status $?!\n");
		last RUNNING;
	    }
	}
    } else {
	$SIG{TERM} = 'DEFAULT';

	if (safeLibOp($vnodeid,'vnodeExec',
		      1,1,$vnodeid,$vmid,"sleep 100000000")) {
	    exit(1);
	}
	exit(0);
    }
}
cleanup();
exit(0);

#
# Clean things up.
#
sub Cleanup()
{
    if ($cleaning) {
	die("*** $0:\n".
	    "    Oops, already cleaning!\n");
    }
    $cleaning = 1;

    # If the container was never built, there is nothing to do.
    return
	if (! -e "$VNDIR/vnode.info" || !defined($vmid));

    # if not halted, try that first
    my ($ret,$err) = safeLibOp($vnodeid,'vnodeState',1,0,$vnodeid,$vmid);
    if ($err) {
	print STDERR "*** ERROR: vnodeState: ".
	    "failed to cleanup $vnodeid: $err\n";
	return;
    }
    if ($ret ne VNODE_STATUS_STOPPED()) {
	print STDERR "cleanup: $vnodeid not stopped, trying to halt it.\n";
	safeLibOp($vnodeid,'vnodeHalt',1,1,$vnodeid,$vmid);
    }
    return
	if ($leaveme);

    # now destroy
    ($ret,$err) = safeLibOp($vnodeid,'vnodeDestroy',1,1,$vnodeid,$vmid);
    if ($err) {
	print STDERR "*** ERROR: failed to destroy $vnodeid: $err\n";
	return;
    }
    unlink("$VNDIR/vnode.info");
    unlink("$VMPATH/vnode.$vmid");
}
    
#
# Print error and exit.
#
sub MyFatal($)
{
    my ($msg) = @_;

    Cleanup();
    die("*** $0:\n".
	"    $msg\n");
}

#
# Helpers:
#
sub safeLibOp($$$$;@) {
    my ($vnode_id,$op,$autolog,$autoerr,@args) = @_;

    my $sargs = '';
    if (@args > 0) {
	$sargs = join(',',@args);
    }
    TBDebugTimeStamp("starting $vmtype $op($sargs)")
	if ($debug);
    my $ret = eval {
	$libops{$vmtype}{$op}->(@args);
    };
    my $err;
    if ($@) {
	$err = $@;
	if ($autolog) {
	    ;
	}
	TBDebugTimeStamp("failed $vmtype $op($sargs): $@")
	    if ($debug);
	return (-1,$err);
    }
    if ($autoerr && $ret) {
	$err = "$op($vnode_id) failed with exit code $ret!";
	if ($autolog) {
	    ;
	}
	TBDebugTimeStamp("failed $vmtype $op($sargs): exited with $ret")
	    if ($debug);
	return ($ret,$err);
    }

    TBDebugTimeStamp("finished $vmtype $op($sargs)")
	if ($debug);

    return $ret;
}
