<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("OSID Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($osid) ||
    strcmp($osid, "") == 0) {
    USERERROR("You must provide an OSID.", 1);
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
# Verify that this uid is a member of the project that owns the OSID.
#
if (!$isadmin && $pid) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of the project that owns OSID $osid.",
		  1);
    }
}

#
# Dump os_info record.
# 
SHOWOSINFO($osid);

# Terminate option.
if ($pid) {
    echo "<p><center>
           Do you want to remove this OSID?
           <A href='deleteosid.php3?osid=$osid'>Yes</a>
          </center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
