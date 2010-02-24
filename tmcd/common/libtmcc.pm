#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#

#
# TMCC library. Provides an interface to the TMCD via a config file
# or directly to it via TCP
#
# TODO: Proxy path in a jail. 
# 
package libtmcc;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw(configtmcc tmcc tmccbossname tmccgetconfig tmccclrconfig
	     tmcccopycache tmccbossinfo
	     TMCCCMD_REBOOT TMCCCMD_STATUS TMCCCMD_STATE TMCCCMD_IFC
	     TMCCCMD_ACCT TMCCCMD_DELAY TMCCCMD_HOSTS TMCCCMD_RPM
	     TMCCCMD_TARBALL TMCCCMD_STARTUP TMCCCMD_STARTSTAT
	     TMCCCMD_READY TMCCCMD_MOUNTS TMCCCMD_ROUTING TMCCCMD_TRAFFIC
	     TMCCCMD_BOSSINFO TMCCCMD_TUNNEL TMCCCMD_NSECONFIGS
	     TMCCCMD_VNODELIST TMCCCMD_SUBNODELIST TMCCCMD_ISALIVE
	     TMCCCMD_SFSHOSTID TMCCCMD_JAILCONFIG
	     TMCCCMD_PLABCONFIG TMCCCMD_SUBCONFIG TMCCCMD_LINKDELAYS
	     TMCCCMD_PROGRAMS TMCCCMD_SYNCSERVER TMCCCMD_KEYHASH TMCCCMD_NODEID
	     TMCCCMD_NTPINFO TMCCCMD_NTPDRIFT TMCCCMD_EVENTKEY TMCCCMD_ROUTELIST
	     TMCCCMD_ROLE TMCCCMD_RUSAGE TMCCCMD_WATCHDOGINFO TMCCCMD_HOSTKEYS
	     TMCCCMD_FIREWALLINFO TMCCCMD_EMULABCONFIG
	     TMCCCMD_CREATOR TMCCCMD_HOSTINFO TMCCCMD_LOCALIZATION
	     TMCCCMD_BOOTERRNO TMCCCMD_BOOTLOG TMCCCMD_BATTERY TMCCCMD_USERENV
	     TMCCCMD_TIPTUNNELS TMCCCMD_TRACEINFO TMCCCMD_ELVINDPORT
             TMCCCMD_PLABEVENTKEYS TMCCCMD_PORTREGISTER
	     TMCCCMD_MOTELOG TMCCCMD_BOOTWHAT TMCCCMD_ROOTPSWD
	     TMCCCMD_LTMAP TMCCCMD_LTPMAP TMCCCMD_TOPOMAP TMCCCMD_LOADINFO
	     TMCCCMD_TPMBLOB TMCCCMD_TPMPUB 
	     );

# Must come after package declaration!
use English;

#
# Turn off line buffering on output
#
$| = 1;

# Load up the paths. Done like this in case init code is needed.
BEGIN
{
    if (! -e "/etc/emulab/paths.pm") {
	die("Yikes! Could not require /etc/emulab/paths.pm!\n");
    }
    require "/etc/emulab/paths.pm";
    import emulabpaths;
}

# The actual TMCC binary.
my $TMCCBIN	= "$BINDIR/tmcc.bin";
my $PROXYDEF    = "$BOOTDIR/proxypath";
my $CACHEDIR	= "$BOOTDIR";
my $CACHENAME   = "tmcc";
my $debug       = 0;
my $beproxy     = 0;

#
# Configuration. The importer of this library should set these values
# accordingly. 
#
%config =
    ( "debug"		=> 0,
      "useudp"		=> 0,
      "beproxy"		=> 0,	# A unix domain path when true.
      "dounix"		=> 0,	# A unix domain path when true.
      "beinetproxy"	=> 0,   # A string of the form "ipaddr:port"
      "server"		=> undef,
      "portnum"		=> undef,
      "version"		=> undef,
      "subnode"		=> undef,
      "keyfile"		=> undef,
      "datafile"	=> undef,
      "timeout"		=> undef,
      "logfile"		=> undef,
      "nocache"         => 0,
      "clrcache"        => 0,
      "noproxy"         => 0,
      "nossl"           => 0,
      "cachedir"        => undef,
      "urn"             => undef,
      "usetpm"          => 0,
    );

