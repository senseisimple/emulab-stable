<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify page/form arguments.
#
if (! isset($_GET['target_uid']))
    $target_uid = $uid;
else
    $target_uid = $_GET['target_uid'];

# Pedantic check of uid before continuing.
if ($target_uid == "" || !TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid uid: '$target_uid'");
}

#
# Check to make sure thats this is a valid UID.
#
if (! TBCurrentUser($target_uid)) {
    USERERROR("The user $target_uid is not a valid user", 1);
}

#
# Only admin people can create SSL certs for another user.
#
if (!$isadmin &&
    strcmp($uid, $target_uid)) {
    USERERROR("You do not have permission to download SSL cert for $user!", 1);
}

$query_result =
    DBQueryFatal("select cert,privkey from user_sslcerts ".
		 "where uid='$target_uid' and encrypted=1");

if (!mysql_num_rows($query_result)) {
    PAGEHEADER("Download SSL Certificate for $target_uid");
    USERERROR("There is no SSL Certificate for $target_uid!", 1);
}
$row  = mysql_fetch_array($query_result);
$cert = $row["cert"];
$key  = $row["privkey"];

header("Content-Type: text/plain");
echo "-----BEGIN RSA PRIVATE KEY-----\n";
echo $key;
echo "-----END RSA PRIVATE KEY-----\n";
echo "-----BEGIN CERTIFICATE-----\n";
echo $cert;
echo "-----END CERTIFICATE-----\n";

