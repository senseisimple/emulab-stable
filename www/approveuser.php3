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

$projectchecks = array();

ignore_user_abort(1);

#
# Walk the list of post variables, looking for the special post format.
# See approveuser_form.php3:
#
#             uid     menu     project/group
#	name=stoller$$approval-testbed/testbed value=approved,denied,postpone
#	name=stoller$$trust-testbed/testbed value=user,local_root
#
# We make two passes over the post vars. The first does a sanity check so
# that we can bail out without doing anything. This allows the user to
# back up and make changes without worrying about some stuff being done and
# other stuff not. 
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
    # Verify an actual user that is being approved.
    #
    if (! TBCurrentUser($user)) {
	TBERROR("Trying to approve unknown user $user.", 1);
    }
    
    #
    # Check that the current uid has the necessary trust level
    # to approver users in the project/group. Also, only project leaders
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
    
    #
    # Check if already approved in the project/group. If already an
    # approved member, something went wrong.
    #
    TBGroupMember($user, $project, $group, $isapproved);
    if ($isapproved) {
	USERERROR("$user is already an approved member of ".
		  "$project/$group!", 1);
    }

    #
    # Verify approval value. 
    #
    if (strcmp($approval, "postpone") &&
	strcmp($approval, "deny") &&
	strcmp($approval, "nuke") &&
	strcmp($approval, "approve")) {
	TBERROR("Invalid approval value $approval in approveuser.php3.", 1);
    }

    #
    # If denying project membership, then there must be equivalent denial
    # for all subgroups. We can either do it for the user, or require that the
    # user understand whats happening. I prefer the latter, so look for this
    # and spit back an error. Note that we cannot rely on the post vars for
    # this, but must look in the DB for the group set, and then check to make
    # sure there are post vars for *all* of them.
    #
    if (strcmp($project, $group) == 0 &&
	(strcmp($approval, "deny") == 0 ||
	 strcmp($approval, "nuke") == 0)) {
	$query_result =
	    DBQueryFatal("select gid from group_membership ".
			 "where uid='$user' and pid='$project' and pid!=gid");
	
	while ($row = mysql_fetch_array($query_result)) {
	    $gid = $row[gid];

            #
            # Create and indirect through post var for subgroup approval value.
            #
	    $foo = "$user\$\$approval-$project/$gid";
	    $subgroup_approval = $$foo;

	    if (!$subgroup_approval ||
		(strcmp($subgroup_approval, "deny") &&
		 strcmp($subgroup_approval, "nuke"))) {
		USERERROR("If you wish to deny/nuke user $user in project ".
			  "$project then you must deny/nuke in all of the ".
			  "subgroups $user is attempting to join.", 1);
	    }
	}
    }

    if (strcmp($approval, "approve") == 0)
	$projectchecks[$user][] = array($project, $group, $newtrust);

    #
    # When operating on a user for a subgroup, the user must already be in the
    # default group, or there must be an appropriate default group operation
    # in the POST vars. In other words, we do not allow users to be
    # approved/denied/postponed to a subgroup without a default group
    # operation as well. At present, all users must be in the default group
    # in addition to subgroups.
    #
    if (strcmp($project, $group) == 0)
	continue;

    TBGroupMember($user, $project, $project, $isapproved);
    if ($isapproved)
	continue;

    #
    # Create and indirect through post var for project approval value.
    #
    $foo = "$user\$\$approval-$project/$project";
    $default_approval = $$foo;
    
    if (!$default_approval || strcmp($default_approval, "") == 0) {
	USERERROR("You must specify an action for $user in the default group ".
		  "as well as the subgroup!", 1);
    }
    if (strcmp($approval, "approve") == 0 &&
	strcmp($default_approval, "approve")) {
	USERERROR("You cannot approve $user in $project/$group without ".
		  "approval in the default group ($project/$project)!", 1);
    }
}

