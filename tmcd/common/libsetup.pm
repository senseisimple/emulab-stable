#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#
# TODO: Signal handlers for protecting db files.

#
# Common routines and constants for the client bootime setup stuff.
#
package libsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( libsetup_init libsetup_setvnodeid libsetup_settimeout cleanup_node
	 getifconfig getrouterconfig gettrafgenconfig gettunnelconfig
	 check_nickname	bootsetup startcmdstatus whatsmynickname
	 TBBackGround TBForkCmd vnodejailsetup plabsetup vnodeplabsetup
	 jailsetup dojailconfig findiface libsetup_getvnodeid
	 ixpsetup libsetup_refresh gettopomap getfwconfig gettiptunnelconfig
	 gettraceconfig genhostsfile getmotelogconfig calcroutes fakejailsetup
	 getlocalevserver genvnodesetup getgenvnodeconfig stashgenvnodeconfig
         getlinkdelayconfig getloadinfo getbootwhat gendhcpdconf
	 forcecopy

	 TBDebugTimeStamp TBDebugTimeStampsOn

	 MFS REMOTE REMOTEDED CONTROL WINDOWS JAILED PLAB LOCALROOTFS IXP
	 USESFS SHADOW FSRVTYPE PROJDIR EXPDIR

	 SIMTRAFGEN SIMHOST ISDELAYNODEPATH JAILHOST DELAYHOST STARGATE
	 ISFW FAKEJAILED LINUXJAILED GENVNODE GENVNODETYPE GENVNODEHOST
	 SHAREDHOST SUBBOSS

	 CONFDIR LOGDIR TMDELAY TMBRIDGES TMJAILNAME TMSIMRC TMCC TMCCBIN
	 TMNICKNAME TMSTARTUPCMD FINDIF
	 TMROUTECONFIG TMLINKDELAY TMDELMAP TMTOPOMAP TMLTMAP TMLTPMAP
	 TMGATEDCONFIG TMSYNCSERVER TMKEYHASH TMNODEID TMEVENTKEY
	 TMCREATOR TMSWAPPER TMFWCONFIG TMGENVNODECONFIG
	 INXENVM
       );

# Must come after package declaration!
use English;

# The tmcc library.
use libtmcc;
use librc;

#
# This is the VERSION. We send it through to tmcd so it knows what version
# responses this file is expecting.
#
# BE SURE TO BUMP THIS AS INCOMPATIBILE CHANGES TO TMCD ARE MADE!
#
sub TMCD_VERSION()	{ 32; };
libtmcc::configtmcc("version", TMCD_VERSION());

# Control tmcc timeout.
sub libsetup_settimeout($) { libtmcc::configtmcc("timeout", $_[0]); };

# Refresh tmcc cache.
sub libsetup_refresh()	   { libtmcc::tmccgetconfig(); };

#
# For virtual (multiplexed nodes). If defined, tack onto tmcc command.
# and use in pathnames. Used in conjunction with jailed virtual nodes.
# I am also using this for subnodes; eventually everything will be subnodes.
#
my $vnodeid;
sub libsetup_setvnodeid($)
{
    my ($vid) = @_;

    if ($vid =~ /^([-\w]+)$/) {
	$vid = $1;
    }
    else {
	die("Bad data in vnodeid: $vid");
    }

    $vnodeid = $vid;
    libtmcc::configtmcc("subnode", $vnodeid);
}
sub libsetup_getvnodeid()
{
    return $vnodeid;
}

#
# True if running inside a jail. Set just below.
#
my $injail;

#
# True if $injail == TRUE and running on Linux.
# Right now this means vserves on RHL.
#
my $inlinuxjail;

#
# True if running inside a vm.
#
my $ingenvnode;

#
# True if running as a fake jail (no jail, just processes).
#
my $nojail;

#
# True if running in a Plab vserver.
#
my $inplab;

#
# Ditto for IXP, although currently there is no "in" IXP setup; it
# is all done from outside.
#
my $inixp;

#
# Shadow mode. Run the client side against a remote tmcd.
#
my $shadow;
my $SHADOWDIR = "$VARDIR/shadow";

#
# Fileserver type.
# Default is "racy NFS" (the historical only choice) until proven otherwise
# (via "mounts" tmcc call).
#
my $fsrvtype = "NFS-RACY";

#
# The role of this pnode
#
my $role;

# Load up the paths. Its conditionalized to be compatabile with older images.
# Note this file has probably already been loaded by the caller.
BEGIN
{
    if (! -e "/etc/emulab/paths.pm") {
	die("Yikes! Could not require /etc/emulab/paths.pm!\n");
    }
    require "/etc/emulab/paths.pm";
    import emulabpaths;
    $SHADOWDIR = "$VARDIR/shadow";

    # Make sure these exist! They will not exist on a PLAB vserver initially.
    mkdir("$VARDIR", 0775);
    mkdir("$VARDIR/jails", 0775);
    mkdir("$VARDIR/vms", 0775);
    mkdir("$VARDIR/db", 0755);
    mkdir("$VARDIR/logs", 0775);
    mkdir("$VARDIR/boot", 0775);
    mkdir("$VARDIR/lock", 0775);
    mkdir("$SHADOWDIR", 0775);
    mkdir("$SHADOWDIR/db", 0755);
    mkdir("$SHADOWDIR/logs", 0775);
    mkdir("$SHADOWDIR/boot", 0775);
    mkdir("$SHADOWDIR/lock", 0775);

    #
    # Shadow mode allows the client side to run against remote tmcd.
    #
    if (exists($ENV{'SHADOW'})) {
	$shadow = $ENV{'SHADOW'};
	my ($server,$urn) = split(',', $shadow);
	#
	# Need to taint check these to avoid breakage later.
	#
	if ($server =~ /^([-\w\.]+)$/) {
	    $server = $1;
	}
	else {
	    die("Bad data in server: $server");
	}
	if ($urn =~ /^([-\w\+\:\.]*)$/) {
	    $urn = $1;
	}
	else {
	    die("Bad data in urn: $urn");
	}

	# The cache needs to go in a difference location.
	libtmcc::configtmcc("cachedir", $SHADOWDIR);
	libtmcc::configtmcc("server", $server);
	libtmcc::configtmcc("urn", $urn);
	# No proxy.
	libtmcc::configtmcc("noproxy", 1);
    }
    #
    # Determine if running inside a jail. This affects the paths below.
    #
    if (-e "$BOOTDIR/jailname") {
	open(VN, "$BOOTDIR/jailname");
	my $vid = <VN>;
	close(VN);

	libsetup_setvnodeid($vid);
	$injail = 1;
	if ($^O eq "linux") {
	    $inlinuxjail = 1;
	}
    }
    elsif (exists($ENV{'FAKEJAIL'})) {
	# Fake jail.
	libsetup_setvnodeid($ENV{'FAKEJAIL'});
	$nojail = 1;
    }
    elsif (-e "$BOOTDIR/plabname") {
	# Running inside a Plab vserver.
	open(VN, "$BOOTDIR/plabname");
	my $vid = <VN>;
	close(VN);

	libsetup_setvnodeid($vid);
	$inplab = 1;
    }
    elsif (-e "$BOOTDIR/vmname") {
	open(VN, "$BOOTDIR/vmname");
	my $vid = <VN>;
	close(VN);

	libsetup_setvnodeid($vid);
	$ingenvnode = 1;

    }

    $role = "";
    # Get our role.
    if (-e "$BOOTDIR/role") {
	open(VN, "$BOOTDIR/role");
	$role = <VN>;
	close(VN);
	chomp($role);
    }
}

#
# This "local" library provides the OS dependent part.
#
use liblocsetup;

#
# These are the paths of various files and scripts that are part of the
# setup library.
#
sub TMCC()		{ "$BINDIR/tmcc"; }
sub TMCCBIN()		{ "$BINDIR/tmcc.bin"; }
sub FINDIF()		{ "$BINDIR/findif"; }
sub TMUSESFS()		{ "$BOOTDIR/usesfs"; }
sub ISSIMTRAFGENPATH()	{ "$BOOTDIR/simtrafgen"; }
sub ISDELAYNODEPATH()	{ "$BOOTDIR/isdelaynode"; }
sub TMTOPOMAP()		{ "$BOOTDIR/topomap";}
sub TMLTMAP()		{ "$BOOTDIR/ltmap";}
sub TMLTPMAP()		{ "$BOOTDIR/ltpmap";}

#
# This path is valid only *outside* the jail when its setup.
#
sub JAILDIR()		{ "$VARDIR/jails/$vnodeid"; }

#
# This path is valid only *outside* the vm.  Sucks, but this is the best we
# can do.  Probably the only way to make this vm-specific if necessary is to
# have them create their dir and symlink.
#
sub GENVNODEDIR()	{ "$VARDIR/vms/$vnodeid"; }

#
# XXX: eventually this needs to come from tmcd, but that's not ready yet.
#
sub GENVNODETYPE() {
    if (-e "$ETCDIR/genvmtype") {
	my $vmtype = `cat $ETCDIR/genvmtype`;
	chomp($vmtype);
	return $vmtype;
    }

    return undef;
}

#
# Also valid outside the jail, this is where we put local project storage.
#
sub LOCALROOTFS() {
    return "/users/local"
	if (REMOTE());
    return "$VARDIR/jails/local"
	if (JAILED());
    return "$VARDIR/vms/local"
	if (GENVNODE());
}

#
# Okay, here is the path mess. There are three environments.
# 1. A local node where everything goes in one place ($VARDIR/boot).
# 2. A virtual node inside a jail or a Plab vserver ($VARDIR/boot).
# 3. A virtual (or sub) node, from the outside.
#
# As for #3, whether setting up a old-style (fake) virtual node or a new style
# jailed node, the code that sets it up needs a different per-vnode path.
#
sub CONFDIR() {
    return "$SHADOWDIR/boot"
	if ($shadow);
    return $BOOTDIR
	if ($injail || $inplab || $ingenvnode);
    return GENVNODEDIR()
	if ($vnodeid && GENVNODETYPE());
    return JAILDIR()
	if ($vnodeid);
    return $BOOTDIR;
}
# Cause of fakejails, we want log files in the right place.
sub LOGDIR() {
    return "$SHADOWDIR/logs"
	if ($shadow);
    return $LOGDIR
	if ($injail || $inplab || $ingenvnode);
    return GENVNODEDIR()
	if ($vnodeid && GENVNODETYPE());
    return JAILDIR()
	if ($vnodeid);
    return $LOGDIR;
}

