#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This file goes in /usr/site/sbin on the CDROM.
#
use English;
use Getopt::Std;
use Fcntl;
use IO::Handle;

#
# Disk related parameters
#

# where to find kernel config output
my $dmesgcmd = "/sbin/dmesg";
my $dmesgfile = "/var/run/dmesg.boot";

# preferred ordering of disks to use
my @preferred = ("ar", "aacd", "amrd", "mlxd", "twed", "ad", "da");

# ordered list of disks found and hash of sizes
my @disklist;
my %disksize;

# min disk size we can use (in MB)
my $MINDISKSIZE = 8000;

#
# Boot configuration for the CDROM. Determine the IP configuration, either
# from the floopy or from the user interactively.
#
# This is run from /etc/rc.network on the cdrom. It will write a new
# file of shell variables resembling what goes in /etc/rc.conf so that
# network configuration can proceed normally. Also write the new values
# to the file floopy data file for next time.
#
sub usage()
{
    print("Usage: waipconfig\n");
    exit(-1);
}
my  $optlist = "";

#
# Turn off line buffering on output
#
STDOUT->autoflush(1);
STDERR->autoflush(1);

#
# Untaint the environment.
# 
$ENV{'PATH'} = "/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:".
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

my $etcdir	= "/etc";
my $rcconflocal	= "$etcdir/rc.conf.local";
my $resolveconf = "$etcdir/resolv.conf";
my $rootuser	= "root";
my $dodhcp	= 1;
    
#
# This is our configuration.
#
my %config = ( interface  => undef,
	       hostname   => undef,
	       domain     => undef,
	       IP         => undef,
	       netmask    => undef,
	       nameserver => undef,
	       gateway    => undef,
	     );

# Must be root
if ($UID != 0) {
    die("*** $0:\n".
	"    Must be root to run this script!\n");
}

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV != 0) {
    usage();
}

#
# Catch ^C and exit with special status. See exit(13) below.
# The wrapper script looks these exits!
# 
sub handler () {
    $SIG{INT} = 'IGNORE';
    exit(12);
}
$SIG{INT}  = \&handler;

# Do it.
BootFromCD();
exit(0);

sub BootFromCD()
{
    #
    # Get initial config.
    # 
    GetNewConfig();

    #
    # Give the user a chance to override the normal operation, and specify
    # alternate parameters.
    #
    while (1) {
	PrintConfig();

	#
	# If not using DHCP, everything must be defined or its a big
	# problem! Keep looping until we have everything.
	#
	if ($dodhcp) {
	    if (!defined($config{'interface'})) {
		    print "Must supply a network interface for DHCP!\n";
		    GetUserConfig();
		    next;
	    }
	}
	else {
	    foreach my $key (keys(%config)) {
		if (!defined($config{$key})) {
		    print "Some configuration parameters are missing!\n";
		    GetUserConfig();
		    next;
		}
	    }
	}

	#
	# Otherwise, one last chance to override.
	#
	if (Prompt("Specify Alternate Configuration?", "No", 10) =~ /no/i) {
	    last;
	}
	GetUserConfig();
    }

    #
    # Generate the rc files.
    #
    if (WriteRCFiles() < 0) {
	print "Could not write rc files.\n";
	exit(-1);
    }
    
    return 0;
}

