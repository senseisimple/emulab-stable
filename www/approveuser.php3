<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Users Approved");

#
# Only known and logged in users can be verified.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Walk the list of post variables, looking for the special post format.
# See approveuser_form.php3:
#
#             uid     menu     project
#	name=stoller$$approval-testbed value=approved,denied,postpone
#	name=stoller$$trust-testbed value=user,local_root
# 
while (list ($header, $value) = each ($HTTP_POST_VARS)) {
    #echo "$header: $value<br>\n";

    $approval_string = strstr($header, "\$\$approval-");
    if (! $approval_string) {
	continue;
    }

    $user     = substr($header, 0, strpos($header, "\$\$", 0));
    $project  = substr($approval_string, strlen("\$\$approval-"));
    $approval = $value;

    if (!$user || strcmp($user, "") == 0) {
	TBERROR("Parse error finding user in approveuser.php3", 1);
    }
    if (!$project || strcmp($project, "") == 0) {
	TBERROR("Parse error finding project in approveuser.php3", 1);
    }
    if (!$approval || strcmp($approval, "") == 0) {
	TBERROR("Parse error finding approval in approveuser.php3", 1);
    }

    #
    # There should be a corresponding trust variable in the POST vars.
    # Note that we construct the variable name and indirect to it.
    #
    $foo      = "$user\$\$trust-$project";
    $newtrust = $$foo;
    if (!$newtrust || strcmp($newtrust, "") == 0) {
	TBERROR("Parse error finding trust in approveuser.php3", 1);
    }
    #echo "User $user,
    #      Project $project, Approval $approval, Trust $newtrust<br>\n";
    if (strcmp($newtrust, "user") && strcmp($newtrust, "local_root")) {
	TBERROR("Invalid trust $newtrust for user $user approveuser.php3.", 1);
    }

    #
    # Get the current status for the user, which we might need to change
    # anyway, and to verify that the user is a valid user. We also need
    # the email address to let user know what happened.
    #
    # We change the status only if this person is joining his first project.
    # In this case, the status will be either "newuser" or "unapproved",
    # and we will change it to "unapproved" or "active", respectively.
    # If the status is "active", we leave it alone. 
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT status,usr_email,usr_name from users where uid='$user'");
    if (! $query_result) {
	TBERROR("Database Error restrieving user status for $user", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	TBERROR("Unknown user $user", 1);
    }
    $row = mysql_fetch_row($query_result);
    $curstatus  = $row[0];
    $user_email = $row[1];
    $user_name  = $row[2];
    #echo "Status = $curstatus, Email = $user_email<br>\n";

    #
    # We need to check that the current uid has the necessary trust level
    # to add this user to the project.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT trust from proj_memb where uid='$uid' and pid='$project'");
    if (! $query_result) {
	TBERROR("Database Error retrieving trust for $uid in $project", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You are not allowed to add users to project $project.", 1);
    }
    $row = mysql_fetch_row($query_result);
    $uidtrust = $row[0];
    if (strcmp($uidtrust, "group_root")) {
	USERERROR("You are not allowed to add users to project $project.", 1);
    }

    #
    # Then we check that that user being added really wanted to be in that
    # project, and is not already there with a valid trust value.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT trust from proj_memb where uid='$user' and pid='$project'");
    if (! $query_result) {
	TBERROR("Database Error retrieving trust for $user in $project", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("User $user is not a member of project $project.", 1);
    }
    $row = mysql_fetch_row($query_result);
    $usertrust = $row[0];
    if (strcmp($usertrust, "none")) {
	USERERROR("User $user is already a member of project $project.", 1);
    }

    #
    # Lets get the uid email for the mail messages below, so that we
    # can stick in a proper return address.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_email,usr_name from users where uid='$uid'");
    if (! $query_result) {
	TBERROR("Database Error retrieving user info for $uid", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	TBERROR("Unknown user $uid", 1);
    }
    $row = mysql_fetch_row($query_result);
    $uid_email = $row[0];
    $uid_name  = $row[1];

    #
    # Lets get project head email, just in case the person doing the approval
    # is not the head of the project. This is polite to do.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_email,usr_name from users as u ".
	"left join projects as p on p.head_uid=u.uid where p.pid='$project'");
    if (! $query_result) {
	TBERROR("Database Error retrieving user info for project $project ".
		"leader", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	TBERROR("Retrieving user info for project $project leader", 1);
    }
    $row = mysql_fetch_row($query_result);
    $phead_email = $row[0];
    $phead_name  = $row[1];
   
    #
    # Well, looks like everything is okay. Change the project membership
    # value appropriately.
    #
    if (strcmp($approval, "postpone") == 0) {
	echo "<p><h3>
                  Membership status for user $user was postponed for
                  later decision.
              </h3>\n";
        continue;
    }
    if (strcmp($approval, "deny") == 0) {
        #
        # Must delete the proj_memb record since we require that the user
        # reapply once denied. Send the luser email to let him know.
        #
        $query_result = mysql_db_query($TBDBNAME,
	    "delete from proj_memb where uid='$user' and pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error removing $user from project membership ".
                    "after being denied.", 1);
        }
        mail("$user_name '$user' <$user_email>",
             "TESTBED: Project '$project' Membership Denied",
	     "\n".
             "This message is to notify you that you have been denied\n".
	     "membership in project $project\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $uid_name <$uid_email>\n".
             "Cc:  $phead_name <$phead_email>\n".
             "Bcc: $TBMAIL_APPROVAL\n".
             "Errors-To: $TBMAIL_WWW");

	echo "<h3><p>
                  User $user was denied membership in project $project.
                  The user will need to reapply again if this was in error.
              </h3>\n";

	continue;
    }
    if (strcmp($approval, "nuke") == 0) {
        #
        # Delete the proj_memb record.
        #
        $query_result = mysql_db_query($TBDBNAME,
	    "delete from proj_memb where uid='$user' and pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error removing $user from project membership ".
                    "after being denied.", 1);
        }

	#
	# See if user is in any other projects (even unapproved).
	# 
        $query_result = mysql_db_query($TBDBNAME,
	    "select * from proj_memb where uid='$user'");
        if (! $query_result) {
	    TBERROR("Database Error getting $user from project membership ".
                    "after being nuked", 1);
        }

	#
	# If yes, then we cannot safely delete the user account.
	#
	if (mysql_num_rows($query_result)) {
	    echo "<h3><p>
                  User $user was denied membership in project $project.<br>
                  Since the user is a member (or requesting membership)
		  in other projects, the account cannot be safely removed.
              </h3>\n";
	    
	    continue;
	}

	#
	# No other project membership. If the user is unapproved/newuser, 
	# it means he was never approved in any project, and so will
	# likely not be missed. He will be unapproved if he did his
	# verification.
	#
	if (strcmp($curstatus, "newuser") &&
	    strcmp($curstatus, "unapproved")) {
	    echo "<h3><p>
                  User $user was denied membership in project $project.<br>
                  Since the user has been approved by, or was active in other
		  projects in the past, the account cannot be safely removed.
              </h3>\n";
	    continue;
	}
	
	$query_result = mysql_db_query($TBDBNAME,
		"delete FROM users where uid='$user'");
	if (! $query_result) {
	    TBERROR("Database Error removing $user from users table ".
                    "after being nuked", 1);
	}
	
	echo "<h3><p>
                  User $user was denied membership in project $project.<br>
		  The account has also be terminated with prejudice!
              </h3>\n";

	continue;
    }
    if (strcmp($approval, "approve") == 0) {
        #
        # Change the trust value in proj_memb accordingly.
        #
	$date_approved = date("Y:m:d", time());
	
        $query_result = mysql_db_query($TBDBNAME,
	    "UPDATE proj_memb set trust='$newtrust', ".
            "date_approved='$date_approved' ".
	    "WHERE uid='$user' and pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error adding $user to project $project.", 1);
        }

        #
        # Change the status if necessary. This only happens for new
	# users being added to their first project. After this, the status is
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
	        TBERROR("Invalid $user status $curstatus in approveuser.php3",
                         1);
	    }
	    $query_result = mysql_db_query($TBDBNAME,
	        "UPDATE users set status='$newstatus' WHERE uid='$user'");
            if (! $query_result) {
	        TBERROR("Database Error changing $user status to $newstatus.",
                         1);
            }
	}

        mail("$user_name '$user' <$user_email>",
             "TESTBED: Project '$project' Membership Approval",
	     "\n".
	     "This message is to notify you that you have been approved\n".
	     "as a member of project $project with $newtrust permissions.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $uid_name <$uid_email>\n".
             "Cc:  $phead_name <$phead_email>\n".
             "Bcc: $TBMAIL_APPROVAL\n".
             "Errors-To: $TBMAIL_WWW");

	#
        # Create user account on control node.
        #
	SUEXEC($uid, "flux", "mkacct-ctrl $user", 0);

	echo "<h3><p>
                  User $user was granted membership in project $project
                  with $newtrust permissions.
              </h3>\n";

	continue;
    }
    TBERROR("Invalid approval value $approval in approveuser.php3.", 1);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
