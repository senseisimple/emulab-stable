<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2005 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Wrapper Wrapper script for cvsweb.cgi
#
chdir("../");
require("defs.php3");

#
# We look for anon access, and if so, redirect to ops web server.
# WARNING: See the LOGGEDINORDIE() calls below.
#
$uid = GETLOGIN();

# Redirect now, to avoid phishing.
if ($uid) {
    LOGGEDINORDIE($uid);
}
else {
    $url = $OPSCVSURL . "?cvsroot=$pid";
	
    header("Location: $url");
    return;
}

#
# Form the real url.
#
$newurl = preg_replace("/cvswebwrap/", "cvsweb", $_SERVER['REQUEST_URI']);

#
# Standard Testbed Header
#
PAGEHEADER("Emulab CVS Repository");

echo "<iframe width=100% height=800 scrolling=yes src='$newurl' border=0 ".
     "style=\"width:100%; height:800; border: 0px\"> ".
     "</iframe>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