sub GetNewConfig()
{
    my $rootpswd;
    
    #
    # First, an intimidating disclaimer
    #
    print "\n";
    print "****************************************************************\n";
    print "*                                                              *\n";
    print "* Frisbee Demonstration CD (version 0.1).                      *\n";
    print "*                                                              *\n";
    print "* This CD will boot up the machine in a RAM/CD-based FreeBSD   *\n";
    print "* system that allows remote ssh access as root to save/restore *\n";
    print "* the hard drive contents.                                     *\n";
    print "*                                                              *\n";
    print "* If this is NOT what you intended, please type 'no' to the    *\n";
    print "* prompt below and the machine will reboot (don't forget to    *\n";
    print "* remove the CD).                                              *\n";
    print "*                                                              *\n";
    print "* You must also know some basic characteristics of the machine *\n";
    print "* you are installing on (network interface) as well as         *\n";
    print "* configuration of the local network if you are not running    *\n";
    print "* DHCP (host name, domain name, IP address, nameserver,        *\n";
    print "* gateway) on your network.                                    *\n";
    print "*                                                              *\n";
    print "****************************************************************\n";
    print "\n";

    if (Prompt("Continue with Frisbee Demonstration?", "No") =~ /no/i) {
	exit(13);
    }

    print "\n";
    print "Please enter a root password. You will need this password to\n";
    print "save your image (using ssh) to another machine on your network.\n";

    while (! defined($rootpswd)) {
	system("stty -echo");
	$rootpswd = Prompt("Root Password");
	system("stty echo");

	if (!defined($rootpswd)) {
	    print "Entering a null root password is VERY dangerous.\n";

	    if (Prompt("Continue with null root password?", "No") =~ /no/i) {
		next;
	    }
	    last;
	}
    }
    
    if (defined($rootpswd)) {
	if (! open(PSWD, "| pw usermod -n $rootuser -h 0 >/tmp/pw.log 2>&1")) {
	    die("Could not start up pw!\n");
	}
	print PSWD "$rootpswd\n";
	close(PSWD) or
	    die("Could not finish up pw!\n");
    }
    else {
	system("chpass -p '' $rootuser >/tmp/pw.log 2>&1") == 0 or
	    die("chpass error! Could not set root password!\n");
    }

    GetUserConfig();
}

#
# Prompt for the configuration from the user. Return -1 if something goes
# wrong.
#
sub GetUserConfig()
{
    print "Please enter the system configuration by hand\n";
    
    $config{'interface'} = Prompt("Network Interface", WhichInterface());

    #
    # Ask if they want to use DHCP. If so, we skip all the other stuff.
    # If not, must prompt for the goo.
    #
    if (Prompt("Use DHCP?", ($dodhcp ? "Yes" : "No")) =~ /yes/i) {
	$dodhcp = 1;
	return 0;
    }
    $dodhcp = 0;
    
    $config{'domain'}    = Prompt("Domain", $config{'domain'});
    $config{'hostname'}  = Prompt("Hostname (without the domain!)",
				  $config{'hostname'});
    $config{'IP'}        = Prompt("IP Address", $config{'IP'});
    $config{'netmask'}   = Prompt("Netmask", WhichNetMask());
    $config{'gateway'}   = Prompt("Gateway IP", WhichGateway());
    $config{'nameserver'}= Prompt("Nameserver IP", $config{'nameserver'});

    return 0;
}

#
# Which network interface. Just come up with a guess, and let the caller
# spit that out in a prompt for verification.
#
sub WhichInterface()
{
    # XXX
    if (defined($config{'interface'})) {
	return $config{'interface'};
    }
    my @allifaces = split(" ", `ifconfig -l`);
		       
    #
    # Prefer the first interface found that is not "lo".
    #
    foreach my $iface (@allifaces) {
	if ($iface =~ /([a-zA-z]+)(\d+)/) {
	    if ($1 eq "lo" || $1 eq "faith" || $1 eq "gif" || $1 eq "tun") {
		next;
	    }

	    # XXX check to make sure it has carrier
	    if (`ifconfig $iface` =~ /.*no carrier/) {
		next;
	    }

	    return $iface;
	}
    }

    return undef;
}

#
# Which network mask.  Default based on the network number.
#
sub WhichNetMask()
{
    # XXX
    if (defined($config{'netmask'})) {
	return $config{'netmask'};
    }

    #
    # XXX this is a nice idea, but will likely be wrong for large
    # institutions that subdivide class B's (e.g., us!)
    #
    if (0 && defined($config{'IP'})) {
	my ($net) = split(/\./, $config{'IP'});
	return "255.0.0.0" if $net < 128;
	return "255.255.0.0" if $net < 192;
    }

    return "255.255.255.0";
}

