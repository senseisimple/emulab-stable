#!/usr/bin/perl -wT

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Common routines and constants for the client bootime setup stuff.
#
package libsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( libsetup_init inform_reboot cleanup_node check_status
	 create_nicknames doifconfig dohostnames
	 doaccounts dorpms dotarballs dostartupcmd install_deltas
	 bootsetup nodeupdate startcmdstatus whatsmynickname
	 TBBackGround TBForkCmd remotenodeupdate remotenodevnodesetup

	 OPENTMCC CLOSETMCC RUNTMCC MFS REMOTE

	 TMCC TMIFC TMDELAY TMRPM TMTARBALLS TMHOSTS
	 TMNICKNAME HOSTSFILE TMSTARTUPCMD FINDIF TMTUNNELCONFIG
	 TMTRAFFICCONFIG TMROUTECONFIG TMVNODEDIR

	 TMCCCMD_REBOOT TMCCCMD_STATUS TMCCCMD_IFC TMCCCMD_ACCT TMCCCMD_DELAY
	 TMCCCMD_HOSTS TMCCCMD_RPM TMCCCMD_TARBALL TMCCCMD_STARTUP
	 TMCCCMD_DELTA TMCCCMD_STARTSTAT TMCCCMD_READY TMCCCMD_TRAFFIC
	 TMCCCMD_BOSSINFO TMCCCMD_VNODELIST TMCCCMD_ISALIVE

       );

# Must come after package declaration!
use English;

#
# This is the home of the setup library on the client machine. The including
# program has to tell us this by calling the init routine below. For example,
# it is /etc/testbed on FreeBSD and /etc/rc.d/testbed on Linux.
#
my $SETUPDIR;

sub libsetup_init($)
{
    my($path) = @_;

    $SETUPDIR = $path;
}

#
# This "local" library provides the OS dependent part. Must load this after
# defining the above function cause the local library invokes it to set the
# $SETUPDIR
#
use liblocsetup;

#
# For virtual (multiplexed nodes). If defined, tack onto tmcc command.
# and use in pathnames. Not sure how this will be used later with jailed
# virtual nodes, since they will run in their own environment, but without
# jail we have to share the same namespace.
#
my $vnodeid	= "";
my $vnodedir;

#
# These are the paths of various files and scripts that are part of the
# setup library.
#
sub TMCC()		{ "$SETUPDIR/tmcc"; }
sub TMIFC()		{ "$SETUPDIR/rc.ifc"; }
sub TMRPM()		{ "$SETUPDIR/rc.rpm"; }
sub TMTARBALLS()	{ "$SETUPDIR/rc.tarballs"; }
sub TMSTARTUPCMD()	{ "$SETUPDIR/startupcmd"; }
sub TMHOSTS()		{ "$SETUPDIR/hosts"; }
sub TMNICKNAME()	{ "$SETUPDIR/nickname"; }
sub FINDIF()		{ "$SETUPDIR/findif"; }
sub HOSTSFILE()		{ "/etc/hosts"; }
sub TMROUTECONFIG()     { ($vnodedir ? $vnodedir : $SETUPDIR) . "/rc.route";}
sub TMTRAFFICCONFIG()	{ ($vnodedir ? $vnodedir : $SETUPDIR) . "/rc.traffic";}
sub TMTUNNELCONFIG()	{ ($vnodedir ? $vnodedir : $SETUPDIR) . "/rc.tunnel";}
sub TMVTUNDCONFIG()	{ ($vnodedir ? $vnodedir : $SETUPDIR) . "/vtund.conf";}
#
# These go in /var/db/emulab. Good for jails!
# 
sub TMMOUNTDB()		{ "/var/db/emulab/mountdb"; }
sub TMSFSMOUNTDB()	{ "/var/db/emulab/sfsmountdb"; }
sub TMPASSDB()		{ "/var/db/emulab/passdb"; }
sub TMGROUPDB()		{ "/var/db/emulab/groupdb"; }

# Make sure this exists!
if (! -e "/var/db/emulab") {
    mkdir("/var/db/emulab", 0775);
}

#
# Whether or not to use SFS (the self-certifying file system).  If this
# is 0, fall back to NFS.  Note that it doesn't hurt to set this to 1
# even if TMCD is not serving out SFS mounts, or if this node is not
# running SFS.  It'll deal and fall back to NFS.
#
my $USESFS		= 1;

#
# This is the VERSION. We send it through to tmcd so it knows what version
# responses this file is expecting.
#
# BE SURE TO BUMP THIS AS INCOMPATIBILE CHANGES TO TMCD ARE MADE!
#
sub TMCD_VERSION()	{ 6; };

#
# These are the TMCC commands. 
#
sub TMCCCMD_REBOOT()	{ "reboot"; }
sub TMCCCMD_STATUS()	{ "status"; }
sub TMCCCMD_IFC()	{ "ifconfig"; }
sub TMCCCMD_ACCT()	{ "accounts"; }
sub TMCCCMD_DELAY()	{ "delay"; }
sub TMCCCMD_HOSTS()	{ "hostnames"; }
sub TMCCCMD_RPM()	{ "rpms"; }
sub TMCCCMD_TARBALL()	{ "tarballs"; }
sub TMCCCMD_STARTUP()	{ "startupcmd"; }
sub TMCCCMD_DELTA()	{ "deltas"; }
sub TMCCCMD_STARTSTAT()	{ "startstatus"; }
sub TMCCCMD_READY()	{ "ready"; }
sub TMCCCMD_MOUNTS()	{ "mounts"; }
sub TMCCCMD_ROUTING()	{ "routing"; }
sub TMCCCMD_TRAFFIC()	{ "trafgens"; }
sub TMCCCMD_BOSSINFO()	{ "bossinfo"; }
sub TMCCCMD_TUNNEL()	{ "tunnels"; }
sub TMCCCMD_NSECONFIGS(){ "nseconfigs"; }
sub TMCCCMD_VNODELIST() { "vnodelist"; }
sub TMCCCMD_ISALIVE()   { "isalive"; }
sub TMCCCMD_SFSHOSTID()	{ "sfshostid"; }
sub TMCCCMD_SFSMOUNTS() { "sfsmounts"; }
sub TMCCCMD_ROUTELIST()	{ "routelist"; }

#
# Some things never change.
# 
my $TARINSTALL  = "/usr/local/bin/install-tarfile %s %s";
my $DELTAINSTALL= "/usr/local/bin/install-delta %s";
my $VTUND       = "/usr/local/sbin/vtund";

#
# This is a debugging thing for my home network.
# 
#my $NODE	= "-p 7778 REDIRECT=192.168.100.1";
#my $NODE	= "-p 7779";
$NODE		= "";

# Locals
my $pid		= "";
my $eid		= "";
my $vname	= "";

# Control tmcc error condition and timeout. Dynamic, not lexical!
$tmccdie        = 1; 
$tmcctimeout    = 0;

# When on the MFS, we do a much smaller set of stuff.
# Cause of the way the packages are loaded (which I do not understand),
# this is computed on the fly instead of once.
sub MFS()	{ if (-e "$SETUPDIR/ismfs") { return 1; } else { return 0; } }

#
# Same for a remote node.
#
sub REMOTE()	{ if (-e "$SETUPDIR/isrem") { return 1; } else { return 0; } }

#
# Open a TMCC connection and return the "stream pointer". Caller is
# responsible for closing the stream and checking return value.
#
# usage: OPENTMCC(char *command, char *args, char *options)
#
sub OPENTMCC($;$$)
{
    my($cmd, $args, $options) = @_;
    my $vn = "";
    local *TM;

    if (!defined($args)) {
	$args = "";
    }
    if (!defined($options)) {
	$options = "";
    }
    if ($vnodeid ne "") {
	$vn = "-n $vnodeid";
    }
    if ($tmcctimeout) {
	$options .= " -t $tmcctimeout";
    }

    my $foo = sprintf("%s -v %d $options $NODE $vn $cmd $args |",
		      TMCC, TMCD_VERSION);

    if (!open(TM, $foo)) {
	print STDERR "Cannot start TMCC: $!\n";
	die("\n") if $tmccdie;
	return undef;
    }
    return (*TM);
}