#
# The rest of these depend on the environment running in (inside/outside jail).
#
sub TMNICKNAME()	{ CONFDIR() . "/nickname";}
sub TMJAILNAME()	{ CONFDIR() . "/jailname";}
sub TMFAKEJAILNAME()	{ CONFDIR() . "/fakejail";}
sub TMJAILCONFIG()	{ CONFDIR() . "/jailconfig";}
sub TMGENVNODECONFIG()  { CONFDIR() . "/genvnodeconfig";}
sub TMSTARTUPCMD()	{ CONFDIR() . "/startupcmd";}
sub TMROUTECONFIG()     { CONFDIR() . "/rc.route";}
sub TMGATEDCONFIG()     { CONFDIR() . "/gated.conf";}
sub TMBRIDGES()		{ CONFDIR() . "/rc.bridges";}
sub TMDELAY()		{ CONFDIR() . "/rc.delay";}
sub TMLINKDELAY()	{ CONFDIR() . "/rc.linkdelay";}
sub TMDELMAP()		{ CONFDIR() . "/delay_mapping";}
sub TMSYNCSERVER()	{ CONFDIR() . "/syncserver";}
sub TMKEYHASH()		{ CONFDIR() . "/keyhash";}
sub TMEVENTKEY()	{ CONFDIR() . "/eventkey";}
sub TMNODEID()		{ CONFDIR() . "/nodeid";}
sub TMROLE()		{ CONFDIR() . "/role";}
sub TMSIMRC()		{ CONFDIR() . "/rc.simulator";}
sub TMCREATOR()		{ CONFDIR() . "/creator";}
sub TMSWAPPER()		{ CONFDIR() . "/swapper";}
sub TMFWCONFIG()	{ CONFDIR() . "/rc.fw";}

#
# This is a debugging thing for my home network.
#
my $NODE = "";
if (defined($ENV{'TMCCARGS'})) {
    if ($ENV{'TMCCARGS'} =~ /^([-\w\s]*)$/) {
	$NODE .= " $1";
    }
    else {
	die("Tainted TMCCARGS from environment: $ENV{'TMCCARGS'}!\n");
    }
}

# Locals
my $pid		= "";
my $eid		= "";
my $vname	= "";
my $TIMESTAMPS  = 0;

# Allow override from the environment;
if (defined($ENV{'TIMESTAMPS'})) {
    $TIMESTAMPS = $ENV{'TIMESTAMPS'};
}

#
# Any reason NOT to hardwire these?
#
sub PROJDIR() {
    my $p = $pid;
    if (!$p) {
	($p, undef, undef) = check_nickname();
	return ""
	    if (!$p);
    }
    return "/proj/$p";
}

sub EXPDIR() {
    my $p = $pid;
    my $e = $eid;
    if (!$p || !$e) {
	($p, $e, undef) = check_nickname();
	return ""
	    if (!$p || !$e);
    }
    return "/proj/$p/exp/$e";
}

# When on the MFS, we do a much smaller set of stuff.
# Cause of the way the packages are loaded (which I do not understand),
# this is computed on the fly instead of once.
sub MFS()	{ if (-e "$ETCDIR/ismfs") { return 1; } else { return 0; } }

#
# Same for a remote node.
#
sub REMOTE()	{ if (-e "$ETCDIR/isrem") { return 1; } else { return 0; } }

#
# Same for a dedicated remote node.
#
sub REMOTEDED()	{ if (-e "$ETCDIR/isremded") { return 1; } else { return 0; } }

#
# Same for a control node.
#
sub CONTROL()	{ if (-e "$ETCDIR/isctrl") { return 1; } else { return 0; } }

#
# Same for a Windows (CygWinXP) node.
#
# XXX  If you change this, look in libtmcc::tmccgetconfig() as well.
sub WINDOWS()	{ if (-e "$ETCDIR/iscygwin") { return 1; } else { return 0; } }

#
# Same for a stargate/garcia node.
#
sub STARGATE()  { if (-e "$ETCDIR/isstargate") { return 1; } else { return 0; } }

#
# Are we jailed? See above.
#
sub JAILED()	{ if ($injail) { return $vnodeid; } else { return 0; } }
sub FAKEJAILED(){ if ($nojail) { return $vnodeid; } else { return 0; } }
sub LINUXJAILED(){ if ($injail && $inlinuxjail) { return $vnodeid; } else { return 0; } }

#
# Are we using the generic vm abstraction for this vnode?  See above.
#
sub GENVNODE()  { if ($ingenvnode) { return $vnodeid; } else { return 0; } }

#
# Are we on plab?
#
sub PLAB()	{ if ($inplab) { return $vnodeid; } else { return 0; } }

#
# Are we on an IXP
#
sub IXP()	{ if ($inixp) { return $vnodeid; } else { return 0; } }

#
# Are we a firewall node
#
sub ISFW()	{ if (-e TMFWCONFIG()) { return 1; } else { return 0; } }

#
# Are we hosting a simulator or maybe just a NSE based trafgen.
#
sub SIMHOST()   { if ($role eq "simhost") { return 1; } else { return 0; } }
sub SIMTRAFGEN(){ if (-e ISSIMTRAFGENPATH())  { return 1; } else { return 0; } }

#
# Are we a subboss?
#
sub SUBBOSS()   { if ($role eq "subboss") { return 1; } else { return 0; } }

# A jail host?
sub JAILHOST()     { return (($role eq "virthost" ||
			      $role eq "sharedhost") ? 1 : 0); }
sub GENVNODEHOST() { if ($role eq "virthost") { return 1; } else { return 0; }}
sub SHAREDHOST()   { return ($role eq "sharedhost" ? 1 : 0); }

# A delay host?  Either a delay node or a node using linkdelays
sub DELAYHOST()	{ if (-e ISDELAYNODEPATH()) { return 1; } else { return 0; } }

# A shadow?
sub SHADOW()	   { return (defined($shadow) ? 1 : 0); }

#
# Is this node using SFS. Several scripts need to know this.
#
sub USESFS()	{ if (-e TMUSESFS()) { return 1; } else { return 0; } }

#
# What type of fileserver is this node using.  Choices are:
#
# NFS-RACY	FreeBSD NFS server with mountd race (the default)
# NFS		NFS server
# LOCAL		No shared filesystems
#
# XXX should come from tmcd
#
sub FSRVTYPE() {
    if (-e "$BOOTDIR/fileserver") {
	open(FD, "$BOOTDIR/fileserver");
	$fsrvtype = <FD>;
	close(FD);
	chomp($fsrvtype);
    }
    return $fsrvtype;
}

# XXX fer now hack: comes from rc.mounts
sub setFSRVTYPE($) {
    $fsrvtype = shift;
    if (open(FD, ">$BOOTDIR/fileserver")) {
	print FD "$fsrvtype\n";
	close(FD);
    }
}

#
# XXX fernow hack so I can readily identify code that is special to Xen VMs
#
sub INXENVM()	{ return ($ingenvnode && GENVNODETYPE() eq "xen"); }

#
# Reset to a moderately clean state.
#
sub cleanup_node ($) {
    my ($scrub) = @_;

    print STDOUT "Cleaning node; removing configuration files\n";
    unlink TMUSESFS, TMROLE, ISSIMTRAFGENPATH, ISDELAYNODEPATH;

    #
    # If scrubbing, also remove the password/group files and DBs so
    # that we revert to base set.
    #
    if ($scrub) {
	unlink TMNICKNAME;
    }
}

