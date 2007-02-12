<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Users Approved");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();

#
# The reason for this call is to make sure that globals are set properly.
#
$reqargs = RequiredPageArguments();

# Local used below.
$projectchecks = array();

# Hmm, is this needed?
ignore_user_abort(1);

#
# Walk the list of post variables, looking for the special post format.
# See approveuser_form.php3:
#
#             uid     menu     project/group
#	name=Uxxxx$$approval-testbed/testbed value=approved,denied,postpone
#	name=Uxxxx$$trust-testbed/testbed value=user,local_root
#
# We make two passes over the post vars. The first does a sanity check so
# that we can bail out without doing anything. This allows the user to
# back up and make changes without worrying about some stuff being done and
# other stuff not. 
#

#
# copy of HTTP_POST_VARS we use to do actual work.
# this is so I can insert an implicit default-group approval
# while iterating over HTTP_POST_VARS. 
# A bit kludgey, indeed.
#
$POST_VARS_COPY = array();
 
while (list ($header, $value) = each ($HTTP_POST_VARS)) {
    #echo "$header: $value<br>\n";
    $POST_VARS_COPY[$header] = $value; 

    $approval_string = strstr($header, "\$\$approval-");
    if (! $approval_string) {
	continue;
    }

    $user     = substr($header, 1, strpos($header, "\$\$", 0) - 1);
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
    $foo      = "U${user}\$\$trust-$project/$group";
    #echo "$foo<br>\n";
    
    $newtrust = $HTTP_POST_VARS[$foo];
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
    if (! ($target_user = User::Lookup($user))) {
	TBERROR("Trying to approve unknown user $user.", 1);
    }
    $target_uid = $target_user->uid();

    # Ditto the project.
    if (! ($target_project = Project::Lookup($project))) {
	TBERROR("Trying to approve user into unknown project $project.", 1);
    }

    # Ditto the group.
    if (! ($target_group = Group::LookupByPidGid($project, $group))) {
	TBERROR("Trying to approve user into unknown group $group", 1);
    }
    
    #
    # Check that the current uid has the necessary trust level
    # to approver users in the project/group. Also, only project leaders
    # can add someone to the default group as group_root.
    #
    if (! $target_group->AccessCheck($this_user, $TB_PROJECT_ADDUSER)) {
	USERERROR("You are not allowed to approve users in ".
		  "$project/$group!", 1);
    }

    if ($newtrust == "group_root" && $project == $group &&
	!$target_project->AccessCheck($this_user,
				      $TB_PROJECT_BESTOWGROUPROOT)) {
	USERERROR("You do not have permission to add new users with group ".
		  "root trust to the default group!", 1);
    }
    
    #
    # Check if already approved in the project/group. If already an
    # approved member, something went wrong.
    #
    $target_group->IsMember($target_user, $isapproved);
    if ($isapproved) {
	USERERROR("$target_uid is already an approved member of ".
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

	# List of subgroup membership in this project.
	$grouplist = $target_project->GroupList($target_user);

	foreach ($grouplist as $subgroup) {
	    $gid = $subgroup->gid();

            #
            # Create and indirect through post var for subgroup approval value.
            #
	    $foo = "$user\$\$approval-$project/$gid";
	    $subgroup_approval = $$foo;

	    if (!$subgroup_approval ||
		(strcmp($subgroup_approval, "deny") &&
		 strcmp($subgroup_approval, "nuke"))) {
		USERERROR("If you wish to deny/nuke user $target_uid in ".
			  "project $project then you must deny/nuke in all ".
			  "of the subgroups $target_uid is attempting to ".
			  "join.", 1);
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

    $target_project->IsMember($target_user, $isapproved);
    if ($isapproved)
	continue;

    #
    # Create and indirect through post var for project approval value.
    #
    $foo = "U${user}\$\$approval-$project/$project";
    $bar = "U${user}\$\$trust-$project/$project";
    $default_approval = $HTTP_POST_VARS[$foo];
    
    if (!$default_approval || strcmp($default_approval, "") == 0) {
	# Implicit group approval as user.
	# Short circuit all the perms-checking, and squeeze it in
	# all the appropriate places.
	
	# 1. For our benefit
	$$foo = $approval;
	
	# 2. For the strcmp below.
	$default_approval = $approval;

	# 3. For the sanity check
	$projectchecks[$user][] = array($project, $project, "user");	

	# 4. For the while loop which does the actual work
	$POST_VARS_COPY[ $foo ] = $approval;
	$$bar = "user";
    }
    if (strcmp($approval, "approve") == 0 &&
	strcmp($default_approval, "approve")) {
	USERERROR("You cannot approve $target_uid in $project/$group without ".
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

	if (! ($target_group = Group::LookupByPidGid($pid, $gid))) {
	    TBERROR("Could not find group object for $project/$group", 1);
	}

	if (! ($target_user = User::Lookup($user))) {
	    TBERROR("Could not find user object for $user", 1);
	}
	$target_uid = $target_user->uid();
	
	#
	# This looks for different trust levels in different subgroups
	# of the same project. We are only checking the form arguments
	# here; we will do a check against the DB below. 
	# 
	if (strcmp($pid, $gid)) {
	    if (isset($grouptrust[$pid]) &&
		strcmp($grouptrust[$pid], $trust)) {
		USERERROR("User $target_uid may not have different trust ".
			  "levels in different subgroups of $pid!", 1);
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

	$target_group->CheckTrustConsistency($target_user, $trust, 1);
    }
    
    reset($value);
}

reset($HTTP_POST_VARS);

#
# Okay, all sanity tests passed for all post vars. Now do the actual work.
# 
while (list ($header, $value) = each ($POST_VARS_COPY)) {
    #echo "$header: $value<br>\n";

    $approval_string = strstr($header, "\$\$approval-");
    if (! $approval_string) {
	continue;
    }

    $user     = substr($header, 1, strpos($header, "\$\$", 0) - 1);
    $projgrp  = substr($approval_string, strlen("\$\$approval-"));
    $project  = substr($projgrp, 0, strpos($projgrp, "/", 0));
    $group    = substr($projgrp, strpos($projgrp, "/", 0) + 1);
    $approval = $value;

    #
    # Corresponding trust value.
    #
    $foo      = "U${user}\$\$trust-$project/$group";
    $newtrust = $HTTP_POST_VARS[$foo];

    #
    # Get the current status for the user, which we might need to change.
    #
    # We change the status only if this person is joining his first project.
    # In this case, the status will be either "newuser" or "unapproved",
    # and we will change it to "unapproved" or "active", respectively.
    # If the status is "active", we leave it alone. 
    #
    if (! ($target_user = User::Lookup($user))) {
	TBERROR("Trying to approve unknown user $user.", 1);
    }
    $curstatus  = $target_user->status();
    $user_email = $target_user->email();
    $user_name  = $target_user->name();
    $user_uid   = $target_user->uid();
    #echo "Status = $curstatus, Email = $user_email<br>\n";

    # Ditto the project and group
    if (! ($target_project = Project::Lookup($project))) {
	TBERROR("Trying to approve user into unknown project $project.", 1);
    }
    if (! ($target_group = Group::LookupByPidGid($project, $group))) {
	TBERROR("Trying to approve user into unknown group $group", 1);
    }

    #
    # Email info for current user.
    #
    $uid_name  = $this_user->name();
    $uid_email = $this_user->email();

    #
    # Email info for the proj/group leaders too.
    #
    $leaders = $target_group->LeaderMailList();
    
    #
    # Well, looks like everything is okay. Change the project membership
    # value appropriately.
    #
    if (strcmp($approval, "postpone") == 0) {
	echo "<p>
                  Membership status for user $user_uid in $project/$group was
                  <b>postponed</b> for later decision.\n";
        continue;
    }
    if (strcmp($approval, "deny") == 0) {
        #
        # Must delete the group_membership record since we require that the 
        # user reapply once denied. Send the luser email to let him know.
        #
	$target_group->DeleteMember($target_user);

        TBMAIL("$user_name '$user_uid' <$user_email>",
             "Membership Denied in '$project/$group'",
	     "\n".
             "This message is to notify you that you have been denied\n".
	     "membership in project/group $project/$group.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Operations\n",
             "From: $uid_name <$uid_email>\n".
             "Cc:  $leaders\n".
             "Bcc: $TBMAIL_AUDIT\n".
             "Errors-To: $TBMAIL_WWW");

	echo "<p>
                User $user_uid was <b>denied</b> membership in $project/$group.
                <br>
                The user will need to reapply again if this was in error.\n";

	continue;
    }
    if (strcmp($approval, "nuke") == 0) {
        #
        # Must delete the group_membership record since we require that the 
        # user reapply once denied. Send the luser email to let him know.
        #
	$target_group->DeleteMember($target_user);

	#
	# See if user is in any other projects (even unapproved).
	#
	$project_list = $target_user->ProjectMembershipList();

	#
	# If yes, then we cannot safely delete the user account.
	#
	if (count($project_list)) {
	    echo "<p>
                  User $user_uid was <b>denied</b> membership in
                  $project/$group.
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
                  User $user_uid was <b>denied</b> membership in
                  $project/$group.
                  <br>
                  Since the user has been approved by, or was active in other
		  projects in the past, the account cannot be safely removed.
                  \n";
	    continue;
	}
	SUEXEC($uid, $TBADMINGROUP, "webrmuser -n -p $project $user_uid", 1); 

	echo "<p>
                User $user_uid was <b>denied</b> membership in $project/$group.
                <br>
		The account has also been <b>terminated</b>!\n";

	continue;
    }
    if (strcmp($approval, "approve") == 0) {
        #
        # Change the status if necessary. This only happens for new
	# users being added to their first project. After this, the status is
        # going to be "active", and we just leave it that way.
	#
	if ($curstatus != TBDB_USERSTATUS_ACTIVE) {
	    if ($curstatus == TBDB_USERSTATUS_UNAPPROVED) {
		$target_user->SetStatus(TBDB_USERSTATUS_ACTIVE);
	    }
	    else {
	        TBERROR("Invalid $user status $curstatus", 1);
	    }
	    if (!($user_interface =
		  $target_project->default_user_interface())) {
		$user_interface = TBDB_USER_INTERFACE_EMULAB;
	    }
	    $target_user->SetUserInterface($user_interface);

            #
            # Create user account on control node.
            #
	    SUEXEC($uid, $TBADMINGROUP, "webtbacct add $user_uid", 1);
	}
        #
	# Only need to add new membership.
	# 
	SUEXEC($uid, $TBADMINGROUP,
	       "webmodgroups -a $project:$group:$newtrust $user_uid", 1);

	echo "<p>
                  User $user_uid was <b>granted</b> membership in
                  $project/$group with $newtrust permissions.\n";
		
	continue;
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
