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
my $defrawdisk	= "/dev/rad0";
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
	       IP         => undef,
	       netmask    => undef,
	       domain     => undef,
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

print "\n";
print "**************************************************************\n";
print "*                                                            *\n";
print "* Netbed installation CD.  This program will wipe your       *\n";
print "* hard disk, and install a fresh Netbed image.               *\n";
print "*                                                            *\n";
print "* If this is not what you intended, please remove the CD and *\n";
print "* reboot.                                                    *\n";
print "*                                                            *\n";
print "* Before you can install this CD, you must first go to       *\n";
print "* www.emulab.net/cdromnewkey.php for a password.             *\n";
print "*                                                            *\n";
print "* Then, figure out what boot disk you want to use.           *\n";
print "* Common disks are ad0 (1st IDE disk) and da0 (1st SCSI).    *\n";
print "*                                                            *\n";
print "**************************************************************\n";
print "\n";

#
# Need a raw disk device.
#
$rawbootdisk = WhichRawDisk();

BootFromCD();

# Done!
exit(0);

sub BootFromCD()
{
    #
    # On the CDROM, the order is to check for a hardwired config,
    # and then a config stashed in the boot block. If neither is there,
    # then prompt the user. Always give the user a chance to override the
    # the current config. 
    # 
    if (CheckConfigFile() != 0 && CheckConfigBlock() != 0) {
	#
	# Else no config found. Must wait until we get one.
	# 
	print "\nNo existing configuration was found!\n";
	GetUserConfig();
    }

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
	Reboot();
    }
    
    #
    # Text version of the latest config. Used later for checking registration
    # and the disk update.
    # 
    if (WriteConfigFile($softconfig) < 0) {
	print "Could not write soft config file.\n";
	Reboot();
    }
    return 0;
}

#
# Prompt for the configuration from the user. Return -1 if something goes
# wrong.
#
sub GetUserConfig()
{
    print "Please enter the system configuration by hand\n";
    
    $config{'interface'} = Prompt("Network Interface", WhichInterface());
    $config{'hostname'}  = Prompt("Hostname (no domain)", $config{'hostname'});
    $config{'domain'}    = Prompt("Domain", $config{'domain'});
    $config{'IP'}        = Prompt("IP Address", $config{'IP'});
    $config{'netmask'}   = Prompt("Netmask", $config{'netmask'});
    $config{'nameserver'}= Prompt("Nameserver", $config{'nameserver'});
    $config{'gateway'}   = Prompt("Gateway IP", $config{'gateway'});

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
    return CheckConfigFile("/tmp/config.$$");
}

#
# Which network interface. Just come up with a guess, and let the caller
# spit that out in a prompt for verification.
#
sub WhichInterface()
{
    # XXX
    if (defined($config{interface})) {
	return $config{interface};
    }
    my @allifaces = split(" ", `ifconfig -l`);
		       
    foreach my $guess ("fxp", "de", "tx", "vx", "wx", "em", "bge", "xl") {
	foreach my $iface (@allifaces) {
	    $iface =~ /([a-zA-z]*)(\d*)/;
	    if ($1 eq $guess && $2 == 0) {
		return $iface;
	    }
	}
    }
    return "fxp0";
}

#
# Which raw disk. Prompt if we cannot come up with a good guess.
#
sub WhichRawDisk()
{
    #
    # Search the drives until we find a valid header.
    # 
    foreach my $disk ("rad", "rda", "raacd") {
	foreach my $drive (0) {
	    my $guess = "/dev/${disk}${drive}";

	    system("$tbboot -v $guess");
	    if (! $?) {
		#
		# Allow for overiding the guess, with short timeout.
		#
		$rawbootdisk =
		    Prompt("Which Raw Disk Device is the boot device?",
			   "$guess", 10);
		goto gotone;
	    }
	}
    }

    #
    # Okay, no candidates. Lets find the first real disk. Use dd
    # to see if the drive is configured.
    #
    foreach my $disk ("rad0", "rda0", "raacd0") {
	my $guess = "/dev/${disk}";

	system("dd if=$guess of=/dev/null bs=512 count=32 >/dev/null 2>&1");
	if (! $?) {
	    #
	    # Allow for overiding the guess, with short timeout.
	    #
	    $rawbootdisk =
		Prompt("Which Raw Disk Device is the boot device?",
		       "$guess", 10);
	    goto gotone;
	}
    }
  gotone:
    
    #
    # If still not defined, then loop forever.
    # 
    while (!defined($rawbootdisk) || ! -e $rawbootdisk) {
	$rawbootdisk = 
	    Prompt("Which Raw Disk Device is the boot device?", $defrawdisk);
    }
    return $rawbootdisk;
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
    print CONFIG "rawbootdisk=\"$rawbootdisk\"\n";
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

sub Reboot()
{
    print "Rebooting ... \n";
    #system("/sbin/reboot");
    exit(-1);
}