# The cache directory is named by the vnodeid. This avoids some confusion.
sub CacheDir()
{
    return "$CACHEDIR/$CACHENAME"
	if (!defined($config{"subnode"}));

    return "$CACHEDIR/$CACHENAME" . "." . $config{"subnode"};
}

# Copy a cachedir. This is for mkjail, which needs to copy the cache
# dir into the jail to avoid another download of the full config.
sub tmcccopycache($$)
{
    my ($vnodeid, $root) = @_;
    my $fromdir = "$CACHEDIR/$CACHENAME" . "." . $vnodeid;
    my $todir   = "$root/$fromdir";

    if (! -d $fromdir) {
	warn("*** WARNING: No such directory $fromdir!\n");
	return -1;
    }
    if (! -d $root) {
	warn("*** WARNING: No such directory $root!\n");
	return -1;
    }
    if (-d $todir) {
	system("rm -rf $todir") == 0 ||
	    warn("*** WARNING: Could not remove old cache $todir!\n");
    }
    return system("cp -rp $fromdir $todir");
}

#
# List of TMCD commands. Some of these have to be passed through to
# tmcd, others can come from a local config file if it exists.
#
my %commandset =
    ( "reboot"		=> {TAG => "reboot"},
      "status"		=> {TAG => "status"},
      "state"		=> {TAG => "state"},
      "ifconfig"	=> {TAG => "ifconfig"},
      "accounts"	=> {TAG => "accounts"},
      "delay"		=> {TAG => "delay"},
      "hostnames"	=> {TAG => "hostnames"},
      "rpms"		=> {TAG => "rpms"},
      "tarballs"	=> {TAG => "tarballs"},
      "startupcmd"	=> {TAG => "startupcmd"},
      "startstatus"	=> {TAG => "startstatus"},
      "ready"		=> {TAG => "ready"},
      "mounts"		=> {TAG => "mounts"},
      "routing"		=> {TAG => "routing"},
      "trafgens"	=> {TAG => "trafgens"},
      "bossinfo"	=> {TAG => "bossinfo"},
      "tunnels"		=> {TAG => "tunnels"},
      "nseconfigs"	=> {TAG => "nseconfigs"},
      "vnodelist"	=> {TAG => "vnodelist"},
      "subnodelist"	=> {TAG => "subnodelist"},
      "isalive"		=> {TAG => "isalive"},
      "sfshostid"	=> {TAG => "sfshostid"},
      "jailconfig"	=> {TAG => "jailconfig"},
      "plabconfig"	=> {TAG => "plabconfig"},
      "subconfig"	=> {TAG => "subconfig"},
      "linkdelay"	=> {TAG => "linkdelay"},
      "programs"	=> {TAG => "programs"},
      "syncserver"	=> {TAG => "syncserver"},
      "keyhash"		=> {TAG => "keyhash"},
      "nodeid"		=> {TAG => "nodeid"},
      "ipodinfo"	=> {TAG => "ipodinfo", PERM => "0600"},
      "ntpinfo"		=> {TAG => "ntpinfo"},
      "ntpdrift"	=> {TAG => "ntpdrift"},
      "sdparams"	=> {TAG => "sdparams"},
      "eventkey"	=> {TAG => "eventkey"},
      "routelist"	=> {TAG => "routelist"},
      "role"		=> {TAG => "role"},
      "rusage"		=> {TAG => "rusage"},
      "watchdoginfo"	=> {TAG => "watchdoginfo"},
      "hostkeys"	=> {TAG => "hostkeys"},
      "firewallinfo"	=> {TAG => "firewallinfo"},
      "emulabconfig"	=> {TAG => "emulabconfig"},
      "creator"		=> {TAG => "creator"},
      "hostinfo"	=> {TAG => "hostinfo"},
      "localization"	=> {TAG => "localization"},
      "booterrno"	=> {TAG => "booterrno"},
      "bootlog"	        => {TAG => "bootlog"},
      "battery"	        => {TAG => "battery"},
      "userenv"	        => {TAG => "userenv"},
      "tiptunnels"      => {TAG => "tiptunnels"},
      "traceinfo"       => {TAG => "traceinfo"},
      "elvindport"      => {TAG => "elvindport"},
      "plabeventkeys"   => {TAG => "plabeventkeys"},
      "motelog"         => {TAG => "motelog"},
      "portregister"    => {TAG => "portregister"},
      "bootwhat"        => {TAG => "bootwhat"},
      "rootpswd"        => {TAG => "rootpswd"},
      "topomap"	        => {TAG => "topomap"},
      "ltmap"	        => {TAG => "ltmap"},
      "ltpmap"	        => {TAG => "ltpmap"},
      "tpmblob"      	=> {TAG => "tpmblob"},
      "tpmpubkey"       => {TAG => "tpmpubkey"},
      "loadinfo"        => {TAG => "loadinfo"},
    );

