<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show User Information");

#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    USERERROR("You must provide a User ID.", 1);
}

#
# Check to make sure thats this is a valid UID.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM users WHERE uid=\"$target_uid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The user $target_uid is not a valid user", 1);
}

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. 
#
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"select proj_memb.* from proj_memb ".
        "left join proj_memb as foo ".
        "on proj_memb.pid=foo.pid and proj_memb.uid='$target_uid' ".
        "where foo.uid='$uid'");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not in the same Project as $target_uid.", 1);
    }
}

SHOWUSER($target_uid);

echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
