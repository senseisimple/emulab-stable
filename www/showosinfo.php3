<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("osinfo_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("OS Descriptor Information");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("osinfo", PAGEARG_OSINFO);

#
# Verify permission.
#
if (!$osinfo->AccessCheck($this_user, $TB_OSID_READINFO)) {
    USERERROR("You do not have permission to access this OS Descriptor!", 1);
}
$osid = $osinfo->osid();

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Delete this OS Descriptor",
		   CreateURL("deleteosid", $osinfo));
if ($isadmin) {
     WRITESUBMENUBUTTON("Create a new OS Descriptor",
			"newosid.php3");
}
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_ez.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

#
# Dump os_info record.
# 
$osinfo->Show();

echo "<h3 align='center'>Experiments using this OS</h3>\n";
$osinfo->ShowExperiments($this_user);
SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