#
# Check node allocation. If the nickname file has been created, use
# that to avoid load on tmcd.
#
# Returns 0 if node is free. Returns list (pid/eid/vname) if allocated.
#
sub check_status ()
{
    my @tmccresults;

    if (tmcc(TMCCCMD_STATUS, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get status from server!\n");
	return -1;
    }
    #
    # This is possible if the boss node does not now about us yet.
    # We want to appear free. Specifically, it could happen on the
    # MFS when trying to bring in brand new nodes. tmcd will not know
    # anything about us, and return no info.
    #
    return 0
	if (! @tmccresults);

    my $status = $tmccresults[0];

    if ($status =~ /^FREE/) {
	unlink TMNICKNAME;
	return 0;
    }

    if ($status =~ /ALLOCATED=([-\@\w]*)\/([-\@\w]*) NICKNAME=([-\@\w]*)/) {
	$pid   = $1;
	$eid   = $2;
	$vname = $3;
    }
    else {
	warn "*** WARNING: Error getting reservation status\n";
	return -1;
    }

    #
    # Stick our nickname in a file in case someone wants it.
    # Do not overwrite; we want to save the original info until later.
    # See bootsetup; indicates project change!
    #
    if (! -e TMNICKNAME()) {
	system("echo '$vname.$eid.$pid' > " . TMNICKNAME());
    }

    return ($pid, $eid, $vname);
}

#
# Check cached nickname. Its okay if we have been deallocated and the info
# is stale. The node will notice that later.
#
sub check_nickname()
{
    if (-e TMNICKNAME) {
	my $nickfile = TMNICKNAME;
	my $nickinfo = `cat $nickfile`;

	if ($nickinfo =~ /([-\@\w]*)\.([-\@\w]*)\.([-\@\w]*)/) {
	    $vname = $1;
	    $eid   = $2;
	    $pid   = $3;

	    return ($pid, $eid, $vname);
	}
    }
    return check_status();
}

#
# Do SFS hostid setup. If we have an SFS host key and we can get a hostid
# from the SFS daemon, then send it to TMCD.
#
sub initsfs()
{
    my $myhostid;

    # Default to no SFS unless we can determine we have it running.
    unlink TMUSESFS()
	if (-e TMUSESFS());

    # Do I have a host key?
    if (! -e "/etc/sfs/sfs_host_key") {
	return;
    }

    # Give hostid to TMCD
    if (-d "/usr/local/lib/sfs-0.6") {
	$myhostid = `sfskey hostid - 2>/dev/null`;
    }
    else {
	$myhostid = `sfskey hostid -s authserv - 2>/dev/null`;
    }
    if (! $?) {
	if ( $myhostid =~ /^([-\.\w_]*:[a-z0-9]*)$/ ) {
	    $myhostid = $1;
	    print STDOUT "  Hostid: $myhostid\n";
	    tmcc(TMCCCMD_SFSHOSTID, "$myhostid");
	}
	elsif ( $myhostid =~ /^(@[-\.\w_]*,[a-z0-9]*)$/ ) {
	    $myhostid = $1;
	    print STDOUT "  Hostid: $myhostid\n";
	    tmcc(TMCCCMD_SFSHOSTID, "$myhostid");
	}
	else {
	    warn "*** WARNING: Invalid hostid\n";
	    return;
	}
	system("touch " . TMUSESFS());
    }
    else {
	warn "*** WARNING: Could not retrieve this node's SFShostid!\n";
    }
}

#
# Get the role of the node and stash it for future libsetup load.
#
sub dorole()
{
    my @tmccresults;

    if (tmcc(TMCCCMD_ROLE, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get role from server!\n");
	return -1;
    }
    return 0
	if (! @tmccresults);

    #
    # There should be just one string. Ignore anything else.
    #
    if ($tmccresults[0] =~ /([\w]*)/) {
	# Storing the value into the global variable
	$role = $1;
    }
    else {
	warn "*** WARNING: Bad role line: $tmccresults[0]";
	return -1;
    }
    system("echo '$role' > " . TMROLE());
    if ($?) {
	warn "*** WARNING: Could not write role to " . TMROLE() . "\n";
    }
    return 0;
}

#
# Get the nodeid
#
sub donodeid()
{
    my $nodeid;
    my @tmccresults;

    if (tmcc(TMCCCMD_NODEID, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get nodeid from server!\n");
	return -1;
    }
    return 0
	if (! @tmccresults);

    #
    # There should be just one string. Ignore anything else.
    #
    if ($tmccresults[0] =~ /([-\w]*)/) {
	$nodeid = $1;
    }
    else {
	warn "*** WARNING: Bad nodeid line: $tmccresults[0]";
	return -1;
    }

    system("echo '$nodeid' > ". TMNODEID);
    if ($?) {
	warn "*** WARNING: Could not write nodeid to " . TMNODEID() . "\n";
    }
    return 0;
}

#
# Parse the router config and return a hash. This leaves the ugly pattern
# matching stuff here, but lets the caller do whatever with it (as is the
# case for the IXP configuration stuff). This is inconsistent with many
# other config scripts, but at some point that will change.
#
sub getifconfig($;$)
{
    my ($rptr,$nocache) = @_;	# Return list to caller (reference).
    my @tmccresults  = ();
    my @ifacelist    = ();	# To be returned to caller.
    my %ifacehash    = ();

    my %tmccopts = ();
    if ($nocache) {
	$tmccopts{"nocache"} = 1;
    }

    if (tmcc(TMCCCMD_IFC, undef, \@tmccresults, %tmccopts) < 0) {
	warn("*** WARNING: Could not get interface config from server!\n");
	@$rptr = ();
	return -1;
    }

    my $ethpat  = q(INTERFACE IFACETYPE=(\w*) INET=([0-9.]*) MASK=([0-9.]*) );
    $ethpat    .= q(MAC=(\w*) SPEED=(\w*) DUPLEX=(\w*) );
    $ethpat    .= q(IFACE=(\w*) RTABID=(\d*) LAN=([-\w\(\)]*));

    my $vethpat = q(INTERFACE IFACETYPE=(\w*) INET=([0-9.]*) MASK=([0-9.]*) );
    $vethpat   .= q(ID=(\d*) VMAC=(\w*) PMAC=(\w*) RTABID=(\d*) );
    $vethpat   .= q(ENCAPSULATE=(\d*) LAN=([-\w\(\)]*) VTAG=(\d*));

    my $setpat  = q(INTERFACE_SETTING MAC=(\w*) );
    $setpat    .= q(KEY='([-\w\.\:]*)' VAL='([-\w\.\:]*)');

    foreach my $str (@tmccresults) {
	my $ifconfig = {};

	if ($str =~ /^$setpat/) {
	    my $mac     = $1;
	    my $capkey  = $2;
	    my $capval  = $3;

	    #
	    # Stash the setting into the setting list, but must find the
	    #
	    if (!exists($ifacehash{$mac})) {
		warn("*** WARNING: ".
		     "Could not map $mac for its interface settings!\n");
		next;
	    }
	    $ifacehash{$mac}->{"SETTINGS"}->{$capkey} = $capval;
	}
	elsif ($str =~ /$ethpat/) {
	    my $ifacetype= $1;
	    my $inet     = $2;
	    my $mask     = $3;
	    my $mac      = $4;
	    my $speed    = $5;
	    my $duplex   = $6;
	    my $iface    = $7;
	    my $rtabid   = $8;
	    my $lan      = $9;

            #
            # XXX GNU Radio hack
            #
            # The GNU Radio interface has a randomly generated MAC addr when
            # it first comes up.  WE have to set it, so just tell the code
            # the name of the interface explicitly to avoid trying to look
            # it up (the iface doesn't even exist yet).
            #
            # We really need another interface flag, like 'ISGNURADIO' since
            # the only current GR iface type is hardwired below (which is bad).
            #
            if ($ifacetype eq "flex900") {
                $iface = "gr0";
            }

	    # The server can specify an iface.
	    if ($iface eq "" &&
		(! ($iface = findiface($mac)))) {
		warn("*** WARNING: Could not map $mac to an interface!\n");
		next;
	    }

	    $ifconfig->{"ISVIRT"}   = 0;
	    $ifconfig->{"TYPE"}     = $ifacetype;
	    $ifconfig->{"IPADDR"}   = $inet;
	    $ifconfig->{"IPMASK"}   = $mask;
	    $ifconfig->{"MAC"}      = $mac;
	    $ifconfig->{"SPEED"}    = $speed;
	    $ifconfig->{"DUPLEX"}   = $duplex;
	    $ifconfig->{"ALIASES"}  = "";	# gone as of version 27
	    $ifconfig->{"IFACE"}    = $iface;
	    $ifconfig->{"VIFACE"}   = $iface;
	    $ifconfig->{"RTABID"}   = $rtabid;
	    $ifconfig->{"LAN"}      = $lan;
	    $ifconfig->{"SETTINGS"} = {};
	    push(@ifacelist, $ifconfig);
	    $ifacehash{$mac}        = $ifconfig;
	}
	elsif ($str =~ /$vethpat/) {
	    my $ifacetype= $1;
	    my $inet     = $2;
	    my $mask     = $3;
	    my $id       = $4;
	    my $vmac     = $5;
	    my $pmac     = $6;
	    my $iface    = undef;
	    my $rtabid   = $7;
	    my $encap    = $8;
	    my $lan      = $9;
	    my $vtag	 = $10;

	    #
	    # Inside a jail, the vmac is really the pmac. That is, when the
	    # veth was created, it was given vmac as its ethernet address.
	    # The pmac refers to the underlying physical interface the veth
	    # is attached to, which we do not see from inside the jail.
	    #
	    if (JAILED() || GENVNODE()) {
		if (! ($iface = findiface($vmac))) {
		    warn("*** WARNING: Could not map $vmac to a veth!\n");
		    next;
		}
	    } else {

		#
		# A veth might not have any underlying physical interface
		# if the link or lan is completely contained on the node.
		# tmcd tells us that by setting the pmac to "none". Note
		# that this obviously is relevant on the physnode, not when
		# called from inside a vnode.
		#
		if ($pmac ne "none") {
		    if (! ($iface = findiface($pmac))) {
			warn("*** WARNING: Could not map $pmac to an iface!\n");
			next;
		    }
		}
	    }

	    $ifconfig->{"ISVIRT"}   = 1;
	    $ifconfig->{"ITYPE"}    = $ifacetype;
	    $ifconfig->{"IPADDR"}   = $inet;
	    $ifconfig->{"IPMASK"}   = $mask;
	    $ifconfig->{"ID"}       = $id;
	    $ifconfig->{"VMAC"}     = $vmac;
	    $ifconfig->{"MAC"}      = $vmac; # XXX
	    $ifconfig->{"PMAC"}     = $pmac;
	    $ifconfig->{"IFACE"}    = $iface;
	    $ifconfig->{"RTABID"}   = $rtabid;
	    $ifconfig->{"ENCAP"}    = $encap;
	    $ifconfig->{"LAN"}      = $lan;
	    $ifconfig->{"VTAG"}     = $vtag;

	    # determine the OS-specific virtual device name
	    $ifconfig->{"VIFACE"}   = os_viface_name($ifconfig);

	    push(@ifacelist, $ifconfig);
	}
	else {
	    warn "*** WARNING: Bad ifconfig line: $str\n";
	}
    }

    @$rptr = @ifacelist;
    return 0;
}

#
# Parse the linkdelay config and return a hash. This leaves the ugly pattern
# matching stuff here, but lets the caller do whatever with it.
#
sub getlinkdelayconfig($;$)
{
    my ($rptr,$nocache) = @_;	# Return list to caller (reference).
    my @tmccresults  = ();
    my @ldlist       = ();	# To be returned to caller.

    my %tmccopts = ();
    if ($nocache) {
	$tmccopts{"nocache"} = 1;
    }

    if (tmcc(TMCCCMD_LINKDELAYS, undef, \@tmccresults, %tmccopts) < 0) {
	warn("*** WARNING: Could not get linkdelay config from server!\n");
	@$rptr = ();
	return -1;
    }

    my $pat = q(LINKDELAY IFACE=([\d\w]+) TYPE=(simplex|duplex) );
    $pat .= q(LINKNAME=([-\d\w]+) VNODE=([-\d\w]+) );
    $pat .= q(INET=([0-9.]*) MASK=([0-9.]*) );
    $pat .= q(PIPE=(\d+) DELAY=([\d\.]+) BW=(\d+) PLR=([\d\.]+) );
    $pat .= q(RPIPE=(\d+) RDELAY=([\d\.]+) RBW=(\d+) RPLR=([\d\.]+) );
    $pat .= q(RED=(\d) LIMIT=(\d+) );
    $pat .= q(MAXTHRESH=(\d+) MINTHRESH=(\d+) WEIGHT=([\d\.]+) );
    $pat .= q(LINTERM=(\d+) QINBYTES=(\d+) BYTES=(\d+) );
    $pat .= q(MEANPSIZE=(\d+) WAIT=(\d+) SETBIT=(\d+) );
    $pat .= q(DROPTAIL=(\d+) GENTLE=(\d+));

    foreach my $str (@tmccresults) {
	my $ldc = {};

	if ($str =~ /^$pat/) {
	    $ldc->{"IFACE"} = $1;
	    $ldc->{"TYPE"} = $2;
	    $ldc->{"LINKNAME"} = $3;
	    $ldc->{"VNODE"} = $4;
	    $ldc->{"INET"} = $5;
	    $ldc->{"MASK"} = $6;
	    $ldc->{"PIPE"} = $7;
	    $ldc->{"DELAY"} = $8;
	    $ldc->{"BW"} = $9;
	    $ldc->{"PLR"} = $10;
	    $ldc->{"RPIPE"} = $11;
	    $ldc->{"RDELAY"} = $12;
	    $ldc->{"RBW"} = $13;
	    $ldc->{"RPLR"} = $14;
	    $ldc->{"RED"} = $15;
	    $ldc->{"LIMIT"} = $16;
	    $ldc->{"MAXTHRESH"} = $17;
	    $ldc->{"MINTHRESH"} = $18;
	    $ldc->{"WEIGHT"} = $19;
	    $ldc->{"LINTERM"} = $20;
	    $ldc->{"QINBYTES"} = $21;
	    $ldc->{"BYTES"} = $22;
	    $ldc->{"MEANPSIZE"} = $23;
	    $ldc->{"WAIT"} = $24;
	    $ldc->{"SETBIT"} = $25;
	    $ldc->{"DROPTAIL"} = $26;
	    $ldc->{"GENTLE"} = $27;

	    push(@ldlist, $ldc);
	}
	else {
	    warn "*** WARNING: Bad linkdelay line: $str\n";
	}
    }

    @$rptr = @ldlist;
    return 0;
}

#
# Read the topomap and return something.
#
sub gettopomap($)
{
    my ($rptr)       = @_;	# Return array to caller (reference).
    my $topomap	     = {};
    my $section;
    my @slots;

    if (! -e TMTOPOMAP()) {
	$rptr = {};
	return -1;
    }

    if (!open(TOPO, TMTOPOMAP())) {
	warn("*** WARNING: ".
	     "gettopomap: Could not open " . TMTOPOMAP() . "!\n");
	@$rptr = ();
	return -1;
    }

    #
    # First line of topo map describes the nodes.
    #
    while (<TOPO>) {
	if ($_ =~ /^\#\s*([-\w]*): ([-\w,]*)$/) {
	    $section = $1;
	    @slots = split(",", $2);

	    $topomap->{$section} = [];
	    next;
	}
	chomp($_);
	my @values = split(",", $_);
	my $rowref = {};

	for (my $i = 0; $i < scalar(@slots); $i++) {
	    $rowref->{$slots[$i]} = (defined($values[$i]) ? $values[$i] : undef);
	}
	push(@{ $topomap->{$section} }, $rowref);
    }
    close(TOPO);
    $$rptr = $topomap;
    return 0;
}

#
# Generate a hosts file given hostname info in tmcc hostinfo format
# Returns 0 on success, non-zero otherwise.
#
sub genhostsfile($@)
{
    my ($pathname, @hostlist) = @_;

    my $HTEMP = "$pathname.new";

    #
    # Note, we no longer start with the 'prototype' file here because we have
    # to make up a localhost line that's properly qualified.
    #
    if (!open(HOSTS, ">$HTEMP")) {
	warn("Could not create temporary hosts file $HTEMP\n");
	return 1;
    }

    my $localaliases = "loghost";

    #
    # Find out our domain name, so that we can qualify the localhost entry
    #
    my $hostname = `hostname`;
    if ($hostname =~ /[^.]+\.(.+)/) {
	$localaliases .= " localhost.$1";
    }

    #
    # First, write a localhost line into the hosts file - we have to know the
    # domain to use here
    #
    print HOSTS os_etchosts_line("localhost", "127.0.0.1",
				 $localaliases), "\n";

    #
    # Now convert each hostname into hosts file representation and write
    # it to the hosts file. Note that ALIASES is for backwards compat.
    # Should go away at some point.
    #
    my $pat  = q(NAME=([-\w\.]+) IP=([0-9\.]*) ALIASES=\'([-\w\. ]*)\');

    foreach my $str (@hostlist) {
	if ($str =~ /$pat/) {
	    my $name    = $1;
	    my $ip      = $2;
	    my $aliases = $3;

	    my $hostline = os_etchosts_line($name, $ip, $aliases);

	    print HOSTS "$hostline\n";
	}
	else {
	    warn("Ignoring bad hosts line: $str");
	}
    }
    close(HOSTS);
    system("mv -f $HTEMP $pathname");
    if ($?) {
	warn("Could not move $HTEMP to $pathname\n");
	return 1;
    }

    return 0;
}

#
# Generate ISC dhcpd configuration file
#
sub gendhcpdconf($$)
{
	my ($outfile, $template) = @_;
	my $tmpfile =  "/tmp/gendhcpdconf.$$";
	my @tmccresults;
	my @nodes;
        # don't cache this stuff, can't get stale dhcpd info!
        my %opthash = ( 'nocache' => 1 );

	return 0 if (!SUBBOSS());

	if (tmcc(TMCCCMD_DHCPDCONF, undef, \@tmccresults, %opthash) == 0
		&& scalar(@tmccresults)) {
	#} else {
	#	fatal("No dhcpd configuration data returned by tmcd\n");
	}

	for (@tmccresults) {
		my $node = {};
		for my $pair (split /\s+/, $_) {
			$pair =~ /([^=]+)=(.*)/;
			$$node{$1} = $2;
		}

		push @nodes, $node;
	}

	if (!open(OF, ">$tmpfile")) {
		warn("Could not open $tmpfile\n");
		return 1;
	}

	if (!open(IF,"<$template")) {
	   warn("Unable to open $template for reading");
	   return 1;
	}
	while (<IF>) {
		if (/^(\s*)\%\%nodes/) {
			my $spaces = $1;

			for my $row (@nodes) {
			    my $ip  = $$row{"IP"};
			    my $mac = $$row{"MAC"};
			    my $node_id;
			    my $next_server = $$row{"TFTP"};
			    my $bootinfo_server = $$row{"BOOTINFO"};
			    my $hostname = $$row{"HOSTNAME"};
			    my $filename = $$row{"FILENAME"};
			    my $singlenet = $$row{"SINGLENET"};
			    my $inner_elab_boot = $$row{"INNER_ELAB_BOOT"};
			    my $plab_boot = $$row{"PLAB_BOOT"};
			    my $booting;
			    my $dns;

			    if (defined $hostname) {
			    	$hostname =
				    "${spaces}\toption host-name \"$hostname\";\n";
			    }

    			    if (defined $filename) {
				$filename =~ s/^"(.*)"$/$1/;
				$filename =
				    "${spaces}\tfilename \"$filename\";\n";
			    }


			    if (defined $next_server) {
				$next_server = "${spaces}\tnext-server " .
					$next_server . ";\n";
			    }

			    if (defined $bootinfo_server) {
				$bootinfo_server = "${spaces}\toption " .
				"PXE.emulab-bootinfo " . $bootinfo_server . ";\n";
			    }

			    if ($inner_elab_boot) {
				if ($singlenet) {
					$booting  = "${spaces}\tignore booting;\n";
				} else {
					$dns = "${spaces}\toption ".
					    "domain-name-servers 1.1.1.1;\n";
				}
			    }

			    #
			    # Handle alternate boot program filename if it exists.
			    # Use mutable nodes.pxe_boot_path if it is defined.
			    # Otherwise use the node_types.pxe_boot_path if it is
			    # defined.  Otherwise don't set anything (use the global
			    # default).
			    #
			    if (defined $filename) {
			        # make sure it is pretty constrained
			        if ($filename =~ /^\/tftpboot\// && $fn !~ /\.\./) {
			            $filename = "${spaces}\tfilename \"$filename\";\n";
			        }
			}

			# Need to make MAC look right..
			$mac =~ s/(..)\B/$1:/g;

			print OF "${spaces}host $ip {\n";
			print OF $filename if $filename;
			print OF $next_server if $next_server;
			print OF $bootinfo_server if $bootinfo_server;
			print OF $dns if $dns;
			print OF $booting if $booting;
			print OF "${spaces}\thardware ethernet $mac;\n";
			print OF $hostname;
			print OF "${spaces}\tfixed-address $ip;\n";
			print OF "${spaces}}\n\n";
			}
		} elsif (/(.*\s*)\%\%subboss_ip(.*)/) {
			my $ip = os_get_ctrlnet_ip();
			print OF "$1" . $ip . "$2\n";
		} else {
		    # It's a regular line
		    print OF $_;
		}
	}
	close(IF);
	close(OF);

	if (-e $outfile) {
		system("cp -fp $outfile ${outfile}.old") == 0 or
		fatal("Could not backup copy of ${outfile}");
	}
	system("mv -f $tmpfile $outfile") == 0 or
	fatal("Could not install new ${outfile}");

	return 0;
}

#
# Convert from MAC to iface name (eth0/fxp0/etc) using little helper program.
#
# If the optional second arg is set, it is an IP address with which we
# validate the interface.  If the queries by MAC and IP return different
# interfaces, we believe the latter.  We do this because some virtual
# interfaces (like vlans and IP aliases on Linux) use the MAC address of
# the underlying physical device.  Hence, look up by MAC on those will
# return the physical interface.
#
sub findiface($;$)
{
    my($mac,$ip) = @_;
    my($iface);

    open(FIF, FINDIF . " $mac |")
	or die "Cannot start " . FINDIF . ": $!";

    $iface = <FIF>;

    if (! close(FIF)) {
	if (!defined($ip)) {
	    return 0;
	}
	#
	# MAC was bogus, if we had an IP, look that up instead
	#
	$iface = "";
    }

    $iface =~ s/\n//g;

    if (defined($ip)) {
	open(FIF, FINDIF . " -i $ip |")
	    or die "Cannot start " . FINDIF . ": $!";
	my $ipiface = <FIF>;
	if (!close(FIF)) {
	    return 0;
	}
	$ipiface =~ s/\n//g;
	if ($ipiface ne "" && $ipiface ne $iface) {
	    $iface = $ipiface;
	}
    }

    return $iface;
}

#
# Return the router configuration. We parse tmcd output here and return
# a list of hash entries to the caller.
#
sub getrouterconfig($$)
{
    my ($rptr, $ptype) = @_;		# Return list and type to caller.
    my @tmccresults = ();
    my @routes      = ();
    my $type;

    if (tmcc(TMCCCMD_ROUTING, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get routes from server!\n");
	@$rptr  = ();
	$$ptype = undef;
	return -1;
    }

    #
    # Scan for router type. If "none" we are done.
    #
    foreach my $line (@tmccresults) {
	if ($line =~ /ROUTERTYPE=(.+)/) {
	    $type = $1;
	    last;
	}
    }
    if (!defined($type) || $type eq "none") {
	@$rptr  = ();
	$$ptype = "none";
	return 0;
    }

    #
    # ROUTERTYPE=manual
    # ROUTE DEST=192.168.2.3 DESTTYPE=host DESTMASK=255.255.255.0 \
    #	NEXTHOP=192.168.1.3 COST=0 SRC=192.168.4.5
    #
    # The SRC ip is used to determine which interface the routes are
    # associated with, since nexthop alone is not enough cause of the
    #
    my $pat = q(ROUTE DEST=([0-9\.]*) DESTTYPE=(\w*) DESTMASK=([0-9\.]*) );
    $pat   .= q(NEXTHOP=([0-9\.]*) COST=([0-9]*) SRC=([0-9\.]*));

    foreach my $line (@tmccresults) {
	if ($line =~ /ROUTERTYPE=(.+)/) {
	    next;
	}
	elsif ($line =~ /$pat/) {
	    my $dip   = $1;
	    my $rtype = $2;
	    my $dmask = $3;
	    my $gate  = $4;
	    my $cost  = $5;
	    my $sip   = $6;

	    #
	    # For IXP.
	    #
	    my $rconfig = {};

	    $rconfig->{"IPADDR"}   = $dip;
	    $rconfig->{"TYPE"}     = $rtype;
	    $rconfig->{"IPMASK"}   = $dmask;
	    $rconfig->{"GATEWAY"}  = $gate;
	    $rconfig->{"COST"}     = $cost;
	    $rconfig->{"SRCIPADDR"}= $sip;
	    push(@routes, $rconfig);
	}
	else {
	    warn("*** WARNING: Bad route config line: $line\n");
	}
    }

    # Special case for distributed route calculation.
    if ($type eq "static" || $type eq "static-ddijk") {
	if (calcroutes(\@routes)) {
	    warn("*** WARNING: Could not get routes from ddijkstra!\n");
	    @$rptr  = ();
	    $$ptype = undef;
	    return -1;
	}
	$type = "static";
    }

    @$rptr  = @routes;
    $$ptype = $type;
    return 0;
}

#
# Special case. If the routertype is "static-ddijk" then we run our
# dijkstra program on the linkmap, and use that to feed the code
# below (it outputs exactly the same goo).
#
# XXX: If we change the return from tmcd, the output of dijkstra will
# suddenly be wrong. Yuck, need a better solution.
#
# We have to generate the input file from the topomap.
#
sub calcroutes ($)
{
    my ($rptr)	= @_;
    my @routes  = ();
    my $linkmap = CONFDIR() . "/linkmap";	# Happens outside jail.
    my $topomap;
    my ($pid, $eid, $myname) = check_nickname();

    if (gettopomap(\$topomap)) {
	warn("*** WARNING: Could not get topomap!\n");
	return -1;
    }

    # Special case of experiment with no lans; no routes needed.
    if (! scalar(@{ $topomap->{"lans"} })) {
	@$rptr = ();
	return 0;
    }

    # Gather up all the link info from the topomap
    my %lans     = ();
    my $nnodes   = 0;

    # The nodes section tells us the name of each node, and all its links.
    foreach my $noderef (@{ $topomap->{"nodes"} }) {
	my $vname  = $noderef->{"vname"};
	my $links  = $noderef->{"links"};

	if (!defined($links)) {
	    # If we have no links, there are no routes to compute.
	    if ($vname eq $myname) {
		@$rptr = ();
		return 0;
	    }
	    next;
	}

	# Links is a string of "$lan1:$ip1 $lan2:$ip2 ..."
	foreach my $link (split(" ", $links)) {
	    my ($lan,$ip) = split(":", $link);

	    if (! defined($lans{$lan})) {
		$lans{$lan} = {};
		$lans{$lan}->{"members"} = {};
	    }
	    $lans{$lan}->{"members"}->{"$vname:$ip"} = $ip;
	}

	$nnodes++;
    }

    # The lans section tells us the masks and the costs.
    foreach my $lanref (@{ $topomap->{"lans"} }) {
	my $vname  = $lanref->{"vname"};
	my $cost   = $lanref->{"cost"};
	my $mask   = $lanref->{"mask"};

	$lans{$vname}->{"cost"} = $cost;
	$lans{$vname}->{"mask"} = $mask;
    }

    #
    # Construct input for Jon's dijkstra program.
    #
    if (! open(MAP, ">$linkmap")) {
	warn("*** WARNING: Could not create $linkmap!\n");
	@$rptr  = ();
	return -1;
    }

    # Count edges, but just once each.
    my $edges = 0;
    foreach my $lan (keys(%lans)) {
	my @members = sort(keys(%{ $lans{$lan}->{"members"} }));

	for (my $i = 0; $i < scalar(@members); $i++) {
	    for (my $j = $i; $j < scalar(@members); $j++) {
		my $member1 = $members[$i];
		my $member2 = $members[$j];

		$edges++
		    if ($member1 ne $member2);
	    }
	}
    }

    # Header line for Jon. numnodes numedges
    print MAP "$nnodes $edges\n";

    # And then a list of edges: node1 ip1 node2 ip2 cost
    foreach my $lan (keys(%lans)) {
	my @members = sort(keys(%{ $lans{$lan}->{"members"} }));
	my $cost    = $lans{$lan}->{"cost"};
	my $mask    = $lans{$lan}->{"mask"};

	for (my $i = 0; $i < scalar(@members); $i++) {
	    for (my $j = $i; $j < scalar(@members); $j++) {
		my $member1 = $members[$i];
		my $member2 = $members[$j];

		if ($member1 ne $member2) {
		    my ($node1,$ip1) = split(":", $member1);
		    my ($node2,$ip2) = split(":", $member2);

		    print MAP "$node1 " . $ip1 . " " .
			      "$node2 " . $ip2 . " $cost\n";
		}
	    }
	}
    }
    close(MAP);
    undef($topomap);
    undef(%lans);

    #
    # Now run the dijkstra program on the input.
    # --compress generates "net" routes
    #
    if (!open(DIJK, "cat $linkmap | $BINDIR/dijkstra --compress --source=$myname |")) {
	warn("*** WARNING: Could not invoke dijkstra on linkmap!\n");
	@$rptr  = ();
	return -1;
    }
    my $pat = q(ROUTE DEST=([0-9\.]*) DESTTYPE=(\w*) DESTMASK=([0-9\.]*) );
    $pat   .= q(NEXTHOP=([0-9\.]*) COST=([0-9]*) SRC=([0-9\.]*));

    while (<DIJK>) {
	if ($_ =~ /ROUTERTYPE=(.+)/) {
	    next;
	}
	if ($_ =~ /$pat/) {
	    my $dip   = $1;
	    my $rtype = $2;
	    my $dmask = $3;
	    my $gate  = $4;
	    my $cost  = $5;
	    my $sip   = $6;

	    my $rconfig = {};
	    $rconfig->{"IPADDR"}   = $dip;
	    $rconfig->{"TYPE"}     = $rtype;
	    $rconfig->{"IPMASK"}   = $dmask;
	    $rconfig->{"GATEWAY"}  = $gate;
	    $rconfig->{"COST"}     = $cost;
	    $rconfig->{"SRCIPADDR"}= $sip;
	    push(@routes, $rconfig);
	}
	else {
	    warn("*** WARNING: Bad route config line: $_\n");
	}
    }
    if (! close(DIJK)) {
	if ($?) {
	    warn("*** WARNING: dijkstra exited with status $?!\n");
	}
	else {
	    warn("*** WARNING: Error closing dijkstra pipe: $!\n");
	}
	@$rptr  = ();
	return -1;
    }
    @$rptr = @routes;
    return 0;
}

#
# Get trafgen configuration.
#
sub gettrafgenconfig($)
{
    my ($rptr)   = @_;
    my @trafgens = ();

    if (tmcc(TMCCCMD_TRAFFIC, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get trafgen config from server!\n");
	return -1;
    }

    my $pat  = q(TRAFGEN=([-\w.]+) MYNAME=([-\w.]+) MYPORT=(\d+) );
    $pat    .= q(PEERNAME=([-\w.]+) PEERPORT=(\d+) );
    $pat    .= q(PROTO=(\w+) ROLE=(\w+) GENERATOR=(\w+));

    foreach my $str (@tmccresults) {
	if ($str =~ /$pat/) {
	    my $trafgen = {};

	    $trafgen->{"NAME"}       = $1;
	    $trafgen->{"SRCHOST"}    = $2;
	    $trafgen->{"SRCPORT"}    = $3;
	    $trafgen->{"PEERHOST"}   = $4;
	    $trafgen->{"PEERPORT"}   = $5;
	    $trafgen->{"PROTO"}      = $6;
	    $trafgen->{"ROLE"}       = $7;
	    $trafgen->{"GENERATOR"}  = $8;
	    push(@trafgens, $trafgen);

	    #
	    # Flag node as doing NSE trafgens for other scripts.
	    #
	    if ($trafgen->{"GENERATOR"} eq "NSE") {
		system("touch " . ISSIMTRAFGENPATH);
		next;
	    }
	}
	else {
	    warn("*** WARNING: Bad traffic line: $str\n");
	}
    }
    @$rptr = @trafgens;
    return 0;
}

#
# Get trace configuration.
#
sub gettraceconfig($)
{
    my ($rptr)    = @_;
    my @traceinfo = ();

    if (tmcc(TMCCCMD_TRACEINFO, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get trace config from server!\n");
	return -1;
    }

    my $pat = q(TRACE LINKNAME=([-\d\w]+) IDX=(\d*) MAC0=(\w*) MAC1=(\w*) );
    $pat   .= q(VNODE=([-\d\w]+) VNODE_MAC=(\w*) );
    $pat   .= q(TRACE_TYPE=([-\d\w]+) );
    $pat   .= q(TRACE_EXPR='(.*)' );
    $pat   .= q(TRACE_SNAPLEN=(\d*));

    foreach my $str (@tmccresults) {
	if ($str =~ /$pat/) {
	    my $trace = {};

	    $trace->{"LINKNAME"}      = $1;
	    $trace->{"IDX"}           = $2;
	    $trace->{"MAC0"}          = $3;
	    $trace->{"MAC1"}          = $4;
	    $trace->{"VNODE"}         = $5;
	    $trace->{"VNODE_MAC"}     = $6;
	    $trace->{"TRACE_TYPE"}    = $7;
	    $trace->{"TRACE_EXPR"}    = $8;
	    $trace->{"TRACE_SNAPLEN"} = $9;
	    push(@traceinfo, $trace);
	}
	else {
	    warn("*** WARNING: Bad traceinfo line: $str\n");
	}
    }
    @$rptr = @traceinfo;
    return 0;
}

#
# Get tunnels configuration.
#
sub gettunnelconfig($)
{
    my ($rptr)   = @_;
    my @tmccresults = ();
    my $tunnels  = {};

    if (tmcc(TMCCCMD_TUNNEL, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get tunnel config from server!\n");
	return -1;
    }

    my $pat  = q(TUNNEL=([\w]+) MEMBER=([\w]+) KEY='(.*)' VALUE='(.*)');

    foreach my $str (@tmccresults) {
	if ($str =~ /$pat/) {
	    my $tunnel = $1;
	    my $member = $2;
	    my $key    = $3;
	    my $value  = $4;

	    $tunnels->{"$tunnel:$member"}->{$key} = $value;
	}
	else {
	    warn("*** WARNING: Bad tunnels line: $str\n");
	}
    }
    $$rptr = $tunnels;
    return 0;
}

#
# Get tiptunnels configuration.
#
sub gettiptunnelconfig($)
{
    my ($rptr)   = @_;
    my @tiptunnels = ();

    if (tmcc(TMCCCMD_TIPTUNNELS, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get tiptunnel config from server!\n");
	return -1;
    }

    my $pat  = q(VNODE=([-\w.]+) SERVER=([-\w.]+) PORT=(\d+) );
    $pat    .= q(KEYLEN=(\d+) KEY=([-\w.]+));

    my $ACLDIR = "/var/log/tiplogs";

    mkdir("$ACLDIR", 0755);
    foreach my $str (@tmccresults) {
	if ($str =~ /$pat/) {
	    if (!open(ACL, "> $ACLDIR/$1.acl")) {
		warn("*** WARNING: ".
		     "gettiptunnelconfig: Could not open $ACLDIR/$1.acl\n");
		return -1;
	    }

	    print ACL "host: $2\n";
	    print ACL "port: $3\n";
	    print ACL "keylen: $4\n";
	    print ACL "key: $5\n";
	    close(ACL);

	    push(@tiptunnels, $1);
	}
	else {
	    warn("*** WARNING: Bad tiptunnels line: $str\n");
	}
    }
    @$rptr = @tiptunnels;
    return 0;
}

#
# Get motelog configuration.
#
sub getmotelogconfig($)
{
    my ($rptr)   = @_;
    my @motelogs = ();

    if (tmcc(TMCCCMD_MOTELOG, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get motelog config from server!\n");
	return -1;
    }

    my $pat  = q(MOTELOGID=([-\w]+) CLASSFILE=([\.]+) SPECFILE=([\.]*));

    foreach my $str (@tmccresults) {
	if ($str =~ /$pat/) {
	    push(@motelogs, { "MOTELOGID" => $1,
			      "CLASSFILE" => $2,
			      "SPECFILE"  => $3
			    });
	}
	else {
	    warn("*** WARNING: Bad motelog line: $str\n");
	}
    }
    @$rptr = @motelogs;
    return 0;
}

#
# Get load info.
#
sub getloadinfo($)
{
    my ($rptr)   = @_;
    my @retval = ();
    my @tmccresults = ();
    # don't cache this stuff, can't get stale reload info!
    my %opthash = ( 'nocache' => 1 );

    if (tmcc(TMCCCMD_LOADINFO, undef, \@tmccresults, %opthash) < 0) {
	warn("*** WARNING: Could not get loadinfo from server!\n");
	return -1;
    }

    # accept any key/val pair with basic formatting
    foreach my $res (@tmccresults) {
	chomp($res);
	my @kvs = split(/\s+/,$res);
	my %resh = ();
	foreach my $kv (@kvs) {
	    my @kvpair = split(/=/,$kv);
	    if (scalar(@kvpair) != 2) {
		warn("*** WARNING: malformed key-val pair in loadinfo: $kv\n");
	    }
	    else {
		$resh{$kvpair[0]} = $kvpair[1];
	    }
	}
	push @retval, \%resh;
    }

    @$rptr = @retval;
    return 0;
}

#
# Get load info.
#
sub getbootwhat($)
{
    my ($rptr)   = @_;
    my @retval = ();
    my @tmccresults = ();
    # don't cache this stuff, can't get stale boot info!
    my %opthash = ( 'nocache' => 1 );

    if (tmcc(TMCCCMD_BOOTWHAT, undef, \@tmccresults, %opthash) < 0) {
	warn("*** WARNING: Could not get bootwhat from server!\n");
	return -1;
    }

    # accept any key/val pair with basic formatting
    foreach my $res (@tmccresults) {
	chomp($res);
	my @kvs = split(/\s+/,$res);
	my %resh = ();
	foreach my $kv (@kvs) {
	    my @kvpair = split(/=/,$kv);
	    if (scalar(@kvpair) != 2) {
		warn("*** WARNING: malformed key-val pair in bootwhat: $kv\n");
	    }
	    else {
		$resh{$kvpair[0]} = $kvpair[1];
	    }
	}
	push @retval, \%resh;
    }

    @$rptr = @retval;
    return 0;
}

#
# Do everything in our power to copy a file.
# The main "specialness" about this function is that it tries to work
# around the old FreeBSD NFS server race with changing the exports list--
# we retry the copy several times before failing.
# Returns one on success, zero on failure.
#
sub forcecopy($$)
{
    my ($ffile, $tfile) = @_;
    my $tries = 1;

    #
    # If the file server has NFS races, we try operations multiple
    # times in case we hit the EPERM window.
    #
    if (FSRVTYPE() eq "NFS-RACY") {
	$tries = 5;
    }

    for (my $i = 0; $i < $tries; $i++) {
	if (system("cp -fp $ffile $tfile >/dev/null 2>&1") == 0) {
	    return 1;
	}
    }
    return 0;
}

my %fwvars = ();

#
# Substitute values of variables in a firewall rule.
#
sub expandfwvars($)
{
    my ($rule) = @_;

    if ($rule->{RULE} =~ /EMULAB_\w+/) {
	foreach my $key (keys %fwvars) {
	    $rule->{RULE} =~ s/$key/$fwvars{$key}/g
		if (defined($fwvars{$key}));
	}
	if ($rule->{RULE} =~ /EMULAB_\w+/) {
	    warn("*** WARNING: Unexpanded firewall variable in: \n".
		 "    $rule->{RULE}\n");
	    return 1;
	}
    }
    return 0;
}

#
# Return the firewall configuration. We parse tmcd output here and return
# a list of hash entries to the caller.
#
sub getfwconfig($$;$)
{
    my ($infoptr, $rptr, $hptr) = @_;
    my @tmccresults = ();
    my $fwinfo      = {};
    my @fwrules     = ();
    my @fwhosts	    = ();
    my %fwhostmacs  = ();

    $$infoptr = undef;
    @$rptr = ();
    if (tmcc(TMCCCMD_FIREWALLINFO, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get firewall info from server!\n");
	return -1;
    }

    my $rempat = q(TYPE=remote FWIP=([\d\.]*));
    my $fwpat  = q(TYPE=([-\w]+) STYLE=(\w+) IN_IF=(\w*) OUT_IF=(\w*) IN_VLAN=(\d+) OUT_VLAN=(\d+));
    my $rpat   = q(RULENO=(\d*) RULE="(.*)");
    my $vpat   = q(VAR=(EMULAB_\w+) VALUE="(.*)");
    my $hpat   = q(HOST=([-\w]+) CNETIP=([\d\.]*) CNETMAC=([\da-f]{12}));
    my $lpat   = q(LOG=([\w,]+));

    $fwinfo->{"TYPE"} = "none";
    foreach my $line (@tmccresults) {
	if ($line =~ /TYPE=([\w-]+)/) {
	    my $type = $1;
	    if ($type eq "none") {
		$fwinfo->{"TYPE"} = $type;
		$$infoptr = $fwinfo;
		return 0;
	    }
	    if ($line =~ /$rempat/) {
		my $fwip = $1;

		$fwinfo->{"TYPE"} = "remote"
		    if (!defined($fwinfo->{"TYPE"}));
		$fwinfo->{"FWIP"} = $fwip;
	    } elsif ($line =~ /$fwpat/) {
		my $style = $2;
		my $inif = $3;
		my $outif = $4;
		my $invlan = $5;
		my $outvlan = $6;

		$fwinfo->{"TYPE"} = $type;
		$fwinfo->{"STYLE"} = $style;
		$fwinfo->{"IN_IF"}  = $inif;
		$fwinfo->{"OUT_IF"} = $outif;
		$fwinfo->{"IN_VLAN"}  = $invlan
		    if ($invlan != 0);
		$fwinfo->{"OUT_VLAN"} = $outvlan
		    if ($outvlan != 0);
	    } else {
		warn("*** WARNING: Bad firewall info line: $line\n");
		return 1;
	    }
	} elsif ($line =~ /$rpat/) {
	    my $ruleno = $1;
	    my $rule = $2;

	    my $fw = {};
	    $fw->{"RULENO"} = $ruleno;
	    $fw->{"RULE"} = $rule;
	    push(@fwrules, $fw);
	} elsif ($line =~ /$vpat/) {
	    $fwvars{$1} = $2;
	} elsif ($line =~ /$hpat/) {
	    my $host = $1;
	    my $ip = $2;
	    my $mac = $3;

	    # create a tmcc hostlist format string
	    push(@fwhosts,
		 "NAME=$host IP=$ip ALIASES=''");

	    # and save off the MACs
	    $fwhostmacs{$host} = $mac;
	} elsif ($line =~ /$lpat/) {
	    for my $log (split(',', $1)) {
		if ($log =~ /^allow|accept$/) {
		    $fwinfo->{"LOGACCEPT"} = 1;
		} elsif ($log =~ /^deny|reject$/) {
		    $fwinfo->{"LOGREJECT"} = 1;
		} elsif ($log eq "tcpdump") {
		    $fwinfo->{"LOGTCPDUMP"} = 1;
		}
	    }
	} else {
	    warn("*** WARNING: Bad firewall info line: $line\n");
	    return 1;
	}
    }

    # XXX inner elab: make sure we have a "myfs" entry
    if (defined($fwhostmacs{"myboss"}) && !defined($fwhostmacs{"myfs"})) {
	for my $host (@fwhosts) {
	    if ($host =~ /NAME=myops/) {
		$host =~ s/ALIASES=''/ALIASES='myfs'/;
	    }
	}
    }

    # info for proxy ARP
    $fwinfo->{"GWIP"} = $fwvars{"EMULAB_GWIP"};
    $fwinfo->{"GWMAC"} = $fwvars{"EMULAB_GWMAC"};
    if (%fwhostmacs) {
	$fwinfo->{"MACS"} = \%fwhostmacs;
    }

    # make a pass over the rules, expanding variables
    my $bad = 0;
    foreach my $rule (@fwrules) {
	$bad += expandfwvars($rule);
    }

    $$infoptr = $fwinfo;
    @$rptr = @fwrules;
    @$hptr = @fwhosts;
    return $bad;
}


#
# All we do is store it away in the file. This makes it avail later.
#
sub dojailconfig()
{
    my @tmccresults;

    if (tmcc(TMCCCMD_JAILCONFIG, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get jailconfig from server!\n");
	return -1;
    }
    return 0
	if (! @tmccresults);

    if (!open(RC, ">" . TMJAILCONFIG)) {
	warn "*** WARNING: Could not write " . TMJAILCONFIG . "\n";
	return -1;
    }
    foreach my $str (@tmccresults) {
	print RC $str;
    }
    close(RC);
    chmod(0755, TMJAILCONFIG);
    return 0;
}

#
# Boot Startup code. This is invoked from the setup OS dependent script,
# and this fires up all the stuff above.
#
sub bootsetup()
{
    my $oldpid;

    # Tell libtmcc to forget anything it knows.
    tmccclrconfig();

    #
    # Watch for a change in project membership. This is not supposed
    # to happen, but it is good to check for this anyway just in case.
    # A little tricky though since we have to know what project we used
    # to be in. Use the nickname file for that.
    #
    if (-e TMNICKNAME) {
	($oldpid) = check_nickname();
	unlink TMNICKNAME;
    }

    #
    # Check allocation. Exit now if not allocated.
    #
    if (! check_status()) {
	print STDOUT "  Node is free!\n";
	cleanup_node(1);
	return undef;
    }

    #
    # Project Change?
    #
    if (defined($oldpid) && ($oldpid ne $pid)) {
	print STDOUT "  Node switched projects: $oldpid\n";
	# This removes the nickname file, so do it again.
	cleanup_node(1);
	check_status();
	# Must reset the passwd/group file. Yuck.
	system("$BINDIR/rc/rc.accounts reset");
    }
    else {
	#
	# Cleanup node. Flag indicates to gently clean ...
	#
	cleanup_node(0);
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    #
    # Setup SFS hostid. Must do this before asking tmcd for config.
    #
    if (!MFS()) {
	print STDOUT "Setting up for SFS ... \n";
	initsfs();
    }

    #
    # Tell libtmcc to get the full config. Note that this must happen
    # AFTER initsfs() right above, since that changes what tmcd
    # is going to tell us.
    #
    tmccgetconfig();

    #
    # Get the role of this node from tmcc which can be one of
    # "node", "virthost", "delaynode" or "simhost".
    # Mainly useful for simulation (nse) stuff
    # Hopefully, this will come out of the tmcc cache and will not
    # be expensive.
    #
    dorole();

    #
    # And the nodeid.
    #
    donodeid();

    return ($pid, $eid, $vname);
}

#
# Shadow mode setup. The server argument is the remote boss we talk to.
#
sub shadowsetup($$)
{
    my ($server, $urn) = @_;

    $shadow = 1;

    # This changes where tmcc is going to store the data.
    libtmcc::configtmcc("cachedir", $SHADOWDIR);
    libtmcc::configtmcc("server", $server);
    libtmcc::configtmcc("urn", $urn);

    # No proxy.
    libtmcc::configtmcc("noproxy", 1);

    # Tell children.
    $ENV{'SHADOW'} = "$server,$urn";
    $ENV{'URN'}    = $urn;

    # Tell libtmcc to forget anything it knows.
    tmccclrconfig();

    #
    # Check allocation. Exit now if not allocated.
    #
    if (! check_status()) {
	return undef;
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    #
    # Tell libtmcc to get the full config.
    #
    tmccgetconfig();

    #
    # Get the role of this node from tmcc which can be one of
    # "node", "virthost", "delaynode" or "simhost".
    # Mainly useful for simulation (nse) stuff
    # Hopefully, this will come out of the tmcc cache and will not
    # be expensive.
    #
    dorole();

    #
    # And the nodeid.
    #
    donodeid();

    my $eiddir = EXPDIR() . "/tbdata";
    os_mkdir($eiddir, "0777");

    return ($pid, $eid, $vname);
}

#
# This happens inside a jail.
#
sub jailsetup()
{
    #
    # Currently, we rely on the outer environment to set our vnodeid
    # into the environment so we can get it! See mkjail.pl.
    #
    my $vid = $ENV{'TMCCVNODEID'};

    #
    # Set global vnodeid for tmcc commands. Must be before all the rest!
    #
    libsetup_setvnodeid($vid);
    $injail   = 1;

    #
    # Create a file inside so that libsetup inside the jail knows its
    # inside a jail and what its ID is.
    #
    system("echo '$vnodeid' > " . TMJAILNAME());
    # Need to unify this with jailname.
    system("echo '$vnodeid' > " . TMNODEID());

    #
    # Always remove the old nickname file.  No need to worry about a project
    # change at this level (see bootsetup) but we do need to make sure we
    # pick up on a vnode/jail being reassigned to a different virtual node.
    #
    unlink TMNICKNAME;

    print STDOUT "Checking Testbed reservation status ... \n";
    if (! check_status()) {
	print STDOUT "  Free!\n";
	return 0;
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    print STDOUT "Checking Testbed jail configuration ...\n";
    dojailconfig();

    return ($pid, $eid, $vname);
}

#
# Bogus emulation of jails without a jail,
#
sub fakejailsetup()
{
    $nojail = 1;

    # Stick this into the environment so that sub processes know.
    $ENV{'FAKEJAIL'} = $vnodeid;

    #
    # Create a file inside so that libsetup inside the jail knows its
    # inside a jail and what its ID is.
    #
    system("echo '$vnodeid' > " . TMFAKEJAILNAME());
    # Need to unify this with jailname.
    system("echo '$vnodeid' > " . TMNODEID());

    #
    # Always remove the old nickname file.  No need to worry about a project
    # change at this level (see bootsetup) but we do need to make sure we
    # pick up on a vnode/jail being reassigned to a different virtual node.
    #
    unlink TMNICKNAME;

    print STDOUT "Checking Testbed reservation status ... \n";
    if (! check_status()) {
	print STDOUT "  Free!\n";
	return 0;
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    return ($pid, $eid, $vname);
}

#
# Remote Node virtual node jail setup. This happens outside the jailed
# env.
#
sub vnodejailsetup($)
{
    my ($vid) = @_;

    #
    # Set global vnodeid for tmcc commands.
    #
    libsetup_setvnodeid($vid);

    #
    # This is the directory where the rc files go.
    #
    if (! -e JAILDIR()) {
	die("*** $0:\n".
	    "    No such directory: " . JAILDIR() . "\n");
    }

    #
    # Always remove the old nickname file.  No need to worry about a project
    # change at this level (see bootsetup) but we do need to make sure we
    # pick up on a vnode/jail being reassigned to a different virtual node.
    #
    unlink TMNICKNAME;

    # Do not bother if somehow got released.
    if (! check_status()) {
	print "Node is free!\n";
	return undef;
    }

    #
    # Create /local directories for users.
    #
    if (! -e LOCALROOTFS()) {
	os_mkdir(LOCALROOTFS(), "0755");
    }
    if (-e LOCALROOTFS()) {
	my $piddir = LOCALROOTFS() . "/$pid";
	my $eiddir = LOCALROOTFS() . "/$pid/$eid";
	my $viddir = LOCALROOTFS() . "/$pid/$vid";

	if (! -e $piddir) {
	    mkdir($piddir, 0777) or
		die("*** $0:\n".
		    "    mkdir filed - $piddir: $!\n");
	}
	if (! -e $eiddir) {
	    mkdir($eiddir, 0777) or
		die("*** $0:\n".
		    "    mkdir filed - $eiddir: $!\n");
	}
	if (! -e $viddir) {
	    mkdir($viddir, 0775) or
		die("*** $0:\n".
		    "    mkdir filed - $viddir: $!\n");
	}
	chmod(0777, $piddir);
	chmod(0777, $eiddir);
	chmod(0775, $viddir);
    }

    #
    # Tell libtmcc to get the full config for the jail. At the moment
    # we do not use SFS inside jails, so okay to do this now (usually
    # have to call initsfs() first). The full config will be copied
    # to the proper location inside the jail by mkjail.
    #
    tmccgetconfig();

    #
    # Get jail config.
    #
    print STDOUT "Checking Testbed jail configuration ...\n";
    dojailconfig();

    return ($pid, $eid, $vname);
}

#
# All we do is store it away in the file. This makes it avail later.
#
sub stashgenvnodeconfig()
{
    my @tmccresults;

    # XXX: use GENVNODECONFIG once it's written!
    if (tmcc(TMCCCMD_JAILCONFIG, undef, \@tmccresults) < 0) {
	warn("*** WARNING: Could not get genvnodeconfig from server!\n");
	return -1;
    }
    return 0
	if (! @tmccresults);

    if (!open(RC, ">" . TMGENVNODECONFIG)) {
	warn "*** WARNING: Could not write " . TMGENVNODECONFIG . "\n";
	return -1;
    }
    foreach my $str (@tmccresults) {
	print RC $str;
    }
    close(RC);
    chmod(0755, TMGENVNODECONFIG);
    return 0;
}

#
# Return the generic vnode config info in a hash.  XXX: For now uses jailconfig.
#
sub getgenvnodeconfig($)
{
    my ($rptr) = @_;
    my @tmccresults = ();
    my %vconfig = ();
    my $issharedhost = SHAREDHOST();

    my %tmccopts = ();
    if ($issharedhost) {
	$tmccopts{"nocache"} = 1;
    }

    # XXX uses jailconfig instead of genvmconfig
    if (tmcc(TMCCCMD_JAILCONFIG, undef, \@tmccresults, %tmccopts) < 0) {
	warn("*** WARNING: Could not get genvmconfig from server!\n");
	%$rptr = ();
	return -1;
    }

    foreach my $line (@tmccresults) {
	if ($line =~ /^(.*)="(.*)"$/ ||
	    $line =~ /^(.*)=(.+)$/) {
	    if ($1 eq 'JAILIP'
		&& $2 =~ /^(\d+\.\d+\.\d+\.\d+),(\d+\.\d+\.\d+\.\d+)$/) {
		$vconfig{"CTRLIP"} = $1;
		$vconfig{"CTRLMASK"} = $2;
	    }
	    else {
		$vconfig{$1} = $2;
	    }
	}
    }

    %$rptr = %vconfig;
    return 0;
}

#
# Virtual node vm setup. This happens outside in the root context.
#
sub genvnodesetup($;$$)
{
    my ($vid) = @_;
    my $issharedhost = SHAREDHOST();

    #
    # Set global vnodeid for tmcc commands.
    #
    libsetup_setvnodeid($vid);

    #
    # This is the directory where the rc files go.
    #
    if (! -e GENVNODEDIR()) {
	os_mkdir(GENVNODEDIR(),"0755");
	#die("*** $0:\n".
	#    "    No such directory: " . GENVNODEDIR() . "\n");
    }

    #
    # Always remove the old nickname file.  No need to worry about a project
    # change at this level (see bootsetup) but we do need to make sure we
    # pick up on a vnode/jail being reassigned to a different virtual node.
    #
    unlink TMNICKNAME;

    # Do not bother if somehow got released.
    if (! check_status()) {
	print "Node is free!\n";
	return undef;
    }

    #
    # Create /local directories for users.
    #
    if (! -e LOCALROOTFS()) {
	os_mkdir(LOCALROOTFS(), "0755");
    }
    if (-e LOCALROOTFS()) {
	my $piddir = LOCALROOTFS() . "/$pid";
	my $eiddir = LOCALROOTFS() . "/$pid/$eid";
	my $viddir = LOCALROOTFS() . "/$pid/$vid";

	if (! -e $piddir) {
	    mkdir($piddir, 0777) or
		die("*** $0:\n".
		    "    mkdir filed - $piddir: $!\n");
	}
	if (! -e $eiddir) {
	    mkdir($eiddir, 0777) or
		die("*** $0:\n".
		    "    mkdir filed - $eiddir: $!\n");
	}
	if (! -e $viddir) {
	    mkdir($viddir, 0775) or
		die("*** $0:\n".
		    "    mkdir filed - $viddir: $!\n");
	}
	chmod(0777, $piddir);
	chmod(0777, $eiddir);
	chmod(0775, $viddir);
    }

    #
    # Tell libtmcc to get the full config for the jail. At the moment
    # we do not use SFS inside jails, so okay to do this now (usually
    # have to call initsfs() first). The full config will be copied
    # to the proper location inside the jail by mkjail.
    #
    tmccclrconfig()
	if ($issharedhost);
    tmccgetconfig();

    #
    # XXX: Get jail config.  For now we just snoop on this, but should do our
    # own later.
    #
    print STDOUT "Checking Testbed generic VM configuration ...\n";
    stashgenvnodeconfig();

    return ($pid, $eid, $vname);
}

#
# This happens inside a Plab vserver.
#
sub plabsetup()
{
    # Tell libtmcc to forget anything it knows.
    tmccclrconfig();

    #
    # vnodeid will either be found in BEGIN block or will be passed to
    # vnodeplabsetup, so it doesn't need to be found here
    #
    print STDOUT "Checking Testbed reservation status ... \n";
    if (! check_status()) {
	print STDOUT "  Free!\n";
	return 0;
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    #
    # Setup SFS hostid.
    #
    print STDOUT "Setting up for SFS ... \n";
    initsfs();

    #
    # Tell libtmcc to get the full config. Note that this must happen
    # AFTER initsfs() right above, since that changes what tmcd
    # is going to tell us.
    #
    tmccgetconfig();

    return ($pid, $eid, $vname);
}

#
# Remote node virtual node Plab setup.  This happens inside the vserver
# environment (because on Plab you can't escape)
#
sub vnodeplabsetup($)
{
    my ($vid) = @_;

    #
    # Set global vnodeid for tmcc commands.
    #
    libsetup_setvnodeid($vid);
    $inplab   = 1;

    # Do not bother if somehow got released.
    if (! check_status()) {
	print "Node is free!\n";
	return undef;
    }

    #
    # Create a file so that libsetup knows it's inside Plab and what
    # its ID is.
    #
    system("echo '$vnodeid' > $BOOTDIR/plabname");
    # Need to unify this with plabname.
    system("echo '$vnodeid' > $BOOTDIR/nodeid");

    return ($pid, $eid, $vname);
}

#
# IXP config. This happens on the outside since there is currently no
# inside setup (until there is a reasonable complete environment).
#
sub ixpsetup($)
{
    my ($vid) = @_;

    #
    # Set global vnodeid for tmcc commands.
    #
    libsetup_setvnodeid($vid);

    #
    # Config files go here.
    #
    if (! -e CONFDIR()) {
	die("*** $0:\n".
	    "    No such directory: " . CONFDIR() . "\n");
    }

    # Do not bother if somehow got released.
    if (! check_status()) {
	print "Node is free!\n";
	return undef;
    }
    $inixp    = 1;

    #
    # Different approach for IXPs. The ixp setup code will call the routines
    # directly.
    #

    return ($pid, $eid, $vname);
}

#
# Report startupcmd status back to TMCD. Called by the runstartup
# script.
#
sub startcmdstatus($)
{
    my($status) = @_;

    return(tmcc(TMCCCMD_STARTSTAT, "$status"));
}

#
# Early on in the boot, we want to reset the hostname. This gets the
# nickname and returns it.
#
# This is going to get invoked very early in the boot process, before the
# normal client initialization. So we have to do a few things to make
# things are consistent.
#
sub whatsmynickname()
{
    #
    # Check allocation. Exit now if not allocated.
    #
    if (! check_status()) {
	return 0;
    }

    return "$vname.$eid.$pid";
}

#
# Return the hostname or IP to use for a local event server.
# Defaults to "localhost" for most nodes or the physical host IP for Xen VMs.
# The value can be overridden on a per-host basis via a local file.
#
sub getlocalevserver()
{
    my $evserver = "localhost";

    if (INXENVM()) {
	#
	# XXX gawdawful hack alert!
	# Will only work with Utah naming convention.
	#
	if ($vnodeid =~ /^pcvm(\d+)-\d+$/) {
	    $evserver = "pc$1";
	}
    }
    if (-e "$BOOTDIR/localevserver") {
	$evserver = `cat $BOOTDIR/localevserver`;
	chomp($evserver);
    }

    return $evserver;
}

#
# Put ourselves into the background, directing output to the log file.
# The caller provides the logfile name, which should have been created
# with mktemp, just to be safe. Returns the usual return of fork.
#
# usage int TBBackGround(char *filename).
#
sub TBBackGround($)
{
    my($logname) = @_;

    my $mypid = fork();
    if ($mypid) {
	return $mypid;
    }
    select(undef, undef, undef, 0.2);

    #
    # We have to disconnect from the caller by redirecting both STDIN and
    # STDOUT away from the pipe. Otherwise the caller (the web server) will
    # continue to wait even though the parent has exited.
    #
    open(STDIN, "< /dev/null") or
	die("opening /dev/null for STDIN: $!");

    # Note different taint check (allow /).
    if ($logname =~ /^([-\@\w.\/]+)$/) {
	$logname = $1;
    } else {
	die("Bad data in logfile name: $logname\n");
    }

    open(STDERR, ">> $logname") or die("opening $logname for STDERR: $!");
    open(STDOUT, ">> $logname") or die("opening $logname for STDOUT: $!");

    return 0;
}

#
# Fork a process to exec a command. Return the pid to wait on.
#
sub TBForkCmd($) {
    my ($cmd) = @_;
    my($mypid);

    $mypid = fork();
    if ($mypid) {
	return $mypid;
    }

    system($cmd);
    exit($? >> 8);
}

#
# Return a timestamp. We don't care about day/date/year. Just the time mam.
#
# TBTimeStamp()
#
my $imported_hires = 0;
my $imported_POSIX = 0;

sub TBTimeStamp()
{
    # To avoid problems with images not having the module installed yet.
    if (! $imported_hires) {
	require Time::HiRes;
	import Time::HiRes;
	$imported_hires = 1;
    }
    my ($seconds, $microseconds) = Time::HiRes::gettimeofday();

    if (! $imported_POSIX) {
	require POSIX;
	import POSIX qw(strftime);
	$imported_POSIX = 1;
    }
    return POSIX::strftime("%H:%M:%S", localtime($seconds)) . ":$microseconds";
}

#
# Print out a timestamp if the TIMESTAMPS configure variable was set.
#
# usage: void TBDebugTimeStamp(@)
#
sub TBDebugTimeStamp(@)
{
    my @strings = @_;
    if ($TIMESTAMPS) {
	print "TIMESTAMP: ", TBTimeStamp(), " ", join("",@strings), "\n";
    }
}

#
# Turn on timestamps locally. We could do this globally by using an
# env variable to pass it along, but lets see if we need that.
#
sub TBDebugTimeStampsOn()
{
    $TIMESTAMPS = 1;
    $ENV{'TIMESTAMPS'} = "1";
}

1;
