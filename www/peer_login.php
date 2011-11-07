<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("user",       PAGEARG_USER,
				 "key",        PAGEARG_STRING);
$optargs = OptionalPageArguments("redirected", PAGEARG_BOOLEAN);

if (! $PORTAL_ENABLE) {
    USERERROR("No Portal", 1);
}

#
# Need this extra redirect so that the cookies get set properly. 
#
if (!isset($redirected) || $redirected == 0) {
    $uri = $_SERVER['REQUEST_URI'] . "&redirected=1";

    header("Location: https://$WWWHOST". $uri);
    return;
}

#
# Check the login table for the user, and see if the key is really
# the md5 of the login hash. If so, do a login. 
#
$target_uid = $user->uid();
$safe_key   = addslashes($key);

$query_result =
    DBQueryFatal("select * from login ".
		 "where uid='$target_uid' and hashhash='$safe_key' and ".
		 "      timeout > UNIX_TIMESTAMP(now())");
if (!mysql_num_rows($query_result)) {
    # Short delay.
    sleep(1);

    PAGEERROR("Invalid peer login request");
}
# Delete the entry so it cannot be reused, even on failure.
DBQueryFatal("delete from login ".
	     "where uid='$target_uid' and hashhash='$safe_key'");

#
# Now do the login, which can still fail.
#  
$dologin_status = DOLOGIN($user->uid(), "", 0, 1);

if ($dologin_status == DOLOGIN_STATUS_WEBFREEZE) {
    # Short delay.
    sleep(1);

    PAGEHEADER("Login");
    echo "<h3>
              Your account has been frozen due to earlier login attempt
              failures. You must contact $TBMAILADDR to have your account
              restored. <br> <br>
              Please do not attempt to login again; it will not work!
              </h3>\n";
    PAGEFOOTER();
    die("");
}
else if ($dologin_status != DOLOGIN_STATUS_OKAY) {
    # Short delay.
    sleep(1);
    PAGEHEADER("Login");

    echo "<h3>Peer login failed. Please contact $TBMAILADDR</h3>\n";
    PAGEFOOTER();
    die("");
}
else {
    #
    # Zap back to front page in secure mode.
    # 
    header("Location: $TBBASE/showuser.php3?user=$target_uid");
}

?>
