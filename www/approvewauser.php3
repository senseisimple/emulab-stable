<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can be verified.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("Only testbed administrators people can access this page!", 1);
}
ignore_user_abort(1);

#
# Walk the list of post variables, looking for the special post format.
# See approvewauser_form.php3:
#
#             uid     menu     node_id
#	name=stoller$$approval-node_id value=approved,denied,postpone
#	name=stoller$$trust-node_id value=user,local_root
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
    $node_id  = substr($approval_string, strlen("\$\$approval-"));
    $approval = $value;

    if (!$user || strcmp($user, "") == 0) {
	TBERROR("Parse error finding user in approvewauser.php3", 1);
    }
    if (!$node_id || strcmp($node_id, "") == 0) {
	TBERROR("Parse error finding node_id in approvewauser.php3", 1);
    }
    if (!$approval || strcmp($approval, "") == 0) {
	TBERROR("Parse error finding approval in approvewauser.php3", 1);
    }

    #
    # There should be a corresponding trust variable in the POST vars.
    # Note that we construct the variable name and indirect to it.
    #
    $foo      = "$user\$\$trust-$node_id";
    $newtrust = $$foo;
    if (!$newtrust || strcmp($newtrust, "") == 0) {
	TBERROR("Parse error finding trust in approvewauser.php3", 1);
    }
    #echo "User $user, NodeID $node_id,
    #      Approval $approval, Trust $newtrust<br>\n";
    
    if (strcmp($newtrust, "user") &&
	strcmp($newtrust, "local_root")) {
	TBERROR("Invalid trust $newtrust for user $user approvewauser.php3.",
		1);
    }

    #
    # Verify an actual user that is being approved.
    #
    if (! ($target_user = User::Lookup($user))) {
	TBERROR("Trying to approve unknown user $user.", 1);
    }
    
    #
    # Check if already approved. If already an approved account,
    # something went wrong.
    #
    $query_result =
	DBQueryFatal("select trust from widearea_accounts ".
		     "where uid='$user' and node_id='$node_id' and ".
		     "      trust!='none'");
    if (mysql_num_rows($query_result)) {
	$row   = mysql_fetch_array($query_result);
	$trust = $row[trust];
	
	USERERROR("$user is already approved on $node_id with $trust!", 1);
    }

    #
    # Verify approval value. 
    #
    if (strcmp($approval, "postpone") &&
	strcmp($approval, "deny") &&
	strcmp($approval, "nuke") &&
	strcmp($approval, "approve")) {
	TBERROR("Invalid approval value $approval in approvewauser.php3.", 1);
    }
}

#
# Standard Testbed Header
#
PAGEHEADER("Widearea Accounts Approval Form");

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
    $node_id  = substr($approval_string, strlen("\$\$approval-"));
    $approval = $value;

    #
    # Corresponding trust value.
    #
    $foo      = "$user\$\$trust-$node_id";
    $newtrust = $$foo;

    #
    # Get the current status for the user, which we might need to change.
    #
    # We change the status only if this person is getting a new account.
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
    #echo "Status = $curstatus, Email = $user_email<br>\n";

    #
    # Email info for current user.
    #
    $uid_name  = $this_user->name();
    $uid_email = $this_user->email();

    #
    # Well, looks like everything is okay. Change the project membership
    # value appropriately.
    #
    if (strcmp($approval, "postpone") == 0) {
	echo "<p>
                  Account status for user $user was
                  <b>postponed</b> for later decision.\n";
        continue;
    }
    if (strcmp($approval, "deny") == 0) {
        #
        # Must delete the widearea_account record since we require that the 
        # user reapply once denied. Send the luser email to let him know.
        #
        $query_result =
	    DBQueryFatal("delete from widearea_accounts ".
			 "where uid='$user' and node_id='$node_id'");

        TBMAIL("$user_name '$user' <$user_email>",
             "Account request on $node_id denied",
	     "\n".
             "This message is to notify you that you have been denied\n".
	     "local account access on $node_id!\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Operations\n",
             "From: $uid_name <$uid_email>\n".
	     "Cc: $TBMAIL_OPS\n".
             "Bcc: $TBMAIL_AUDIT\n".
             "Errors-To: $TBMAIL_WWW");

	echo "<p>
                User $user was <b>denied</b> an account on $node_id.
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
	    DBQueryFatal("delete from widearea_accounts ".
			 "where uid='$user' and node_id='$node_id'");

	#
	# See if user is in any other projects (even unapproved).
	#
	$project_list = $target_user->ProjectMembershipList();

	#
	# If yes, then we cannot safely delete the user account.
	#
	if (count($project_list)) {
	    echo "<p>
                  User $user was <b>denied</b> an account on $node_id.
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
                  User $user was <b>denied</b> an account on $node_id.
                  <br>
                  Since the user has been approved by, or was active in other
		  projects in the past, the account cannot be safely removed.
                  \n";
	    continue;
	}
	$target_user->Delete();
	
	echo "<p>
                User $user was <b>denied</b> an account on $node_id.
                <br>
		The account has also been <b>terminated</b> with prejudice!\n";

	continue;
    }
    if (strcmp($approval, "approve") == 0) {
        #
        # Change the trust value accordingly.
        #
        $query_result =
	    DBQueryFatal("UPDATE widearea_accounts ".
			 "set trust='$newtrust',date_approved=now() ".
			 "WHERE uid='$user' and node_id='$node_id'");

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
	        TBERROR("Invalid $user status $curstatus!", 1);
	    }
	    $target_user->SetStatus($newstatus);
	}

	$url = CreateURL("showpubkeys", $target_user);

        TBMAIL("$user_name '$user' <$user_email>",
	       "Widearea account granted on '$node_id' ",
	       "\n".
	       "This message is to notify you that you have been granted an\n".
	       "account on $node_id with $newtrust permissions.\n".
	       "\n".
	       "In order to log into this node, you must upload an ssh key\n".
	       "via: ${TBBASE}/$url\n".
	       "\n\n".
	       "Thanks,\n".
	       "Testbed Operations\n",
	       "From: $uid_name <$uid_email>\n".
	       "Cc:  $TBMAIL_OPS\n".
	       "Bcc: $TBMAIL_AUDIT\n".
	       "Errors-To: $TBMAIL_WWW");

	echo "<p>
                  User $user was <b>granted</b> an account on $node_id
                  with $newtrust permissions.\n";

	#
	# XXX
	#
	DBQueryFatal("update nodes set update_accounts=update_accounts+1 ".
		     "where node_id='$node_id' and update_accounts<2");
		
	continue;
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
