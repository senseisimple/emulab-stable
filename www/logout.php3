<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

# Get current login.
$this_user = CheckLoginOrDie(CHECKLOGIN_MODMASK);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("target_user",   PAGEARG_USER,
				 "next_page",     PAGEARG_STRING);

if (isset($target_user)) {
    # Only admin users can logout someone other then themself.
    if (!$isadmin && !$target_user->SameUser($this_user)) {
	PAGEHEADER("Logout");
	echo "<center>
                  <h3>You do not have permission to logout other users!</h3>
              </center>\n";
	PAGEFOOTER();
    }
}
else {
    $target_user = $this_user;
}
$target_user = $this_user;
$target_uid  = $uid;

if (DOLOGOUT($target_user) != 0) {
    PAGEHEADER("Logout");
    echo "<center><h3>Logout '$target_uid' failed!</h3></center>\n";
    PAGEFOOTER();
    return;
}

#
# Success. Zap the user back to the front page, in nonsecure mode, or a page
# the caller specified
# 
if (isset($next_page)) {
    header("Location: $next_page");
} else {
    header("Location: $TBBASE/");
}
?>


