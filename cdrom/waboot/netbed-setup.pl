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
# This file goes in boss:/z/testbed/distributions on boss so that is can
# be passed over via wget to the CDROM on widearea nodes.
#
sub usage()
{
    print("Usage: netbed-setup.pl <bootdisk> <ipaddr>\n");
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
$ENV{'PATH'} = "/tmp:/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:".
    "/usr/local/bin:/usr/site/bin:/usr/site/sbin:/usr/local/etc/testbed";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Definitions
#
my $WWW         = "https://www.emulab.net";
#my $WWW        = "http://golden-gw.ballmoss.com:8080/~stoller/testbed";
#my $WWW        = "https://www.emulab.net/~stoller/www";
my $etcdir	= "/etc";
# Hardwired cdkey.
my $cdkeyfile	= "$etcdir/emulab.cdkey";
# Hardwired config.
my $setupconfig	= "$etcdir/emulab-setup.txt";
# Good for onetime install without being prompted! See note below.
my $installkey  = "$etcdir/emulab.ikey";
# Current private key.
my $privkeyfile = "$etcdir/emulab.pkey";
# File to place in toplevel of each slice; holds original MD5.
my $slicetag    = "slicetag";
# The current IP config, as determined by waipconfig.pl at boot.
my $softconfig	= "$etcdir/emulab-soft.txt";
my $tbboot	= "tbbootconfig";
my $wget	= "wget";
my $logfile     = "/tmp/netbed-setup.log";

#
# Locals
#
my $tempdevice  = "s2c";
my $rootdevice  = "s1a";
my $slicexpart  = "e";
my $slicexdev;
my $needstore   = 0;
my $fromscratch = 0;
my $needumount  = 0;
my $showprogress= 1;
my $cdkey;
my $privkey;
my %slicesizes;
	
my $PATIENCEMSG  = "This could take several minutes, please be patient!\n";

my %config = ( privkey        => undef,
	       fdisk          => undef,
	       slice1_image   => undef,
	       slice1_md5     => undef,
	       slice3_image   => undef,
	       slice3_md5     => undef,
	       slice4_image   => undef,
	       slice4_md5     => undef,
	       slicex_slice   => undef,
	       slicex_mount   => undef,
	       slicex_tarball => undef,
	       slicex_md5     => undef,
	     );

# These errors must match what the web page returns.
my $CDROMSTATUS_OKAY	 =	0;
my $CDROMSTATUS_MISSINGARGS =	100;
my $CDROMSTATUS_INVALIDARGS =	101;
my $CDROMSTATUS_BADCDKEY =	102;
my $CDROMSTATUS_BADPRIVKEY =	103;
my $CDROMSTATUS_BADIPADDR =	104;
my $CDROMSTATUS_BADREMOTEIP =	105;
my $CDROMSTATUS_IPADDRINUSE =	106;
my $CDROMSTATUS_OTHER =		199;

my %weberrors =
    ( $CDROMSTATUS_MISSINGARGS  => "Missing Page Arguments",
      $CDROMSTATUS_INVALIDARGS  => "Invalid Page Arguments",
      $CDROMSTATUS_BADCDKEY     => "Invalid CD Key",
      $CDROMSTATUS_BADPRIVKEY   => "Invalid Private Key",
      $CDROMSTATUS_BADIPADDR    => "IP address does not match key",
      $CDROMSTATUS_BADREMOTEIP  => "IP address does not match remote IP",
      $CDROMSTATUS_IPADDRINUSE  => "IP address already registered at Netbed",
      $CDROMSTATUS_OTHER        => "Unknown Error",
    );

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (@ARGV != 2) {
    usage();
}
my $rawbootdisk = $ARGV[0];
my $IP = $ARGV[1];

#
# Untaint the arguments.
#
if ($rawbootdisk =~ /^([\w\/]+)$/) {
    $rawbootdisk = $1;
}
else {
    fatal("Tainted argument $rawbootdisk!");
}
if ($IP =~ /^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})$/) {
    $IP = $1;
}
else {
    fatal("Tainted argument $IP!");
}

#
# Grab hostname for passing back to website so it can cons up a nickname.
# Strictly optional, but is nice for the web page display.
#
my $hostname = `hostname`;
if ($hostname =~ /^([-\w\.\@]*)$/) {
    $hostname = $1;
}
else {
    fatal("Tainted hostname $hostname!");
}