#
# These are the TMCC commands for the user. They are exported above.
#
sub TMCCCMD_REBOOT()	{ $commandset{"reboot"}->{TAG}; }
sub TMCCCMD_STATUS()	{ $commandset{"status"}->{TAG}; }
sub TMCCCMD_STATE()	{ $commandset{"state"}->{TAG}; }
sub TMCCCMD_IFC()	{ $commandset{"ifconfig"}->{TAG}; }
sub TMCCCMD_ACCT()	{ $commandset{"accounts"}->{TAG}; }
sub TMCCCMD_DELAY()	{ $commandset{"delay"}->{TAG}; }
sub TMCCCMD_HOSTS()	{ $commandset{"hostnames"}->{TAG}; }
sub TMCCCMD_RPM()	{ $commandset{"rpms"}->{TAG}; }
sub TMCCCMD_TARBALL()	{ $commandset{"tarballs"}->{TAG}; }
sub TMCCCMD_STARTUP()	{ $commandset{"startupcmd"}->{TAG}; }
sub TMCCCMD_STARTSTAT()	{ $commandset{"startstatus"}->{TAG}; }
sub TMCCCMD_READY()	{ $commandset{"ready"}->{TAG}; }
sub TMCCCMD_MOUNTS()	{ $commandset{"mounts"}->{TAG}; }
sub TMCCCMD_ROUTING()	{ $commandset{"routing"}->{TAG}; }
sub TMCCCMD_TRAFFIC()	{ $commandset{"trafgens"}->{TAG}; }
sub TMCCCMD_BOSSINFO()	{ $commandset{"bossinfo"}->{TAG}; }
sub TMCCCMD_TUNNEL()	{ $commandset{"tunnels"}->{TAG}; }
sub TMCCCMD_NSECONFIGS(){ $commandset{"nseconfigs"}->{TAG}; }
sub TMCCCMD_VNODELIST() { $commandset{"vnodelist"}->{TAG}; }
sub TMCCCMD_SUBNODELIST(){ $commandset{"subnodelist"}->{TAG}; }
sub TMCCCMD_ISALIVE()   { $commandset{"isalive"}->{TAG}; }
sub TMCCCMD_SFSHOSTID()	{ $commandset{"sfshostid"}->{TAG}; }
sub TMCCCMD_JAILCONFIG(){ $commandset{"jailconfig"}->{TAG}; }
sub TMCCCMD_PLABCONFIG(){ $commandset{"plabconfig"}->{TAG}; }
sub TMCCCMD_SUBCONFIG() { $commandset{"subconfig"}->{TAG}; }
sub TMCCCMD_LINKDELAYS(){ $commandset{"linkdelay"}->{TAG}; }
sub TMCCCMD_PROGRAMS()  { $commandset{"programs"}->{TAG}; }
sub TMCCCMD_SYNCSERVER(){ $commandset{"syncserver"}->{TAG}; }
sub TMCCCMD_KEYHASH()   { $commandset{"keyhash"}->{TAG}; }
sub TMCCCMD_NODEID()    { $commandset{"nodeid"}->{TAG}; }
sub TMCCCMD_NTPINFO()   { $commandset{"ntpinfo"}->{TAG}; }
sub TMCCCMD_NTPDRIFT()  { $commandset{"ntpdrift"}->{TAG}; }
sub TMCCCMD_EVENTKEY()  { $commandset{"eventkey"}->{TAG}; }
sub TMCCCMD_ROUTELIST()	{ $commandset{"routelist"}->{TAG}; }
sub TMCCCMD_ROLE()	{ $commandset{"role"}->{TAG}; }
sub TMCCCMD_RUSAGE()	{ $commandset{"rusage"}->{TAG}; }
sub TMCCCMD_WATCHDOGINFO(){ $commandset{"watchdoginfo"}->{TAG}; }
sub TMCCCMD_HOSTKEYS()  { $commandset{"hostkeys"}->{TAG}; }
sub TMCCCMD_FIREWALLINFO(){ $commandset{"firewallinfo"}->{TAG}; }
sub TMCCCMD_EMULABCONFIG(){ $commandset{"emulabconfig"}->{TAG}; }
sub TMCCCMD_CREATOR     (){ $commandset{"creator"}->{TAG}; }
sub TMCCCMD_HOSTINFO    (){ $commandset{"hostinfo"}->{TAG}; }
sub TMCCCMD_LOCALIZATION(){ $commandset{"localization"}->{TAG}; }
sub TMCCCMD_BOOTERRNO   (){ $commandset{"booterrno"}->{TAG}; }
sub TMCCCMD_BOOTLOG     (){ $commandset{"bootlog"}->{TAG}; }
sub TMCCCMD_BATTERY     (){ $commandset{"battery"}->{TAG}; }
sub TMCCCMD_USERENV     (){ $commandset{"userenv"}->{TAG}; }
sub TMCCCMD_TIPTUNNELS  (){ $commandset{"tiptunnels"}->{TAG}; }
sub TMCCCMD_TRACEINFO   (){ $commandset{"traceinfo"}->{TAG}; }
sub TMCCCMD_ELVINDPORT  (){ $commandset{"elvindport"}->{TAG}; }
sub TMCCCMD_PLABEVENTKEYS(){ $commandset{"plabeventkeys"}->{TAG}; }
sub TMCCCMD_MOTELOG()   { $commandset{"motelog"}->{TAG}; }
sub TMCCCMD_PORTREGISTER(){ $commandset{"portregister"}->{TAG}; }
sub TMCCCMD_BOOTWHAT()  { $commandset{"bootwhat"}->{TAG}; }
sub TMCCCMD_ROOTPSWD()  { $commandset{"rootpswd"}->{TAG}; }
sub TMCCCMD_TOPOMAP(){ $commandset{"topomap"}->{TAG}; }
sub TMCCCMD_LTMAP()  { $commandset{"ltmap"}->{TAG}; }
sub TMCCCMD_LTPMAP()  { $commandset{"ltpmap"}->{TAG}; }
sub TMCCCMD_TPMBLOB()  { $commandset{"tpmblob"}->{TAG}; }
sub TMCCCMD_TPMPUB()  { $commandset{"tpmpubkey"}->{TAG}; }
sub TMCCCMD_LOADINFO()  { $commandset{"loadinfo"}->{TAG}; }

