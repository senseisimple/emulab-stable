<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006 University of Utah and the Flux Group.
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
	      CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER|
	      CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);

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
    DBQueryFatal("select status,wikionly from users ".
		 "where uid='$uid'");

if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Database Error retrieving status for $uid!", 1);
}
$status   = $row[0];
$wikionly = $row[1];

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
    $usr_addr    = stripslashes($row[usr_addr]);
    $usr_addr2	 = stripslashes($row[usr_addr2]);
    $usr_city	 = stripslashes($row[usr_city]);
    $usr_state	 = stripslashes($row[usr_state]);
    $usr_zip	 = stripslashes($row[usr_zip]);
    $usr_country = stripslashes($row[usr_country]);
    $usr_name    = stripslashes($row[usr_name]);
    $usr_phone   = $row[usr_phone];
    $usr_title   = stripslashes($row[usr_title]);
    $usr_affil   = stripslashes($row[usr_affil]);

     while ($row = mysql_fetch_array($group_result)) {
	 $pid = $row[pid];
	 $gid = $row[gid];

	 TBProjLeader($pid, $projleader_uid);
	 TBGroupLeader($pid, $gid, $grpleader_uid);

	 if (!strcmp($projleader_uid, $uid)) {
	     #
	     # Project leader verifying himself. 
	     # 
	     TBUserInfo($projleader_uid, $leader_name, $leader_email);
	     TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

	     $projinfo_result =
		 DBQueryFatal("select * from projects where pid='$pid'");

	     $row		= mysql_fetch_array($projinfo_result);
	     $proj_name		= stripslashes($row[name]);
	     $proj_URL          = $row[URL];
	     $proj_funders      = stripslashes($row[funders]);
	     $proj_public       = ($row[public] ? "Yes" : "No");
	     $proj_linked       = ($row[linked_to_us] ? "Yes" : "No");
	     $proj_whynotpublic = stripslashes($row[public_whynot]);
	     $proj_members      = $row[num_members];
	     $proj_pcs          = $row[num_pcs];
	     $proj_plabpcs      = $row[num_pcplab];
	     $proj_ronpcs       = $row[num_ron];
	     $proj_why		= stripslashes($row[why]);
	     $proj_expires      = $row[expires];
	     
	     TBMAIL($TBMAIL_APPROVAL,
		"New Project '$pid' ($uid)",
		"'$usr_name' wants to start project '$pid'.\n".
		"\n".
		"Name:            $usr_name ($uid)\n".
		"Returning User?: No\n".
		"Email:           $usr_email\n".
		"User URL:        $usr_URL\n".
		"Project:         $proj_name\n".
		"Expires:         $proj_expires\n".
		"Project URL:     $proj_URL\n".
		"Public URL:      $proj_public\n".
		"Why Not Public:  $proj_whynotpublic\n".
		"Linked to Us?:   $proj_linked\n".
		"Funders:         $proj_funders\n".
		"Job Title:       $usr_title\n".
		"Affiliation:     $usr_affil\n".
		"Address 1:       $usr_addr\n".
		"Address 2:       $usr_addr2\n".
		"City:            $usr_city\n".
		"State:           $usr_state\n".
		"ZIP/Postal Code: $usr_zip\n".
		"Country:         $usr_country\n".
		"Phone:           $usr_phone\n".
		"Members:         $proj_members\n".
		"PCs:             $proj_pcs\n".
		"Planetlab PCs:   $proj_plabpcs\n".
		"RON PCs:         $proj_ronpcs\n".
		"Unix GID:        $unix_name ($unix_gid)\n".
		"Reasons:\n$proj_why\n\n".
		"Please review the application and when you have made \n".
		"a decision, go to $TBWWW and\n".
		"select the 'Project Approval' page.\n\n".
		"They are expecting a result within 72 hours.\n", 
		"From: $usr_name '$uid' <$usr_email>\n".
		"Reply-To: $TBMAIL_APPROVAL\n".
		"Errors-To: $TBMAIL_WWW");
	 }
	 else {
	     TBUserInfo($grpleader_uid, $leader_name, $leader_email);
	     $allleaders = TBLeaderMailList($pid,$gid);
	     
	     TBMAIL("$leader_name '$grpleader_uid' <$leader_email>",
		"$uid $pid Project Join Request",
		"$usr_name is trying to join your group $gid in project $pid.".
		"\n".
		"\n".
		"Contact Info:\n".
		"Name:            $usr_name\n".
		"Emulab ID:       $uid\n".
		"Email:           $usr_email\n".
		"User URL:        $usr_URL\n".
		"Job Title:       $usr_title\n".
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
    $newstatus = ($wikionly ? "active" : "unapproved");
    
    DBQueryFatal("update users set status='$newstatus' where uid='$uid'");

    TBMAIL($TBMAIL_AUDIT,
	   "User '$uid' has been verified",
	   "\n".
	   "User '$uid' has been verified.\n".
           "Status has been changed from 'newuser' to '$newstatus'\n".
	   "\n".
	   "Testbed Operations\n",
	   "From: $TBMAIL_OPS\n".
	   "Errors-To: $TBMAIL_WWW");

    if ($wikionly) {
	#
	# For wikionly accounts, build the account now.
	# Just builds the wiki account of course (nothing else).
	#
	SUEXEC("nobody", $TBADMINGROUP, "webtbacct add $uid",
	       SUEXEC_ACTION_DIE);

	#
	# The backend sets the actual WikiName
	# 
	$query_result =
	    DBQueryFatal("select wikiname from users where uid='$uid'");

	if (($row = mysql_fetch_row($query_result)) == 0) {
	    TBERROR("Database Error retrieving status for $uid!", 1);
	}
	$wikiname = $row[0];

	echo "You have been verified. You may now access the Wiki at<br>".
	    "<a href='$WIKIURL/$wikiname'>$WIKIURL/$wikiname</a>\n";
    }
    else {
	INFORMLEADERS($uid);

	echo "<p>".
	     "You have now been verified. However, your application ".
	    "has not yet been approved. You will receive ".
	    "email when that has been done.\n";
    }
}
else {
    TBERROR("Bad user status '$status' for $uid!", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