#
# Sanity check. I hate this stuff.
# 
while (list ($user, $value) = each ($projectchecks)) {
    $projtrust   = array();
    $grouptrust  = array();
    $pidlist     = array();
    
    while (list ($a, $b) = each ($value)) {
	$pid   = $b[0];
	$gid   = $b[1];
	$trust = $b[2];
	$foo   = $projtrust[$pid];
	$bar   = $grouptrust[$pid];

	#echo "$user $pid $gid $trust $foo $bar<br>\n";

	#
	# This looks for different trust levels in different subgroups
	# of the same project. We are only checking the form arguments
	# here; we will do a check against the DB below. 
	# 
	if (strcmp($pid, $gid)) {
	    if (isset($grouptrust[$pid]) &&
		strcmp($grouptrust[$pid], $trust)) {
		USERERROR("User $user may not have different trust levels in ".
			  "different subgroups of $pid!", 1);
	    }
	    $grouptrust[$pid] = $trust;
	}
	else {
	    #
	    # Stash the project default group trust so that we can also
	    # do a consistency check against it.
	    #
	    $projtrust[$pid] = $trust;
	}
	$pidlist[$pid] = $pid;
    }
    
    reset($value);

    while (list ($pid, $foo) = each ($pidlist)) {
	# Skip if no subgroups were being approved.
	if (! isset($grouptrust[$pid]))
	    continue;

	#
	# This does a consistency check against subgroups in the DB.
	# If we are approving to any subgroups in the form submittal,
	# make sure that the user is not in any other subgroups of the
	# project with a different trust level.
	#
	$query_result =
	    DBQueryFatal("select trust from group_membership ".
			 "where uid='$user' and pid='$pid' ".
			 " and pid!=gid and trust!='none' ".
			 " and trust!='$grouptrust[$pid]'");

	if (mysql_num_rows($query_result)) {	    
	    USERERROR("User $user may not have different trust levels in ".
		      "different subgroups of $pid!", 1);
	}

	#
	# This does a level check between the subgroups and the project.
	# Do not allow a higher trust level in the default group than in
	# the subgroups.
	# 
	if (isset($projtrust[$pid]))
	    $ptrust = TBTrustConvert($projtrust[$pid]);
	else
	    $ptrust = TBProjTrust($user, $pid);
	
	$bad = 0;

	$query_result =
	    DBQueryFatal("select trust from group_membership ".
			 "where uid='$user' and trust!='none' ".
			 " and pid='$pid' and gid!=pid");

	while ($row = mysql_fetch_array($query_result)) {
	    if ($ptrust > TBTrustConvert($row[0])) {
		$bad = 1;
		break;
	    }
	}
	#echo "F $user $bad $ptrust $pid $grouptrust[$pid]<br>\n";

	if ($bad ||
	    $ptrust > TBTrustConvert($grouptrust[$pid])) {
	    USERERROR("User $user may not have a higher trust level in ".
		      "the default group of $pid, than in a subgroup!", 1);
	}
    }
}

reset($HTTP_POST_VARS);

#
# Okay, all sanity tests passed for all post vars. Now do the actual work.
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

    #
    # Corresponding trust value.
    #
    $foo      = "$user\$\$trust-$project/$group";
    $newtrust = $$foo;

    #
    # Get the current status for the user, which we might need to change.
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
    # Email info for current user.
    # 
    TBUserInfo($uid, $uid_name, $uid_email);

    #
    # Email info for the group leader too.
    #
    TBGroupLeader($project, $group, $groupleader);
    TBUserInfo($groupleader, $phead_name, $phead_email);
    
    #
    # Well, looks like everything is okay. Change the project membership
    # value appropriately.
    #
    if (strcmp($approval, "postpone") == 0) {
	echo "<p>
                  Membership status for user $user in $project/$group was
                  <b>postponed</b> for later decision.\n";
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

        TBMAIL("$user_name '$user' <$user_email>",
             "Membership Denied in '$project/$group'",
	     "\n".
             "This message is to notify you that you have been denied\n".
	     "membership in project/group $project/$group.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $uid_name <$uid_email>\n".
             "Cc:  $phead_name <$phead_email>\n".
             "Bcc: $TBMAIL_AUDIT\n".
             "Errors-To: $TBMAIL_WWW");

	echo "<p>
                User $user was <b>denied</b> membership in $project/$group.
                <br>
                The user will need to reapply again if this was in error.\n";

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
	    echo "<p>
                  User $user was <b>denied</b> membership in $project/$group.
                  <br>
                  Since the user is a member (or requesting membership)
		  in other projects, the account cannot be safely removed.\n";
	    
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
	    echo "<p>
                  User $user was <b>denied</b> membership in $project/$group.
                  <br>
                  Since the user has been approved by, or was active in other
		  projects in the past, the account cannot be safely removed.
                  \n";
	    continue;
	}
	
	$query_result = DBQueryFatal("delete FROM users where uid='$user'");
	
	echo "<p>
                User $user was <b>denied</b> membership in $project/$group.
                <br>
		The account has also been <b>terminated</b> with prejudice!\n";

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
	    elseif (strcmp($curstatus, "unverified") == 0) {
		$newstatus = "unverified";
	    }
	    else {
	        TBERROR("Invalid $user status $curstatus in approveuser.php3",
                         1);
	    }
	    $query_result =
		DBQueryFatal("UPDATE users set status='$newstatus' ".
			     "WHERE uid='$user'");
	}

        TBMAIL("$user_name '$user' <$user_email>",
             "Membership Approved in '$project/$group' ",
	     "\n".
	     "This message is to notify you that you have been approved\n".
	     "as a member of project/group $project/$group with\n".
	     "$newtrust permissions.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $uid_name <$uid_email>\n".
             "Cc:  $phead_name <$phead_email>\n".
             "Bcc: $TBMAIL_AUDIT\n".
             "Errors-To: $TBMAIL_WWW");

	echo "<p>
                  User $user was <b>granted</b> membership in $project/$group
                  with $newtrust permissions.\n";

	#
        # Create user account on control node.
        #
	SUEXEC($uid, "flux", "webmkacct -a $user", 0);
	SUEXEC($uid, "flux", "websetgroups $user", 0);
		
	continue;
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
