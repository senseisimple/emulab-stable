<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("ImageID Information");

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
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_READINFO)) {
    USERERROR("You do not have permission to access ImageID $imageid.", 1);
}

#
# Dump record.
# 
SHOWIMAGEID($imageid, 0);

#
# Edit option.
#
if (TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_MODIFYINFO)) {
    $fooid = rawurlencode($imageid);
    echo "<p><center>
               Do you want to edit this ImageID?
              <A href='editimageid_form.php3?imageid=$fooid'>Yes</a>
            </center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
