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

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("You must provide an ImageID.", 1);
}

#
# Check to make sure thats this is a valid Image.
#
$query_result = mysql_db_query($TBDBNAME,
       "SELECT pid FROM images WHERE imageid='$imageid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The ImageID `$imageid' is not a valid ImageID.", 1);
}
$row = mysql_fetch_array($query_result);
$pid = $row[pid];

#
# Verify that this uid is a member of the project that owns the IMAGEID.
#
if (!$isadmin && $pid) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of the project that owns ".
		  "ImageID $imageid.", 1);
    }
}

#
# Dump record.
# 
SHOWIMAGEID($imageid, 0);

#
# Edit option, but only if admin or the imageid has a pid. No pid means
# a global imageid, and only admin people can change those.
#
if ($isadmin || $pid) {
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
