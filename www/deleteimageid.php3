<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete an ImageID");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Must provide the ImageID!
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
  USERERROR("The ImageID was not provided!", 1);
}

if (! TBValidImageID($imageid)) {
    USERERROR("The ImageID `$imageid' is not a valid ImageID.", 1);
}

#
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_DESTROY)) {
    USERERROR("You do not have permission to destroy ImageID $imageid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          ImageID termination canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to delete ImageID `$imageid?'
          </h2>\n";
    
    echo "<form action='deleteimageid.php3' method=post>";
    echo "<input type=hidden name=imageid value='$imageid'>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Delete the record,
#
DBQueryFatal("DELETE FROM images WHERE imageid='$imageid'");

echo "<p>
      <center><h2>
      ImageID `$imageid' has been deleted!
      </h2></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
