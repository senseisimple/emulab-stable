<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();

# Verify page arguments.
$reqargs = RequiredPageArguments("project",  PAGEARG_PROJECT,
				 "approval", PAGEARG_STRING);
$optargs = OptionalPageArguments("head_uid", PAGEARG_STRING,
				 "user_interface", PAGEARG_STRING,
				 "message", PAGEARG_ANYTHING,
				 "silent", PAGEARG_BOOLEAN,
				 "pcplab_okay", PAGEARG_BOOLEAN,
				 "ron_okay", PAGEARG_BOOLEAN);

$sendemail = 1;
if (isset($silent) && $silent) {
    $sendemail = 0;
}

#
# Standard Testbed Header
#
PAGEHEADER("New Project Approved");

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN();
if (! $isadmin) {
    USERERROR("You do not have admin privileges to approve projects!", 1);
}

#
# See if we are in an initial Emulab setup.
#
$FirstInitState = (TBGetFirstInitState() == "approveproject");

#
# Grab the head_uid for this project. This verifies it is a valid project.
#
if (! ($this_project = $project)) {
    TBERROR("Unknown project", 1);
}
# For error messages.
$pid = $this_project->pid();

echo "<center><h2>
      Approving Project '$pid' ...
      </h2></center>";

if (! ($leader = $this_project->GetLeader())) {
    TBERROR("Error getting leader for $pid", 1);
}
$headuid = $this_project->head_uid();

#
# If the user wanted to change the head uid, do that now (we change both
# the head_uid and the leader of the default project)
#
if ($approval == "approve" && isset($head_uid) && $head_uid != "") {
    if (! ($newleader = User::Lookup($head_uid))) {
	TBERROR("Unknown user $head_uid", 1);
    }
    if ($this_project->ChangeLeader($newleader) < 0) {
	TBERROR("Error changing leader to $head_uid", 1);
    }
    $leader  = $newleader;
    $headuid = $head_uid;
}

if (!isset($user_interface) ||
    !in_array($user_interface, $TBDB_USER_INTERFACE_LIST)) {
    $user_interface = TBDB_USER_INTERFACE_EMULAB;
}

#
# Get the current status for the headuid, which we might need to change
# anyway, and to verify that the user is a valid user. We also need
# the email address to let the user know what happened.
#
# We change the status only if this person is starting his first project.
# In this case, the status will be either "newuser" or "unapproved",
# and we will change it to "unapproved" or "active", respectively.
# If the status is "active", we leave it alone. 
#
$curstatus     = $leader->status();
$headuid_email = $leader->email();
$headname      = $leader->name();
#$headidx       = $leader->uid_idx();
#echo "Status = $curstatus, Email = $headuid_email<br>\n";

#
# Then we check that the headuid is really listed in the group_membership
# table (default group), just to be sure. 
#
if (! $this_project->IsMember($leader, $ignore)) {
    USERERROR("User $headuid is not the leader of project $pid.", 1);
}

