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
$this_user = CheckLoginOrDie(CHECKLOGIN_UNVERIFIED|CHECKLOGIN_NEWUSER|
			     CHECKLOGIN_WEBONLY|CHECKLOGIN_WIKIONLY);
$uid       = $this_user->uid();

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
$status   = $this_user->status();
$wikionly = $this_user->wikionly();

#
# No multiple verifications.
# 
if (! strcmp($status, TBDB_USERSTATUS_ACTIVE) ||
    ! strcmp($status, TBDB_USERSTATUS_UNAPPROVED)) {
    USERERROR("You have already been verified. If you did not perform ".
	      "this verification, please notify Testbed Operations.", 1);
}

#
# The user is logged in, so all we need to do is confirm the key.
#
if ($key != $this_user->verify_key()) {
    USERERROR("The given key \"$key\" is incorrect. ".
	      "Please enter the correct key.", 1);
}

function INFORMLEADERS($this_user) {
    global $TBWWW, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;

    #
    # Grab user info.
    #
    $usr_email   = $this_user->email();
    $usr_URL     = $this_user->URL();
    $usr_addr    = $this_user->addr();
    $usr_addr2	 = $this_user->addr2();
    $usr_city	 = $this_user->city();
    $usr_state	 = $this_user->state();
    $usr_zip	 = $this_user->zip();
    $usr_country = $this_user->country();
    $usr_name    = $this_user->name();
    $usr_phone   = $this_user->phone();
    $usr_title   = $this_user->title();
    $usr_affil   = $this_user->affil();
    $uid_idx     = $this_user->uid_idx();
    $uid         = $this_user->uid();

    #
    # Get the list of all project/groups this users has tried to join
    # but whose membership messages where delayed until the user verified
    # himself.
    #
    $group_result =
	DBQueryFatal("select gid_idx from group_membership ".
		     "where uid_idx='$uid_idx' and trust='none'");
				     
     while ($row = mysql_fetch_array($group_result)) {
	 $gid_idx = $row["gid_idx"];

	 if (! ($group = Group::Lookup($gid_idx))) {
	     TBERROR("Could not find group record for $gid_idx", 1);
	 }
	 $project     = $group->Project();
	 $projleader  = $project->GetLeader();
	 $groupleader = $group->GetLeader();
	 $pid         = $project->pid();
	 $gid         = $group->gid();

	 if ($this_user->SameUser($projleader)) {
	     #
	     # Project leader verifying himself. 
	     # 
	     $proj_name		= $project->name();
	     $proj_URL          = $project->URL();
	     $proj_funders      = $project->funders();
	     $proj_public       = ($project->public() ? "Yes" : "No");
	     $proj_linked       = ($project->linked_to_us() ? "Yes" : "No");
	     $proj_whynotpublic = $project->public_whynot();
	     $proj_members      = $project->num_members();
	     $proj_pcs          = $project->num_pcs();
	     $proj_plabpcs      = $project->num_pcplab();
	     $proj_ronpcs       = $project->num_ron();
	     $proj_why		= $project->why();
	     $unix_gid          = $group->unix_gid();
	     $unix_name         = $group->unix_name();
	     
	     TBMAIL($TBMAIL_APPROVAL,
		"New Project '$pid' ($uid)",
		"'$usr_name' wants to start project '$pid'.\n".
		"\n".
		"Name:            $usr_name ($uid)\n".
		"Returning User?: No\n".
		"Email:           $usr_email\n".
		"User URL:        $usr_URL\n".
		"Project:         $proj_name\n".
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
	     $leader_name  = $groupleader->name();
	     $leader_email = $groupleader->email();
	     $leader_uid   = $groupleader->uid();
	     $allleaders   = TBLeaderMailList($pid,$gid);
	     
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
    $this_user->SetStatus(TBDB_USERSTATUS_ACTIVE());
    
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
    $newstatus = ($wikionly ?
		  TBDB_USERSTATUS_ACTIVE() : TBDB_USERSTATUS_UNAPPROVED());
    
    $this_user->SetStatus(TBDB_USERSTATUS_UNAPPROVED());

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
	$this_user->Refresh();
	$wikiname = $this_user->wikiname();

	echo "You have been verified. You may now access the Wiki at<br>".
	    "<a href='$WIKIURL/$wikiname'>$WIKIURL/$wikiname</a>\n";
    }
    else {
	INFORMLEADERS($this_user);

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