#
# Get the plain device names (ad0, rad0).
# Note, there is no difference in block/raw devices anymore.
#
my $rawdevice   = substr($rawbootdisk, rindex($rawbootdisk, '/') + 1);
my $blockdevice = $rawdevice;

#
# See if this is the first time. Look for the magic boot header.
#
$fromscratch = mysystem("$tbboot -v $rawbootdisk");

GetInstructions();

#
# Do we even need a temp store? After the initial setup, we won't
# need it unless updating the disk. See Checkin() for the variable.
#
if ($needstore) {
    #
    # If the disk is raw, we need to get the fdisk params and lay it down.
    #
    if ($fromscratch) {
	my $fdisk  = $config{fdisk};
	my $adjust = 0;
	my $FDSK;

	fatal("Testbed Central did not tell us what fdisk table to use!")
	    if (!defined($fdisk));

	print "Creating a temporary filesystem on slice 2 to hold images.\n";
	print "    (ignore kernel generated \"no disk label\" messages.)\n";
	
	#
	# First time through we need to lay down a DOS partition table.
	#
	mysystem("dd if=/dev/zero of=${rawbootdisk} bs=1k count=1024");
	fatal("Failed to clear disk of old goop!")
	    if ($?);
	
	mysystem("fdisk -I $rawbootdisk");
	fatal("Failed to initialize DOS partition table!")
	    if ($?);

	#
	# Read the summary info to see if we are going to have a problem
	# with fdisk changing the sizes!
	#
	my $summary = `fdisk -s $rawbootdisk | head -1`;
	if ($summary =~ /^.*cyl (\d*) hd (\d*) sec/) {
	    if ($1 != 255) {
		$adjust = $1 * $2;
	    }
	}

	#
	# Get the fdisk table. 
	# 
	if ($fdisk =~ /^http:.*$/ || $fdisk =~ /^ftp:.*$/) {
	    mysystem("$wget -nv -O /tmp/fdisk.table $fdisk");
	    fatal("Failed to transfer fdisk table: $fdisk!")
		if ($?);
	    
	    $fdisk = "/tmp/fdisk.table";
	}
	else {
	    $fdisk = "/${fdisk}";
	}

	#
	# We are going to feed the fdisk table one line at a time
	# so we can apply the adjustment. Yuck.
	# 
	if (! ($FDSK = mypopen("| fdisk -f - $rawbootdisk"))) {
	    fatal("Could not start fdisk on $rawbootdisk!");
	}
	foreach my $line (`cat $fdisk`) {
	    chomp($line);
	    my ($key, $part, $sysid, $start, $size) = split(' ', $line);

	    if (defined($start)) {
		$size += $adjust;
		if ($part != 1) {
		    $start += $adjust;
		}
		print $FDSK "$key $part $sysid $start $size\n";
	    }
	    else {
		print $FDSK "$key $part\n";
	    }
	}
	if (!close($FDSK)) {
	    fatal("Failed to lay down initial DOS partition table!");
	}

	#
	# Clear old disklabels to avoid errors.
	#

	#
	# Give the boot slice an auto label to prevent a bunch of warnings.
	# Its going to get overwritten anyway. 
	# 
	mysystem("disklabel -w -r ${blockdevice}s1 auto");
    }

    #
    # Turn slice2 into a filesystem we can stash the image on.
    # This slice is reserved forever! But its only 1gig.
    #
    # Make it active so that the machine will not boot without the cd.
    #
    mysystem("echo \"a 2\" | fdisk -f - $rawbootdisk");
    
    mysystem("disklabel -w -r ${blockdevice}s2 auto");
    fatal("Failed to put a disklabel on ${blockdevice}s2!")
	if ($?);
	
    mysystem("newfs /dev/${rawdevice}${tempdevice}");
    fatal("Failed to put a filesystem on /dev/${rawdevice}${tempdevice}!")
	if ($?);

    mysystem("mount /dev/${blockdevice}${tempdevice} /mnt");
    fatal("Failed to mount /dev/${blockdevice}${tempdevice}! on /mnt!")
	if ($?);
    $needumount = 1;

    #
    # Find out the actual slice sizes so that we can modify the C
    # partitions of images we lay down. Why? Cause if fdisk modified the
    # size, then the kernel is going to complain all the time that the
    # size of the slice does not match the size of the C partition.
    # 
    foreach my $line (`fdisk -s $rawbootdisk`) {
	chomp($line);
	if ($line =~ /\s*(\d):\s*\d*\s*(\d*)/) {
	    $slicesizes{$1} = $2;
	}
    }
}

