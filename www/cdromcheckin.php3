<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
define("CDROMSTATUS_OTHER",		199);

#
# Spit back a text message we can display to the user on the console
# of the node running the checkin. We could return an http error, but
# that would be of no help to the user on the other side.
# 
function SPITSTATUS($status)
{
    header("Content-Type: text/plain");
    echo "emulab_status=$status\n";
}

#
# This page is not intended to be invoked by humans!
#
if ((!isset($cdkey) || !strcmp($cdkey, "")) ||
    (!isset($privkey) || !strcmp($privkey, "")) ||
    (!isset($IP) || !strcmp($IP, ""))) {
    SPITSTATUS(CDROMSTATUS_MISSINGARGS);
    return;
}

if (!ereg("[0-9a-zA-Z]+", $cdkey) ||
    !ereg("[0-9a-zA-Z ]+", $privkey) ||
    !ereg("[0-9\.]+", $IP)) {
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
	
	SUEXEC("nobody", $TBADMINGROUP, "webnewwanode -w -t pcwa -i $IP", 0);
    }
    DBQueryFatal("update widearea_privkeys ".
		 "set privkey=nextprivkey,updated=now(),nextprivkey=NULL ".
		 "where privkey='$privkey'");

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

    #
    # For now we just give them a new privkey. There is no image upgrade
    # path in place yet.
    #
    $newkey = GENHASH();
    DBQueryFatal("update widearea_privkeys ".
		 "set nextprivkey='$newkey',updated=now() ".
		 "where IP='$IP' and privkey='$privkey'");

    header("Content-Type: text/plain");
    echo "privkey=$newkey\n";
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
	     "set nextprivkey='$newkey',updated=now()".
	     "where privkey='$privkey'");

header("Content-Type: text/plain");
echo "privkey=$newkey\n";
if ($cdvers == 1) {
    echo "fdisk=image.fdisk\n";
    echo "slice1_image=slice1.ndz\n";
    echo "slice1_md5=cb810b43f49d15b3ac4122ff42f8925d\n";
    echo "slicex_mount=/users\n";
    echo "slicex_tarball=slicex.tar.gz\n";
    echo "slicex_md5=1f84fbc3434d174151ac3a2b8389799a\n";
}
else {
    echo "fdisk=http://${WWWHOST}/images/image.fdisk\n";
    echo "slice1_image=http://${WWWHOST}/images/slice1-v2.ndz\n";
    echo "slice1_md5=5389a2687cee16ff212bbd842585a5d5\n";
    echo "slicex_mount=/users\n";
    echo "slicex_tarball=http://${WWWHOST}/images/slicex.tar.gz\n";
    echo "slicex_md5=1f84fbc3434d174151ac3a2b8389799a\n";
}
echo "emulab_status=0\n";

