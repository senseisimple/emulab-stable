<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

#
# This page gets loaded as the result of a logout click.
#
# $uid optionally comes in as a variable so admins can logout other users.
#
$user      = $_GET['user'];
$next_page = $_GET['next_page'];

# Pedantic page argument checking.
if (isset($user) && ($user == "" || !User::ValidWebID($user))) {
    PAGEARGERROR("Illegal characters in '$user'");
}

# Get current login.
# Only admin users can logout someone other then themself.
$this_user = CheckLoginOrDie(CHECKLOGIN_MODMASK);
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (isset($user)) {
    if (! ($target_user = User::Lookup($user))) {
	PAGEHEADER("Logout");
	USERERROR("The user $user is not a valid user", 1);
	PAGEFOOTER();
	return;
    }
    $target_uid = $target_user->uid();
    
    if (!$isadmin && !$target_user->SameUser($this_user)) {
	PAGEHEADER("Logout");
	echo "<center>
                  <h3>You do not have permission to logout '$target_uid'</h3>
              </center>\n";
	PAGEFOOTER();
	return;
    }
}
else {
    $target_user = $this_user;
    $target_uid  = $uid;
}

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
if ($next_page) {
    header("Location: $next_page");
} else {
    header("Location: $TBBASE/");
}
?>


