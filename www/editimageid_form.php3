<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit ImageID Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Verify form arguments.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("You must provide an ImageID.", 1);
}

if (!TBValidImageID($imageid)) {
    USERERROR("The ImageID $imageid is not a valid ImageID.", 1);
}

#
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_MODIFYINFO)) {
    USERERROR("You do not have permission to access ImageID $imageid.", 1);
}

SHOWIMAGEID($imageid, 1);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
