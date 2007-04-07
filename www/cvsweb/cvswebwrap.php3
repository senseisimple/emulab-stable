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

# Must be logged in.
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Form the real url.
#
$newurl  = preg_replace("/cvswebwrap/", "cvsweb", $_SERVER['REQUEST_URI']);
$newurl = preg_replace("/php3/","php3/",$newurl);

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
