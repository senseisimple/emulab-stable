<?php
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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT head_uid from projects where pid='$pid'");
if (! $query_result) {
    TBERROR("Database Error restrieving project leader for $pid", 1);
}
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Unknown project $pid", 1);
}
$headuid = $row[0];

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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT status,usr_email from users where uid='$headuid'");
if (! $query_result) {
    TBERROR("Database Error restrieving user status for $headuid", 1);
}
if (mysql_num_rows($query_result) == 0) {
    TBERROR("Unknown user $headuid", 1);
}
$row = mysql_fetch_row($query_result);
$curstatus     = $row[0];
$headuid_email = $row[1];
#echo "Status = $curstatus, Email = $headuid_email<br>\n";

#
# Then we check that the headuid is really listed in the proj_memb
# table, just to be sure.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT trust from proj_memb where uid='$headuid' and pid='$pid'");
if (! $query_result) {
    TBERROR("Database Error retrieving trust for $headuid in $pid", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("User $headuid is not the leader of project $pid.", 1);
}

#
# Well, looks like everything is okay. Change the project approval
# value appropriately.
#
if (strcmp($approval, "postpone") == 0) {
    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed for later decision.
          </h3>\n";
}
elseif (strcmp($approval, "moreinfo") == 0) {
    mail("$headuid_email",
         "TESTBED: Project Approval Postponed",
         "\n".
         "This message is to notify you that your project application\n".
         "for $pid has been postponed until we have more information\n".
         "You can just reply to this message to provide more information\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_CONTROL\n".
         "Errors-To: $TBMAIL_WWW");

    echo "<p><h3>
             Project approval for project $pid (User: $headuid) was
             postponed pending the reception of more information.
          </h3>\n";
}
elseif ((strcmp($approval, "deny") == 0) ||
	(strcmp($approval, "destroy") == 0)) {
    #
    # Must delete the proj_memb and project records since we require a
    # new application once denied. Send the luser email to let him know. 
    #
    $query_result = mysql_db_query($TBDBNAME,
	    "delete from proj_memb where uid='$headuid' and pid='$pid'");
    if (! $query_result) {
        TBERROR("Database Error removing project membership record for ".
                "project $pid (user: $headuid) after being denied.",
                1);
    }
    $query_result = mysql_db_query($TBDBNAME,
	    "delete from projects where pid='$pid'");
    if (! $query_result) {
        TBERROR("Database Error removing project record for project ".
                "project $pid (user: $headuid) after being denied.",
                1);
    }

    mail("$headuid_email",
         "TESTBED: Project Denied",
         "\n".
         "This message is to notify you that your project application\n".
         "for $pid has been denied\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_CONTROL\n".
         "Errors-To: $TBMAIL_WWW");

    #
    # Well, if the "destroy" option was given, kill the users account
    # from the database.
    #
    if (strcmp($approval, "destroy") == 0) {
        $query_result = mysql_db_query($TBDBNAME,
	    "delete from users where uid='$headuid'");
        if (! $query_result) {
	    TBERROR("Database Error removing user record for $headuid ".
                    "after project $pid was denied(destroyed).", 
                    1);
        }

        mail("$headuid_email",
             "TESTBED: Account Terminated",
    	     "\n".
             "This message is to notify you that your account has been \n".
             "terminated because your project $pid was denied\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $TBMAIL_CONTROL\n".
             "Cc: $TBMAIL_CONTROL\n".
             "Errors-To: $TBMAIL_WWW");
    }

    echo "<h3><p>
              Project $pid (User: $headuid) has been denied.
          </h3>\n";
}
elseif (strcmp($approval, "approve") == 0) {
    #
    # Change the trust value in proj_memb to group_root, and set the
    # project "approved" field to true. 
    #
    $query_result = mysql_db_query($TBDBNAME,
	    "UPDATE proj_memb set trust='group_root' ".
            "WHERE uid='$headuid' and pid='$pid'");
    if (! $query_result) {
        TBERROR("Database Error adding $headuid to project $pid.", 1);
    }

    $query_result = mysql_db_query($TBDBNAME,
        "UPDATE projects set approved='1' WHERE pid='$pid'");
    if (! $query_result) {
        TBERROR("Database Error setting approved field for ".
                "project $pid.", 1);
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
        $query_result = mysql_db_query($TBDBNAME,
	    "UPDATE users set status='$newstatus' WHERE uid='$headuid'");
        if (! $query_result) {
            TBERROR("Database Error changing $headuid status to ".
                    "$newstatus.",
                    1);
        }

        #
        # For new leaders, write their email addresses to files to be used for
        # generating messages.
        #
        $fp = fopen($TBLIST_LEADERS, "a");
        if (! $fp) {
            TBERROR("Could not open $TBLIST_LEADERS to add new ".
		    "project leader email: $headuid_email\n", 0);
        }
        else {
            fwrite($fp, "$headuid_email\n");
            fclose($fp);
        }  

        $fp = fopen($TBLIST_USERS, "a");
        if (! $fp) {
            TBERROR("Could not open $TBLIST_USERS to add new ".
		    "project leader email: $headuid_email\n", 0);
        }
        else {
            fwrite($fp, "$headuid_email\n");
            fclose($fp);
        }
    }

    mail("$headuid_email",
         "TESTBED: Project Approval",
         "\n".
	 "This message is to notify you that your project $pid\n".
	 "has been approved.\n".
         "\n$message".
         "\n\n".
         "Thanks,\n".
         "Testbed Ops\n".
         "Utah Network Testbed\n",
         "From: $TBMAIL_CONTROL\n".
         "Cc: $TBMAIL_CONTROL\n".
         "Errors-To: $TBMAIL_WWW");

    #
    # Create the user accounts. Must be done *before* we create the
    # project directory!
    # 
    SUEXEC($uid, "flux", "mkacct-ctrl_wrapper $pid $headuid", 0);	 	
    #
    # Create the project directory. If it fails, we will find out about it.
    #
    SUEXEC($uid, "flux", "mkprojdir_wrapper $pid", 0);

    echo "<h3><p>
              Project $pid (User: $headuid) has been approved.
          </h3>\n";
}
else {
    TBERROR("Invalid approval value $approval in approveproject.php3.", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
