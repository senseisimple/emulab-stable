<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Confirm Verification");

#
# Only known and logged in users can be verified. 
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid,
	      CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER|CHECKLOGIN_WEBONLY);

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
$keymatch = TBGetVerificationKey($uid);

if (strcmp($key, $keymatch)) {
    USERERROR("The given key \"$key\" is incorrect. ".
	      "Please enter the correct key.", 1);
}

function INFORMLEADERS($uid) {
    global $TBWWW, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;

    #
    # Get the list of all project/groups this users has tried to join
    # but whose membership messages where delayed until the user verified
    # himself.
    #
    $group_result =
	DBQueryFatal("select * from group_membership ".
		     "where uid='$uid' and trust='none'");

    #
    # Grab user info.
    #
    $userinfo_result =
	DBQueryFatal("select * from users where uid='$uid'");

    $row	 = mysql_fetch_array($userinfo_result);
    $usr_email   = $row[usr_email];
    $usr_URL     = $row[usr_URL];
    $usr_addr    = $row[usr_addr];
    $usr_addr2	 = $row[usr_addr2];
    $usr_city	 = $row[usr_city];
    $usr_state	 = $row[usr_state];
    $usr_zip	 = $row[usr_zip];
    $usr_country = $row[usr_country];
    $usr_name    = $row[usr_name];
    $usr_phone   = $row[usr_phone];
    $usr_title   = $row[usr_title];
    $usr_affil   = $row[usr_affil];

     while ($row = mysql_fetch_array($group_result)) {
	 $pid = $row[pid];
	 $gid = $row[gid];

	 TBGroupLeader($pid, $gid, $leader_uid);
	 TBUserInfo($leader_uid, $leader_name, $leader_email);

	 $allleaders = TBLeaderMailList($pid,$gid);

	 if (strcmp($leader_uid, $uid)) {
	     #
	     # Send email only if this is not the group leader verifying
	     # his account!
	     # 
	     TBMAIL("$leader_name '$leader_uid' <$leader_email>",
		"$uid $pid Project Join Request",
		"$usr_name is trying to join your group $gid in project $pid.".
		"\n".
		"\n".
		"Contact Info:\n".
		"Name:            $usr_name\n".
		"Emulab ID:       $uid\n".
		"Email:           $usr_email\n".
		"User URL:        $usr_URL\n".
		"Title:           $usr_title\n".
		"Affiliation:     $usr_affil\n".
		"Address 1:       $usr_addr\n".
		"Address 2:       $usr_addr2\n".
		"City:            $usr_city\n".
		"State:           $usr_state\n".
		"ZIP/Postal Code: $usr_zip\n".
		"Country:         $usr_country\n".
		"Phone:           $usr_phone\n".
		"\n".
		"Please return to $TBWWW,\n".
		"log in, select the 'New User Approval' page, and enter\n".
		"your decision regarding $usr_name's membership in your\n".
		"project.\n\n".
		"Thanks,\n".
		"Testbed Operations\n",
		"From: $TBMAIL_APPROVAL\n".
		"Cc: $allleaders\n".
		"Bcc: $TBMAIL_AUDIT\n".
		"Errors-To: $TBMAIL_WWW");
	 }
     }
}

if (strcmp($status, TBDB_USERSTATUS_UNVERIFIED) == 0) {
    DBQueryFatal("update users set status='active' where uid='$uid'");

    TBMAIL($TBMAIL_AUDIT,
	   "User '$uid' has been verified",
	   "\n".
	   "User '$uid' has been verified.\n".
           "Status has been changed from 'unverified' to 'active'\n".
	   "\n".
	   "Testbed Operations\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");

    echo "<p>".
         "Because your membership has already been approved, ".
	 "you are now an active user of emulab. ".
	 "Click on the 'Home' link at your left, and any options ".
	 "that are now available to you will appear.\n";
}
elseif (strcmp($status, TBDB_USERSTATUS_NEWUSER) == 0) {
    DBQueryFatal("update users set status='unapproved' where uid='$uid'");

    TBMAIL($TBMAIL_AUDIT,
	   "User '$uid' has been verified",
	   "\n".
	   "User '$uid' has been verified.\n".
           "Status has been changed from 'newuser' to 'unapproved'\n".
	   "\n".
	   "Testbed Operations\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");

    INFORMLEADERS($uid);

    echo "<p>".
	 "You have now been verified. However, your application ".
	 "has not yet been approved. You will receive ".
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

