<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

# No Pageheader since we spit out a redirection below.

#
# We must get the redirection arguments,
#
$reqargs = RequiredPageArguments("redirect_to", PAGEARG_STRING);

#
# Check format. Also figure out the target.
#
if (! preg_match("/^http[s]?:\/\/([-\w\.]*)\//", $redirect_to, $matches)) {
    PAGEARGERROR("Invalid redirection argument!");
}
$redirect_host = $matches[1];

#
# Right now all we allow is www.datapository.net, and that is really
# nfs.emulab.net.
#
if ($redirect_host != "www.datapository.net" &&
    $redirect_host != "nfs.emulab.net") {
    PAGEARGERROR("Invalid redirection host '$redirect_host'");
}

#
# Okay, now see if the user is logged in. If not, the user will be
# be brought back here after logging in.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Generate a cookie. 
# 
$authhash = GENHASH();

#
# Send it over to the server where it will save it.
#
SUEXEC("nobody", "nobody", "xlogin $redirect_host $uid $authhash",
       SUEXEC_ACTION_DIE);

#
# Now redirect the user over, passing along the hash in the URL.
# 
header("Location: ${redirect_to}?user=${uid}&auth=${authhash}");

?>