#
# Now go through each possible slice except slice 2, since we use slice 2
# as temp storage when first creating the disk. The fdisk table we got from
# Emulab will have reserved that slice. 
#
for ($i = 1; $i <= 4; $i++) {
    my $ED;
    
    if ($i == 2) {
	next;
    }
    my $image = $config{"slice${i}_image"};
    my $hash  = $config{"slice${i}_md5"};
    
    if (!defined($image) || $image eq "skipit") {
	next;
    }

    #
    # If told to load a local image from the CDROM, its really easy!
    # 
    if (! ($image =~ /^http:.*$/ || $image =~ /^ftp:.*$/)) {
	#
	# First, check the hash if provided.
	#
	if (defined($hash)) {
	    print "Checking MD5 hash of the CDROM image.\n";
	    print $PATIENCEMSG;

	    my $hval = `md5 -q /$image`;
	    chomp($hval);

	    fatal("CDROM image has invalid hash!")
		if ($hash ne $hval);
	}

	#
	# Then load up the disk straight from the CDROM.
	#
	print "Loading image for slice $i from $image.\n";
	print $PATIENCEMSG;

	if ($showprogress) {
	    print "(Output indicates uncompressed bytes written to disk.)\n";
	    system("imageunzip -o -s $i /$image $rawbootdisk");
	} else {
	    mysystem("imageunzip -s $i /$image $rawbootdisk");
	}
	fatal("Failed to lay down image /$image!")
	    if ($?);
	goto finished;
    }

    #
    # Transfer the image to the temp store and then unzip to the slice.
    #
    if ($image =~ /^http:.*$/ || $image =~ /^ftp:.*$/) {
      again:
	print "Transferring image to temp storage for slice $i from:\n".
	      "    $image \n";
	print $PATIENCEMSG;
	
	if ($showprogress) {
	    system("$wget -O /mnt/image.ndz $image");
	} else {
	    mysystem("$wget -nv -O /mnt/image.ndz $image");
	}
	fatal("Failed to transfer image!")
	    if ($?);

	#
	# If an md5 hash was provided, compare it.
	#
	if (defined($hash)) {
	    print "Checking MD5 hash of the image.\n";
	    print $PATIENCEMSG;

	    my $hval = `md5 -q /mnt/image.ndz`;
	    chomp($hval);

	    if ($hash ne $hval) {
		print "*** Invalid hash! Getting the image again.\n";
		sleep(60);
		goto again;
	    }
	}

	print "Writing image from temp storage to slice $i.\n";
	print $PATIENCEMSG;

	if ($showprogress) {
	    print "(Output indicates uncompressed bytes written to disk.)\n";
	    system("imageunzip -o -s $i /mnt/image.ndz $rawbootdisk");
	} else {
	    mysystem("imageunzip -s $i /mnt/image.ndz $rawbootdisk");
	}
	fatal("Failed to lay down image!")
	    if ($?);

	unlink("/mnt/image.ndz") or
	    fatal("Failed to unlink /mnt/image.ndz!");
    }
  finished:
    #
    # Update the the label, changing the C partition to match the actual
    # slice size.
    #
    $ENV{'EDITOR'} = "ed";
    if (! ($ED = mypopen("| disklabel -e -r ${rawdevice}s${i}"))) {
	fatal("Could not edit label on ${rawdevice}s${i}!");
    }
    print $ED "/^  c: /\n";
    print $ED "t\n";
    print $ED "s/c: [0-9]*/c: $slicesizes{$i}/\n";
    print $ED "w\n";
    print $ED "q\n";
    if (!close($ED)) {
	fatal("Error editing disklabel on ${rawdevice}s${i}!");
    }
}

#
# Unmount the temp store.
#
UnMountMnt();

#
# Create extra FS.
# 
if (defined($config{"slicex_mount"}) &&
    defined($config{"slicex_slice"})) {
    my $slicex_mount = $config{"slicex_mount"};
    my $slicex_slice = $config{"slicex_slice"};
    
    if ($fromscratch) {
	#
	# Easy. Just make it and remember the device for fstab.
	#
	if (MakeFS($slicex_mount, $slicex_slice) < 0) {
	    fatal("Error creating extra filesystem");
	}
    }
    else {
	#
	# Harder. We have to trust that we are being told the truth!
	#
	$slicexdev = "s${slicex_slice}${slicexpart}";
    }
}

