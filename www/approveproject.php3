<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Project Approved");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privileges to approve projects!", 1);
}

#
# See if we are in an initial Emulab setup.
#
$FirstInitState = (TBGetFirstInitState() == "approveproject");

echo "<center><h1>
      Approving Project '$pid' ...
      </h1></center>";

#
# Grab the head_uid for this project. This verifies it is a valid project.
#
$query_result = 
    DBQueryFatal("SELECT head_uid from projects where pid='$pid'");
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Unknown project $pid", 1);
}
$headuid = $row[0];

#
# If the user wanted to change the head uid, do that now (we change both
# the head_uid and the leader of the default project)
#
if (isset($head_uid) && strcmp($head_uid,"")) {
    $headuid = $head_uid;
    DBQueryFatal("UPDATE projects set head_uid='$headuid' where pid='$pid'");
    DBQueryFatal("UPDATE groups set leader='$headuid' where pid='$pid' and " .
	    "gid='$pid'");
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
$query_result = 
    DBQueryFatal("SELECT status,usr_email,usr_name from users ".
		 "where uid='$headuid'");
if (mysql_num_rows($query_result) == 0) {
    TBERROR("Unknown user $headuid", 1);
}
$row = mysql_fetch_row($query_result);
$curstatus     = $row[0];
$headuid_email = $row[1];
$headname      = $row[2];
#echo "Status = $curstatus, Email = $headuid_email<br>\n";

#
# Then we check that the headuid is really listed in the group_membership
# table (default group), just to be sure. 
#
$query_result =
    DBQueryFatal("SELECT trust from group_membership where ".
		 "uid='$headuid' and pid='$pid' and gid='$pid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("User $headuid is not the leader of project $pid.", 1);
}

#
# Well, looks like everything is okay. Change the project approval
# value appropriately.
#
if (strcmp($approval, "postpone") == 0) {
    if (isset($message) && strcmp($message, "")) {
	USERERROR("You requested postponement for $pid, but there is a ".
		  "message in the text box which will vanish. If that is ".
		  "not what you intended, the back button will give you ".
		  "another chance, with text intact.", 1);
    }
    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed for later decision.
          </h3>\n";
}
elseif (strcmp($approval, "moreinfo") == 0) {
    TBMAIL("$headname '$headuid' <$headuid_email>",
         "Project '$pid' Approval Postponed",
         "\n".
         "This message is to notify you that your project application\n".
         "for $pid has been postponed until we have more information\n".
         "or you take certain actions.  You can just reply to this message\n".
         "to provide that information or report your actions.\n".
         "You can just reply to this message to provide more information.\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Operations\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_APPROVAL\n".
         "Errors-To: $TBMAIL_WWW");

    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed pending the reception of more information.
          </h3>\n";
}
elseif ((strcmp($approval, "deny") == 0) ||
	(strcmp($approval, "annihilate") == 0) ||
	(strcmp($approval, "destroy") == 0)) {
    SUEXEC($uid, $TBADMINGROUP, "rmproj $pid", 1);
    
    if (strcmp($approval, "annihilate")) {
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

    #
    # Well, if the "destroy" option was given, kill the users account.
    #
    if ((strcmp($approval, "annihilate") == 0) ||
	(strcmp($approval, "destroy") == 0)) {
	SUEXEC($uid, $TBADMINGROUP, "webrmuser $headuid", 1); 
	
	if (strcmp($approval, "annihilate")) {
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
    }

    echo "<h3><p>
              Project $pid (User: $headuid) has been denied.
          </h3>\n";
}
elseif (strcmp($approval, "approve") == 0) {

    #
    # Change the status if necessary. This only happens for new users
    # being approved in their first project. After this, the status is
    # going to be "active", and we just leave it that way.
    #
    if (strcmp($curstatus, "active")) {
        if (strcmp($curstatus, "unapproved") == 0) {
	    $newstatus = "active";
        }
        else {
	    TBERROR("Invalid $headuid status $curstatus in ".
                    "approveproject.php3", 1);
        }
	DBQueryFatal("UPDATE users set status='$newstatus', ".
		     "       user_interface='$user_interface' ".
		     "WHERE uid='$headuid'");
    }

    #
    # Set the project "approved" field to true. 
    #
    DBQueryFatal("update projects set approved='1', ".
		 "       default_user_interface='$user_interface' ".
		 "where pid='$pid'");

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
	    DBQueryFatal("UPDATE projects set pcremote_ok='$foo' ".
			 "WHERE pid='$pid'");
    }

    #
    # Invoke the script. This does it all. If it fails, we will find out
    # about it.
    #
    echo "<br>
          Project '$pid' is being created!<br><br>
          This will take a minute or two. <b>Please</b> do not click the Stop
          button during this time. If you do not receive notification within
          a reasonable amount of time, please contact $TBMAILADDR.\n";
    flush();

    SUEXEC($uid, $TBADMINGROUP, "webmkproj $pid", SUEXEC_ACTION_DIE);

    TBMAIL("$headname '$headuid' <$headuid_email>",
         "Project '$pid' Approval",
         "\n".
	 "This message is to notify you that your project '$pid'\n".
	 "has been approved.  We recommend that you save this link so that\n".
	 "you can send it to people you wish to have join your project.\n".
	 "Otherwise, tell them to go to ${TBBASE} and join it.\n".
	 "\n".
	 "    ${TBBASE}/joinproject.php3?target_pid=$pid\n".
         "\n".
	 "$message\n".
         "\n".
         "Thanks,\n".
         "Testbed Operations\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_APPROVAL\n".
         "Errors-To: $TBMAIL_WWW");

    if (!$FirstInitState) {
	echo "<p><b>
                 Project $pid (User: $headuid) has been approved.
                </b>\n";
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
 	# Freeze the initial user.
        #
        DBQueryFatal("update users set ".
                     "  status='" . TBDB_USERSTATUS_FROZEN . "' ".
	             "where uid='$FIRSTUSER'");

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
