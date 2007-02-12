<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only admin users ...
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (!$isadmin) {
    USERERROR("You do not have permission to change login names!", 1);
}

#
# Verify page/form arguments.
#
$reqargs = RequiredPageArguments("target_user", PAGEARG_USER);
$optargs = OptionalPageArguments("submit",      PAGEARG_STRING,
				 "new_uid",     PAGEARG_STRING);

$target_uid = $target_user->uid();
$target_idx = $target_user->uid_idx();

if ($target_user->status() != TBDB_USERSTATUS_UNAPPROVED) {
    USERERROR("The user $target_uid must be ".
	      "unapproved (but verified) to change!", 1);
}

function SPITFORM($target_user, $new_uid, $error)
{
    global $TBDB_UIDLEN;
    
    $target_uid   = $target_user->uid();
    $target_webid = $target_user->webid();
    
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
	  <input type=hidden name=user value=$target_webid>
          </form>
          </table>\n";

    echo "<br><br>\n";
    echo "<center>\n";
    $target_user->Show();
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# If not clicked, then put up a form.
#
if (! isset($submit)) {
    SPITFORM($target_user, "", null);
    return;
}

# Sanity checks
$error = null;

if (! isset($new_uid) || $new_uid == "") {
    $error = "UID: Must supply a new UID";
}
elseif (!TBvalid_uid($new_uid)) {
    $error = "UID: " . TBFieldErrorString();
}
elseif (User::Lookup($new_uid) || posix_getpwnam($new_uid)) {
    $error = "UID: Already in use. Pick another";
}

if ($error) {
    SPITFORM($target_user, $new_uid, $error);
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
       "webchangeuid $target_idx $new_uid", SUEXEC_ACTION_USERERROR);

# Stop the busy indicator and zap to user page.
STOPBUSY();
PAGEREPLACE(CreateURL("showuser", $target_user));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


