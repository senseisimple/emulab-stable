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
#             uid     menu     project/group
#	name=stoller$$approval-testbed/testbed value=approved,denied,postpone
#	name=stoller$$trust-testbed/testbed value=user,local_root
# 
while (list ($header, $value) = each ($HTTP_POST_VARS)) {
    #echo "$header: $value<br>\n";

    $approval_string = strstr($header, "\$\$approval-");
    if (! $approval_string) {
	continue;
    }

    $user     = substr($header, 0, strpos($header, "\$\$", 0));
    $projgrp  = substr($approval_string, strlen("\$\$approval-"));
    $project  = substr($projgrp, 0, strpos($projgrp, "/", 0));
    $group    = substr($projgrp, strpos($projgrp, "/", 0) + 1);
    $approval = $value;

    if (!$user || strcmp($user, "") == 0) {
	TBERROR("Parse error finding user in approveuser.php3", 1);
    }
    if (!$project || strcmp($project, "") == 0) {
	TBERROR("Parse error finding project in approveuser.php3", 1);
    }
    if (!$group || strcmp($group, "") == 0) {
	TBERROR("Parse error finding group in approveuser.php3", 1);
    }
    if (!$approval || strcmp($approval, "") == 0) {
	TBERROR("Parse error finding approval in approveuser.php3", 1);
    }

    #
    # There should be a corresponding trust variable in the POST vars.
    # Note that we construct the variable name and indirect to it.
    #
    $foo      = "$user\$\$trust-$project/$group";
    $newtrust = $$foo;
    if (!$newtrust || strcmp($newtrust, "") == 0) {
	TBERROR("Parse error finding trust in approveuser.php3", 1);
    }
    #echo "User $user, Project $project,
    #      Group $group, Approval $approval, Trust $newtrust<br>\n";
    
    if (strcmp($newtrust, "user") &&
	strcmp($newtrust, "local_root") &&
	strcmp($newtrust, "group_root")) {
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
    $query_result =
        DBQueryFatal("SELECT status,usr_email,usr_name from users where ".
		     "uid='$user'");
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
    # to add this user to the project/group. Also, only project leaders
    # can add someone as group_root. This should probably be encoded in
    # the permission stuff.
    #
    if (! TBProjAccessCheck($uid, $project, $group, $TB_PROJECT_ADDUSER)) {
	USERERROR("You are not allowed to approve users in ".
		  "$project/$group!", 1);
    }
    TBProjLeader($project, $projleader);
    if (strcmp($uid, $projleader) &&
	strcmp($newtrust, "group_root") == 0) {
	USERERROR("You do not have permission to add new users with group ".
		  "root status!", 1);
    }
    
    TBUserInfo($uid, $uid_name, $uid_email);

    #
    # If already in the group skip.
    #
    TBGroupMember($user, $project, $group, $isapproved);
    if ($isapproved) {
	continue;
    }

    #
    # Lets get group leader email, just in case the person doing the approval
    # is not the head of the project or group. This is polite to do.
    #
    $query_result =
	DBQueryFatal("SELECT usr_email,usr_name from users as u ".
		     "left join groups as g on g.leader=u.uid ".
		     "where g.pid='$project' and g.gid='$group'");
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
        # Must delete the group_membership record since we require that the 
        # user reapply once denied. Send the luser email to let him know.
        #
        $query_result =
	    DBQueryFatal("delete from group_membership ".
			 "where uid='$user' and pid='$project' and ".
			 "      gid='$group'");

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
        # Must delete the group_membership record since we require that the 
        # user reapply once denied. Send the luser email to let him know.
        #
        $query_result =
	    DBQueryFatal("delete from group_membership ".
			 "where uid='$user' and pid='$project' and ".
			 "      gid='$group'");

	#
	# See if user is in any other projects (even unapproved).
	#
        $query_result =
	    DBQueryFatal("select * from group_membership where uid='$user'");

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
	
	$query_result = DBQueryFatal("delete FROM users where uid='$user'");
	
	echo "<h3><p>
                  User $user was denied membership in project $project.<br>
		  The account has also been terminated with prejudice!
              </h3>\n";

	continue;
    }
    if (strcmp($approval, "approve") == 0) {
        #
        # Change the trust value in group_membership accordingly.
        #
        $query_result =
	    DBQueryFatal("UPDATE group_membership ".
			 "set trust='$newtrust',date_approved=now() ".
			 "WHERE uid='$user' and pid='$project' and ".
			 "      gid='$group'");

	#
	# Messy. If this is a new user joining a subgroup, and that new user
	# is not already in the project, we need to add a second record to
	# the project membership. 
	#
	if (strcmp($project, $group)) {
	    TBGroupMember($user, $project, $project, $isapproved);

	    if (! $isapproved) {
		$query_result =
		    DBQueryFatal("UPDATE group_membership ".
				 "set trust='$newtrust',date_approved=now() ".
				 "WHERE uid='$user' and pid='$project' and ".
				 "      gid='$project'");
	    }
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
	    $query_result =
		DBQueryFatal("UPDATE users set status='$newstatus' ".
			     "WHERE uid='$user'");
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
