<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Image Descriptor");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("You must provide an ImageID.", 1);
}

if (! TBValidImageID($imageid)) {
    USERERROR("ImageID '$imageid' is not a valid ImageID!", 1);
}

#
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_READINFO)) {
    USERERROR("You do not have permission to access ImageID $imageid.", 1);
}

SUBPAGESTART();
SUBMENUSTART("More Options");
$fooid = rawurlencode($imageid);
WRITESUBMENUBUTTON("Edit this Image Descriptor",
		   "editimageid_form.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Delete this Image Descriptor",
		   "deleteimageid.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_ez.php3");
if ($isadmin) {
    WRITESUBMENUBUTTON("Create a new OS Descriptor",
		       "newosid_form.php3");
}
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

#
# Dump record.
# 
SHOWIMAGEID($imageid, 0);

SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
