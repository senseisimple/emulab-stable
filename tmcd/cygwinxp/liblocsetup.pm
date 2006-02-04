#!/usr/bin/perl -wT
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#

#
# CygWin specific routines and constants for the client bootime setup stuff.
#
package liblocsetup;
use Exporter;
@ISA = "Exporter";
@EXPORT =
    qw ( $CP $LN $RM $MV $TOUCH $EGREP $CHOWN $CHMOD $MOUNT $UMOUNT
	 $NTS $NET $HOSTSFILE
	 $TMPASSWD $SFSSD $SFSCD $RPMCMD
	 os_account_cleanup os_accounts_start os_accounts_end os_accounts_sync
	 os_ifconfig_line os_etchosts_line
	 os_setup os_groupadd os_groupgid os_useradd os_userdel os_usermod os_mkdir
	 os_ifconfig_veth
	 os_routing_enable_forward os_routing_enable_gated
	 os_routing_add_manual os_routing_del_manual os_homedirdel
	 os_groupdel os_samba_mount 
	 os_getnfsmounts os_getnfsmountpoints os_noisycmd
	 os_fwconfig_line os_fwrouteconfig_line
       );

# Must come after package declaration!
use English;

# Load up the paths. Its conditionalized to be compatabile with older images.
# Note this file has probably already been loaded by the caller.
BEGIN
{
    if (-e "/etc/emulab/paths.pm") {
	require "/etc/emulab/paths.pm";
	import emulabpaths;
    }
    else {
	my $ETCDIR  = "/etc/rc.d/testbed";
	my $BINDIR  = "/etc/rc.d/testbed";
	my $VARDIR  = "/etc/rc.d/testbed";
	my $BOOTDIR = "/etc/rc.d/testbed";
    }
}

use librc;

#
# Various programs and things specific to CygWin on XP and that we want to export.
# 
$CP		= "/bin/cp";
$LN		= "/bin/ln";
$RM		= "/bin/rm";
$MV		= "/bin/mv";
$TOUCH		= "/bin/touch";
$EGREP		= "/bin/egrep -q";
$CHOWN		= "/bin/chown";
$CHMOD		= "/bin/chmod";
$MOUNT		= "/bin/mount";
$UMOUNT		= "/bin/umount";

# Cygwin.
$MKPASSWD	= "/bin/mkpasswd";
$MKGROUP	= "/bin/mkgroup";
$AWK		= "/bin/gawk";
$BASH		= "/bin/bash";

# Windows under Cygwin.
$NTS		= "/cygdrive/c/WINDOWS/system32";
$NET		= "$NTS/net";
$NETSH		= "$NTS/netsh";
$NTE		= "$NTS/drivers/etc";

$HOSTSFILE	= "$NTE/hosts";
#$HOSTSFILE	= "/etc/hosts";

#
# These are not exported
#
my $ADDUSERS	= "$BINDIR/addusers.exe";
my $DEVCON	= "$BINDIR/devcon.exe";
my $IFCONFIGBIN = "$NETSH interface ip set address";
my $IFCONFIG	= "$IFCONFIGBIN name=\"%s\" source=static addr=%s mask=%s";
my $IFC_1000MBS = "1000baseTx";
my $IFC_100MBS  = "100baseTx";
my $IFC_10MBS   = "10baseT";
my $IFC_FDUPLEX = "FD";
my $IFC_HDUPLEX = "HD";
my @LOCKFILES   = ("/etc/group.lock", "/etc/gshadow.lock");
my $MKDIR	= "/bin/mkdir";
my $RMDIR	= "/bin/rmdir";
my $GATED	= "/usr/sbin/gated";
my $ROUTE	= "$NTS/route";
my $SHELLS	= "/etc/shells";
my $DEFSHELL	= "/bin/tcsh";
my $winusersfile = "$BOOTDIR/winusers";
my $usershellsfile = "$BOOTDIR/usershells";
my $XIMAP	= "$BOOTDIR/xif_map";

#
# system() with error checking.
#
sub mysystem($)
{
    my ($cmd) = @_;
    if (system($cmd) != 0) {
	warning("Failed: ($cmd), $!\n");
    }
}