#
# Which gateway IP.  Use the more or less traditional .1 for the network
# indicated by (IP & netmask).
#
sub WhichGateway()
{
    # XXX
    if (defined($config{'gateway'})) {
	return $config{'gateway'};
    }

    #
    # Grab IP and netmask, combine em, stick 1 in the low quad,
    # make sure the result isn't the IP, and return that.
    # Parsing tricks from the IPv4Addr package.
    #
    if (defined($config{'IP'}) && defined($config{'netmask'})) {
	my $addr = unpack("N", pack("CCCC", split(/\./, $config{'IP'})));
	my $mask = unpack("N", pack("CCCC", split(/\./, $config{'netmask'})));
	my $gw = ($addr & $mask) | 1;
	if ($gw != $addr) {
	    return join(".", unpack("CCCC", pack("N", $gw)));
	}
    }

    return undef;
}

sub PrintConfig()
{
    print "\nCurrent Configuration:\n";

    if ($dodhcp) {
	print "  interface=" . $config{'interface'} . "\n";
	print "  DHCP=Yes\n";
	return;
    }
    
    foreach my $key (keys(%config)) {
	my $val = $config{$key};

	if (!defined($val)) {
	    $val = "*** undefined ***";
	}

	print "  $key=$val\n";
    }
    print "\n";
    return 0;
}

#
# Write an rc.conf style file which can be included by sh. This is based
# on the info we get. We also write a resolve.conf
#
sub WriteRCFiles()
{
    my $path = "$rcconflocal";

    if (! open(CONFIG, "> $path")) {
	print("$path could not be opened for writing: $!\n");
	return -1;
    }
    print "Writing $path\n";
    print CONFIG "#\n";
    print CONFIG "# DO NOT EDIT! This file is autogenerated at reboot.\n";
    print CONFIG "#\n";

    print CONFIG "network_interfaces=\"".
	         "\$network_interfaces $config{'interface'}\"\n";

    #
    # Its very simple if using DHCP!
    #
    if ($dodhcp) {
	print CONFIG "ifconfig_$config{interface}=\"DHCP\"\n";
	close(CONFIG);
	return 0;
    }

    print CONFIG "hostname=\"$config{hostname}.$config{domain}\"\n";
    print CONFIG "ifconfig_$config{interface}=\"".
	         "inet $config{IP} netmask $config{netmask}\"\n";
    print CONFIG "defaultrouter=\"$config{gateway}\"\n";
    print CONFIG "# EOF\n";
    close(CONFIG);

    $path = $resolveconf;
    print "Writing $path\n";
    if (! open(CONFIG, "> $path")) {
	print("$path could not be opened for writing: $!\n");
	return -1;
    }
    print CONFIG "search $config{domain}\n";
    print CONFIG "nameserver $config{nameserver}\n";
    close(CONFIG);

    return 0;
}

#
# Spit out a prompt and a default answer. If optional timeout supplied,
# then wait that long before returning the default. Otherwise, wait forever.
#
sub Prompt($$;$)
{
    my ($prompt, $default, $timeout) = @_;

    if (!defined($timeout)) {
	$timeout = 10000000;
    }

    print "$prompt";
    if (defined($default)) {
	print " [$default]";
    }
    print ": ";

    eval {
	local $SIG{ALRM} = sub { die "alarm\n" }; # NB: \n required
	
	alarm $timeout;
	$_ = <STDIN>;
	alarm 0;
    };
    if ($@) {
	if ($@ ne "alarm\n") {
	    die("Unexpected interrupt in prompt\n");
	}
	#
	# Timed out.
	#
	print "\n";
	return $default;
    }
    return undef
	if (!defined($_));
	
    chomp();
    if ($_ eq "") {
	return $default;
    }

    return $_;
}