#
# Localize the root filesystem (might not be needed of course).
# 
LocalizeRoot();
WritePrivKey();

#
# Wow, made it. Now write the current configuration back to the magic
# header and set things up to reboot from the disk instead of the CDROM.
#
WriteConfigBlock();

#
# Let Emulab know we made it through
#
FinishedInstructions();

#
# Wrapper does the rest.
# 
exit 0;

#
# Check in with emulab for instructions.
#
sub GetInstructions()
{
    my $path  = $setupconfig;

    #
    # Talk to emulab and see what to do. We use wget to invoke a secure
    # script on the webserver, passing it the cdkey.
    # We give it our IP as an argument plus our private key.
    #
    if (! -s $path) {
	my $roottag = 0;

	if (! $fromscratch) {
	    $roottag = GetRootTag();
	}
      again:
	$path = "/tmp/emulab-config.$$";
	undef($privkey);
	
	#
	# The cdkey describes the cd.
	# 
	if (! -s $cdkeyfile) {
	    fatal("No CD key on the CD!");
	}
	$cdkey = `cat $cdkeyfile`;
	chomp($cdkey);
	
	#
	# If not from scratch, then get the current privkey.
	# If from scratch, then prompt user for the privkey, which had to
	# come from emulab beforehand.
	#
	if (! $fromscratch) {
	    $privkey = `$tbboot -e - $rawbootdisk`;
	    chomp($privkey);
	}
	else {
	    #
	    # We allow for the initial install key to be on the CDROM.
	    # Its only good for one install though! It will be rejected
	    # if its tried again. Thats not so bad though; it just means
	    # that if the disk gets totally scrogged we need to edit the
	    # DB state slightly to allow the key to be used. That would
	    # be on demand!
	    #
	    if (-s $installkey) {
		$privkey = `cat $installkey`;
		chomp($privkey);
	    }
	    else {
		while (!defined($privkey)) {
		    $privkey =
			Prompt("Please enter your 16 character CD password".
			       " (no spaces)", undef);
		}
	    }
	}

	while (1) {
	    print "Checking in at Netbed Central for configuration ...\n";
	    
	    mysystem("$wget -q -O $path ".
		     "'${WWW}/cdromcheckin.php3".
		     "?cdkey=$cdkey&IP=$IP&privkey=$privkey".
		     "&wahostname=$hostname&roottag=$roottag'");
	    if (!$?) {
		last;
	    }
	    print "Error checking in. Will try again in one minute ...\n";
	    sleep(60);
	}
	
	if (! -s $path) {
	    fatal("Could not get a valid setup configuration from $WWW!");
	}
    }

    #
    # Read the output file. It will tell us what to do.
    #
    if (! open(INSTR, $path)) {
	fatal("$path could not be opened for reading: $!");
    }
    while (<INSTR>) {
	chomp();
        SWITCH1: {
	    /^emulab_status=(.*)$/ && do {
		$config{emulab_status} = $1;
		last SWITCH1;
	    };
	    /^privkey=(.*)$/ && do {
		$config{privkey} = $1;
		last SWITCH1;
	    };
	    /^fdisk=(.*)$/ && do {
		$config{fdisk} = $1;
		last SWITCH1;
	    };
	    /^slice(\d)_image=(.*)$/ && do {
		$config{"slice${1}_image"} = $2;
		$needstore++;
		last SWITCH1;
	    };
	    /^slice(\d)_md5=(.*)$/ && do {
		$config{"slice${1}_md5"} = $2;
		last SWITCH1;
	    };
	    /^slicex_mount=(.*)$/ && do {
		$config{"slicex_mount"} = $1;
		last SWITCH1;
	    };
	    /^slicex_slice=(.*)$/ && do {
		$config{"slicex_slice"} = $1;
		last SWITCH1;
	    };
	    /^slicex_tarball=(.*)$/ && do {
		$config{"slicex_tarball"} = $1;
		last SWITCH1;
	    };
	    /^slicex_md5=(.*)$/ && do {
		$config{"slicex_md5"} = $1;
		last SWITCH1;
	    };
	    print STDERR "Invalid instruction: $_\n";
	}
    }
    close(INSTR);

    if (defined($config{"emulab_status"}) &&
	$config{"emulab_status"} != $CDROMSTATUS_OKAY) {

	my $err = $config{"emulab_status"};
	my $msg = $weberrors{$err};

	if ($err == $CDROMSTATUS_BADPRIVKEY &&
	    Prompt("Invalid CD password. Try again?", "Yes") =~ /yes/i) {
	    delete($config{"emulab_status"});
	    goto again;
	}
	fatal("Netbed Checkin Error: '$msg'");
    }
    return 0;
}

