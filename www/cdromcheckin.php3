<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

# These error codes must match whats in register.pl on the cd.
define("CDROMSTATUS_OKAY"	,	0);
define("CDROMSTATUS_MISSINGARGS",	100);
define("CDROMSTATUS_INVALIDARGS",	101);
define("CDROMSTATUS_BADCDKEY",		102);
define("CDROMSTATUS_BADPRIVKEY",	103);
define("CDROMSTATUS_BADIPADDR",		104);
define("CDROMSTATUS_BADREMOTEIP",	105);
define("CDROMSTATUS_IPADDRINUSE",	106);
define("CDROMSTATUS_UPGRADEERROR",	107);
define("CDROMSTATUS_OTHER",		199);

#
# Config setup.  This shouldn't be hard-coded in the long term.

$slicexbase = "slicex_slice=3\nslicex_mount=/users\n";
$confslicex = "slicex_tarball=slicex.tar.gz
slicex_sig=http://${WWWHOST}/images/slicex-v4.tar.gz.sig
";

$version_tags[5] = "24a4f173a7716d47bbc047f8387c86af";
$confmaps["24a4f173a7716d47bbc047f8387c86af"] = "fdisk=image.fdisk
fdisk_sig=https://${WWWHOST}/images/image.fdisk.sig
slice1_image=slice1.ndz
slice1_alt_image=http://${WWWHOST}/images/slice1-v5.ndz
slice1_sig=https://${WWWHOST}/images/slice1-v5.ndz.sig
slice1_md5=24a4f173a7716d47bbc047f8387c86af
" . $slicexbase;

$version_tags[4] = "c9f8578517f5ebb0eca70b69dce144d";
$confmaps["c9f8578517f5ebb0eca70b69dce144d"] = "fdisk=image.fdisk
fdisk_sig=https://${WWWHOST}/images/image.fdisk.sig
slice1_image=slice1.ndz
slice1_alt_image=http://${WWWHOST}/images/slice1-v4.ndz
slice1_sig=https://${WWWHOST}/images/slice1-v4.ndz.sig
slice1_md5=c9f8578517f5ebb0eca70b69dce144db
" . $slicexbase;

$confmaps["d326a1f604489c43b488fa80a88221f4"] = "slice1_image=slice1.ndz
slice1_sig=slice1.ndz.sig
slice1_md5=d326a1f604489c43b488fa80a88221f4\n" . $slicexbase;

#
# Spit back a text message we can display to the user on the console
# of the node running the checkin. We could return an http error, but
# that would be of no help to the user on the other side.
# 
function SPITSTATUS($status)
{
    global $REMOTE_ADDR, $REQUEST_URI;
    
    header("Content-Type: text/plain");
    echo "emulab_status=$status\n";

    if ($status) {
	    TBERROR("CDROM Checkin Error ($status) from $REMOTE_ADDR:\n\n".
		    "$REQUEST_URI\n", 0);
    }
}

#
# This page is not intended to be invoked by humans!
#
if ((!isset($cdkey) || !strcmp($cdkey, ""))) {
    SPITSTATUS(CDROMSTATUS_MISSINGARGS);
    return;
}

if (!ereg("[0-9a-zA-Z]+", $cdkey)) {
    SPITSTATUS(CDROMSTATUS_INVALIDARGS);
    return;
}

#
# Make sure this is a valid CDkey.
#
$query_result =
    DBQueryFatal("select * from cdroms where cdkey='$cdkey'");
if (! mysql_num_rows($query_result)) {
    SPITSTATUS(CDROMSTATUS_BADCDKEY);
    return;
}
$row    = mysql_fetch_array($query_result);
$cdvers = $row[version];

