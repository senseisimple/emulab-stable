<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$WIKISUPPORT) {
    header("Location: index.php3");
    return;
}

# No Pageheader since we spit out a redirection below.
$this_user = CheckLoginOrDie(CHECKLOGIN_USERSTATUS|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();

#
# The page to zap to on the other side
#
if (isset($redurl) && $redurl == "") {
    unset($redurl);
}

#
# Look for our wikicookie. If the browser has it, then there is nothing
# more to do; just redirect the user over to the wiki.
#
if (isset($_COOKIE[$WIKICOOKIENAME])) {
    $wikihash = $_COOKIE[$WIKICOOKIENAME];
    
    header("Location: ${WIKIURL}?username=${uid}&bosscred=${wikihash}" .
	   (isset($redurl) ? "&redurl=${redurl}" : ""));
    return;
}

#
# Generate a cookie. Send it over to the wiki server and stash it into
# the users browser for subsequent requests (until logout).
# 
$wikihash = GENHASH();

SUEXEC("nobody", "nobody", "wikixlogin $uid $wikihash", SUEXEC_ACTION_DIE);

setcookie($WIKICOOKIENAME, $wikihash, 0, "/", $TBAUTHDOMAIN, $TBSECURECOOKIES);
header("Location: ${WIKIURL}?username=${uid}&bosscred=${wikihash}" .
	   (isset($redurl) ? "&redurl=${redurl}" : ""));
?>
