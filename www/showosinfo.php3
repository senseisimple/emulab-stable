<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("OS Descriptor Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify form arguments.
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
    USERERROR("You must provide an OSID.", 1);
}

if (! TBValidOSID($osid)) {
    USERERROR("The OS Descriptor '$osid' is not valid!", 1);
}

#
# Verify permission.
#
if (!TBOSIDAccessCheck($uid, $osid, $TB_OSID_READINFO)) {
    USERERROR("You do not have permission to access OS Descriptor $osid!", 1);
}

SUBPAGESTART();
SUBMENUSTART("More Options");
WRITESUBMENUBUTTON("Delete this OS Descriptor",
		   "deleteosid.php3?osid=$osid");
WRITESUBMENUBUTTON("Create a new OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_explain.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
SUBMENUEND();

#
# Dump os_info record.
# 
SHOWOSINFO($osid);

SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
