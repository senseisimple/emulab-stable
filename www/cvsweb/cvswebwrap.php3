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

# Must be logged in.
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
#
$optargs = OptionalPageArguments("template",   PAGEARG_TEMPLATE,
				 "project",    PAGEARG_PROJECT,
				 "embedded",   PAGEARG_BOOLEAN);
if (!isset($embedded)) {
    $embedded = 0;
}

#
# Form the real url.
#
$newurl  = preg_replace("/cvswebwrap/", "cvsweb", $_SERVER['REQUEST_URI']);
$newurl = preg_replace("/php3/","php3/",$newurl);

#
# Standard Testbed Header
#
PAGEHEADER("Emulab CVS Repository");

if (isset($project)) {
    ;
}
elseif (isset($template)) {
    echo $template->PageHeader();
}

echo "<div><iframe src='$newurl' class='outputframe' ".
	"id='outputframe' name='outputframe'></iframe></div>\n";
echo "</center><br>\n";

echo "<script type='text/javascript' language='javascript'>\n";
echo "SetupOutputArea('outputframe', false);\n"; 
echo "</script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