#
# Tell Emulab we finished its instruction.
#
sub FinishedInstructions()
{
    $path = "/tmp/emulab-finished.$$";
    
    #
    # No need to do this when operating off local instructions.
    # 
    if (-s $setupconfig) {
	return 0;
    }

  again:
    while (1) {
	print "Letting Netbed Central know we finished ...\n";
	    
	mysystem("$wget -q -O $path ".
		 "'${WWW}/cdromcheckin.php3".
		 "?cdkey=$cdkey&IP=$IP&privkey=$privkey".
		 "&wahostname=$hostname&updated=1'");
	if (!$?) {
	    last;
	}
	print "Error checking in. Will try again in one minute ...\n";
	sleep(60);
    }
    if (! -s $path) {
	fatal("Could not update Netbed Central with status!");
    }

    #
    # Read the output file to see if there was an error.
    #
    if (! open(INSTR, $path)) {
	fatal("$path could not be opened for reading: $!");
    }
    while (<INSTR>) {
	chomp();
        SWITCH1: {
	    /^emulab_status=(.*)$/ && do {
		$config{emulab_status} = $1;
		last SWITCH1;
	    };
	    fatal("Invalid Response: $_");
	}
    }
    close(INSTR);

    if ($config{"emulab_status"} != $CDROMSTATUS_OKAY) {
	my $status = $config{"emulab_status"};
	my $msg    = $weberrors{$status};

	fatal("Netbed Update Error: '$msg'");
    }
    return 0;
}

