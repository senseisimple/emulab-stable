<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Beware empty spaces (cookies)!
# 
require("defs.php3");

#
# This page gets loaded as the result of a logout click.
#
# $uid comes in as a variable.
#
if (isset($uid) && strcmp($uid, "")) {
    DOLOGOUT($uid);
    unset($uid);

    #
    # Zap the user back to the front page, in nonsecure mode.
    # 
    header("Location: $TBBASE/");
    return;
}

#
# Standard Testbed Header
#
PAGEHEADER("Logout");

echo "<center><h3>Logout attempt failed!</h3></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
