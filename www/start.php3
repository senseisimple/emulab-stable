<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
#
# Beware empty spaces (cookies)!
#
# The point of this mess is to redirect people from the top level 
# index page (ie: no page specified) to the most appropriate page.
#
# 1. If no UID cookie comes in from the browser, just redirect to the
#    to the index page, maintaining secure/insecure mode. That is, if the
#    page was accessed via https, use https when redirecting to the index
#    page.
#
# 2. If a UID cookie comes in, check to see if the user is logged in. If
#    so, redirect to his user information page. If the user is logged in,
#    but came in with http, then we cannot actually confirm it (MAYBEVALID),
#    since the hash cookie will not be sent along, so we redirect back using
#    https so that we can confirm it (hash cookie sent). If it turns out that
#    the user is not logged in, fall back to #1 above.
# 
require("defs.php3");

if (($uid = GETUID())) {
    $check_status = CHECKLOGIN($uid) & CHECKLOGIN_STATUSMASK;

    if ($check_status == CHECKLOGIN_LOGGEDIN) {
	$LOC = "$TBBASE/showuser.php3?target_uid=$uid";
    }
    elseif (isset($SSL_PROTOCOL)) {
	$LOC = "$TBBASE/index.php3";
    }
    elseif ($check_status == CHECKLOGIN_MAYBEVALID) {
	$LOC = "$TBBASE/start.php3";
    }
    else 
	$LOC = "$TBDOCBASE/index.php3";
}
elseif (isset($SSL_PROTOCOL)) {
    $LOC = "$TBBASE/index.php3";
}
else {
    $LOC = "$TBDOCBASE/index.php3";
}

header("Location: $LOC");
?>