#
# Caller uses this routine to set configuration of this library
# 
sub configtmcc($$)
{
    my ($opt, $val) = @_;

    if (!exists($config{$opt})) {
	print STDERR "*** $0:\n".
	             "    Invalid libtmcc option: $opt/$val\n";
	return -1;
    }
    if ($opt eq "cachedir") {
	$CACHEDIR = $val;
    }
    elsif ($opt eq "server") {
	$ENV{'BOSSNAME'} = $val;
    }
    $config{$opt} = $val;
}

#
# Convert the config hash to an option string. This is separate so that
# the user can provide their own option hash on a single tmcc call, which
# *augments* the global options. First argument is a string to augment.
# Returns the augmented string.
#
sub optionstring($%)
{
    my ($options, %opthash) = @_;

    if ($opthash{"debug"}) {
	$options .= " -d";
	$debug    = 1;
    }
    if ($opthash{"useudp"}) {
	$options .= " -u";
    }
    if ($opthash{"nossl"}) {
	$options .= " -i";
    }
    if ($opthash{"usetpm"}) {
	$options .= " -T";
    }
    if ($opthash{"beproxy"}) {
	$options .= " -x " . $opthash{"beproxy"};
	$beproxy  = 1;
    }
    if ($opthash{"dounix"}) {
	$options .= " -l " . $opthash{"dounix"};
    }
    if ($opthash{"beinetproxy"}) {
	$options .= " -X " . $opthash{"beinetproxy"};
	$beproxy  = 1;
    }
    if (defined($opthash{"server"})) {
	$options .= " -s " . $opthash{"server"};
    }
    if (defined($opthash{"subnode"})) {
	$options .= " -n " . $opthash{"subnode"};
    }
    if (defined($opthash{"portnum"})) {
	$options .= " -p " . $opthash{"portnum"};
    }
    if (defined($opthash{"version"})) {
	$options .= " -v " . $opthash{"version"};
    }
    if (defined($opthash{"keyfile"})) {
	$options .= " -k " . $opthash{"keyfile"};
    }
    if (defined($opthash{"datafile"})) {
	$options .= " -f " . $opthash{"datafile"};
    }
    if (defined($opthash{"timeout"})) {
	$options .= " -t " . $opthash{"timeout"};
    }
    if (defined($opthash{"logfile"})) {
	$options .= " -o " . $opthash{"logfile"};
    }
    return $options;
}