#
# Node is requesting latest instructions. Give it the script. This could
# be on a per CDROM basis, but for now its hardwired.
# I know, the hardwired paths are bad; move into the cdroms DB table, even
# if duplicated. 
# 
if (isset($needscript)) {
    header("Content-Type: text/plain");

    if ($cdvers <= 3) {
	echo "MD5=5ed48bdf6def666d5a5d643cbc2ce4d0\n";
	echo "URL=https://${WWWHOST}/images/netbed-setup-v3.pl\n";
    }
    elseif ($cdvers == 69) {
	echo "URL=https://${WWWHOST}/images/netbed-setup-v69.pl\n";
	echo "SIG=https://${WWWHOST}/images/netbed-setup-v69.pl.sig\n";
    }
    elseif ($cdvers == 4) {
	echo "URL=https://${WWWHOST}/images/netbed-setup-v4.pl\n";
	echo "SIG=https://${WWWHOST}/images/netbed-setup-v4.pl.sig\n";
    }
    elseif ($cdvers == 5) {
	echo "URL=https://${WWWHOST}/images/netbed-setup-v5.pl\n";
	echo "SIG=https://${WWWHOST}/images/netbed-setup-v5.pl.sig\n";
    }
    else {
      SPITSTATUS(CDROMSTATUS_INVALIDARGS);
      return;
    }
    echo "emulab_status=0\n";
    return;
}

#
# Otherwise, move onto config instructions.
#
if ((!isset($privkey) || !strcmp($privkey, "")) ||
    (!isset($IP) || !strcmp($IP, ""))) {
    SPITSTATUS(CDROMSTATUS_MISSINGARGS);
    return;
}

if (!ereg("[0-9a-zA-Z ]+", $privkey) ||
    !ereg("[0-9\.]+", $IP)) {
    SPITSTATUS(CDROMSTATUS_INVALIDARGS);
    return;
}

if (isset($wahostname) &&
    !ereg("[-_0-9a-zA-Z\.]+", $wahostname)) {
    SPITSTATUS(CDROMSTATUS_INVALIDARGS);
    return;
}

#
# Generate a nickname if given hostname.
#
unset($nickname);
if (isset($wahostname)) {
    if (strpos($wahostname, ".")) {
	$nickname = str_replace(".", "-", $wahostname);
    }
    else {
	$nickname = $wahostname;
    }
}

if (isset($roottag) &&
    !ereg("[0-9a-zA-Z]+", $roottag)) {
    SPITSTATUS(CDROMSTATUS_INVALIDARGS);
    return;
}

#
# Grab the privkey record. First squeeze out any spaces.
#
$privkey = str_replace(" ", "", $privkey);

$query_result =
    DBQueryFatal("select * from widearea_privkeys where privkey='$privkey'");

if (!mysql_num_rows($query_result)) {
    SPITSTATUS(CDROMSTATUS_BADPRIVKEY);
    return;
}
$warow  = mysql_fetch_array($query_result);
$privIP = $warow[IP];

#
# If the node is reporting that its finished updating, then make its
# current privkey its nextprivkey.
# 
if (isset($updated) && $updated == 1) {
    if (! strcmp($privIP, "1.1.1.1")) {
        #
        # If the node is brand new, then need to create several records.
	# Pass it off to a perl script.
        #
	DBQueryFatal("update widearea_privkeys ".
		     "set IP='$IP' ".
		     "where privkey='$privkey'");

	$newargs = "-w -i $IP ";
	if (isset($nickname)) {
	    $newargs .= " -n $nickname ";
	}

	$type = "pcwa";
	if (isset($wahostname)) {
            #
            # XXX Parse hostname to see if a ron node.
	    # Silly, but effective since we will never have anything else
	    # besides wa/ron.
            #
	    if (strstr($wahostname, ".ron.") ||
		strstr($wahostname, "roncluster")) {
		$type = "pcron";
	    }
	}
	$newargs .= " -t $type ";
	SUEXEC("nobody", $TBADMINGROUP, "webnewwanode $newargs", 0);

	#
	# Send email to user reminding to register node.
	#
	$user_name = $warow[user_name];
	$user_email= $warow[user_email];
	$lockkey   = $warow[lockkey];

	TBMAIL("$user_name <$user_email>",
	   "Thanks for installing a NetBed CDROM!",
	   "It would be very helpful if you could please go to:\n".
	   "\n".
	   "    ${TBBASE}/widearea_register.php?cdkey=$lockkey&IP=$IP\n".
	   "\n".
	   "and tell us a few things about the node (processor, connection\n".
	   "type, geographical info, etc.). You can also register for a\n".
	   "local account on your node by providing this info to us.\n".
	   "\n".
	   "Thanks,\n".
	   "Testbed Operations\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");
    }

    DBQueryFatal("update widearea_privkeys ".
		 "set privkey=nextprivkey,updated=now(),nextprivkey=NULL ".
		 (isset($roottag) ? ",rootkey='$roottag'" : "") .
		 "where privkey='$privkey'");

    #
    # Check for image upgrade completed. Delete record. 
    #
    $query_result =
	DBQueryFatal("delete from widearea_updates ".
		     "where IP='$IP' and roottag='$roottag' and " .
		     "      update_started is not null");

    #
    # Update with new nickname. Nice if node changes its real hostname.
    #
    if (isset($nickname)) {
	$nodeid = TBIPtoNodeID($IP);
	if ($nodeid) {
	    DBQueryFatal("update reserved set vname='$nickname' ".
			 "where node_id='$nodeid'");
	}
    }

    SPITSTATUS(CDROMSTATUS_OKAY);
    return;
}

