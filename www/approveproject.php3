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
    USERERROR("You do not have admin privledges to approve projects!", 1);
}

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
		  "message in the text box. Is this what you intended?", 1);
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
         "for $pid has been postponed until we have more information.\n".
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
	(strcmp($approval, "destroy") == 0)) {
    #
    # Must delete the group_membership and project records since we require a
    # new application once denied. Send the luser email to let him know.
    # This order is actually important. Release project record last to
    # avoid (incredibly unlikely) name collision with another new project.
    #
    DBQueryFatal("delete from group_membership ".
		 "where uid='$headuid' and pid='$pid' and gid='$pid'");
    DBQueryFatal("delete from groups where pid='$pid' and gid='$pid'");
    DBQueryFatal("delete from projects where pid='$pid'");

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

    #
    # Well, if the "destroy" option was given, kill the users account
    # from the database.
    #
    if (strcmp($approval, "destroy") == 0) {
	DBQueryFatal("delete from users where uid='$headuid'");

        TBMAIL("$headname '$headuid' <$headuid_email>",
             "Account '$headuid' Terminated",
    	     "\n".
             "This message is to notify you that your account has been \n".
             "terminated because your project $pid was denied.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Operations\n",
             "From: $TBMAIL_APPROVAL\n".
             "Bcc: $TBMAIL_APPROVAL\n".
             "Errors-To: $TBMAIL_WWW");
    }

    echo "<h3><p>
              Project $pid (User: $headuid) has been denied.
          </h3>\n";
}
elseif (strcmp($approval, "approve") == 0) {
    #
    # Change the trust value in group_membership to group_root, and set the
    # project "approved" field to true. 
    #
    DBQueryFatal("UPDATE group_membership ".
		 "set trust='project_root',date_approved=now() ".
		 "WHERE uid='$headuid' and pid='$pid' and gid='$pid'");

    DBQueryFatal("UPDATE projects set approved='1' WHERE pid='$pid'");

    #
    # XXX
    # Temporary Plab hack.
    #
    $pcremote_ok = array();
    if (isset($pcplab_okay) &&
	!strcmp($pcplab_okay, "Yep")) {
	    $pcremote_ok[] = "pcplab";
    }
    if (isset($ron_okay) &&
	!strcmp($ron_okay, "Yep")) {
	    $pcremote_ok[] = "pcron";
    }
    if (count($pcremote_ok)) {
	    $foo = implode(",", $pcremote_ok);
	    DBQueryFatal("UPDATE projects set pcremote_ok='$foo' ".
			 "WHERE pid='$pid'");
    }

    #
    # Change the status if necessary. This only happens for new users
    # being approved in their first project. After this, the status is
    # going to be "active", and we just leave it that way.
    #
    if (strcmp($curstatus, "active")) {
        if (strcmp($curstatus, "newuser") == 0) {
	    $newstatus = "unverified";
        }
        elseif (strcmp($curstatus, "unapproved") == 0) {
	    $newstatus = "active";
        }
        else {
	    TBERROR("Invalid $headuid status $curstatus in ".
                    "approveproject.php3", 1);
        }
	DBQueryFatal("UPDATE users set status='$newstatus' ".
		     "WHERE uid='$headuid'");
    }

    TBMAIL("$headname '$headuid' <$headuid_email>",
         "Project '$pid' Approval",
         "\n".
	 "This message is to notify you that your project $pid\n".
	 "has been approved.\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Operations\n",
         "From: $TBMAIL_APPROVAL\n".
         "Bcc: $TBMAIL_APPROVAL\n".
         "Errors-To: $TBMAIL_WWW");

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

    SUEXEC($uid, $TBADMINGROUP, "webmkproj $pid", 1); 

    echo "<p><b>
             Project $pid (User: $headuid) has been approved.
          </b>\n";
}
else {
    TBERROR("Invalid approval value $approval in approveproject.php3.", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
