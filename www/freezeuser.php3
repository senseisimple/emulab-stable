<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Freeze User Account");

#
# Only known and logged in users allowed.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}
if (isset($action)) {
    if (strcmp($action, "freeze") && strcmp($action, "thaw")) {
	USERERROR("You can freeze a user or you can thaw a user!", 1);
    }
}
else {
    $action = "freeze";
}

if (!strcmp($action, "thaw")) {
    $tag    = "thawed";
    $dbaction = TBDB_USERSTATUS_ACTIVE;
}
else {
    $tag      = "frozen";
    $dbaction = TBDB_USERSTATUS_FROZEN;
}
$isadmin = ISADMIN($uid);

#
# Confirm target is a real user.
#
if (! TBCurrentUser($target_uid)) {
    USERERROR("No such user '$target_uid'", 1);
}

#
# Confirm a valid op.
#
$userstatus = TBUserStatus($target_uid);
if (!strcmp($action, "thaw") &&
     strcmp($userstatus, TBDB_USERSTATUS_FROZEN)) {
    USERERROR("You cannot thaw someone who is not frozen!", 1);
}
if (!strcmp($action, "freeze")) {
    if (!strcmp($userstatus, TBDB_USERSTATUS_FROZEN)) {
	USERERROR("You cannot freeze someone who is already iced over!", 1);
    }
    if (strcmp($userstatus, TBDB_USERSTATUS_ACTIVE)) {
	USERERROR("You cannot freeze someone who is not active!", 1);
    }
}

#
# Requesting? Fire off email and we are done. 
# 
if (isset($request) && $request) {
    TBUserInfo($uid, $uid_name, $uid_email);

    TBMAIL($TBMAIL_OPS,
	   "$action User Request: '$target_uid'",
	   "$uid is requesting that user account '$target_uid' be $tag!\n",
	   "From: $uid_name '$uid' <$uid_email>\n".
	   "Errors-To: $TBMAIL_WWW");

    echo "A request to $action user '$target_uid' has been sent to Testbed
          Operations. If you do not hear back within a reasonable amount
          of time, please contact $TBMAILADDR.\n";

    #
    # Standard Testbed Footer
    #
    PAGEFOOTER();
    return;
}

#
# Only admins.
#
if (!$isadmin) {
    USERERROR("You do not have permission to $action user '$target_uid'", 1);
}

#
# We do a double confirmation, running this script multiple times. 
#
if ($canceled) {
    echo "<center><h2><br>
          The $action has been canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><br>\n";

    echo "Are you <b>REALLY</b> sure you want to $action user '$target_uid'\n";
    
    echo "<form action=freezeuser.php3 method=post>";
    echo "<input type=hidden name=target_uid value=\"$target_uid\">\n";
    echo "<input type=hidden name=action value=\"$action\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (!$confirmed_twice) {
    echo "<center><br>
	  Okay, lets be sure.<br>\n";

    echo "Are you <b>REALLY REALLY</b> sure you want to $action user
              '$target_uid'\n";
    
    echo "<form action=freezeuser.php3 method=post>";
    echo "<input type=hidden name=target_uid value=\"$target_uid\">\n";
    echo "<input type=hidden name=confirmed value=Confirm>\n";
    echo "<input type=hidden name=action value=\"$action\">\n";
    echo "<b><input type=submit name=confirmed_twice value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

DBQueryFatal("update users set status='$dbaction' ".
	     "where uid='$target_uid'");

STARTBUSY("User '$target_uid' is being ${tag}!");

#
# All the real work is done in the script.
#
SUEXEC($uid, $TBADMINGROUP, "webtbacct $action $target_uid",
       SUEXEC_ACTION_DIE);

/* Clear indicators */
STOPBUSY();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