#
# Close connection. Die on error.
# 
sub CLOSETMCC($) {
    my($TM) = @_;
    
    if (! close($TM)) {
	if ($?) {
	    print STDERR "TMCC exited with status $?!\n";
	}
	else {
	    print STDERR "Error closing TMCC pipe: $!\n";
	}
	die("\n") if $tmccdie;
	return 0;
    }
    return 1;
}

#
# Run a TMCC command with the provided arguments.
#
# usage: RUNTMCC(char *command, char *args, char *options)
#
sub RUNTMCC($;$$)
{
    my($cmd, $args, $options) = @_;
    my($TM);

    if (!defined($args)) {
	$args = "";
    }
    if (!defined($options)) {
	$options = "";
    }
    
    $TM = OPENTMCC($cmd, $args, $options);

    close($TM)
	or die $? ? "TMCC exited with status $?" : "Error closing pipe: $!";
    
    return 0;
}

#
# Inform the master we have rebooted.
#
sub inform_reboot()
{
    RUNTMCC(TMCCCMD_REBOOT);
    return 0;
}

#
# Reset to a moderately clean state.
#
sub cleanup_node ($) {
    my ($scrub) = @_;
    
    print STDOUT "Cleaning node; removing configuration files ...\n";
    unlink TMIFC, TMRPM, TMSTARTUPCMD, TMNICKNAME, TMTARBALLS;
    unlink TMROUTECONFIG, TMTRAFFICCONFIG, TMTUNNELCONFIG;
    unlink TMMOUNTDB . ".db";

    #
    # If scrubbing, remove the password/group file DBs so that we revert
    # to base set.
    # 
    if ($scrub) {
	unlink TMPASSDB . ".db";
	unlink TMGROUPDB . ".db";
    }

    if (! REMOTE()) {
	printf STDOUT "Resetting %s file\n", HOSTSFILE;
	if (system($CP, "-f", TMHOSTS, HOSTSFILE) != 0) {
	    printf "Could not copy default %s into place: $!\n", HOSTSFILE;
	    exit(1);
	}
    }

    return os_cleanup_node($scrub);
}

#
# Check node allocation.
#
# Returns 0 if node is free. Returns list (pid/eid/vname) if allocated.
#
sub check_status ()
{
    my $TM;
    
    $TM = OPENTMCC(TMCCCMD_STATUS);
    $_  = <$TM>;
    CLOSETMCC($TM);

    if ($_ =~ /^FREE/) {
	return 0;
    }
    
    if ($_ =~ /ALLOCATED=([-\@\w.]*)\/([-\@\w.]*) NICKNAME=([-\@\w.]*)/) {
	$pid   = $1;
	$eid   = $2;
	$vname = $3;
    }
    else {
	warn "*** WARNING: Error getting reservation status\n";
	return 0;
    }
    return ($pid, $eid, $vname);
}

#
# Stick our nickname in a file in case someone wants it.
#
sub create_nicknames()
{
    open(NICK, ">" . TMNICKNAME)
	or die("Could not open nickname file: $!");
    print NICK "$vname.$eid.$pid\n";
    close(NICK);

    return 0;
}

#
# Process mount directives from TMCD. We keep track of all the mounts we
# have added in here so that we delete just the accounts we added, when
# project membership changes. Same goes for project directories on shared
# nodes. We use a simple perl DB for that.
#
sub domounts()
{
    my $TM;
    my %MDB;
    my %mounts;
    my %deletes;
    my %sfsmounts;
    my %sfsdeletes;

    $TM = OPENTMCC(TMCCCMD_MOUNTS, "USESFS=$USESFS");

    while (<$TM>) {
	if ($_ =~ /^REMOTE=([-:\@\w\.\/]+) LOCAL=([-\@\w\.\/]+)/) {
	    $mounts{$1} = $2;
	}
	elsif ($_ =~ /^SFS REMOTE=([-:\@\w\.\/]+) LOCAL=([-\@\w\.\/]+)/) {
	    $sfsmounts{$1} = $2;
	}
	else {
	    warn "*** WARNING: Malformed mount information: $_\n";
	}
    }
    CLOSETMCC($TM);
    
    #
    # The MFS version does not support (or need) this DB stuff. Just mount
    # them up.
    #
    if (MFS()) {
	while (($remote, $local) = each %mounts) {
	    if (! -e $local) {
		if (! os_mkdir($local, 0770)) {
		    warn "*** WARNING: Could not make directory $local: $!\n";
		    next;
		}
	    }
	
	    print STDOUT "  Mounting $remote on $local\n";
	    if (system("$MOUNT $remote $local")) {
		warn "*** WARNING: Could not $MOUNT $remote on $local: $!\n";
		next;
	    }
	}
	return 0;
    }

    dbmopen(%MDB, TMMOUNTDB, 0660);
    
    #
    # First mount all the mounts we are told to. For each one that is not
    # currently mounted, and can be mounted, add it to the DB.
    # 
    while (($remote, $local) = each %mounts) {
	if (system("$MOUNT | $EGREP ' $local '") == 0) {
	    $MDB{$remote} = $local;
	    next;
	}

	if (! -e $local) {
	    if (! os_mkdir($local, 0770)) {
		warn "*** WARNING: Could not make directory $local: $!\n";
		next;
	    }
	}
	
	print STDOUT "  Mounting $remote on $local\n";
	if (system("$MOUNT $remote $local")) {
	    warn "*** WARNING: Could not $MOUNT $remote on $local: $!\n";
	    next;
	}

	$MDB{$remote} = $local;
    }

    #
    # Now unmount the ones that we mounted previously, but are now no longer
    # in the mount set (as told to us by the TMCD). Note, we cannot delete 
    # them directly from MDB since that would mess up the foreach loop, so
    # just stick them in temp and postpass it.
    #
    while (($remote, $local) = each %MDB) {
	if (defined($mounts{$remote})) {
	    next;
	}

	if (system("$MOUNT | $EGREP ' $local '")) {
	    $deletes{$remote} = $local;
	    next;
	}

	print STDOUT "  Unmounting $local\n";
	if (system("$UMOUNT $local")) {
	    warn "*** WARNING: Could not unmount $local\n";
	    next;
	}
	
	#
	# Only delete from set if we can actually unmount it. This way
	# we can retry it later (or next time).
	# 
	$deletes{$remote} = $local;
    }
    while (($remote, $local) = each %deletes) {
	delete($MDB{$remote});
    }

    # Write the DB back out!
    dbmclose(%MDB);

    #
    # Now, do basically the same thing over again, but this time for
    # SFS mounted stuff
    #

    if (scalar(%sfsmounts)) {
	dbmopen(%MDB, TMSFSMOUNTDB, 0660);
	
	#
	# First symlink all the mounts we are told to. For each one
	# that is not currently symlinked, and can be, add it to the
	# DB.
	#
	while (($remote, $local) = each %sfsmounts) {
	    if (-l $local) {
		if (readlink($local) eq ("/sfs/" . $remote)) {
		    $MDB{$remote} = $local;
		    next;
		}
		if (readlink($local) ne ("/sfs/" . $remote)) {
		    print STDOUT "  Unlinking incorrect symlink $local\n";
		    if (! unlink($local)) {
			warn "*** WARNING: Could not unlink $local: $!\n";
			next;
		    }
		}
	    }
	    elsif (-d $local) {
		if (! rmdir($local)) {
		    warn "*** WARNING: Could not rmdir $local: $!\n";
		    next;
		}
	    }
	    
	    $dir = $local;
	    $dir =~ s/(.*)\/[^\/]*$/$1/;
	    if ($dir ne "" && ! -e $dir) {
		print STDOUT "  Making directory $dir\n";
		if (! os_mkdir($dir, 755)) {
		    warn "*** WARNING: Could not make directory $local: $!\n";
		    next;
		}
	    }
	    print STDOUT "  Symlinking $remote on $local\n";
	    if (! symlink("/sfs/" . $remote, $local)) {
		warn "*** WARNING: Could not make symlink $local: $!\n";
		next;
	    }
	    
	    $MDB{$remote} = $local;
	}

	#
	# Now delete the ones that we symlinked previously, but are
	# now no longer in the mount set (as told to us by the TMCD).
	# Note, we cannot delete them directly from MDB since that
	# would mess up the foreach loop, so just stick them in temp
	# and postpass it.
	#
	while (($remote, $local) = each %MDB) {
	    if (defined($sfsmounts{$remote})) {
		next;
	    }
	    
	    if (! -e $local) {
		$sfsdeletes{$remote} = $local;
		next;
	    }
	    
	    print STDOUT "  Deleting symlink $local\n";
	    if (! unlink($local)) {
		warn "*** WARNING: Could not delete $local: $!\n";
		next;
	    }
	    
	    #
	    # Only delete from set if we can actually unlink it.  This way
	    # we can retry it later (or next time).
	    #
	    $sfsdeletes{$remote} = $local;
	}
	while (($remote, $local) = each %sfsdeletes) {
	    delete($MDB{$remote});
	}

	# Write the DB back out!
	dbmclose(%MDB);	
    }
    else {
	# There were no SFS mounts reported, so disable SFS
	$USESFS = 0;
    }

    return 0;
}