#
# OS dependent part of cleanup node state.
# 
sub os_account_cleanup()
{
    # Undo what rc.mounts and rc.accounts did.  

    # The users list could be gotten from multiple places; let's use the /users
    # directory as truth.
    if (opendir(DIRHANDLE, "/users")) {
	while ($name = readdir(DIRHANDLE)) {
	    if ($name =~ m/^\.+/ || $name =~ m/^Administrator$/) {
		next;
	    }
	    print "Removing user: $name\n";

	    # There is always an NT account.
	    mysystem("$NET user $name /delete >& /dev/null");

	    # There will only be an NT homedir if the user has logged in sometime.
	    system("$CHMOD -Rf 777 C:/'Documents and Settings'/$name >& /dev/null");
	    system("$CHOWN -Rf root C:/'Documents and Settings'/$name >& /dev/null");
	    system("$RM -rf C:/'Documents and Settings'/$name ");
	    # It sometimes also makes user.PCnnn, user.PCnnn.000, etc.
	    system("$CHMOD -Rf 777 C:/'Documents and Settings'/$name.* >& /dev/null");
	    system("$CHOWN -Rf root C:/'Documents and Settings'/$name.* >& /dev/null");
	    system("$RM -rf C:/'Documents and Settings'/$name.*");

	    # Unmount the homedir so we can get to the mount point.
	    system("$UMOUNT /users/$name");
	    system("$RMDIR /users/$name");
	}
	closedir(DIRHANDLE);

	# Make the CygWin /etc/passwd and /etc/group files match Windows.
	os_accounts_sync();
    }

    # Clean out the user /sshkeys directories, leaving /sshkeys/root alone.
    if (opendir(DIRHANDLE, "/sshkeys")) {
	while ($name = readdir(DIRHANDLE)) {
	    if ($name =~ m/^\.+/ || $name =~ m/^root$/) {
		next;
	    }

	    # Open up an existing key dir to the root user.  Even though root
	    # is in the Administrators group, it's locked out by permissions.
	    mysystem("$CHMOD 777 /sshkeys/$name");
	    mysystem("$CHOWN -Rf root /sshkeys/$name");
	    mysystem("$RM -rf /sshkeys/$name");
	}
	closedir(DIRHANDLE);
    }

    # Clean out the /proj subdirectories.
    if (opendir(DIRHANDLE, "/proj")) {
	while ($name = readdir(DIRHANDLE)) {
	    if ($name =~ m/^\.+/) {
		next;
	    }
	    print "Removing project: $name\n";

	    # Unmount the project dir so we can get to the mount point.
	    mysystem("$UMOUNT /proj/$name");
	    mysystem("$RMDIR /proj/$name");
	}
    }

    # Just unmount /share, everybody gets one.
    mysystem("$UMOUNT /share");
}

# 
# Make the CygWin /etc/passwd and /etc/group files match Windows.
# 
sub os_accounts_sync()
{
    unlink @LOCKFILES;

    # Generate the CygWin password and group files from the registry users.
    # Note that the group membership is not reported into the CygWin files.
    print "Resetting the CygWin passwd and group files.\n";

    my $cmd = "$MKPASSWD -l | $AWK -F: '";
    $cmd   .=   'BEGIN{ OFS=":" } ';
    # Make root's UID zero.  Put non-root homedirs under /users, not /home.
    $cmd   .=   '{ if ($1=="root") $3="0"; else sub("/home/", "/users/"); print }'; 
    $cmd   .= "'";
    # Apply the users' shell preferences.
    $cmd   .= " | sed -f $usershellsfile"
	if (-e $usershellsfile);
    $cmd   .= " > /etc/passwd.new";
    ##print "$cmd\n";
    if (system("$cmd") != 0) {
	warning("Could not generate /etc/password.new file: $!\n");
	return -1;
    }

    # Work around "/etc/passwd: Device or resource busy".
    $cmd    = "$MV /etc/passwd /etc/passwd.prev";
    ##print "$cmd\n";
    if (system("$cmd") != 0) {
	warning("Could not $cmd $!\n");
	return -1;
    }
    $cmd    = "$MV /etc/passwd.new /etc/passwd";
    ##print "$cmd\n";
    if (system("$cmd") != 0) {
	warning("Could not $cmd $!\n");
	return -1;
    }

    $cmd  = "$MKGROUP -l | $AWK '";
    # Make a duplicate group line that is a wheel alias for Administrators.
    $cmd .= '/^Administrators:/{print "wheel" substr($0, index($0,":"))} {print}';
    $cmd .= "' > /etc/group";
    ##print "$cmd\n";
    if (system("$cmd") != 0) {
	warning("Could not generate /etc/group file: $!\n");
	return -1;
    }
    
    return 0;
}