#
# Create a filesystem on the last unused slice. 
#
sub MakeFS($$)
{
    my ($mntpoint, $slice) = @_;
    my $ED;
    my $foo;

    print "Looking for a DOS partition that can be resized ...\n";

    my $pline = `growdisk -f $rawbootdisk`;
    if ($?) {
	print STDERR "*** Oops, no extra DOS slices to use!\n";
	return 1;
    }
    chomp($pline);
    my (undef, $growslice, $type, $start, $size) = split(/\s/, $pline);

    if ($growslice != $slice) {
	print STDERR
	    "*** Oops, extra DOS slice mismatch ($slice,$growslice)!\n";
	return 1;
    }

    mysystem("echo \"$pline\" | fdisk -f - $rawbootdisk");
    if ($?) {
	print STDERR "*** Oops, could not setup extra DOS slice $slice!\n";
	return -1;
    }

    mysystem("disklabel -w -r ${rawdevice}s${slice} auto");
    if ($?) {
	print STDERR "*** Oops, could not disklabel ${rawdevice}s${slice}!\n";
	return -1;
    }

    # Make sure the kernel really has the new label!
    mysystem("disklabel -r ${rawdevice}s${slice} > /tmp/disklabel");
    mysystem("disklabel -R -r ${rawdevice}s${slice} /tmp/disklabel");

    $ENV{'EDITOR'} = "ed";
    if (! ($ED = mypopen("| disklabel -e -r ${rawdevice}s${slice}"))) {
	print STDERR
	    "*** Oops, could not edit label on ${rawdevice}s${slice}!\n";
	return -1;
    }
    print $ED "/^  c: /\n";
    print $ED "t\n";
    print $ED "s/c: /${slicexpart}: /\n";
    print $ED "w\n";
    print $ED "q\n";
    if (!close($ED)) {
	print STDERR
	    "*** Oops, error editing label on ${rawdevice}s${slice}!\n";
	return -1;
    }

    print "Creating filesystem on $mntpoint (${rawdevice}s${slice}e).\n";
    print "    (ignore kernel generated \"no disk label\" messages.)\n";
    mysystem("newfs -U ${rawdevice}s${slice}e");
    if ($?) {
	print STDERR "*** Oops, could not newfs ${rawdevice}s${slice}e!\n";
	return -1;
    }
    $foo = "s${slice}${slicexpart}";
	
    #
    # If there was a tarball then mount up the filesystem and unpack it.
    # We have to drag it across, and I assume that the FS is big enough
    # to hold the tarball and the unpacked contents.
    #
    if ($config{"slicex_tarball"}) {
	my $tarball = $config{"slicex_tarball"};
	my $tarhash = $config{"slicex_md5"};
	    
	mysystem("mount /dev/${blockdevice}${foo} /mnt");
	fatal("Failed to mount /dev/${blockdevice}${foo} on /mnt!")
	    if ($?);
	$needumount = 1;

	chdir("/mnt") or
	    fatal("Could not chdir to /mnt!");

	#
	# If told to use a local image from the CDROM, its really easy!
	# 
	if (! ($tarball =~ /^http:.*$/ || $tarball =~ /^ftp:.*$/)) {
	    #
	    # First, check the hash if provided.
	    #
	    if (defined($tarhash)) {
		print "Checking MD5 hash of the tarball.\n";
		print $PATIENCEMSG;

		my $hval = `md5 -q /$tarball`;
		chomp($hval);

		fatal("Tarball has invalid hash!")
		    if ($tarhash ne $hval);
	    }

	    print "Unpacking $tarball to $mntpoint.\n";
	    print $PATIENCEMSG;
	
	    mysystem("tar -zxf /$tarball");
	    fatal("Failed to untar /$tarball!")
		if ($?);
	}
	else {
	  again:
	    #
	    # Transfer the image to the temp store and then unzip to the slice.
	    #
	    print "Transferring tarball for $mntpoint from:\n".
		  "    $tarball\n";
	    print $PATIENCEMSG;
	
	    mysystem("$wget -nv -O /mnt/slicex.tar.gz $tarball");
	    fatal("Failed to transfer tarball!")
		if ($?);

	    #
	    # If an md5 hash was provided, compare it.
	    #
	    if (defined($tarhash)) {
		print "Checking MD5 hash of the tarball.\n";
		print $PATIENCEMSG;

		my $hval = `md5 -q /mnt/slicex.tar.gz`;
		chomp($hval);

		if ($tarhash ne $hval) {
		    print "*** Invalid hash! Getting the tarball again.\n";
		    sleep(60);
		    goto again;
		}
	    }

	    print "Unpacking tarball.\n";
	    print $PATIENCEMSG;

	    mysystem("tar -zxf /mnt/slicex.tar.gz");
	    fatal("Failed to untar /mnt/slicex.tar.gz!")
		if ($?);
	}
	
	chdir("/") or
	    fatal("Could not chdir to /!");
	
	mysystem("umount /mnt");
	fatal("Failed to unmount /mnt!")
	    if ($?);
	$needumount = 0;
    }
    
    $slicexdev = $foo;
    return 0;
}

#
# Localize the root filesystem by putting out a proper rc.conf.local,
# resolv.conf, fstab, etc.
#
sub LocalizeRoot()
{
    my $root = "${blockdevice}${rootdevice}";
    my $ED;
	
    if (! defined($config{"slice1_image"})) {
	return 0;
    }

    print "Localizing root filesystem on $root ...\n";
    MountRoot();

    #
    # Copy over the easy ones.
    #
    mysystem("cp -p /etc/rc.conf.local /mnt/etc");
    fatal("Failed to copy /etc/rc.conf.local to /mnt/etc!")
	if ($?);
    mysystem("cp -p /etc/resolv.conf /mnt/etc");
    fatal("Failed to copy /etc/resolv.conf to /mnt/etc!")
	if ($?);
    mysystem("cp -p /etc/namedb/localhost.rev /mnt/etc/namedb");
    fatal("Failed to copy /etc/namedb/localhost.rev to /mnt/etc!")
	if ($?);

    #
    # Write MD5 to toplevel. Serves as poor version info.
    #
    my $hash = $config{"slice1_md5"};
    mysystem("echo '$hash' > /mnt/$slicetag");
    fatal("Failed to create /mnt/$slicetag!")
	if ($?);

    #
    # Edit fstab, replacing the s1 entries with the current device.
    #
    chmod(0644, "/mnt/etc/fstab");
    
    if (! ($ED = mypopen("| ed /mnt/etc/fstab "))) {
	fatal("Could not edit /mnt/etc/fstab!");
    }
#   print $ED "1,\$s;/dev/.*\\(s1[a-z]*\\);/dev/${blockdevice}\\1;p\n";
    print $ED "1,\$s;/dev/[a-z]*[0-9]\\(.*[a-z]\\);/dev/${blockdevice}\\1;p\n";

    #
    # Append a mountpoint for the extra fs.
    #
    if (defined($config{"slicex_mount"}) && defined($slicexdev)) {
	my $slicex_mount = $config{"slicex_mount"};

	print $ED "\$\n";
	print $ED "a\n";
	print $ED "/dev/${blockdevice}${slicexdev}\t\t$slicex_mount\t\t".
	    "ufs\trw,nosuid\t0\t2\n";
	print $ED ".\n";
    }
    
    print $ED "w\n";
    print $ED "q\n";
    if (!close($ED)) {
	fatal("Error editing /mnt/etc/fstab!");
    }

    chdir("/mnt/dev") or
	fatal("Could not chdir to /mnt/dev!");
    mysystem("./MAKEDEV ${blockdevice}");
    mysystem("./MAKEDEV ${blockdevice}s1c ${blockdevice}s2c");
    mysystem("./MAKEDEV ${blockdevice}s3c ${blockdevice}s4c");
    chdir("/") or
	fatal("Could not chdir to /!");
    
    mysystem("umount /mnt");
    $needumount = 0;

    return 0;
}