#
# Run the external tmcc program with the proper arguments.
#
# usage: tmcc(char *command, char *arguments, list \$results, hash %options)
#        returns -1 if tmcc fails for any reason.
#        returns  0 if tmcc succeeds.
#
# If a "results" argument (pass by reference) has been provided, then
# take the results of tmcc, and store a list of strings into it.
# If an options hash is passed, use that to extend the global config options.
# 
sub runtmcc ($;$$%)
{
    my ($cmd, $args, $results, %optconfig) = @_;
    my @strings = ();
    my $options;

    $options = optionstring("", %config);
    $options = optionstring($options, %optconfig)
	if (%optconfig);

    # Must be last option, before command
    if (defined($config{"urn"})) {
	$options .= " URN=" . $config{"urn"};
    }

    if (!defined($args)) {
	$args = "";
    }
    my $string = "$TMCCBIN $options $cmd $args";
    if ($debug) {
	print STDERR "$string\n";
    }

    #
    # Special case. If a proxy option is given, exec and forget.
    #
    if ($beproxy) {
	exec($string);
	die("exec failure: $string\n");
    }
    if (!open(TM, "$string |")) {
	print STDERR "Cannot start TMCC: $!\n";
	return -1;
    }
    while (<TM>) {
	push(@strings, $_);
    }
    if (! close(TM)) {
	if ($?) {
	    print STDERR "TMCC exited with status $?!\n";
	}
	else {
	    print STDERR "Error closing TMCC pipe: $!\n";
	}
	return -1;
    }
    @$results = @strings
	if (defined($results));
    return 0;
}

#
# Standard entrypoint that is intended to behave just like the
# external program. Return -1 on failure, 0 on success.
#
sub tmcc ($;$$%)
{
    my ($cmd, $args, $results, %opthash) = @_;

    #
    # Clear cache first if requested.
    #
    if ($config{"clrcache"}) {
	tmccgetconfig();
    }

    #
    # Allow per-command setting of nocache and noproxy
    #
    my $nocache;
    if (defined($opthash{"nocache"})) {
	$nocache = $opthash{"nocache"};
    } else {
	$nocache = $config{"nocache"};
    }
    my $noproxy;
    if (defined($opthash{"noproxy"})) {
	$noproxy = $opthash{"noproxy"};
    } else {
	$noproxy = $config{"noproxy"};
    }

    #
    # See if this is a cmd we can get from the local config stash. 
    #
    if (!$nocache && (!defined($args) || $args eq "")) {
	foreach my $key (keys(%commandset)) {
	    my $tag = $commandset{$key}->{TAG};

	    if ($cmd eq $tag) {
		#
		# If we can get it, great! Otherwise go to tmcd.
		#
		my $filename = CacheDir() . "/$tag";
		my @strings  = ();

		if (-e $filename && open(TD, $filename)) {
		    #
		    # Read file contents and return
		    #
		    print STDERR "Fetching locally from $filename\n"
			if ($debug);
		
		    while (<TD>) {
			next
			    if ($_ =~ /^\*\*\* $tag$/);

			push(@strings, $_);
		    }
		    @$results = @strings
			if (defined($results));
		    return 0;
		}
		last;
	    }
	}
    }

    #
    # If proxypath was not specified, check for a proxypath file,
    # unless they explicilty specified not to use a proxy.
    #
    if (!$config{"dounix"} && !$noproxy && -e $PROXYDEF) {
	#
	# Suck out the path and untaint. 
	# 
	open(PP, "$BOOTDIR/proxypath");
	my $ppath = <PP>;
	close(PP);

	if ($ppath =~ /^([-\w\.\/]+)$/) {
	    $config{"dounix"} = $1;
	}
	else {
	    die("Bad data in tmccproxy path: $ppath");
	}
    }
    return(runtmcc($cmd, $args, $results, %opthash));
}

