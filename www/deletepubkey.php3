<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# No PAGEHEADER since we spit out a redirect later.
# 

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid, CHECKLOGIN_USERSTATUS|CHECKLOGIN_WEBONLY);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($target_uid) ||
    strcmp($target_uid, "") == 0) {
    USERERROR("Improper form arguments!", 1);
}
if (!isset($key) ||
    strcmp($key, "") == 0) {
    USERERROR("Improper form arguments!", 1);
}

#
# Check to make sure thats this is a valid UID.
#
if (! TBCurrentUser($target_uid)) {
    USERERROR("The user $target_uid is not a valid user", 1);
}

#
# Verify that this uid is a member of one of the projects that the
# target_uid is in. Must have proper permission in that group too. 
#
if (!$isadmin &&
    strcmp($uid, $target_uid)) {

    if (! TBUserInfoAccessCheck($uid, $target_uid, $TB_USERINFO_MODIFYINFO)) {
	USERERROR("You do not have permission to change ${user}'s keys!", 1);
    }
}

#
# Get the actual key.
#
$query_result =
    DBQueryFatal("select * from user_pubkeys ".
		 "where uid='$target_uid' and comment='$key'");

if (! mysql_num_rows($query_result)) {
    USERERROR("Public Key '$key' for user '$target_uid' does not exist!", 1);
}

$row    = mysql_fetch_array($query_result);
$pubkey = $row[pubkey];
$chunky = chunk_split($pubkey, 70, "<br>\n");

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    PAGEHEADER("SSH Public Key Maintenance");
    
    echo "<center><h2><br>
          SSH Public Key deletion canceled!
          </h2></center>\n";

    echo "<br>
          Back to <a href='showpubkeys.php3?target_uid=$target_uid'>
                 ssh public keys</a> for user '$uid'.\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    PAGEHEADER("SSH Public Key Maintenance");

    echo "<center><h3><br>
          Are you <b>REALLY</b>
          sure you want to delete this SSH Public Key for user '$target_uid'?
          </h3>\n";
    
    echo "<form action='deletepubkey.php3?target_uid=$target_uid&key=$key'
                method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    echo "<table align=center border=1 cellpadding=2 cellspacing=2>
           <tr>
              <td>$chunky</td>
           </tr>
          </table>\n";
    
    PAGEFOOTER();
    return;
}

#
# Audit
#
TBUserInfo($uid, $uid_name, $uid_email);
TBUserInfo($target_uid, $targuid_name, $targuid_email);

TBMAIL("$targuid_name <$targuid_email>",
     "SSH Public Key for '$target_uid' Deleted",
     "\n".
     "SSH Public Key for '$target_uid' deleted by '$uid'.\n".
     "\n".
     "$chunky\n".
     "\n".
     "Thanks,\n".
     "Testbed Operations\n",
     "From: $uid_name <$uid_email>\n".
     "Cc: $TBMAIL_AUDIT\n".
     "Errors-To: $TBMAIL_WWW");

DBQueryFatal("delete from user_pubkeys ".
	     "where uid='$target_uid' and comment='$key'");

DBQueryFatal("update users set usr_modified=now() ".
	     "where uid='$target_uid'");

#
# mkacct updates the user pubkeys.
# 
MKACCT($uid, "webmkacct $target_uid");

header("Location: showpubkeys.php3?target_uid=$target_uid");

?>
