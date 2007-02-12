<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Wrapper Wrapper script for cvsweb.cgi
#
chdir("../");
require("defs.php3");

#
# Make sure that URL args are cleaned.
#
RequiredPageArguments();

#
# We look for anon access, and if so, redirect to ops web server.
# WARNING: See the LOGGEDINORDIE() calls below.
#
$this_user = CheckLogin($check_status);

# Redirect now, to avoid phishing.
if ($this_user) {
    CheckLoginOrDie();
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

?>

<head>
<style type="text/css">

#cvsfr {
  width: 100%;
  height: 800px;
}

</style>
</head>

<div id="cvscon">
<iframe id="cvsfr" scrolling=yes src="<?php echo $newurl ?>" border=0></iframe>
</div>

<?php

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