#
# Write out the privkey
#
sub WritePrivKey()
{
    if (! $needumount) {
	MountRoot();
    }
    
    if (defined($config{"privkey"})) {
	my $key = $config{"privkey"};
	
	mysystem("echo \"$key\" > /mnt/${privkeyfile}");
	fatal("Could not create /mnt/${privkeyfile}")
	    if ($?);
	chmod(0640, "/mnt/${privkeyfile}") or
	    fatal("Could not chmod 0640 /mnt/${privkeyfile}");
    }
    UnMountMnt();
    
    return 0;
}

#
# Mount up the root partition.
#
sub MountRoot()
{
    my $root  = "${blockdevice}${rootdevice}";
    my $rroot = "${rawdevice}${rootdevice}";

    #
    # Need to fsck it. If its dirty, the mount will fail and we will
    # be hosed. If it fails fsck, well we are hosed anyway!
    #
    mysystem("fsck -y /dev/${rroot}");
    fatal("Could not fsck root filesystem (/dev/${rroot}!")
	if ($?);

    mysystem("mount /dev/${root} /mnt");
    fatal("Failed to mount /dev/${root}! on /mnt!")
	if ($?);
    $needumount = 1;

    return 0;
}

#
# Unmount the mounted filesystem.
#
sub UnMountMnt()
{
    if ($needumount) {
	mysystem("umount /mnt");
	$needumount = 0;
	
	fatal("Failed to umount /mnt!")
	    if ($?);
    }
    return 0;
}

#
# When running from the CDROM write the config info to the magic spot.
# 
sub WriteConfigBlock()
{
    my $cmd;

    $cmd = "$tbboot -f -d -w $softconfig -k 1 -c 0 ";

    if (defined($config{privkey})) {
	$cmd .= "-e $config{privkey} ";
    }
    $cmd .= "  $rawbootdisk";

    mysystem("$cmd");
    fatal("Failed to write header block info!")
	if ($?);

    return 0;
}

#
# Get the tag for the root slice. 
# 
sub GetRootTag()
{
    MountRoot();

    if (! -e "/mnt/$slicetag") {
	UnMountMnt();
	return 0;
    }

    my $tag = `cat /mnt/$slicetag`;
    chomp($tag);

    UnMountMnt();
    return $tag;
}

#
# Print error and exit.
#
sub fatal($)
{
    my ($msg) = @_;

    if ($needumount) {
	chdir("/");
	mysystem("umount /mnt");
    }
    
    die("*** $0:\n".
	"    $msg\n");
}

#
# Run a command string, redirecting output to a logfile.
#
sub mysystem($)
{
    my ($command) = @_;

    if (defined($logfile)) {
	system("echo \"$command\" >> $logfile");
	system("($command) >> $logfile 2>&1");
    }
    else {
	print "Command: '$command\'\n";
	system($command);
    }
    return $?;
}

sub mypopen($)
{
    my ($command) = @_;
    local *FD;

    if (defined($logfile)) {
	system("echo \"$command\" >> $logfile");
	open(FD, "$command >> $logfile 2>&1")
	    or return undef;
    }
    else {
	print "Command: '$command\'\n";
	open(FD, "$command")
	    or return undef;
    }
    return *FD;
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