#
# Generate and return an ifconfig line that is approriate for putting
# into a shell script (invoked at bootup).
#
my %dev_map = ();
sub os_ifconfig_line($$$$$$$;$$)
{
    my ($iface, $inet, $mask, $speed, $duplex, $aliases,
	$iface_type, $settings, $rtabid) = @_;
    my ($uplines, $downlines);

    if (! $dev_map) {
	# Map from non-control interface names, e.g. "Local Area Connection #4"
	# to the Device Instance ID's used as devcon arguments, e.g.
	# "@PCI\VEN_8086&DEV_1010&SUBSYS_10128086&REV_01\5&2FA58B96&0&210030".
	if (! open(DEVMAP, $XIMAP)) {
	    warning("Cannot open $XIMAP $!\n");
	}
	else {
	    while (my $dev_line = <DEVMAP>) {
		chomp($dev_line);
		my ($dev_name, $dev_inst) = split(":", $dev_line, 2);
		$dev_map{$dev_name} = $dev_inst;
	    }
	    close(DEVMAP);
	}
    }

    if ($inet ne "") {
	# Startup.
	$uplines   .= qq{\n    #================================\n    };
	$uplines   .= qq{echo "Enabling $iface on $inet"\n    };
	#
	# Re-enable device if necessary (getmac Transport is "Disconnected".)
	my $test   =  qq[getmac /v /fo csv | awk -F, '/^"$iface"/{print \$4}'];
	$uplines   .= qq{if [ \`$test\` = '"Disconnected"' ]; then\n    };
	$uplines   .=   "  $DEVCON enable '$dev_map{$iface}'\n    ";
	$uplines   .= qq{  sleep 5\n    };
	$uplines   .= qq{fi\n    };
	#
	# Configure.
	$uplines   .= sprintf($IFCONFIG, $iface, $inet, $mask) . qq{\n    };
	#
	# Check that the configuration took!
	my $showip =  qq[$NETSH interface ip show address name="$iface"];
	$test      =  qq[$showip | awk '/IP Address:/{print \$NF}'];
	$uplines   .= qq{if [ \`$test\` != $inet ]; then\n    };
	#
	# Re-do it if not.
	$uplines   .= qq{  echo "    Config failed on $iface, retrying."\n    };
	$uplines   .=   "  $DEVCON disable '$dev_map{$iface}'\n    ";
	$uplines   .= qq{  sleep 5\n    };
	$uplines   .=   "  $DEVCON enable '$dev_map{$iface}'\n    ";
	$uplines   .= qq{  sleep 5\n    };
	$uplines   .= sprintf($IFCONFIG, $iface, $inet, $mask) . qq{\n    };
	#
	# Re-check.
	$uplines   .= qq{  if [ \`$test\` != $inet ]; then\n    };
	$uplines   .= qq{    echo "    Reconfig still failed on $iface."\n    };
	$uplines   .= qq{  else echo "    Reconfig succeeded on $iface."\n    };
	$uplines   .= qq{  fi\n    };
	$uplines   .= qq{fi};

	# Shutdown.
	$downlines .= qq{echo "Disabling $iface from $inet"\n    };
	$downlines .=   "$DEVCON disable '$dev_map{$iface}'\n";
    }
    
    return ($uplines, $downlines);
}

#
# Specialized function for configing locally hacked veth devices.
#
sub os_ifconfig_veth($$$$$;$$$)
{
    return "";
}

#
# Generate and return an string that is approriate for putting
# into /etc/hosts.
#
sub os_etchosts_line($$$)
{
    my ($name, $ip, $aliases) = @_;
    
    # Note: space rather than tab after the host name on Windows.
    return sprintf("%s %s %s", $ip, $name, $aliases);
}

#
# On Windows NT, accumulate an input file for the addusers command.
# See "AddUsers Automates Creation of a Large Number of Users",
# http://support.microsoft.com/default.aspx?scid=kb;en-us;199878
# 
# The file format is comma-delimited, as follows:
# 
# [Users]
# User Name,Full name,Password,Description,HomeDrive,Homepath,Profile,Script
# 
# [Global] or [Local]
# Group Name,Comment,UserName,...
# 
my @groupNames;
my %groupsByGid;
my %groupMembers;
sub os_accounts_start()
{
    # Remember group info to be put out at the end.
    @groupNames = ();
    %groupsByGid = ();
    %groupMembers = ();

    if (! open(WINUSERS, "> $winusersfile")) {
	warning("os_accounts_start: Cannot create $winusersfile .\n");
	return -1;
    }

    # Don't wipe out previous user shell preferences, just add new ones.
    if (! open(USERSHELLS, ">> $usershellsfile")) {
	warning("os_accounts_start: Cannot create or append to $usershellsfile .\n");
	return -1;
    }

    # Users come before groups in the addusers.exe account stream.
    # Notice the <CR><LF>'s!  It's a Windows file.
    print WINUSERS "[Users]\r\n";

    return 0;
}

#
# Remember the mapping from an existing group GID to its name.
#
sub os_groupgid($$)
{
    my($group, $gid) = @_;

    $groupsByGid{$gid} = $group;    # Remember the name associated with the gid.

    return 0;
}

#
# Add a new group
# 
sub os_groupadd($$)
{
    my($group, $gid) = @_;

    push(@groupNames, $group);      # Remember all of the group names.
    os_groupgid($group, $gid);

    return 0;
}

#
# Delete an old group
# 
sub os_groupdel($)
{
    my($group) = @_;

    # Unimplemented.
    return -1;
}

#
# Remove a user account.
# 
sub os_userdel($)
{
    my($login) = @_;

    # Unimplemented.
    return -1;
}

#
# Modify user group membership and password.
# Changing the login shell is unimplemented.
# 
sub os_usermod($$$$$$)
{
    my($login, $gid, $glist, $pswd, $root, $shell) = @_;

    if ($root) {
	$glist .= ",0";
    }
    if ($glist ne "") {
	##print "glist '$glist'\n";
	my $gname;
	foreach my $grp (split(/,/, $glist)) {
	    if ( $grp eq "0" ) {
		$gname = "Administrators";
	    }
	    else {
		$gname = $groupsByGid{$grp};
	    }
	    ##print "login $login, grp $grp, gname '$gname'\n";
	    my $cmd = "$NET localgroup $gname | tr -d '\\r' | grep -q '^$login\$'";
	    ##print "    $cmd\n";
	    if (system($cmd)) {
		# Add members into groups using the "net localgroup /add" command.
		$cmd = "$NET localgroup $gname $login /add";
		##print "    $cmd\n";
		if (system($cmd) != 0) {
		    warning("os_usermod error ($cmd)\n");
		}
	    }
	}
    }

    $cmd = "echo -e '$pswd\\n$pswd' | passwd $login >& /dev/null";
    ##print "    $cmd\n";
    if (system($cmd) != 0) {
	warning("os_usermod error ($cmd)\n");
    }
}

#
# Add a user.
# 
sub os_useradd($$$$$$$$$)
{
    my($login, $uid, $gid, $pswd, $glist, $homedir, $gcos, $root, $shell) = @_;

    # Groups have to be created before we can add members.
    my $gname = $groupsByGid{$gid};
    warning("Missing group name for gid $gid.\n")
	if (!$gname);
    $groupMembers{$gname} .= "$login ";
    $groupMembers{'Administrators'} .= "$login "
	if ($root);
    foreach my $gid (split(/,/, $glist)) {
	$gname = $groupsByGid{$gid};
	if ($gname) {
	    $groupMembers{$gname} .= "$login ";
	}
	else {
	    warning("Missing group name for gid $gid.\n");
	}
    }
		     
    # Map the shell into a full path.
    $shell = MapShell($shell);
    # Change the ones that are different from the default from mkpasswd, /bin/bash.
    print USERSHELLS "/^$login:/s|/bin/bash\$|$shell|\n"
	if ($shell !~ "/bin/bash");

    # Use the leading 8 chars of the Unix MD5 passwd hash as a known random
    # password, both here and in Samba.  Skip over a "$1$" prefix.
    my $pwd = $pswd;
    $pwd =~ s/^(\$1\$)(.{8}).*/$2/;
    
    print WINUSERS "$login,$gcos,$pwd,,,,,\r\n";

    return 0;
}

#
# Finish the input for the addusers command.
#
sub os_accounts_end()
{
    # Dump out the group *creation* lines.
    print WINUSERS "[Local]\r\n";
    foreach my $grp (@groupNames) {
	# Ignore group membership here.  See "net localgroup" below.
	print WINUSERS "$grp,Emulab $grp group,\r\n";
    }
    close WINUSERS;
    close USERSHELLS;
       
    # Create the whole batch of groups and accounts listed in the file.
    # /p options: Users don't have to change passwords, and they never expire.
    print "Creating the Windows users and groups.\n";
    my $winfile = "C:/cygwin$winusersfile";
    $winfile =~ s|/|\\|g;
    my $cmd = "$ADDUSERS /c '$winfile' /p:le";
    ##print "    $cmd\n";
    if (system($cmd) != 0) {
	warning("AddUsers error ($cmd)\n");
	return -1;
    }

    # Add members into groups using the "net localgroup /add" command.
    # (Addusers only creates groups, it can't add a user to an existing group.)
    while (my($grp, $members) = each %groupMembers) {
	foreach my $mbr (split(/ /,$members)) {
	    print "  Adding $mbr to $grp.\n";
	    my $cmd = "$NET localgroup $grp $mbr /add > /dev/null";
	    ##print "    $cmd\n";
	    if (system($cmd) != 0) {
		warning("net localgroup error ($cmd)\n");
	    }
	}
    }

    # Make the CygWin /etc/passwd and /etc/group files match Windows.
    # Note that the group membership is not reported into the CygWin files.
    return os_accounts_sync();
}

#
# Remove a homedir. Might someday archive and ship back.
#
sub os_homedirdel($$)
{
    return 0;
}

#
# Create a directory including all intermediate directories.
#
sub os_mkdir($$)
{
    my ($dir, $mode) = @_;

    if (system("$MKDIR -p -m $mode $dir")) {
	return 0;
    }
    return 1;
}

#
# OS Dependent configuration. 
# 
sub os_setup()
{
    return 0;
}
    
#
# OS dependent, routing-related commands
#
sub os_routing_enable_forward()
{
    return "";
}

sub os_routing_enable_gated($)
{
    return "";
}

sub os_routing_add_manual($$$$$;$)
{
    my ($routetype, $destip, $destmask, $gate, $cost, $rtabid) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE add $destip $gate";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE add $destip mask $destmask $gate";
    } elsif ($routetype eq "default") {
	$cmd = "$ROUTE add 0.0.0.0 $gate";
    } else {
	warning("Bad routing entry type: $routetype\n");
	$cmd = "";
    }

    # There appears to be a race with interfaces coming on-line.
    #     The route addition failed: Either the interface index is wrong or
    #     the gateway does not lie on the same network as the interface. Check
    #     the IP Address Table for the machine.
    # Re-doing the command later succeeds.
    # Wrap the route command in a loop to make sure it gets done.
    # Don't loop forever.
    $cmd = "n=1; while ! ( $ROUTE print | grep -Fq $destip ); do \n
                echo $cmd;\n
                $cmd\n
                let n++; if [[ \$n > 5 ]]; then break; fi
                sleep 5\n
            done";

    return $cmd;
}

sub os_routing_del_manual($$$$$;$)
{
    my ($routetype, $destip, $destmask, $gate, $cost, $rtabid) = @_;
    my $cmd;

    if ($routetype eq "host") {
	$cmd = "$ROUTE delete $destip";
    } elsif ($routetype eq "net") {
	$cmd = "$ROUTE delete $destip";
    } elsif ($routetype eq "default") {
	$cmd = "$ROUTE delete 0.0.0.0";
    } else {
	warning("Bad routing entry type: $routetype\n");
	$cmd = "";
    }

    return $cmd;
}

# Map a shell name to a full path using /etc/shells
sub MapShell($)
{
   my ($shell) = @_;

   if ($shell eq "") {
       return $DEFSHELL;
   }

   my $fullpath = `grep '/${shell}\$' $SHELLS`;

   if ($?) {
       return $DEFSHELL;
   }

   # Sanity Check
   if ($fullpath =~ /^([-\w\/]*)$/) {
       $fullpath = $1;
   }
   else {
       $fullpath = $DEFSHELL;
   }
   return $fullpath;
}

sub os_samba_mount($$$)
{
    my ($local, $host, $verbose) = @_;

    # Unmount each one first, ignore errors.
    system("$UMOUNT $local");

    # Make the CygWin mount from the Samba path to the local mount point directory.
    my $sambapath = $local;
    $sambapath =~ s|^/proj/(.*)|proj-$1|;
    $sambapath =~ s|^/groups/(.*)/(.*)|$1-$2|;
    $sambapath =~ s|.*/(.*)|$1|;
    $sambapath = "//$host/$sambapath";
    if (! -e $local) {
	print "os_samba_mount: Making CygWin '$local' mount point directory.\n"
	    if ($verbose);
	if (! os_mkdir($local, "0755")) {  # Will make whole path if necessary.
	    warning("os_samba_mount: Could not make mount point $local.\n");
	}
    }
    elsif (! -d $local) {
	warning("os_samba_mount: Mount point $local is not a directory.\n");
    }
    print "Mounting '$local' from '$sambapath'.\n"
	if ($verbose);

    # If we don't turn on the -E/--no-executable flag, CygWin mount warns us:
    #     mount: defaulting to '--no-executable' flag for speed since native path
    #            references a remote share.  Use '-f' option to override.
    # Even with -E, exe's and scripts still work properly, so put it in.
    $cmd = "$MOUNT -f -E $sambapath $local";
    if (system($cmd) != 0) {
	warning("os_samba_mount: Failed, $cmd.\n");
    }
}

# Extract the local mount point from a remote NFS mount path.
sub os_mountlocal($)
{
    my ($remote) = @_;
    my $local = $remote;
    $local =~ s|^.*:||;			# Remove server prefix.
    $local =~ s|^/q/proj/|/proj/|;	# Remove /q prefix from /proj.
    return $local;
}

# Execute a noisy bash command, throwing away the output unless we ask for it.
sub os_noisycmd($$)
{
    my ($cmd, $verbose) = @_;
    my $bashcmd = "$BASH -c '$cmd'" . ($verbose ? "" : " > /dev/null");
    my $ret = system($bashcmd);
    ##print "os_noisycmd cmd '$cmd', ret $ret\n";
    return $ret
}

sub os_fwconfig_line($@)
{
    my ($fwinfo, @fwrules) = @_;
    my ($upline, $downline);
    my $errstr = "*** WARNING: Windows firewall not implemented\n";

    warn $errstr;
    $upline = "echo $errstr; exit 1";
    $downline = "echo $errstr; exit 1";

    return ($upline, $downline);
}

sub os_fwrouteconfig_line($$$)
{
    my ($orouter, $fwrouter, $routestr) = @_;
    my ($upline, $downline);

    #
    # XXX assume the original default route should be used to reach servers.
    #
    # For setting up the firewall, this means we create explicit routes for
    # each host via the original default route.
    #
    # For tearing down the firewall, we just remove the explicit routes
    # and let them fall back on the now re-established original default route.
    #
    $upline  = "for vir in $routestr; do\n";
    $upline .= "        $ROUTE delete \$vir >/dev/null 2>&1\n";
    $upline .= "        $ROUTE add -host \$vir gw $orouter || {\n";
    $upline .= "            echo \"Could not establish route for \$vir\"\n";
    $upline .= "            exit 1\n";
    $upline .= "        }\n";
    $upline .= "    done";

    $downline  = "for vir in $routestr; do\n";
    $downline .= "        $ROUTE delete \$vir >/dev/null 2>&1\n";
    $downline .= "    done";

    return ($upline, $downline);
}

1;