#
# If the record has a valid IP address and matches the node connecting
# then all we do is return a new private key.
#
if (strcmp($privIP, "1.1.1.1")) {
    if (strcmp($privIP, $IP)) {
	SPITSTATUS(CDROMSTATUS_BADIPADDR);
	return;
    }
    if (strcmp($REMOTE_ADDR, $IP)) {
	SPITSTATUS(CDROMSTATUS_BADREMOTEIP);
	return;
    }

    $upgrade_instructions = 0;
    #
    # Check for image upgrade. Rather simplistic right now.
    #
    $query_result =
	DBQueryFatal("select * from widearea_updates where IP='$IP'");
    if (mysql_num_rows($query_result)) {
	$row     = mysql_fetch_array($query_result);
	$newroot = $row["roottag"];
	$force   = $row["force"];

	# Do not upgrade if the root tag already matches. ???
	if (!strcmp($force, "yes")|| !isset($roottag) || strcmp($roottag, $newroot)) {
	    $update  = 1;

	    DBQueryFatal("update widearea_updates set update_started=now() ".
			 "where IP='$IP'");

	    $upgrade_instructions = $confmaps[$newroot];
		
	    if (!$upgrade_instructions) {
		SPITSTATUS(CDROMSTATUS_UPGRADEERROR);
		return;
	    }
	}
    }

    #
    # Generate a new privkey and stash in the DB. Always stash the root
    # tag the node is running. If upgrading, it will be overwritten above
    # when it completes. This helps to figure out where things went wrong.
    #
    $newkey = GENHASH();
    DBQueryFatal("update widearea_privkeys ".
		 "set nextprivkey='$newkey',updated=now(),cdkey='$cdkey' ".
		 (isset($roottag) ? ",rootkey='$roottag'" : "") .
		 "where IP='$IP' and privkey='$privkey'");

    header("Content-Type: text/plain");
    echo "privkey=$newkey\n";
    if ($upgrade_instructions) 
	echo "$upgrade_instructions\n";
    echo "emulab_status=0\n";
    
    return;
}

#
# We need to check for duplicate IPs. 
#
$query_result =
    DBQueryFatal("select * from widearea_privkeys where IP='$IP'");

if (mysql_num_rows($query_result)) {
    SPITSTATUS(CDROMSTATUS_IPADDRINUSE);
    return;
}

#
# Generate a new privkey and return the info. The DB is updated with the
# next key, but it does not become effective until it responds that it
# finished okay. This leaves a window open, but the eventual plan it to
# get rid of these priv keys anyway and go to per-node cert files we update
# on the fly.
# 
$newkey = GENHASH();
DBQueryFatal("update widearea_privkeys ".
	     "set nextprivkey='$newkey',updated=now(),cdkey='$cdkey'".
	     "where privkey='$privkey'");

