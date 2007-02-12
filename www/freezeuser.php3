<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006, 2007 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("target_user",     PAGEARG_USER);
$optargs = OptionalPageArguments("action",	    PAGEARG_STRING,
				 "request",	    PAGEARG_BOOLEAN,
				 "canceled",        PAGEARG_STRING,
				 "confirmed",       PAGEARG_STRING,
				 "confirmed_twice", PAGEARG_STRING);

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

# Need these below.
$target_uid = $target_user->uid();
$userstatus = $target_user->status();

#
# Confirm a valid op.
#
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
    $uid_name  = $this_user->name();
    $uid_email = $this_user->email();

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
if (isset($canceled) && $canceled) {
    echo "<center><h2><br>
          The $action has been canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    echo "<center><br>\n";

    echo "Are you <b>REALLY</b> sure you want to $action user '$target_uid'\n";

    $url = CreateURL("freezeuser", $target_user, "action", $action);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (!isset($confirmed_twice)) {
    echo "<center><br>
	  Okay, lets be sure.<br>\n";

    echo "Are you <b>REALLY REALLY</b> sure you want to $action user
              '$target_uid'\n";
    
    $url = CreateURL("freezeuser", $target_user, "action", $action);
    
    echo "<form action='$url' method=post>";
    echo "<input type=hidden name=confirmed value=Confirm>\n";
    echo "<b><input type=submit name=confirmed_twice value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

# Change the DB first; backend requires it.
$target_user->SetStatus($dbaction);

STARTBUSY("User '$target_uid' is being ${tag}!");
SUEXEC($uid, $TBADMINGROUP, "webtbacct $action $target_uid",SUEXEC_ACTION_DIE);
STOPBUSY();

# Back to user display.
PAGEREPLACE(CreateURL("showuser", $target_user));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
