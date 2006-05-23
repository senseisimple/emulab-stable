<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

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
if (!TBvalid_uid($target_uid)) {
    PAGEARGERROR("Invalid characters in UID");
}
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("You do not have permission to do this!", 1);
}

#
# Confirm target is a real user.
#
if (! TBCurrentUser($target_uid)) {
    USERERROR("No such user '$target_uid'", 1);
}

if (DOLOGIN_MAGIC($target_uid) < 0) {
    USERERROR("Could not log you in as $target_uid", 1);
}
# So the menu and headers get spit out properly.
$_COOKIE[$TBNAMECOOKIE] = $target_uid;

PAGEHEADER("SU as User");

echo "<center>";
echo "<br><br>";
echo "<font size=+2>You are now logged in as <b>$target_uid</b></font>\n";
echo "<br><br>";
echo "<font size=+1>Be Careful!</font>\n";
echo "</center>";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