header("Content-Type: text/plain");
echo "privkey=$newkey\n";
if ($cdvers == 1) {
    if (0) {
	# one shot setup for ISI (or anyone else who has a corrupt image)
	echo "fdisk=http://${WWWHOST}/images/image.fdisk\n";
	echo "slice1_image=http://${WWWHOST}/images/slice1-v1.ndz\n";
	echo "slice1_md5=2970e2cf045f5872c6728eeea3b51dae\n";
	echo "slicex_mount=/users\n";
	echo "slicex_tarball=http://${WWWHOST}/images/slicex-v2.tar.gz\n";
	echo "slicex_md5=a5274072a40ebf2fda8b8596a6e60e0d\n";
    } else {
	echo "fdisk=image.fdisk\n";
	echo "slice1_image=slice1.ndz\n";
	echo "slice1_md5=cb810b43f49d15b3ac4122ff42f8925d\n";
	echo "slicex_mount=/users\n";
	echo "slicex_tarball=http://${WWWHOST}/images/slicex.tar.gz\n";
	echo "slicex_md5=a5274072a40ebf2fda8b8596a6e60e0d\n";
    }
}
elseif ($cdvers == 2) {
    echo "fdisk=image.fdisk\n";
    echo "slice1_image=slice1.ndz\n";
    echo "slice1_md5=402f00f2e46d22507cef3d19846b48f8\n";
    echo "slicex_mount=/users\n";
    echo "slicex_tarball=slicex.tar.gz\n";
    echo "slicex_md5=a5274072a40ebf2fda8b8596a6e60e0d\n";
}
elseif ($cdvers == 3) {
    if (0) {
	echo "fdisk=http://${WWWHOST}/images/image.fdisk\n";
	echo "slice1_image=http://${WWWHOST}/images/slice1-v3.ndz\n";
	echo "slice1_md5=a9af4a08310a314c5b38e5e33a2b9c3e\n";
	echo "slicex_slice=3\n";
	echo "slicex_mount=/users\n";
	echo "slicex_tarball=http://${WWWHOST}/images/slicex-v3.tar.gz\n";
	echo "slicex_md5=0a3398cee6104850adaee7afbe75f008\n";
    }
    else {
	echo "fdisk=image.fdisk\n";
	echo "slice1_image=slice1.ndz\n";
	echo "slice1_md5=a9af4a08310a314c5b38e5e33a2b9c3e\n";
	echo "slicex_slice=3\n";
	echo "slicex_mount=/users\n";
	echo "slicex_tarball=slicex.tar.gz\n";
	echo "slicex_md5=0a3398cee6104850adaee7afbe75f008\n";
    }
}
elseif ($cdvers == 69) {
    if (!strcmp($REMOTE_ADDR, "155.101.132.191")) {
	echo "fdisk=image.fdisk\n";
	echo "fdisk_sig=image.fdisk.sig\n";
	echo "slice1_image=slice1.ndz\n";
	echo "slice1_sig=slice1.ndz.sig\n";
	# Still return this for the root tag. Might change later.
	echo "slice1_md5=20d04a3ba96043788e2d31d52e7e7165\n";
    }
    else {
	echo "fdisk=http://${WWWHOST}/images/image.fdisk\n";
	echo "fdisk_sig=https://${WWWHOST}/images/image.fdisk.sig\n";
	echo "slice1_image=http://${WWWHOST}/images/slice1-v4.ndz\n";
	echo "slice1_sig=https://${WWWHOST}/images/slice1-v4.ndz.sig\n";
	# Still return this for the root tag. Might change later.
	echo "slice1_md5=20d04a3ba96043788e2d31d52e7e7165\n";
    }
}
else {
    if ($version_tags[$cdvers]) {
        echo $confmaps[$version_tags[$cdvers]];
	echo $confslicex;
    }
    else {
        SPITSTATUS(CDROMSTATUS_OTHER);
	return;
    }
}

echo "emulab_status=0\n";
