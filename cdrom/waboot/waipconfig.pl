#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin:/usr/local/etc/testbed";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# The raw disk device. 
#
my $defrawdisk	= "/dev/ad0";
my $rawbootdisk;
my $etcdir	= "/etc";
my $rcconflocal	= "$etcdir/rc.conf.local";
my $resolveconf = "$etcdir/resolv.conf";
my $hardconfig	= "$etcdir/emulab-hard.txt";
my $softconfig	= "$etcdir/emulab-soft.txt";
my $tbboot	= "tbbootconfig";
    
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
	       bootdisk   => undef,
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
    # First check for a config file on the CD.
    # If found, use that info.  bootdisk must be set.
    #
    if (CheckConfigFile() != 0 || !defined($config{'bootdisk'})) {
	#
	# Give them multiple tries to find a disk with an existing
	# configuration block.
	#
	while (1) {
	    $rawbootdisk = WhichRawDisk();
	    if (CheckConfigBlock() == 0) {
		last;
	    }
	    print "No existing configuration was found on $rawbootdisk\n";
	    if (Prompt("Try another disk for existing configuration?",
		       "No", 10) =~ /no/i) {
		#
		# Don't want to try another disk,
		# consider this a first time install.
		#
		GetNewConfig();
		last;
	    }
	}
    }
    $rawbootdisk = $config{'bootdisk'};

    #
    # Give the user a chance to override the normal operation, and specify
    # alternate parameters.
    #
    while (1) {
	PrintConfig();

	#
	# Everything must be defined or its a big problem! Keep looping
	# until we have everything. 
	#
	foreach my $key (keys(%config)) {
	    if (!defined($config{$key})) {
		print "Some configuration parameters are missing!\n";
		GetUserConfig();
		next;
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
    
    #
    # Text version of the latest config. Used later for checking registration
    # and the disk update.
    # 
    if (WriteConfigFile($softconfig) < 0) {
	print "Could not write soft config file.\n";
	exit(-1);
    }
    return 0;
}

sub GetNewConfig()
{
    #
    # First, an intimidating disclaimer
    #
    print "\n";
    print "****************************************************************\n";
    print "*                                                              *\n";
    print "* Netbed installation CD (version 0.1).                        *\n";
    print "*                                                              *\n";
    print "* THIS PROGRAM WILL WIPE YOUR YOUR HARD DISK!                  *\n";
    print "* It will then install a fresh Netbed image.                   *\n";
    print "*                                                              *\n";
    print "* If this is NOT what you intended, please type 'no' to the    *\n";
    print "* prompt below and the machine will reboot (don't forget to    *\n";
    print "* remove the CD).                                              *\n";
    print "*                                                              *\n";
    print "* Before you can install this CD, you must first go to         *\n";
    print "* www.emulab.net/cdromnewkey.php for a password.               *\n";
    print "*                                                              *\n";
    print "* You must also know some basic characteristics of the machine *\n";
    print "* you are installing on (disk device and network interface) as *\n";
    print "* well as configuration of the local network (host and domain  *\n";
    print "* name, IP address for machine, nameserver and gateway).       *\n";
    print "*                                                              *\n";
    print "****************************************************************\n";
    print "\n";

    if (Prompt("Continue with installation?", "No") =~ /no/i) {
	exit(13);
    }

    #
    # All right, we gave them a chance.
    # Here we should explain what the possible disk/network options are,
    # preferably by going out and discovering what exists.
    #

    print "****************************************************************\n";
    print "*                                                              *\n";
    print "* The installation script attempts to intuit default values    *\n";
    print "* for much of installation information.  In the process it may *\n";
    print "* generate error messages from the kernel which may be safely  *\n";
    print "* ignored.                                                     *\n";
    print "*                                                              *\n";
    print "****************************************************************\n";

    GetUserConfig();
}

#
# Prompt for the configuration from the user. Return -1 if something goes
# wrong.
#
sub GetUserConfig()
{
    print "Please enter the system configuration by hand\n";
    
    $config{'bootdisk'}  = Prompt("System boot disk", $rawbootdisk);
    $config{'interface'} = Prompt("Network Interface", WhichInterface());
    $config{'hostname'}  = Prompt("Hostname (no domain)", $config{'hostname'});
    $config{'domain'}    = Prompt("Domain", $config{'domain'});
    $config{'IP'}        = Prompt("IP Address", $config{'IP'});
    $config{'netmask'}   = Prompt("Netmask", WhichNetMask());
    $config{'gateway'}   = Prompt("Gateway IP", WhichGateway());
    $config{'nameserver'}= Prompt("Nameserver IP", $config{'nameserver'});

    # XXX
    $rawbootdisk = $config{'bootdisk'};

    return 0;
}

#
# Check for hardwired configuration file. We do this on both the CDROM boot
# and the disk boot.
#
sub CheckConfigFile(;$)
{
    my ($path) = @_;

    if (!defined($path)) {
	$path = "$hardconfig";
    }
	
    if (! -e $path) {
	return -1;
    }
    if (! open(CONFIG, $path)) {
	print("$path could not be opened for reading: $!\n");
	return -1;
    }
    while (<CONFIG>) {
	chomp();
	ParseConfigLine($_);
    }
    close(CONFIG);
    return 0;
}

#
# Check Config block.
#
sub CheckConfigBlock()
{
    #
    # See if a valid header block.
    #
    system("$tbboot -v $rawbootdisk");
    if ($?) {
	return -1;
    }

    #
    # Valid block, so read the configuration out to a temp file
    # and then parse it normally.
    # 
    system("$tbboot -r /tmp/config.$$ $rawbootdisk");
    if ($?) {
	return -1;
    }
    if (CheckConfigFile("/tmp/config.$$") == 0) {
	# XXX bootdisk is not part of the on disk info
	$config{'bootdisk'} = $rawbootdisk;
	return 0;
    }
    return -1;
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
	    if ($1 eq "lo") {
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

#
# Which raw disk. Prompt if we cannot come up with a good guess.
# Note: raw and block devices are one in the same now.
#
sub WhichRawDisk()
{
    #
    # Find the list of configured disks
    #
    my @list = DiskList();

    #
    # Search the drives looking for one with a valid header.
    # 
    foreach my $disk (@list) {
	my $guess = "/dev/${disk}";

	system("$tbboot -v $guess");
	if (! $?) {
	    #
	    # Allow for overiding the guess, with short timeout.
	    #
	    $rawbootdisk = Prompt("Which Disk Device is the boot device?",
				  "$guess", 10);
	    goto gotone;
	}
    }

    #
    # None with configuration info, just use the first existing disk
    # which is large enough and is actually accessible.
    #
    foreach my $disk (@list) {
	my $guess = "/dev/${disk}";

	if (DiskSize($disk) >= $MINDISKSIZE && DiskReadable($disk)) {
	    #
	    # Allow for overiding the guess, with short timeout.
	    #
	    $rawbootdisk = Prompt("Which Disk Device is the boot device?",
				  "$guess", 10);
	    goto gotone;
	}
    }
  gotone:
    
    #
    # If still not defined, then loop forever.
    # 
    while (!defined($rawbootdisk) || ! -e $rawbootdisk) {
	$rawbootdisk = Prompt("Which Disk Device is the boot device?",
			      $defrawdisk);
    }
    return $rawbootdisk;
}

#
# Create a list of all disks and their sizes.
#
sub DiskList()
{
    if (-x $dmesgcmd) {
	GetDisks($dmesgcmd);
    }

    # if we didn't grab anything there, try the /var/run file
    if (@disklist == 0 && -r $dmesgfile) {
	GetDisks("cat $dmesgfile");
    }

    return @disklist;
}

sub DiskSize($)
{
    my ($name) = @_;

    if (defined($disksize{$name})) {
	return $disksize{$name};
    }
    return 0;
}

sub DiskReadable($)
{
    my ($disk) = @_;
    my $dev = "/dev/$disk";

    if (!system("dd if=$dev of=/dev/null bs=512 count=32 >/dev/null 2>&1")) {
	return(1);
    }
    return(0);
}

sub GetDisks($)
{
    my ($cmd) = @_;
    my @units = (0, 1, 2, 3);
    my @cmdout = `$cmd`;

    #
    # Arbitrary: we prefer disk type over unit number;
    # e.g. ad1 is better than da0.
    #
    foreach my $disk (@preferred) {
	foreach my $unit (@units) {
	    my $dmesgpat = "^($disk$unit):.* (\\d+)MB.*\$";
	    foreach my $line (@cmdout) {
		if ($line =~ /$dmesgpat/) {
		    my $name = $1;
		    my $size = $2;
		    if (!defined($disksize{$name})) {
			push(@disklist, $name);
		    }
		    $disksize{$name} = $size;
		}
	    }
	}
    }
}


#
# Write a config file suitable for input to the testbed boot header program.
#
sub WriteConfigFile($)
{
    my ($path) = @_;

    print "Writing $path\n";
    if (! open(CONFIG, "> $path")) {
	print("$path could not be opened for writing: $!\n");
	return -1;
    }
    foreach my $key (keys(%config)) {
	my $val = $config{$key};

	print CONFIG "$key=$val\n";
    }
    close(CONFIG);
    return 0;
}

sub PrintConfig()
{
    print "\nCurrent Configuration:\n";
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
    print CONFIG "hostname=\"$config{hostname}.$config{domain}\"\n";
    print CONFIG "ifconfig_$config{interface}=\"".
	         "inet $config{IP} netmask $config{netmask}\"\n";
    print CONFIG "defaultrouter=\"$config{gateway}\"\n";
    print CONFIG "# Netbed info\n";
    print CONFIG "netbed_disk=\"$rawbootdisk\"\n";
    print CONFIG "netbed_IP=\"$config{IP}\"\n";
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
# Parse config lines. Kinda silly.
#
sub ParseConfigLine($)
{
    my($line) = @_;

    if ($line =~ /(.*)="(.+)"/ ||
	$line =~ /^(.*)=(.+)$/) {
	print "$1 $2\n";
	exists($config{$1}) and
	    $config{$1} = $2;
    }
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
