<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users allowed.
#
$this_user = CheckLoginOrDie();

if (!ISADMIN()) {
    USERERROR("You do not have permission to do this!", 1);
}

#
# Verify arguments.
#
$reqargs = RequiredPageArguments("target_user", PAGEARG_USER);

if (DOLOGIN_MAGIC($target_user->uid(), $target_user->uid_idx()) < 0) {
    USERERROR("Could not log you in as $target_uid", 1);
}
# So the menu and headers get spit out properly.
$_COOKIE[$TBNAMECOOKIE] = $target_user->webid();

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
