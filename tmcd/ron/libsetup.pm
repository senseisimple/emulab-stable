#!/usr/bin/perl -wT

#
# XXX: The user homedir from boss is ignored; whatever local default is. 
#      Not removing homedirs of deleted accounts.
#      Not overwriting existing ssh authorized_keys with new key.
#      Bossinfo comes from /usr/local/etc/testbed/bossnod.
#      Remote nodes use sshremote (instead of sshtb).
#      A node is remote if its class is pcremote (libdb.pm).
#      sshremote requires "emulabman" user on remote node with root key.
#
# On eureka:
#      Install /usr/local/etc/testbed dir as root.
#      Create /etc/pw.conf pointing default homedir to /z.
#      Need to allow setui perl scripts (chown/chmod /usr/bin/suidperl).
#
 
#
# Common routines and constants for the client bootime setup stuff.
#
package libsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( libsetup_init doaccounts nodeupdate TBBackGround
       );

# Must come after package declaration!
use English;

#
# This is the home of the setup library on the client machine. The including
# program has to tell us this by calling the init routine below. 
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
# These are the paths of various files and scripts that are part of the
# setup library.
#
sub TMCC()		{ "$SETUPDIR/tmcc"; }
sub TMPASSDB()		{ "$SETUPDIR/passdb"; }
sub TMGROUPDB()		{ "$SETUPDIR/groupdb"; }

#
# This is the VERSION. We send it through to tmcd so it knows what version
# responses this file is expecting.
#
# BE SURE TO BUMP THIS AS INCOMPATIBILE CHANGES TO TMCD ARE MADE!
#
sub TMCD_VERSION()	{ 4; };

#
# These are the TMCC commands.
#
sub TMCCCMD_ACCT()	{ "accounts"; }

#
# This is a debugging thing for my home network.
# 
#my $NODE	= "REDIRECT=155.101.132.101";
$NODE		= "";

#
# Open a TMCC connection and return the "stream pointer". Caller is
# responsible for closing the stream and checking return value.
#
# usage: OPENTMCC(char *command, char *args)
#
sub OPENTMCC($;$)
{
    my($cmd, $args) = @_;
    local *TM;

    if (!defined($args)) {
	$args = "";
    }
    my $foo = sprintf("%s -v %d %s %s %s |",
		      TMCC, TMCD_VERSION, $NODE, $cmd, $args);

    open(TM, $foo)
	or die "Cannot start $TMCC: $!";

    return (*TM);
}

#
# Run a TMCC command with the provided arguments.
#
# usage: RUNTMCC(char *command, char *args)
#
sub RUNTMCC($;$)
{
    my($cmd, $args) = @_;
    my($TM);

    if (!defined($args)) {
	$args = "";
    }
    $TM = OPENTMCC($cmd, $args);

    close($TM)
	or die $? ? "TMCC exited with status $?" : "Error closing pipe: $!";
    
    return 0;
}

sub doaccounts ()
{
    my %newaccounts = ();
    my %newgroups   = ();
    my %deletes     = ();
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
	    $newgroups{"emu-$1"} = $2
	}
	elsif ($_ =~ /^ADDUSER LOGIN=([0-9a-z]+)/) {
	    #
	    # Account info goes in the hash table.
	    # 
	    $newaccounts{$1} = $_;
	    next;
	}
	else {
	    warn "*** WARNING: Bad accounts line: $_\n";
	}
    }
    close($TM);

    dbmopen(%PWDDB, TMPASSDB, 0660) or
	die("Cannot open " . TMPASSDB . ": $!\n");
	
    dbmopen(%GRPDB, TMGROUPDB, 0660) or
	die("Cannot open " . TMGROUPDB . ": $!\n");

    #
    # First create any groups that do not currently exist. Add each to the
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
    dbmclose(%GRPDB);

    #
    # Repeat the same sequence for accounts, except we remove old accounts
    # first. 
    # 
    while (($login, $uid) = each %PWDDB) {
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
	# Must ask for the current home dir since we rely on pw.conf.
	#
	if (defined($homedir) &&
	    index($homedir, "/${login}")) {
	    print "Removing home directory: $homedir\n";
	    if (system("rm -rf $homedir")) {
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
    $pat   .= q(EMULABPUBKEY="(.*)" HOMEPUBKEY="(.*)");

    while (($login, $info) = each %newaccounts) {
	if ($info =~ /$pat/) {
	    $pswd  = $2;
	    $uid   = $3;
	    $gid   = $4;
	    $root  = $5;
	    $name  = $6;
	    $hdir  = $7;
	    $glist = $8;
	    $ekey  = $9;
	    $hkey  = $10;
	    if ( $name =~ /^(([^:]+$|^))$/ ) {
		$name = $1;
	    }
	    print "User: $login/$uid/$gid/$root/$name/$hdir/$glist\n";

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
		print "Updating $login login info.\n";
		os_usermod($login, $gid, "$glist", $root);
		next;
	    }
	    print "Adding $login account.\n";

	    if (os_useradd($login, $uid, $gid, $pswd, 
			   "$glist", $hdir, $name, $root)) {
		warn "*** WARNING: Error adding new user $login\n";
		next;
	    }
	    # Add to DB only if successful. 
	    $PWDDB{$login} = $uid;

	    #
	    # Create .ssh dir and populate it with an authkeys file.
	    # Must ask for the current home dir since we rely on pw.conf.
	    #
	    my (undef,undef,undef,undef,
		undef,undef,undef,$homedir) = getpwuid($uid);
	    my $sshdir = "$homedir/.ssh";
	    
	    if (! -e $sshdir && ($ekey ne "" || $hkey ne "")) {
		if (! mkdir($sshdir, 0700)) {
		    warn("*** WARNING: Could not mkdir $sshdir: $!\n");
		    next;
		}
		if (!chown($uid, $gid, $sshdir)) {
		    warn("*** WARNING: Could not chown $sshdir: $!\n");
		    next;
		}
		if (!open(AUTHKEYS, "> $sshdir/authorized_keys")) {
		    warn("*** WARNING: Could not open $sshdir/keys: $!\n");
		    next;
		}
		if ($ekey ne "") {
		    print AUTHKEYS "$ekey\n";
		}
		if ($hkey ne "") {
		    print AUTHKEYS "$hkey\n";
		}
		close(AUTHKEYS);

		if (!chown($uid, $gid, "$sshdir/authorized_keys")) {
		    warn("*** WARNING: Could not chown $sshdir/keys: $!\n");
		    next;
		}
		if (!chmod(0600, "$sshdir/authorized_keys")) {
		    warn("*** WARNING: Could not chmod $sshdir/keys: $!\n");
		    next;
		}
	    }
	}
	else {
	    warn("*** Bad accounts line: $info\n");
	}
    }
    # Write the DB back out!
    dbmclose(%PWDDB);

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
    # Do account stuff.
    # 
    print STDOUT "Checking Testbed group/user configuration ... \n";
    doaccounts();

    return 0;
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

1;

