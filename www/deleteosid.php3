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
$isadmin = ISADMIN($uid);

#
# Must provide the OSID!
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
  USERERROR("The OSID was not provided!", 1);
}

#
# Check to make sure thats this is a valid OSID.
#
$query_result = mysql_db_query($TBDBNAME,
       "SELECT pid FROM os_info WHERE osid='$osid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The OSID `$osid' is not a valid OSID.", 1);
}
$row = mysql_fetch_array($query_result);
$pid = $row[pid];

#
# Verify that this uid is a member of the project that owns the OSID,
# and that the uid has root permission.
#
if (!$isadmin) {
    if (!isset($pid) || strcmp($pid, "") == 0) {
	USERERROR("You do not have permission to delete OSID `$osid'", 1);
    }

    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\" ".
	"and (trust='local_root' or trust='group_root')");
    
    if (! $query_result) {
	$err = mysql_error();
	TBERROR("Database Error finding project membership: $uid: $err\n", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You do have permission to delete an OSID in ".
		  "project: `$pid'.", 1);
    }
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
$query_result = mysql_db_query($TBDBNAME,
       "DELETE FROM os_info WHERE osid=\"$osid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error deleting OSID `$osid': $err\n", 1);
}

echo "<p>
      <center><h2>
      OSID `$osid' has been deleted!
      </h2></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
