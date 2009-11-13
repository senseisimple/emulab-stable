#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#

#
# This is a stub library to provide a few things that libtestbed on
# boss provides.
#
package libtestbed;
use Exporter;
@ISA    = "Exporter";
@EXPORT = qw( TB_BOSSNODE TB_EVENTSERVER
	      TBScriptLock TBScriptUnlock 
	      TBSCRIPTLOCK_OKAY TBSCRIPTLOCK_TIMEDOUT
	      TBSCRIPTLOCK_IGNORE TBSCRIPTLOCK_FAILED TBSCRIPTLOCK_GLOBALWAIT
	    );

# Must come after package declaration!
use English;
# For locking below
use Fcntl ':flock';

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

# Need this.
use libtmcc;

#
# Return name of the bossnode.
#
sub TB_BOSSNODE()
{
    return tmccbossname();
}

#
# Return name of the event server.
#
sub TB_EVENTSERVER()
{
    # duplicate behavior of tmcc bossinfo function
    my @searchdirs = ( "/etc/testbed","/etc/emulab","/etc/rc.d/testbed",
		       "/usr/local/etc/testbed","/usr/local/etc/emulab" );
    my $bossnode = TB_BOSSNODE();
    my $eventserver = '';

    foreach my $d (@searchdirs) {
	if (-e "$d/eventserver" && !(-z "$d/eventserver")) {
	    $eventserver = `cat $d/eventserver`;
	    last;
	}
    }
    if ($eventserver eq '') {
	my @ds = split(/\./,$bossnode,2);
	if (scalar(@ds) == 2) {
	    # XXX event-server hardcode
	    $eventserver = "event-server.$ds[1]";
	}
    }
    if ($eventserver eq '') {
	$eventserver = "event-server";
    }

    return $eventserver;
}

#
# Serialize an operation (script).
#
my $lockname;
my $lockhandle;

# Return Values.
sub TBSCRIPTLOCK_OKAY()		{ 0;  }
sub TBSCRIPTLOCK_TIMEDOUT()	{ 1;  }
sub TBSCRIPTLOCK_IGNORE()	{ 2;  }
sub TBSCRIPTLOCK_FAILED()	{ -1; }
sub TBSCRIPTLOCK_GLOBALWAIT()	{ 1; }

# 
# There are two kinds of serialization.
#
#   * Usual Kind: Each party just waits for a chance to go.
#   * Other Kind: Only the first party really needs to run; the others just
#                 need to wait. For example; exports_setup operates globally,
#                 so there is no reason to run it more then once. We just
#                 need to make sure that everyone waits for the one that is
#		  running to finish. Use the global option for this.
#
sub TBScriptLock($;$$$)
{
    my ($token, $global, $waittime, $lockhandle_ref) = @_;
    local *LOCK;

    if (!defined($waittime)) {
	$waittime = 30;
    }
    elsif ($waittime == 0) {
	$waittime = 99999999;
    }
    $global = 0
	if (defined($global) || $global != TBSCRIPTLOCK_GLOBALWAIT());
    $lockname = "/var/tmp/testbed_${token}_lockfile";

    my $oldmask = umask(0000);

    if (! open(LOCK, ">>$lockname")) {
	print STDERR "Could not open $lockname!\n";
	umask($oldmask);
	return TBSCRIPTLOCK_FAILED();
    }
    umask($oldmask);

    if (! $global) {
	#
	# A plain old serial lock.
	#
	my $tries = 0;
	while (flock(LOCK, LOCK_EX|LOCK_NB) == 0) {
	    print "Another $token is in progress (${tries}s). Waiting ...\n"
		if (($tries++ % 60) == 0);

	    $waittime--;
	    if ($waittime == 0) {
		print STDERR "Could not get the lock after a long time!\n";
		return TBSCRIPTLOCK_TIMEDOUT();
	    }
	    sleep(1);
	}
	# Okay, got the lock. Save the handle. We need it below.
	if (defined($lockhandle_ref)) {
	    $$lockhandle_ref = *LOCK;
	}
	else {
	    $lockhandle = *LOCK;
	}
	return TBSCRIPTLOCK_OKAY();
    }

    #
    # Okay, a global lock.
    #
    # If we don't get it the first time, we wait for:
    # 1) The lock to become free, in which case we do our thing
    # 2) The time on the lock to change, in which case we wait for that 
    #    process to finish, and then we are done since there is no
    #    reason to duplicate what the just finished process did.
    #
    if (flock(LOCK, LOCK_EX|LOCK_NB) == 0) {
	my $oldlocktime = (stat(LOCK))[9];
	my $gotlock = 0;
	
	while (1) {
	    print "Another $token in progress. Waiting a moment ...\n";
	    
	    if (flock(LOCK, LOCK_EX|LOCK_NB) != 0) {
		# OK, got the lock
		$gotlock = 1;
		last;
	    }
	    my $locktime = (stat(LOCK))[9];
	    if ($locktime != $oldlocktime) {
		$oldlocktime = $locktime;
		last;
	    }
	    
	    $waittime--;
	    if ($waittime <= 0) {
		print STDERR "Could not get the lock after a long time!\n";
		return TBSCRIPTLOCK_TIMEDOUT();
	    }
	    sleep(1);
	}

	$count = 0;
	#
	# If we did not get the lock, wait for the process that did to finish.
	#
	if (!$gotlock) {
	    while (1) {
		if ((stat(LOCK))[9] != $oldlocktime) {
		    return TBSCRIPTLOCK_IGNORE();
		}
		if (flock(LOCK, LOCK_EX|LOCK_NB) != 0) {
		    close(LOCK);
		    return TBSCRIPTLOCK_IGNORE();
		}

		$waittime--;
		if ($waittime <= 0) {
		    print STDERR
			"Process with the lock did not finish after ".
			"a long time!\n";
		    return TBSCRIPTLOCK_TIMEDOUT();
		}
		sleep(1); 
	    }
	}
    }
    #
    # Perl-style touch(1)
    #
    my $now = time;
    utime $now, $now, $lockname;
    
    if (defined($lockhandle_ref)) {
	$$lockhandle_ref = *LOCK;
    }
    else {
	$lockhandle = *LOCK;
    }
    return TBSCRIPTLOCK_OKAY();
}

#
# Unlock; Just need to close the file (releasing the lock).
#
sub TBScriptUnlock(;$)
{
    my ($lockhandle_arg) = @_;
    if (defined($lockhandle_arg)) {
	close($lockhandle_arg);
    }
    else {
	close($lockhandle)
	    if defined($lockhandle);
    }
}

1;

