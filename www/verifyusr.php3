<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Confirm Verification");

#
# Only known and logged in users can be verified. 
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid, CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER);

#
# Must provide the key!
# 
if (!isset($key) || strcmp($key, "") == 0) {
    USERERROR("Missing field; ".
              "Please go back and fill out the \"key\" field!", 1);
}

#
# Grab the status and do the modification.
#
$query_result =
    DBQueryFatal("select status from users where uid='$uid'");

if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Database Error retrieving status for $uid!", 1);
}
$status = $row[0];

#
# No multiple verifications!
# 
if (! strcmp($status, TBDB_USERSTATUS_ACTIVE) ||
    ! strcmp($status, TBDB_USERSTATUS_UNAPPROVED)) {
    USERERROR("You have already been verified. If you did not perform ".
	      "this verification, please notify Testbed Operations.", 1);
}

#
# The user is logged in, so all we need to do is confirm the key.
# Make sure it matches.
#
$keymatch = GENKEY($uid);

if (strcmp($key, $keymatch)) {
    USERERROR("The given key \"$key\" is incorrect. ".
	      "Please enter the correct key.", 1);
}

if (strcmp($status, TBDB_USERSTATUS_UNVERIFIED) == 0) {
    $query_result = mysql_db_query($TBDBNAME,
	"update users set status='active' where uid='$uid'");
    if (!$query_result) {
        $err = mysql_error();
        TBERROR("Database Error setting status for $uid: $err\n", 1);
    }

    TBMAIL($TBMAIL_AUDIT,
	   "User '$uid' has been verified",
	   "\n".
	   "User '$uid' has been verified.\n".
           "Status has been changed from 'unverified' to 'active'\n".
	   "\n".
	   "Testbed Ops\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");

    echo "<p>".
         "Because your project leader has already approved ".
	 "your membership in the project, you are now an active user ".
	 "of emulab.  Click on the 'Home' link at your left, and any options ".
	 "that are now available to you will appear.\n";
}
elseif (strcmp($status, TBDB_USERSTATUS_NEWUSER) == 0) {
    $query_result = mysql_db_query($TBDBNAME,
	"update users set status='unapproved' where uid='$uid'");
    if (!$query_result) {
        $err = mysql_error();
        TBERROR("Database Error setting status for $uid: $err\n", 1);
    }

    TBMAIL($TBMAIL_AUDIT,
	   "User '$uid' has been verified",
	   "\n".
	   "User '$uid' has been verified.\n".
           "Status has been changed from 'newuser' to 'unapproved'\n".
	   "\n".
	   "Testbed Ops\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");

    echo "<p>".
	 "You have now been verified. However, your application ".
	 "has not yet been approved by the project leader. You will receive ".
	 "email when that has been done.\n";
}
else {
    TBERROR("Bad user status '$status' for $uid!", 1);
}
    

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

