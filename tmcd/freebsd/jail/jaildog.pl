#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
use Getopt::Std;
use English;
use Errno;
use POSIX qw(strftime);

#
# Watchdog for inside a jail. All this does is look for account updates.
# Might do more later.
#
sub usage()
{
    print "Usage: jaildog [-t timeout]\n";
    exit(1);
}
my $optlist = "t:";

#
# Turn off line buffering on output
#
$| = 1;

#
# Untaint path
#
$ENV{'PATH'} = "/bin:/sbin:/usr/bin:/usr/local/bin:/usr/local/sbin";
delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};

#
# Must be root to run this.
# 
if ($UID != 0) {
    die("*** $0:\n".
	"    Must be root to run this script!\n");
}

#
# Load the OS independent support library. It will load the OS dependent
# library and initialize itself. 
#
if (-d "/usr/local/etc/emulab") {
    use lib "/usr/local/etc/emulab";
    $ENV{'PATH'} .= ":/usr/local/etc/emulab";
}
elsif (-d "/etc/testbed") {
    use lib "/etc/testbed";
    $ENV{'PATH'} .= ":/etc/testbed";
}
use libsetup;

# Locals
my $timeout = (60 * 60 * 12);	# In seconds of course. 
my $logname = "/var/tmp/emulab-jaildog.debug";
my $pidfile = "/var/run/emulab-jaildog.pid";
my $vnodeid;

#
# Parse command arguments. Once we return from getopts, all that should be
# left are the required arguments.
#
%options = ();
if (! getopts($optlist, \%options)) {
    usage();
}
if (defined($options{"t"})) {
    $timeout = $options{"t"};
}
if (@ARGV) {
    usage();
}

#
# Put this into the background and log its output. We *must* do this cause
# we do not want to halt the boot if the testbed is down!
# 
if (1 && TBBackGround($logname)) {
    #
    # Parent exits normally
    #
    exit(0);
}

#
# Setup a handler to catch TERM, and kill our process group.
#
my $pgrp = getpgrp(0);

sub handler () {
    $SIG{TERM} = 'IGNORE';
    $SIG{INT} = 'IGNORE';
    kill('TERM', -$pgrp);
    sleep(5);
    exit(0);
}
$SIG{TERM} = \&handler;
$SIG{INT}  = \&handler;

#
# Write our pid into the pid file so we can be killed later (when the
# experiment is torn down). We must do this first so that we can be
# killed before we change the sig handlers
#
open(PFILE, "> $pidfile")
    or die("Could not open $pidfile: $!");
print PFILE "$PID\n";
close(PFILE);

print "Getting our Emulab configuration ...\n";
if (! ($vnodeid = jailedsetup())) {
    die("*** $0:\n".
	"    Did not get our jailname!\n");
}

if (-x TMTARBALLS()) {
    print "Installing Tarballs ...\n";
    system(TMTARBALLS());
}

if (-x TMSTARTUPCMD()) {
    print "Running startup command ...\n";
    system("runstartup");
}

#
# Inform TMCD that we are up and running.
#
print "Informing Emulab Operations that we're up and running ...\n";
system("tmcc state ISUP");

#
# Fire off a child that does nothing but tell the boss we are alive.
#
my $mypid = fork();
if (! $mypid) {
    my $failed  = 0;
    
    print "Keep alive starting up ... \n";

    while (1) {
	#
	# Run tmcc in UDP mode. The command is ignored at the other end.
	# Its just the connection that tells tmcd we are alive.
	# Since its UDP, we try it a couple of times if it fails. 
	#
	my $retries = 3;

	while ($retries) {
#	    my $options = "-p 7778 REDIRECT=192.168.100.1";
	    my $options = "";
	    if (REMOTE()) {
		$options .= " -u -t 3";
	    }
	    if (defined($vnodeid)) {
		$options .= " -n $vnodeid";
	    }
	    my $result = `tmcc $options isalive`;
	    if (! $?) {
		my $date = POSIX::strftime("20%y/%m/%d %H:%M:%S", localtime());
		
		chomp $result;
		my (undef,$update) = split("=", $result);
		if ($update || $failed) {
		    print "Running an update at $date ...\n";
		    system("update -i");
		    $failed = $?;
		}
		last;
	    }
	    $retries--;
	}
	if (!$retries) {
	    print "keep alive returned $?\n";
	}
	sleep(60);
    }
    exit(0);
}

#
# Loop!
# 
while (1) {
    sleep($timeout);
    
    my $date = POSIX::strftime("20%y/%m/%d %H:%M:%S", localtime());

    print "Dogging it at $date\n";
    
    #
    # Run account update. Use immediate mode so that it exits right away
    # if the lock is taken (another update already running).
    #
    print "Looking for new Emulab accounts ...\n";
    system("update -i");
}

exit(0);