#
# Do SFS hostid setup.
# Creates an SFS host key for this node, if it doesn't already exist,
# and sends it to TMCD
#
sub dosfshostid ()
{
    my $TM;
    my $myhostid;

    # Do I already have a host key?
    if (! -e "/etc/sfs/sfs_host_key") {
	warn "*** This node does not have a host key, skipping SFS stuff\n";
	$USESFS = 0;
	return 1;
    }

    # Give hostid to TMCD
    $myhostid = `sfskey hostid - 2>/dev/null`;
    if (! $?) {
	if ( $myhostid =~ /^([-\.\w_]*:[a-z0-9]*)$/ ) {
	    $myhostid = $1;
	    print STDOUT "  Hostid: $myhostid\n";
	    RUNTMCC(TMCCCMD_SFSHOSTID, "$myhostid");
	}
	else {
	    warn "*** WARNING: Invalid hostid\n";
	}
    }
    else {
	warn "*** WARNING: Could not retrieve this node's SFShostid!\n";
	$USESFS = 0;
    }

    return 0;
}

#
# Do interface configuration.    
# Write a file of ifconfig lines, which will get executed.
#
sub doifconfig ()
{
    my $TM;
    
    #
    # Kinda ugly, but there is too much perl goo included by Socket to put it
    # on the MFS. 
    # 
    if (MFS()) {
	return 1;
    }
    require Socket;
    import Socket;
    
    $TM = OPENTMCC(TMCCCMD_IFC);

    #
    # Open a connection to the TMCD, and then open a local file into which
    # we write ifconfig commands (as a shell script).
    # 
    open(IFC, ">" . TMIFC)
	or die("Could not open " . TMIFC . ": $!");
    print IFC "#!/bin/sh\n";
    
    while (<$TM>) {
	my $pat;

	#
	# Note that speed has a units spec: (K|M)bps
	# 
	$pat  = q(INTERFACE=(\d*) INET=([0-9.]*) MASK=([0-9.]*) MAC=(\w*) );
	$pat .= q(SPEED=(\w*) DUPLEX=(\w*));
	
	if ($_ =~ /$pat/) {
	    my $iface;

	    my $inet     = $2;
	    my $mask     = $3;
	    my $mac      = $4;
	    my $speed    = $5; 
	    my $duplex   = $6;
	    my $routearg = inet_ntoa(inet_aton($inet) & inet_aton($mask));

	    if ($iface = findiface($mac)) {
		my $ifline =
		    os_ifconfig_line($iface, $inet, $mask, $speed, $duplex);
		    
		print STDOUT "  $ifline\n";
		print IFC "$ifline\n";
		print IFC TMROUTECONFIG . " $routearg up\n";
	    }
	    else {
		warn "*** WARNING: Bad MAC: $mac\n";
	    }
	}
	else {
	    warn "*** WARNING: Bad ifconfig line: $_";
	}
    }
    CLOSETMCC($TM);
    close(IFC);
    chmod(0755, TMIFC);

    return 0;
}

#
# Convert from MAC to iface name (eth0/fxp0/etc) using little helper program.
# 
sub findiface($)
{
    my($mac) = @_;
    my($iface);

    open(FIF, FINDIF . " $mac |")
	or die "Cannot start " . FINDIF . ": $!";

    $iface = <FIF>;
    
    if (! close(FIF)) {
	return 0;
    }
    
    $iface =~ s/\n//g;
    return $iface;
}

#
# Do router configuration stuff. This just writes a file for someone else
# to deal with.
#
sub dorouterconfig ()
{
    my @stuff   = ();
    my $routing = 0;
    my %upmap   = ();
    my %downmap = ();
    my $TM;

    $TM = OPENTMCC(TMCCCMD_ROUTING);
    while (<$TM>) {
	push(@stuff, $_);
    }
    CLOSETMCC($TM);

    if (! @stuff) {
	return 0;
    }

    #
    # Look for router type. If none, we still write the file since other
    # scripts expect this to exist.
    # 
    foreach my $line (@stuff) {
	if (($line =~ /ROUTERTYPE=(.+)/) && ($1 ne "none")) {
	    $routing = 1;
	    last;
	}
    }
    
    open(RC, ">" . TMROUTECONFIG)
	or die("Could not open " . TMROUTECONFIG . ": $!");

    print RC "#!/bin/sh\n";
    print RC "# auto-generated by libsetup.pm, DO NOT EDIT\n";

    if (! $routing) {
	print RC "true\n";
	close(RC);
	chmod(0755, TMROUTECONFIG);
	return 0;
    }

    #
    # Now convert static route info into OS route commands
    # Also check for use of gated and remember it.
    #
    my $usegated = 0;
    my $pat;

    #
    # ROUTERTYPE=manual
    # ROUTE DEST=192.168.2.3 DESTTYPE=host DESTMASK=255.255.255.0 \
    #	NEXTHOP=192.168.1.3 COST=0
    #
    $pat = q(ROUTE DEST=([0-9\.]*) DESTTYPE=(\w*) DESTMASK=([0-9\.]*) );
    $pat .= q(NEXTHOP=([0-9\.]*) COST=([0-9]*));

    my $usemanual = 0;
    foreach my $line (@stuff) {
	if ($line =~ /ROUTERTYPE=(gated|ospf)/) {
	    $usegated = 1;
	} elsif ($line =~ /ROUTERTYPE=(manual|static)/) {
	    $usemanual = 1;
	} elsif ($usemanual && $line =~ /$pat/) {
	    my $dip   = $1;
	    my $rtype = $2;
	    my $dmask = $3;
	    my $gate  = $4;
	    my $cost  = $5;
	    my $routearg = inet_ntoa(inet_aton($gate) & inet_aton($dmask));

	    if (! defined($upmap{$routearg})) {
		$upmap{$routearg} = [];
		$downmap{$routearg} = [];
	    }
	    $rcline = os_routing_add_manual($rtype, $dip, $dmask, $gate,$cost);
	    push(@{$upmap{$routearg}}, $rcline);
	    $rcline = os_routing_del_manual($rtype, $dip, $dmask, $gate,$cost);
	    push(@{$downmap{$routearg}}, $rcline);
	} else {
	    warn "*** WARNING: Bad routing line: $line\n";
	}
    }

    print RC "case \"\$1\" in\n";
    foreach my $arg (keys(%upmap)) {
	print RC "  $arg)\n";
	print RC "    case \"\$2\" in\n";
	print RC "      up)\n";
	foreach my $rcline (@{$upmap{$arg}}) {
	    print RC "        $rcline\n";
	}
	print RC "      ;;\n";
	print RC "      down)\n";
	foreach my $rcline (@{$downmap{$arg}}) {
	    print RC "        $rcline\n";
	}
	print RC "      ;;\n";
	print RC "    esac\n";
	print RC "  ;;\n";
    }
    print RC "  enable)\n";

    #
    # Turn on IP forwarding
    #
    my $rcline = os_routing_enable_forward();
    print RC "    $rcline\n";

    #
    # Finally, enable gated if desired.
    #
    # Note that we allow both manually-specified static routes and gated
    # though more work may be needed on the gated config files to make
    # this work (i.e., to import existing kernel routes).
    #
    if ($usegated) {
	$rcline = os_routing_enable_gated();
	print RC "    $rcline\n";
    }
    print RC "  ;;\n";
    print RC "esac\n";
    print RC "exit 0\n";

    close(RC);
    chmod(0755, TMROUTECONFIG);

    return 0;
}