#
# Well, looks like everything is okay. Change the project approval
# value appropriately.
#
if ($approval == "postpone") {
    if (isset($message) && $message != "") {
	echo "<table class=stealth align=center border=0>";
	echo "<tr><td class=stealth>";
	echo "You requested postponement for $pid, but there is a ".
	    "message in the text box which will vanish. If that is ".
	    "not what you intended, the Back button below will give you ".
	    "another chance, with text intact";
	echo "</td></tr>";
	echo "<tr><td class=stealth align=center>";
	echo "<form action='approveproject_form.php3?project=$pid'
               method=post>";

	if (isset($head_uid)) {
	    echo "<input type=hidden name=head_uid value=$head_uid>\n";
	}
	echo "<input type=hidden name=user_interface value=$user_interface>\n";
	echo "<input type=hidden name=silent value=$silent>\n";
	echo "<input type=hidden name=pcplab_okay value=$pcplab_okay>\n";
	echo "<input type=hidden name=ron_okay value=$ron_okay>\n";
	echo "<input type=hidden name=message value='".
	    htmlspecialchars($message, ENT_QUOTES) . "'>\n";
    
	echo "<b><input type=submit name=back value=Back></b>\n";
	echo "</form>\n";
	echo "</td></tr></table>";
	PAGEFOOTER();
	return;
    }
    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed for later decision.
          </h3>\n";
}
elseif (strcmp($approval, "moreinfo") == 0) {
    SendProjAdminMail
        ($pid, "ADMIN", "$headname '$headuid' <$headuid_email>",
         "Project '$pid' Approval Postponed",
         "\n".
         "This message is to notify you that your project application\n".
         "for $pid has been postponed until we have more information\n".
         "or you take certain actions.  You can just reply to this message\n".
         "to provide that information or report your actions.\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Operations\n");

    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed pending the reception of more information.
          </h3>\n";
}
elseif ((strcmp($approval, "deny") == 0) ||
	(strcmp($approval, "destroy") == 0)) {
    
    #
    # If the "destroy" option was given, kill the users account.
    #
    if (strcmp($approval, "destroy") == 0) {
	#
	# Take the user out of the project group first.
	#
	SUEXEC($uid, $TBADMINGROUP, "webmodgroups -r $pid:$pid $headuid", 1);

	#
	# See if user is in any other projects (even unapproved).
	#
	$project_list = $leader->ProjectMembershipList();

	#
	# If yes, then we cannot safely delete the user account.
	#
	if (count($project_list)) {
	    echo "<p>
                  User $headuid was <b>denied</b> starting project $pid.
                  <br>
                  Since the user is a member (or requesting membership)
		  in other projects, the account cannot be safely removed.
		  <br>\n";
	}
	else {
	    #
	    # No other project membership. If the user is unapproved/newuser,
	    # it means he was never approved in any project, and so will
	    # likely not be missed. He will be unapproved if he did his
	    # verification.
	    #
	    if (strcmp($curstatus, "newuser") &&
		strcmp($curstatus, "unapproved")) {
		echo "<p>
		      User $headuid was <b>denied</b> starting project $pid.
		      <br>
		      Since the user has been approved by, or was active in other
		      projects in the past, the account cannot be safely removed.
		      \n";
	    }
	    else {
		SUEXEC($uid, $TBADMINGROUP, "webrmuser -n -p $pid $headuid", 1); 

		if ($sendemail) {
		    TBMAIL("$headname '$headuid' <$headuid_email>",
			   "Account '$headuid' Terminated",
			   "\n".
			   "This message is to notify you that your account has \n".
			   "been terminated because your project $pid was denied.\n".
			   "\n\n".
			   "Thanks,\n".
			   "Testbed Operations\n",
			   "From: $TBMAIL_APPROVAL\n".
			   "Bcc: $TBMAIL_APPROVAL\n".
			   "Errors-To: $TBMAIL_WWW");
		}
		echo "<h3><p>
			User $headuid was <b>denied</b> starting project $pid.
			<br>
			The account has also been <b>terminated</b>!
		      </h3>\n";
	    }
	}
    }
    else {
	echo "<h3><p>
		  Project $pid (User: $headuid) has been denied.
	      </h3>\n";
    }

    SUEXEC($uid, $TBADMINGROUP, "webrmproj $pid", 1);

    if ($sendemail) {
	TBMAIL("$headname '$headuid' <$headuid_email>",
	       "Project '$pid' Denied",
	       "\n".
	       "This message is to notify you that your project application\n".
	       "for $pid has been denied.\n".
	       "\n$message".
	       "\n\n".
	       "Thanks,\n".
	       "Testbed Operations\n",
	       "From: $TBMAIL_APPROVAL\n".
	       "Bcc: $TBMAIL_APPROVAL\n".
	       "Errors-To: $TBMAIL_WWW");
    }

}
elseif (strcmp($approval, "approve") == 0) {
    $optargs = "";
    
    # Sanity check the leader status.
    if ($curstatus != TBDB_USERSTATUS_ACTIVE &&
	$curstatus != TBDB_USERSTATUS_UNAPPROVED) {
	TBERROR("Invalid $headuid status $curstatus", 1);
    }
    # Why is this here?
    $leader->SetUserInterface($user_interface);

    #
    # XXX
    # Temporary Plab hack.
    #
    $pcremote_ok = array();
    if (isset($pcplab_okay) &&
	!strcmp($pcplab_okay, "Yep")) {
	    $pcremote_ok[] = "pcplabphys";
    }
    # RON implies pcwa too.
    if (isset($ron_okay) &&
	!strcmp($ron_okay, "Yep")) {
	    $pcremote_ok[] = "pcron";
	    $pcremote_ok[] = "pcwa";
    }
    if (count($pcremote_ok)) {
	    $foo = implode(",", $pcremote_ok);
	    $this_project->SetRemoteOK($foo);
    }

    unset($tmpfname);
    if (isset($message)) {
	$tmpfname = tempnam("/tmp", "approveproj");
	$fp = fopen($tmpfname, "w");
	fwrite($fp, $message);
	fclose($fp);
	
	$optargs = " -f " . escapeshellarg($tmpfname);
    }

    #
    # Invoke the script. This does it all. If it fails, we will find out
    # about it.
    #
    STARTBUSY("Project '$pid' is being created");
    
    $retval = SUEXEC($uid, $TBADMINGROUP, "webmkproj $optargs $pid",
		     SUEXEC_ACTION_IGNORE);

    CLEARBUSY();

    if (isset($tmpfname)) {
	unlink($tmpfname);
    }
    if ($retval) {
	# Lets tack the message onto the output so we have a record.
	if (isset($message)) {
	    $suexec_output .= "\n\n*** Saved approval message text:\n\n";
	    $suexec_output .= $message;
	}
	SUEXECERROR(SUEXEC_ACTION_DIE);
	return;
    }

    if (!$FirstInitState) {
	sleep(1);
	PAGEREPLACE(CreateURL("showproject", $this_project));
    }
    else {
	echo "<br><br><font size=+1>\n";
	echo "Congratulations! You have successfully setup your initial Emulab
              Project. You should now ".
	      "<a href=login.php3?vuid=$headuid>login</a>
              using the account you just
              created so that you can continue setting up your new Emulab!
              </font><br>\n";

        #
        # Move to next phase. 
        # 
        TBSetFirstInitState("Ready");
    }
}
else {
    TBERROR("Invalid approval value $approval in approveproject.php3.", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