#
# This is done very often! It should always work since we figure it out
# locally, so return the info directly.
#
sub tmccbossname()
{
    my @tmccresults;

    if (runtmcc(TMCCCMD_BOSSINFO, undef, \@tmccresults) < 0 ||
	!scalar(@tmccresults)) {
	warn("*** WARNING: Could not get bossinfo from tmcc!\n");
	return undef;
    }
    my ($bossname) = split(" ", $tmccresults[0]);

    #
    # Taint check. Nice to do for the caller. Also strips any newline.
    #
    if ($bossname =~ /^([-\w\.]*)$/) {
	$bossname = $1;
    }
    else {
	warn("*** WARNING: Tainted bossname from tmcc: $bossname\n");
	return undef;
    }
    return $bossname;
}

#
# Ditto, but return both parts of the info.
#
sub tmccbossinfo()
{
    my @tmccresults;

    if (runtmcc(TMCCCMD_BOSSINFO, undef, \@tmccresults) < 0 ||
	!scalar(@tmccresults)) {
	warn("*** WARNING: Could not get bossinfo from tmcc!\n");
	return undef;
    }
    my ($bossname, $bossip) = split(" ", $tmccresults[0]);

    #
    # Taint check. Nice to do for the caller. Also strips any newline.
    #
    if ($bossname =~ /^([-\w\.]*)$/) {
	$bossname = $1;
    }
    else {
	warn("*** WARNING: Tainted bossname from tmcc: $bossname\n");
	return undef;
    }
    if ($bossip =~ /^([\d\.]*)$/) {
	$bossip = $1;
    }
    else {
	warn("*** WARNING: Tainted boss IP from tmcc: $bossip\n");
	return undef;
    }
    return ($bossname, $bossip);
}

#
# Special entrypoint to clear the current config cache (say, at reboot).
#
sub tmccclrconfig()
{
    my $dir = CacheDir();
	
    if (-d $dir) {
	system("rm -rf $dir");
    }
}

#
# Special entrypoint to download the entire configuration and cache for
# subsequent calls.
#
sub tmccgetconfig()
{
    my @tmccresults;
    my $cdir = CacheDir();

    #
    # Check for proxypath file, but do not override config option. 
    #
    if (!$config{"dounix"} && -e $PROXYDEF) {
	#
	# Suck out the path and untaint. 
	# 
	open(PP, "$BOOTDIR/proxypath");
	my $ppath = <PP>;
	close(PP);

	if ($ppath =~ /^([-\w\.\/]+)$/) {
	    $config{"dounix"} = $1;
	}
	else {
	    die("Bad data in tmccproxy path: $ppath");
	}
    }

    tmccclrconfig();
    if (!mkdir("$cdir", 0775)) {
	warn("*** WARNING: Could not mkdir $cdir: $!\n");
	return -1;
    }
   
    # XXX  Can't "use libsetup" in libtmcc to reference the WINDOWS() function.
    my $arg = (-e "$ETCDIR/iscygwin") ? "windows" : undef;
    if (runtmcc("fullconfig", $arg, \@tmccresults) < 0 ||
	!scalar(@tmccresults)) {
	warn("*** WARNING: Could not get fullconfig from tmcd!\n");
	return -1;
    }

    my $str;
    while ($str = shift(@tmccresults)) {
	if ($str =~ /^\*\*\* ([-\w]*)$/) {
	    my $param = $1;
	    
	    if (open(TD, "> $cdir/$param")) {
		#
		# Set the permission on the file first if necessary
		# XXX the commandset hash is odd
		#
		foreach my $key (keys(%commandset)) {
		    my $tag = $commandset{$key}->{TAG};
		    if ($param eq $tag) {
			if (defined($commandset{$key}->{PERM})) {
			    chmod(oct($commandset{$key}->{PERM}),
				  "$cdir/$param");
			}
			last;
		    }
		}
		while (@tmccresults) {
		    $str = shift(@tmccresults);
		    
		    last
			if ($str =~ /^\*\*\* $param$/);
		    print TD $str;
		}
		close(TD);
	    }
	    else {
		warn("*** WARNING: Could not create $cdir/$str: $!\n");
		return -1;
	    }
	}
    }
    return 0;
}

1;