#
# Host names configuration (/etc/hosts). 
#
sub dohostnames ()
{
    my $TM;

    #
    # Start with fresh copy, since the hosts files is potentially updated
    # after the node boots via the update command.
    # 
    if (system($CP, "-f", TMHOSTS, HOSTSFILE) != 0) {
	printf STDERR "Could not copy default %s into place: $!\n", HOSTSFILE;
	return 1;
    }
    
    $TM = OPENTMCC(TMCCCMD_HOSTS);

    open(HOSTS, ">>" . HOSTSFILE)
	or die("Could not open $HOSTSFILE: $!");

    #
    # Now convert each hostname into hosts file representation and write
    # it to the hosts file. Note that ALIASES is for backwards compat.
    # Should go away at some point.
    #
    my $pat  = q(NAME=([-\w\.]+) IP=([0-9\.]*) ALIASES=\'([-\w\. ]*)\');
    
    while (<$TM>) {
	if ($_ =~ /$pat/) {
	    my $name    = $1;
	    my $ip      = $2;
	    my $aliases = $3;
	    
	    my $hostline = os_etchosts_line($name, $ip, $aliases);
	    
	    print STDOUT "  $hostline\n";
	    print HOSTS  "$hostline\n";
	}
	else {
	    warn "*** WARNING: Bad hosts line: $_";
	}
    }
    CLOSETMCC($TM);
    close(HOSTS);

    return 0;
}

sub doaccounts()
{
    my %newaccounts = ();
    my %newgroups   = ();
    my %pubkeys1    = ();
    my %pubkeys2    = ();
    my @sfskeys     = ();
    my %deletes     = ();
    my %lastmod     = ();
    my %PWDDB;
    my %GRPDB;

    my $TM = OPENTMCC(TMCCCMD_ACCT);

    #
    # The strategy is to keep a record of all the groups and accounts
    # added by the testbed system so that we know what to remove. We
    # use a vanilla perl dbm for that, one for the groups and one for
    # accounts. 
    #
    # First just get the current set of groups/accounts from tmcd.
    #
    while (<$TM>) {
	if ($_ =~ /^ADDGROUP NAME=([-\@\w.]+) GID=([0-9]+)/) {
	    #
	    # Group info goes in the hash table.
	    #
	    my $gname = "$1";
	    
	    if (REMOTE()) {
		$gname = "emu-${gname}";
	    }
	    $newgroups{"$gname"} = $2
	}
	elsif ($_ =~ /^ADDUSER LOGIN=([0-9a-z]+)/) {
	    #
	    # Account info goes in the hash table.
	    # 
	    $newaccounts{$1} = $_;
	    next;
	}
	elsif ($_ =~ /^PUBKEY LOGIN=([0-9a-z]+) KEY="(.*)"/) {
	    #
	    # Keys go into hash as a list of keys.
	    #
	    my $login = $1;
	    my $key   = $2;

	    #
	    # P1 or P2 key. Must be treated differently below.
	    #
	    if ($key =~ /^\d+\s+.*$/) {
		if (! defined($pubkeys1{$login})) {
		    $pubkeys1{$login} = [];
		}
		push(@{$pubkeys1{$login}}, $key);
	    }
	    else {
		if (! defined($pubkeys2{$login})) {
		    $pubkeys2{$login} = [];
		}
		push(@{$pubkeys2{$login}}, $key);
	    }
	    next;
	}
	elsif ($_ =~ /^SFSKEY KEY="(.*)"/) {
	    #
	    # SFS key goes into the array.
	    #
	    push(@sfskeys, $1);
	    next;
	}
	else {
	    warn "*** WARNING: Bad accounts line: $_\n";
	}
    }
    CLOSETMCC($TM);

    if (! MFS()) {
	#
	# One the MFS, these will just start out as empty hashes.
	# 
	dbmopen(%PWDDB, TMPASSDB, 0660) or
	    die("Cannot open " . TMPASSDB . ": $!\n");
	
	dbmopen(%GRPDB, TMGROUPDB, 0660) or
	    die("Cannot open " . TMGROUPDB . ": $!\n");
    }

    #
    # Create any groups that do not currently exist. Add each to the
    # DB as we create it.
    #
    while (($group, $gid) = each %newgroups) {
	my ($exists,undef,$curgid) = getgrnam($group);
	
	if ($exists) {
	    if ($gid != $curgid) {
		warn "*** WARNING: $group/$gid mismatch with existing group\n";
	    }
	    next;
	}

	print "Adding group: $group/$gid\n";
	    
	if (os_groupadd($group, $gid)) {
	    warn "*** WARNING: Error adding new group $group/$gid\n";
	    next;
	}
	# Add to DB only if successful. 
	$GRPDB{$group} = $gid;
    }

    #
    # Now remove the ones that we created previously, but are now no longer
    # in the group set (as told to us by the TMCD). Note, we cannot delete 
    # them directly from the hash since that would mess up the foreach loop,
    # so just stick them in temp and postpass it.
    #
    while (($group, $gid) = each %GRPDB) {
	if (defined($newgroups{$group})) {
	    next;
	}

	print "Removing group: $group/$gid\n";
	
	if (os_groupdel($group)) {
	    warn "*** WARNING: Error removing group $group/$gid\n";
	    next;
	}
	# Delete from DB only if successful. 
	$deletes{$group} = $gid;
    }
    while (($group, $gid) = each %deletes) {
	delete($GRPDB{$group});
    }
    %deletes = ();

    # Write the DB back out!
    if (! MFS()) {
	dbmclose(%GRPDB);
    }

    #
    # Repeat the same sequence for accounts, except we remove old accounts
    # first. 
    # 
    while (($login, $info) = each %PWDDB) {
	my $uid = $info;
	
	#
	# Split out the uid from the serial. Note that this was added later
	# so existing DBs might not have a serial yet. We save the serial
	# for later. 
	#
	if ($info =~ /(\d*):(\d*)/) {
	    $uid = $1;
	    $lastmod{$login} = $2;
	}
	
	if (defined($newaccounts{$login})) {
	    next;
	}

	my ($exists,undef,$curuid,undef,
	    undef,undef,undef,$homedir) = getpwnam($login);

	#
	# If the account is gone, someone removed it by hand. Remove it
	# from the DB so we do not keep trying.
	#
	if (! defined($exists)) {
	    warn "*** WARNING: Account for $login was already removed!\n";
	    $deletes{$login} = $login;
	    next;
	}

	#
	# Check for mismatch, just in case. If there is a mismatch remove it
	# from the DB so we do not keep trying.
	#
	if ($uid != $curuid) {
	    warn "*** WARNING: ".
		 "Account uid for $login has changed ($uid/$curuid)!\n";
	    $deletes{$login} = $login;
	    next;
	}
	
	print "Removing user: $login\n";
	
	if (os_userdel($login) != 0) {
	    warn "*** WARNING: Error removing user $login\n";
	    next;
	}

	#
	# Remove the home dir. 
	#
	# Must ask for the current home dir in case it came from pw.conf.
	#
	if (defined($homedir) &&
	    index($homedir, "/${login}")) {
	    if (os_homedirdel($login, $homedir) != 0) {
	        warn "*** WARNING: Could not remove homedir $homedir.\n";
	    }
	}
	
	# Delete from DB only if successful. 
	$deletes{$login} = $login;
    }
    
    while (($login, $foo) = each %deletes) {
	delete($PWDDB{$login});
    }

    my $pat = q(ADDUSER LOGIN=([0-9a-z]+) PSWD=([^:]+) UID=(\d+) GID=(.*) );
    $pat   .= q(ROOT=(\d) NAME="(.*)" HOMEDIR=(.*) GLIST="(.*)" );
    $pat   .= q(SERIAL=(\d+));

    while (($login, $info) = each %newaccounts) {
	if ($info =~ /$pat/) {
	    $pswd  = $2;
	    $uid   = $3;
	    $gid   = $4;
	    $root  = $5;
	    $name  = $6;
	    $hdir  = $7;
	    $glist = $8;
	    $serial= $9;
	    if ( $name =~ /^(([^:]+$|^))$/ ) {
		$name = $1;
	    }

	    #
	    # See if update needed, based on the serial number we get.
	    # If its different, the account info has changed.
	    # 
	    my $doupdate = 0;
	    if (!defined($lastmod{$login}) || $lastmod{$login} != $serial) {
		$doupdate = 1;
	    }
	    
	    my ($exists,undef,$curuid) = getpwnam($login);

	    if ($exists) {
		if (!defined($PWDDB{$login})) {
		    warn "*** WARNING: ".
			 "Skipping since $login existed before EmulabMan!\n";
		    next;
		}
		if ($curuid != $uid) {
		    warn "*** WARNING: ".
			 "$login/$uid uid mismatch with existing login.\n";
		    next;
		}
		if ($doupdate) {
		    print "Updating: ".
			"$login/$uid/$gid/$root/$name/$hdir/$glist\n";
		    
		    os_usermod($login, $gid, "$glist", $pswd, $root);

		    #
		    # Note that we changed the info for next time.
		    # 
		    $PWDDB{$login} = "$uid:$serial";
		}
	    }
	    else {
		print "Adding: $login/$uid/$gid/$root/$name/$hdir/$glist\n";

		if (os_useradd($login, $uid, $gid, $pswd, 
			       "$glist", $hdir, $name, $root)) {
		    warn "*** WARNING: Error adding new user $login\n";
		    next;
		}
		# Add to DB only if successful. 
		$PWDDB{$login} = "$uid:$serial";
	    }

	    #
	    # Skip ssh stuff if a local node or not updating (if the
	    # user did not exist, $doupdate will be true).
	    # 
	    if (!REMOTE() || !$doupdate) {
		next;
	    }

	    #
	    # Create .ssh dir and populate it with an authkeys file.
	    # Must ask for the current home dir since we rely on pw.conf.
	    #
	    my (undef,undef,undef,undef,
		undef,undef,undef,$homedir) = getpwuid($uid);
	    my $sshdir = "$homedir/.ssh";
	    
	    TBNewsshKeyfile($sshdir, $uid, $gid, 1, @{$pubkeys1{$login}});
	    TBNewsshKeyfile($sshdir, $uid, $gid, 2, @{$pubkeys2{$login}});
	}
	else {
	    warn("*** Bad accounts line: $info\n");
	}
    }
    # Write the DB back out!
    if (! MFS()) {
	dbmclose(%PWDDB);
    }

    #
    # Create sfs_users file and populate it with public SFS keys
    #
    if ($USESFS) {
	my $sfsusers = "/etc/sfs/sfs_users";
	
	if (!open(SFSKEYS, "> ${sfsusers}.new")) {
	    warn("*** WARNING: Could not open ${sfsusers}.new: $!\n");
	    goto bad;
	}
	    
	print SFSKEYS "#\n";
	print SFSKEYS "# DO NOT EDIT! This file auto generated by ".
	    "Emulab.Net account software.\n";
	print SFSKEYS "#\n";
	print SFSKEYS "# Please use the web interface to edit your ".
	    "SFS public key list.\n";
	print SFSKEYS "#\n";
	foreach my $key (@sfskeys) {
	    print SFSKEYS "$key\n";
	}
	close(SFSKEYS);
	    
	# Because sfs_users only contains public keys, sfs_users.pub is
	# exactly the same
	if (system("cp -p -f ${sfsusers}.new ${sfsusers}.pub.new")) {
	    warn("*** WARNING Could not copy ${sfsusers}.new to ".
		 "${sfsusers}.pub.new: $!\n");
	    goto bad;
	}
	    
	if (!chown(0, 0, "${sfsusers}.new")) {
	    warn("*** WARNING: Could not chown ${sfsusers}.new: $!\n");
	    goto bad;
	}
	if (!chmod(0600, "${sfsusers}.new")) {
	    warn("*** WARNING: Could not chmod ${sfsusers}.new: $!\n");
	    goto bad;
	}
	    
	if (!chown(0, 0, "${sfsusers}.pub.new")) {
	    warn("*** WARNING: Could not chown ${sfsusers}.pub.new: $!\n");
	    goto bad;
	}
	if (!chmod(0644, "${sfsusers}.pub.new")) {
	    warn("*** WARNING: Could not chmod ${sfsusers}.pub.new: $!\n");
	    goto bad;
	}
	    
	# Save off old key files and move in new ones
	foreach my $keyfile ("${sfsusers}", "${sfsusers}.pub") {
	    if (-e $keyfile) {
		if (system("cp -p -f $keyfile $keyfile.old")) {
		    warn("*** Could not save off $keyfile: $!\n");
		    next;
		}
		if (!chown(0, 0, "$keyfile.old")) {
		    warn("*** Could not chown $keyfile.old: $!\n");
		}
		if (!chmod(0600, "$keyfile.old")) {
		    warn("*** Could not chmod $keyfile.old: $!\n");
		}
	    }
	    if (system("mv -f $keyfile.new $keyfile")) {
		warn("*** Could not mv $keyfile.new $keyfile.new: ~!\n");
	    }
	}
      bad:
    }
    
    return 0;
}

#
# RPM configuration. 
#
sub dorpms ()
{
    my @rpms = ();
    
    my $TM = OPENTMCC(TMCCCMD_RPM);
    while (<$TM>) {
	push(@rpms, $_);
    }
    CLOSETMCC($TM);

    if (! @rpms) {
	return 0;
    }
    
    open(RPM, ">" . TMRPM)
	or die("Could not open " . TMRPM . ": $!");
    print RPM "#!/bin/sh\n";
    
    foreach my $rpm (@rpms) {
	if ($rpm =~ /RPM=(.+)/) {
	    my $rpmline = os_rpminstall_line($1);
		    
	    print STDOUT "  $rpmline\n";
	    print RPM    "echo \"Installing RPM $1\"\n";
	    print RPM    "$rpmline\n";
	}
	else {
	    warn "*** WARNING: Bad RPMs line: $rpm";
	}
    }
    close(RPM);
    chmod(0755, TMRPM);

    return 0;
}

#
# TARBALL configuration. 
#
sub dotarballs ()
{
    my @tarballs = ();

    my $TM = OPENTMCC(TMCCCMD_TARBALL);
    while (<$TM>) {
	push(@tarballs, $_);
    }
    CLOSETMCC($TM);

    if (! @tarballs) {
	return 0;
    }
    
    open(TARBALL, ">" . TMTARBALLS)
	or die("Could not open " . TMTARBALLS . ": $!");
    print TARBALL "#!/bin/sh\n";
    
    foreach my $tarball (@tarballs) {
	if ($tarball =~ /DIR=(.+)\s+TARBALL=(.+)/) {
	    my $tbline = sprintf($TARINSTALL, $1, $2);
		    
	    print STDOUT  "  $tbline\n";
	    print TARBALL "echo \"Installing Tarball $2 in dir $1 \"\n";
	    print TARBALL "$tbline\n";
	}
	else {
	    warn "*** WARNING: Bad Tarballs line: $tarball";
	}
    }
    close(TARBALL);
    chmod(0755, TMTARBALLS);

    return 0;
}

#
# Experiment startup Command.
#
sub dostartupcmd ()
{
    my $startupcmd;
    
    my $TM = OPENTMCC(TMCCCMD_STARTUP);
    $_ = <$TM>;
    if (defined($_)) {
	$startupcmd = $_;
    }
    CLOSETMCC($TM);

    if (! $startupcmd) {
	return 0;
    }
    
    open(RUN, ">" . TMSTARTUPCMD)
	or die("Could not open $TMSTARTUPCMD: $!");
    
    if ($startupcmd =~ /CMD=(\'.+\') UID=([0-9a-z]+)/) {
	print  STDOUT "  Will run $1 as $2\n";
	print  RUN    "$startupcmd";
    }
    else {
	warn "*** WARNING: Bad startupcmd line: $startupcmd";
    }

    close(RUN);
    chmod(0755, TMSTARTUPCMD);

    return 0;
}

sub dotrafficconfig()
{
    my $didopen = 0;
    my $pat;
    my $TM;
    my $boss;
    my $startnse = 0;
    my $nseconfig = "";
    
    #
    # Kinda ugly, but there is too much perl goo included by Socket to put it
    # on the MFS. 
    # 
    if (MFS()) {
	return 1;
    }
    require Socket;
    import Socket;
    
    $TM = OPENTMCC(TMCCCMD_BOSSINFO);
    my $bossinfo = <$TM>;
    ($boss) = split(" ", $bossinfo);

    #
    # XXX hack: workaround for tmcc cmd failure inside TCL
    #     storing the output of a few tmcc commands in
    #     $SETUPDIR files for use by NSE
    #
    if (! REMOTE()) {
	open(BOSSINFCFG, ">$SETUPDIR/tmcc.bossinfo") or
	    die "Cannot open file $SETUPDIR/tmcc.bossinfo: $!";
	print BOSSINFCFG "$bossinfo";
	close(BOSSINFCFG);
    }

    CLOSETMCC($TM);
    my ($pid, $eid, $vname) = check_status();

    my $cmdline = "$SETUPDIR/trafgen -s $boss";
    if ($pid) {
	$cmdline .= " -E $pid/$eid";
    }

    #
    # XXX hack: workaround for tmcc cmd failure inside TCL
    #     storing the output of a few tmcc commands in
    #     $SETUPDIR files for use by NSE
    #
    # Also nse stuff is mixed up with traffic config right
    # now because of having FullTcp based traffic generation.
    # Needs to move to a different place
    if (! REMOTE()) {
	my $record_sep;

	$record_sep = $/;
	undef($/);
	$TM = OPENTMCC(TMCCCMD_IFC);
	open(IFCFG, ">$SETUPDIR/tmcc.ifconfig") or
	    die "Cannot open file $SETUPDIR/tmcc.ifconfig: $!";
	print IFCFG <$TM>;
	close(IFCFG);
	CLOSETMCC($TM);

	$TM = OPENTMCC(TMCCCMD_ROUTELIST);
	open(ROUTELIST, ">$SETUPDIR/tmcc.routelist") or
	    die "Cannot open file $SETUPDIR/tmcc.routelist: $!";
        print ROUTELIST <$TM>;
	close(ROUTELIST);
	CLOSETMCC($TM);
	$/ = $record_sep;
	
	open(TRAFCFG, ">$SETUPDIR/tmcc.trafgens") or
	    die "Cannot open file $SETUPDIR/tmcc.trafgens: $!";    
    }

    $TM = OPENTMCC(TMCCCMD_TRAFFIC);

    $pat  = q(TRAFGEN=([-\w.]+) MYNAME=([-\w.]+) MYPORT=(\d+) );
    $pat .= q(PEERNAME=([-\w.]+) PEERPORT=(\d+) );
    $pat .= q(PROTO=(\w+) ROLE=(\w+) GENERATOR=(\w+));

    while (<$TM>) {

	if (! REMOTE()) {
	    print TRAFCFG "$_";
	}
	if ($_ =~ /$pat/) {
	    #
	    # The following is specific to the modified TG traffic generator:
	    #
	    #  trafgen [-s serverip] [-p serverport] [-l logfile] \
	    #	     [ -N name ] [-P proto] [-R role] [ -E pid/eid ] \
	    #	     [ -S srcip.srcport ] [ -T targetip.targetport ]
	    #
	    # N.B. serverport is not needed right now
	    #
	    my $name = $1;
	    my $ownaddr = inet_ntoa(my $ipaddr = gethostbyname($2));
	    my $ownport = $3;
	    my $peeraddr = inet_ntoa($ipaddr = gethostbyname($4));
	    my $peerport = $5;
	    my $proto = $6;
	    my $role = $7;
	    my $generator = $8;
	    my $target;
	    my $source;

	    # Skip if not specified as a TG generator. At some point
	    # work in Shashi's NSE work.
	    if ($generator ne "TG") {
		$startnse = 1;
		if (! $didopen) {
		    open(RC, ">" . TMTRAFFICCONFIG)
			or die("Could not open " . TMTRAFFICCONFIG . ": $!");
		    print RC "#!/bin/sh\n";
		    $didopen = 1;
		}
		next;
	    }

	    if ($role eq "sink") {
		$target = "$ownaddr.$ownport";
		$source = "$peeraddr.$peerport";
	    }
	    else {
		$target = "$peeraddr.$peerport";
		$source = "$ownaddr.$ownport";
	    }

	    if (! $didopen) {
		open(RC, ">" . TMTRAFFICCONFIG)
		    or die("Could not open " . TMTRAFFICCONFIG . ": $!");
		print RC "#!/bin/sh\n";
		$didopen = 1;
	    }
	    print RC "$cmdline -N $name -S $source -T $target -P $proto ".
		"-R $role >/tmp/${name}-${pid}-${eid}.debug 2>&1 &\n";
	}
	else {
	    warn "*** WARNING: Bad traffic line: $_";
	}
    }
    if (! REMOTE()) {
	close(TRAFCFG);
    }

    if( $startnse ) {
	print RC "$SETUPDIR/startnse &\n";
    }
    CLOSETMCC($TM);

    #
    # XXX hack: workaround for tmcc cmd failure inside TCL
    #     storing the output of a few tmcc commands in
    #     $SETUPDIR files for use by NSE
    #
    if (! REMOTE()) {
	open(NSECFG, ">$SETUPDIR/tmcc.nseconfigs") or
	    die "Cannot open file $SETUPDIR/tmcc.nseconfigs: $!";
	$TM = OPENTMCC(TMCCCMD_NSECONFIGS);
	$record_sep = $/;
	undef($/);
	$nseconfig = <$TM>;
	$/ = $record_sep;
	print NSECFG $nseconfig;
	CLOSETMCC($TM);
	close(NSECFG);
    }
	    
    # XXX hack: need a separate section for starting up NSE when we
    #           support simulated nodes
    if( ! $startnse ) {
	
	if( $nseconfig ) {

	    # start NSE if 'tmcc nseconfigs' is not empty
	    if ( ! $didopen ) {
		open(RC, ">" . TMTRAFFICCONFIG)
		    or die("Could not open " . TMTRAFFICCONFIG . ": $!");
		print RC "#!/bin/sh\n";
		$didopen = 1;	
	    }
	    print RC "$SETUPDIR/startnse &\n";
	}
    }
    
    if ($didopen) {
	printf RC "%s %s\n", TMCC(), TMCCCMD_READY();
	close(RC);
	chmod(0755, TMTRAFFICCONFIG);
    }
    return 0;
}

sub dotunnels()
{
    my @tunnels;
    my $pat;
    my $TM;
    my $didserver = 0;

    #
    # Kinda ugly, but there is too much perl goo included by Socket to put it
    # on the MFS. 
    # 
    if (MFS()) {
	return 1;
    }
    require Socket;
    import Socket;
    
    $TM = OPENTMCC(TMCCCMD_TUNNEL);
    while (<$TM>) {
	push(@tunnels, $_);
    }
    CLOSETMCC($TM);

    if (! @tunnels) {
	return 0;
    }
    my ($pid, $eid, $vname) = check_status();

    open(RC, ">" . TMTUNNELCONFIG)
	or die("Could not open " . TMTUNNELCONFIG . ": $!");
    print RC "#!/bin/sh\n";
    print RC "kldload if_tap\n";

    open(CONF, ">" . TMVTUNDCONFIG)
	or die("Could not open " . TMVTUNDCONFIG . ": $!");

    print(CONF
	  "options {\n".
	  "  ifconfig    /sbin/ifconfig;\n".
	  "  route       /sbin/route;\n".
	  "}\n".
	  "\n".
	  "default {\n".
	  "  persist     yes;\n".
	  "  stat        yes;\n".
	  "  keepalive   yes;\n".
	  "  type        ether;\n".
	  "}\n".
	  "\n");
    
    $pat  = q(TUNNEL=([-\w.]+) ISSERVER=(\d) PEERIP=([-\w.]+) );
    $pat .= q(PEERPORT=(\d+) PASSWORD=([-\w.]+) );
    $pat .= q(ENCRYPT=(\d) COMPRESS=(\d) INET=([-\w.]+) );
    $pat .= q(MASK=([-\w.]+) PROTO=([-\w.]+));

    foreach my $tunnel (@tunnels) {
	if ($tunnel =~ /$pat/) {
	    #
	    # The following is specific to vtund!
	    #
	    my $name     = $1;
	    my $isserver = $2;
	    my $peeraddr = $3;
	    my $peerport = $4;
	    my $password = $5;
	    my $encrypt  = ($6 ? "yes" : "no");
	    my $compress = ($7 ? "yes" : "no");
	    my $inetip   = $8;
	    my $mask     = $9;
	    my $proto    = $10;
	    my $routearg = inet_ntoa(inet_aton($inetip) & inet_aton($mask));

	    my $cmd = "$VTUND -n -P $peerport -f ". TMVTUNDCONFIG;

	    if ($isserver) {
		if (!$didserver) {
		    print RC
			"$cmd -s >/tmp/vtund-${pid}-${eid}.debug 2>&1 &\n";
		    $didserver = 1;
		}
	    }
	    else {
		print RC "$cmd $name $peeraddr ".
		    " >/tmp/vtun-${pid}-${eid}-${name}.debug 2>&1 &\n";
	    }
	    #
	    # Sheesh, vtund fails if it sees "//" in a path. 
	    #
	    my $config = TMROUTECONFIG;
	    $config =~ s/\/\//\//g;
	    
	    print(CONF
		  "$name {\n".
		  "  password      $password;\n".
		  "  compress      $compress;\n".
		  "  encrypt       $encrypt;\n".
		  "  proto         $proto;\n".
		  "\n".
		  "  up {\n".
		  "    # Connection is Up\n".
		  "    ifconfig \"%% $inetip netmask $mask\";\n".
		  "    program " . $config . " \"$routearg up\" wait;\n".
		  "  };\n".
		  "  down {\n".
		  "    # Connection is Down\n".
		  "    ifconfig \"%% down\";\n".
		  "    program " . $config . " \"$routearg down\" wait;\n".
		  "  };\n".
		  "}\n\n");
	}
	else {
	    warn "*** WARNING: Bad tunnel line: $tunnel";
	}
    }

    close(CONF);
    close(RC);
    chmod(0755, TMTUNNELCONFIG);
    return 0;
}

#
# Boot Startup code. This is invoked from the setup OS dependent script,
# and this fires up all the stuff above.
#
sub bootsetup()
{
    #
    # Inform the master that we have rebooted.
    #
    inform_reboot();

    #
    # Check allocation. Exit now if not allocated.
    #
    print STDOUT "Checking Testbed reservation status ... \n";
    if (! check_status()) {
	print STDOUT "  Free!\n";
	cleanup_node(1);
	return 0;
    }
    print STDOUT "  Allocated! $pid/$eid/$vname\n";

    #
    # Cleanup node. Flag indicates to gently clean ...
    # 
    cleanup_node(0);

    #
    # Setup a nicknames file. 
    #
    create_nicknames();

    if ($USESFS) {
	if (! MFS()) {
	    #
	    # Setup SFS hostid.
	    #
	    print STDOUT "Setting up for SFS ... \n";
	    dosfshostid();
	}
	else {
	    # No SFS on the MFS.
	    $USESFS = 0;
	}
    }

    #
    # Mount the project and user directories and symlink SFS "mounted"
    # directories
    #
    print STDOUT "Mounting project and home directories ... \n";
    domounts();

    #
    # Do account stuff.
    # 
    print STDOUT "Checking Testbed group/user configuration ... \n";
    doaccounts();

    if (! MFS()) {
	#
	# Okay, lets find out about interfaces.
	#
	print STDOUT "Checking Testbed interface configuration ... \n";
	doifconfig();

        #
        # Do tunnels
        # 
        print STDOUT "Checking Testbed tunnel configuration ... \n";
        dotunnels();

	#
	# Host names configuration (/etc/hosts). 
	#
	print STDOUT "Checking Testbed hostnames configuration ... \n";
	dohostnames();

	#
	# Router Configuration.
	#
	print STDOUT "Checking Testbed routing configuration ... \n";
	dorouterconfig();

	#
	# Traffic generator Configuration.
	#
	print STDOUT "Checking Testbed traffic generation configuration ...\n";
	dotrafficconfig();

	#
	# RPMS
	# 
	print STDOUT "Checking Testbed RPM configuration ... \n";
	dorpms();

	#
	# Tar Balls
	# 
	print STDOUT "Checking Testbed Tarball configuration ... \n";
	dotarballs();
    }

    #
    # Experiment startup Command.
    #
    print STDOUT "Checking Testbed Experiment Startup Command ... \n";
    dostartupcmd();

    #
    # OS specific stuff
    #
    os_setup();

    return 0;
}

#
# These are additional support routines for other setup scripts.
#
#
# Node update. This gets fired off after reboot to update accounts,
# mounts, etc. Its the start of shared node support. Quite rough at
# the moment. 
#
sub nodeupdate()
{
    #
    # Check allocation. If the node is now free, then do a cleanup
    # to reset the password files. The node should have its disk
    # reloaded to be safe, at the very least a reboot, but thats for
    # the future. We also need to kill processes belonging to people
    # whose accounts have been killed. Need to check the atq also for
    # queued commands.
    #
    if (! check_status()) {
	print "Node is free. Cleaning up password and group files.\n";
	cleanup_node(1);
	return 0;
    }

    if ($USESFS) {
	if (! MFS()) {
	    #
	    # Setup SFS hostid.
	    #
	    print STDOUT "Setting up for SFS ... \n";
	    dosfshostid();
	}
	else {
	    # No SFS on the MFS.
	    $USESFS = 0;
	}
    }

    #
    # Mount the project and user directories and symlink SFS "mounted"
    # directories
    #
    print STDOUT "Mounting project and home directories ... \n";
    domounts();

    #
    # Host names configuration (/etc/hosts). 
    #
    print STDOUT "Checking Testbed hostnames configuration ... \n";
    dohostnames();

    #
    # Do account stuff.
    # 
    print STDOUT "Checking Testbed group/user configuration ... \n";
    doaccounts();

    return 0;
}

#
# Remote node update. This gets fired off after reboot to update
# accounts, mounts, etc. Its the start of shared node support. Quite
# rough at the moment.
#
sub remotenodeupdate()
{
    #
    # Do account stuff.
    #
    {
	local $tmcctimeout = 10;
    
	if ($USESFS) {
	    #
	    # Setup SFS hostid.
	    #
	    print STDOUT "Setting up for SFS ... \n";
	    dosfshostid();
	}

	print STDOUT "Checking Testbed group/user configuration ... \n";
	doaccounts();

	print STDOUT "Mounting project and home directories ... \n";
	domounts();
    }

    return 0;
}

#
# This happens inside a jail. We have to pass the vnode, which is actually
# the hostname for the jailed environment. 
#
sub jailedsetup()
{
    my $hostname = `hostname`;
    
    # Untaint and strip newline.
    if ($hostname =~ /^([-\w\.]+)$/) {
	$hostname = $1;
    }
    else {
	die("Tainted hostname $hostname!\n");
    }
    
    #
    # Set global vnodeid for tmcc commands.
    #
    $vnodeid  = $hostname;

    #
    # Do account stuff.
    #
    {
	local $tmcctimeout = 10;
    
	if ($USESFS) {
	    #
	    # Setup SFS hostid.
	    #
	    print STDOUT "Setting up for SFS ... \n";
	    dosfshostid();
	}

	print STDOUT "Checking Testbed group/user configuration ... \n";
	doaccounts();
    }

    return 0;
}

#
# Remote Node virtual node setup.
#
sub remotenodevnodesetup($$)
{
    my ($vid, $vdir) = @_;

    #
    # Set global vnodeid for tmcc commands.
    #
    $vnodeid  = $vid;

    #
    # This is the directory where the rc files go.
    #
    if (! -e $vdir) {
	die("*** $0:\n".
	    "    No such directory: $vdir\n");
    }
    $vnodedir = $vdir;

    # Do not bother if somehow got released.
    if (! check_status()) {
	print "Node is free!\n";
	return undef;
    }

    #
    # Do tunnels
    # 
    print STDOUT "Checking Testbed tunnel configuration ... \n";
    dotunnels();

    #
    # Router Configuration.
    #
    print STDOUT "Checking Testbed routing configuration ... \n";
    dorouterconfig();

    #
    # Traffic generator Configuration.
    #
    print STDOUT "Checking Testbed traffic generation configuration ...\n";
    dotrafficconfig();

    return 0;
}

#
# Report startupcmd status back to the TMCC. Called by the runstartup
# script. 
#
sub startcmdstatus($)
{
    my($status) = @_;

    RUNTMCC(TMCCCMD_STARTSTAT, "$status");
    return 0;
}

#
# Install deltas. Return 0 if nothing happened. Return -1 if there was
# an error. Return 1 if deltas installed, which tells the caller to reboot.
#
# This is going to get invoked very early in the boot process, possibly
# before the normal client initialization. So we have to do a few things
# to make things are consistent. 
#
sub install_deltas ()
{
    my @deltas = ();
    my $reboot = 0;

    #
    # Inform the master that we have rebooted.
    #
    inform_reboot();

    #
    # Check allocation. Exit now if not allocated.
    #
    if (! check_status()) {
	return 0;
    }

    #
    # Now do the actual delta install.
    # 
    my $TM = OPENTMCC(TMCCCMD_DELTA);
    while (<$TM>) {
	push(@deltas, $_);
    }
    CLOSETMCC($TM);

    #
    # No deltas. Just exit and let the boot continue.
    #
    if (! @deltas) {
	return 0;
    }

    #
    # Mount the project directory.
    #
    domounts();

    #
    # Install all the deltas, and hope they all install okay. We reboot
    # if any one does an actual install (they may already be installed).
    # If any fails, then give up.
    # 
    foreach $delta (@deltas) {
	if ($delta =~ /DELTA=(.+)/) {
	    print STDOUT  "Installing DELTA $1 ...\n";

	    system(sprintf($DELTAINSTALL, $1));
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
    # Inform the master that we have rebooted. THIS MUST BE DONE!
    #
    inform_reboot();

    #
    # Check allocation. Exit now if not allocated.
    #
    if (! check_status()) {
	return 0;
    }

    return "$vname.$eid.$pid";
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
# Generate ssh authorized_keys files. Either protocol 1 or 2.
# Returns 0 on success, -1 on failure.
#
sub TBNewsshKeyfile($$$$$)
{
    my ($sshdir, $uid, $gid, $protocol, @pkeys) = @_;
    my $keyfile = "$sshdir/authorized_keys";
	
    if (! -e $sshdir) {
	if (! mkdir($sshdir, 0700)) {
	    warn("*** WARNING: Could not mkdir $sshdir: $!\n");
	    return -1;
	}
	if (!chown($uid, $gid, $sshdir)) {
	    warn("*** WARNING: Could not chown $sshdir: $!\n");
	    return -1;
	}
    }
    if ($protocol == 2) {
	$keyfile .= "2";
    }

    if (!open(AUTHKEYS, "> ${keyfile}.new")) {
	warn("*** WARNING: Could not open ${keyfile}.new: $!\n");
	return -1;
    }
    print AUTHKEYS "#\n";
    print AUTHKEYS "# DO NOT EDIT! This file auto generated by ".
	"Emulab.Net account software.\n";
    print AUTHKEYS "#\n";
    print AUTHKEYS "# Please use the web interface to edit your ".
	"public key list.\n";
    print AUTHKEYS "#\n";
    
    foreach my $key (@pkeys) {
	print AUTHKEYS "$key\n";
    }
    close(AUTHKEYS);

    if (!chown($uid, $gid, "${keyfile}.new")) {
	warn("*** WARNING: Could not chown ${keyfile}.new: $!\n");
	return -1;
    }
    if (!chmod(0600, "${keyfile}.new")) {
	warn("*** WARNING: Could not chmod ${keyfile}.new: $!\n");
	return -1;
    }
    if (-e "${keyfile}") {
	if (system("cp -p -f ${keyfile} ${keyfile}.old")) {
	    warn("*** Could not save off ${keyfile}: $!\n");
	    return -1;
	}
	if (!chown($uid, $gid, "${keyfile}.old")) {
	    warn("*** Could not chown ${keyfile}.old: $!\n");
	}
	if (!chmod(0600, "${keyfile}.old")) {
	    warn("*** Could not chmod ${keyfile}.old: $!\n");
	}
    }
    if (system("mv -f ${keyfile}.new ${keyfile}")) {
	warn("*** Could not mv ${keyfile} to ${keyfile}.new: $!\n");
    }
    return 0;
}

1;
