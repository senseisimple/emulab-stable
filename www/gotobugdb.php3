<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$BUGDBSUPPORT) {
    header("Location: index.php3");
    return;
}

# No Pageheader since we spit out a redirection below.
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|
		     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY); # XXX BUGDBONLY ?
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# The project to zap to on the other side
#
if (isset($project_title) && $project_title == "") {
    unset($project_title);
}

if (isset($do) && $do == "") {
    unset($do);
}

#
# Look for our cookie. If the browser has it, then there is nothing
# more to do; just redirect the user over to the bugdb.
#
if (isset($_COOKIE[$BUGDBCOOKIENAME])) {
    $myhash = $_COOKIE[$BUGDBCOOKIENAME];
    
    header("Location: ${BUGDBURL}?username=${uid}&bosscred=${myhash}" .
	   (isset($do) ? "&do=${do}" : "") .
	   (isset($project_title) ? "&project_title=${project_title}" : ""));
    return;
}

#
# Generate a cookie. Send it over to the bugdb server and stash it into
# the users browser for subsequent requests (until logout).
# 
$myhash = GENHASH();

SUEXEC("nobody", "nobody", "bugdbxlogin $uid $myhash", SUEXEC_ACTION_DIE);

$pp_args = "${BUGDBURL}?";
$pp_args .= (isset($do) ? "&do=${do}" : "");
$pp_args .= (isset($project_title) ? "&project_title=${project_title}" : "");

$pp_args = rawurlencode($pp_args);

setcookie($BUGDBCOOKIENAME, $myhash, 0, "/", $TBAUTHDOMAIN, $TBSECURECOOKIES);
header("Location: ${BUGDBURL}?do=authenticate&username=${uid}&bosscred=${myhash}&prev_page=" . $pp_args);
?>
