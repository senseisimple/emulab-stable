<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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
$target_uid = $_GET['target_uid'];

# Pedantic page argument checking. Good practice!
if (isset($target_uid) && $target_uid == "") {
    PAGEARGERROR();
}

# Get current login.
# Only admin users can logout someone other then themself.
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

if (!isset($target_uid))
    $target_uid = $uid;

if ($target_uid != $uid && !ISADMIN($uid)) {
    PAGEHEADER("Logout");
    echo "<center>
          <h3>You do not have permission to logout '$target_uid'
          </h3></center>\n";
    PAGEFOOTER();
    return;
}

if (DOLOGOUT($target_uid) != 0) {
    PAGEHEADER("Logout");
    echo "<center><h3>Logout '$target_uid' failed!</h3></center>\n";
    PAGEFOOTER();
    return;
}

#
# Success. Zap the user back to the front page, in nonsecure mode.
# 
header("Location: $TBBASE/");
?>


