#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
@EXPORT = qw(configtmcc tmcc tmccbossname
	     TMCCCMD_REBOOT TMCCCMD_STATUS TMCCCMD_STATE TMCCCMD_IFC
	     TMCCCMD_ACCT TMCCCMD_DELAY TMCCCMD_HOSTS TMCCCMD_RPM
	     TMCCCMD_TARBALL TMCCCMD_STARTUP TMCCCMD_DELTA TMCCCMD_STARTSTAT
	     TMCCCMD_READY TMCCCMD_MOUNTS TMCCCMD_ROUTING TMCCCMD_TRAFFIC
	     TMCCCMD_BOSSINFO TMCCCMD_TUNNEL TMCCCMD_NSECONFIGS
	     TMCCCMD_VNODELIST TMCCCMD_SUBNODELIST TMCCCMD_ISALIVE
	     TMCCCMD_SFSHOSTID TMCCCMD_SFSMOUNTS TMCCCMD_JAILCONFIG
	     TMCCCMD_PLABCONFIG TMCCCMD_SUBCONFIG TMCCCMD_LINKDELAYS
	     TMCCCMD_PROGRAMS TMCCCMD_SYNCSERVER TMCCCMD_KEYHASH TMCCCMD_NODEID
	     TMCCCMD_NTPINFO TMCCCMD_NTPDRIFT
	     );

# Must come after package declaration!
use English;

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
      "server"		=> undef,
      "portnum"		=> undef,
      "version"		=> undef,
      "subnode"		=> undef,
      "keyfile"		=> undef,
      "timeout"		=> undef,
      "logfile"		=> undef,
    );

#
# List of TMCD commands. Some of these have to be passed through to
# tmcd, others can come from a local config file if it exists.
#
my %commandset =
    ( "reboot"		=> {TAG => "reboot",		LOCAL => 0},
      "status"		=> {TAG => "status",		LOCAL => 0},
      "state"		=> {TAG => "state",		LOCAL => 0},
      "ifconfig"	=> {TAG => "ifconfig",		LOCAL => 1},
      "accounts"	=> {TAG => "accounts",		LOCAL => 1},
      "delay"		=> {TAG => "delay",		LOCAL => 1},
      "hostnames"	=> {TAG => "hostnames",		LOCAL => 1},
      "rpms"		=> {TAG => "rpms",		LOCAL => 1},
      "tarballs"	=> {TAG => "tarballs",		LOCAL => 1},
      "startupcmd"	=> {TAG => "startupcmd",	LOCAL => 1},
      "deltas"		=> {TAG => "deltas",		LOCAL => 1},
      "startstatus"	=> {TAG => "startstatus",	LOCAL => 0},
      "ready"		=> {TAG => "ready",		LOCAL => 0},
      "mounts"		=> {TAG => "mounts",		LOCAL => 1},
      "routing"		=> {TAG => "routing",		LOCAL => 1},
      "trafgens"	=> {TAG => "trafgens",		LOCAL => 1},
      "bossinfo"	=> {TAG => "bossinfo",		LOCAL => 0},
      "tunnels"		=> {TAG => "tunnels",		LOCAL => 1},
      "nseconfigs"	=> {TAG => "nseconfigs",	LOCAL => 1},
      "vnodelist"	=> {TAG => "vnodelist",		LOCAL => 1},
      "subnodelist"	=> {TAG => "subnodelist",	LOCAL => 1},
      "isalive"		=> {TAG => "isalive",		LOCAL => 0},
      "sfshostid"	=> {TAG => "sfshostid",		LOCAL => 0},
      "sfsmounts"	=> {TAG => "sfsmounts",		LOCAL => 1},
      "jailconfig"	=> {TAG => "jailconfig",	LOCAL => 1},
      "plabconfig"	=> {TAG => "plabconfig",	LOCAL => 1},
      "subconfig"	=> {TAG => "subconfig",		LOCAL => 1},
      "linkdelays"	=> {TAG => "linkdelays",	LOCAL => 1},
      "programs"	=> {TAG => "programs",		LOCAL => 1},
      "syncserver"	=> {TAG => "syncserver",	LOCAL => 1},
      "keyhash"		=> {TAG => "keyhash",		LOCAL => 1},
      "nodeid"		=> {TAG => "nodeid",		LOCAL => 1},
      "ntpinfo"		=> {TAG => "ntpinfo",		LOCAL => 1},
      "ntprift"		=> {TAG => "ntpdrift.",		LOCAL => 0},
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
sub TMCCCMD_DELTA()	{ $commandset{"deltas"}->{TAG}; }
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
sub TMCCCMD_SFSMOUNTS() { $commandset{"sfsmounts"}->{TAG}; }
sub TMCCCMD_JAILCONFIG(){ $commandset{"jailconfig"}->{TAG}; }
sub TMCCCMD_PLABCONFIG(){ $commandset{"plabconfig"}->{TAG}; }
sub TMCCCMD_SUBCONFIG() { $commandset{"subconfig"}->{TAG}; }
sub TMCCCMD_LINKDELAYS(){ $commandset{"linkdelays"}->{TAG}; }
sub TMCCCMD_PROGRAMS()  { $commandset{"programs"}->{TAG}; }
sub TMCCCMD_SYNCSERVER(){ $commandset{"syncserver"}->{TAG}; }
sub TMCCCMD_KEYHASH()   { $commandset{"keyhash"}->{TAG}; }
sub TMCCCMD_NODEID()    { $commandset{"nodeid"}->{TAG}; }
sub TMCCCMD_NTPINFO()   { $commandset{"ntpinfo"}->{TAG}; }
sub TMCCCMD_NTPDRIFT()  { $commandset{"ntpdrift"}->{TAG}; }

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
    if ($opthash{"beproxy"}) {
	$options .= " -x " . $opthash{"beproxy"};
	$beproxy  = 1;
    }
    if ($opthash{"dounix"}) {
	$options .= " -l " . $opthash{"dounix"};
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

    if (!defined($args)) {
	$args = "";
    }
    my $string = "$TMCCBIN $options $cmd $args";
    if ($debug) {
	print STDERR "$string\n";
    }

    #
    # Special case. If the proxy option is given, exec and forget.
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

1;

