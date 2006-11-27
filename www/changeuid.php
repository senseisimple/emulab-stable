<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only admin users ...
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("You do not have permission to login names!", 1);
}

#
# Verify page/form arguments. Note that the target uid comes initially as a
# page arg, but later as a form argument, hence this odd check.
#
if (! isset($_POST['submit'])) {
    # First page load. Default to current user.
    if (! isset($_GET['target_uid']))
	$target_uid = $uid;
    else
	$target_uid = $_GET['target_uid'];
}
else {
    # Form submitted. Make sure we have a target_uid and a new_uid.
    if (! isset($_POST['target_uid']) || $_POST['target_uid'] == "" ||
	! isset($_POST['new_uid']) || $_POST['new_uid'] == "") {
	PAGEARGERROR("Invalid form arguments.");
    }
    $target_uid = $_POST['target_uid'];
    $new_uid    = $_POST['new_uid'];
}

# Pedantic check of uid before continuing.
if ($target_uid == "" || !TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid uid: '$target_uid'");
}

# Find user. Must be unapproved (verified user). Any other state is too hard.
if (! ($user = User::LookupByUid($target_uid))) {
    USERERROR("The user $target_uid is not a valid user", 1);
}
if ($user->status() != TBDB_USERSTATUS_UNAPPROVED) {
    USERERROR("The user $target_uid must be ".
	      "unapproved (but verified) to change!", 1);
}

function SPITFORM($user, $new_uid, $error)
{
    global $TBDB_UIDLEN;
    
    $target_uid = $user->uid();
    
    #
    # Standard Testbed Header.
    #
    PAGEHEADER("Change login UID for user");

    if ($error) {
	echo "<center>
              <font size=+1 color=red>$error</font>
              </center><br>\n";
    }
    else {
	echo "<center>
              <font size=+1>
              Please enter the new UID for user '$target_uid'<br><br>
              </font>
              </center>\n";
    }

    echo "<table align=center border=1>
          <form action=changeuid.php method=post>
          <tr>
              <td>New UID:</td>
              <td><input type=text
                         name=\"new_uid\"
                         value=\"$new_uid\"
	                 size=$TBDB_UIDLEN
	                 maxlength=$TBDB_UIDLEN></td>
          </tr>
          <tr>
             <td align=center colspan=2>
                 <b><input type=submit value=\"Change UID\"
                           name=submit></b>
             </td>
          </tr>
	  <input type=hidden name=target_uid value=$target_uid>
          </form>
          </table>\n";

    echo "<br><br>\n";
    echo "<center>\n";
    SHOWUSER($target_uid);
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# If not clicked, then put up a form.
#
if (! isset($_POST['submit'])) {
    SPITFORM($user, "", null);
    return;
}

# Sanity checks
$error = null;

if (!TBvalid_uid($new_uid)) {
    $error = "UID: " . TBFieldErrorString();
}
elseif (User::LookupByUid($new_uid) || posix_getpwnam($new_uid)) {
    $error = "UID: Already in use. Pick another";
}

if ($error) {
    SPITFORM($user, $new_uid, $error);
    return;
}

#
# Standard Testbed Header.
#
PAGEHEADER("Change login UID for user");

# Okay, call out to backend to change.

STARTBUSY("Changing UID");

#
# Run the backend script.
#
SUEXEC($uid, $TBADMINGROUP,
       "webchangeuid $target_uid $new_uid", SUEXEC_ACTION_USERERROR);

# Stop the busy indicator and zap to user page.
STOPBUSY();
PAGEREPLACE("showuser.php3?target_uid=$new_uid");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


