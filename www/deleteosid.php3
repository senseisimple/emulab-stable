<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete an OSID");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Must provide the OSID!
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
  USERERROR("The OSID was not provided!", 1);
}

if (! TBValidOSID($osid)) {
    USERERROR("The OSID `$osid' is not a valid OSID.", 1);
}

#
# Verify permission.
#
if (!TBOSIDAccessCheck($uid, $osid, $TB_OSID_DESTROY)) {
    USERERROR("You do not have permission to access OSID $osid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          OSID termination canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to delete OSID `$osid?'
          </h2>\n";
    
    echo "<form action=\"deleteosid.php3\" method=\"post\">";
    echo "<input type=hidden name=osid value=\"$osid\">\n";
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
DBQueryFatal("DELETE FROM os_info WHERE osid='$osid'");

echo "<p>
      <center><h2>
      OSID `$osid' has been deleted!
      </h2></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
