<html>
<head>
<title>New Users Approved</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can be verified.
#
$uid = "";
if (ereg("php3\?([[:alnum:]]+)", $REQUEST_URI, $Vals)) {
    $uid=$Vals[1];
    addslashes($uid);
}
else {
    unset($uid);
}
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT admin from users where uid='$uid' and admin='1'" );
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting admin status for $uid: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You do not have admin privledges to approve projects!", 1);
}

echo "<center><h1>
      Project Approval Results
      </h1></center>";

#
# Walk the list of post variables, looking for the special post format.
# See approveproject_form.php3:
#
#            project   option
#	name=testbed$$approval value=approve,deny,postpone
# 
while (list ($header, $value) = each ($HTTP_POST_VARS)) {
    #echo "$header: $value<br>\n";

    $approval_string = strstr($header, "\$\$approval");
    if (! $approval_string) {
	continue;
    }

    $project  = substr($header, 0, strpos($header, "\$\$", 0));
    $approval = $value;

    if (!$project || strcmp($project, "") == 0) {
	TBERROR("Parse error finding project in approveproject.php3", 1);
    }
    if (!$approval || strcmp($approval, "") == 0) {
	TBERROR("Parse error finding approval in approveproject.php3", 1);
    }
    #echo "Project $project, Approval $approval<br>\n";

    #
    # Grab the head_uid for this project. This verifies it is a valid project.
    #
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT head_uid from projects where pid='$project'");
    if (! $query_result) {
	TBERROR("Database Error restrieving project leader for $projecr", 1);
    }
    if (($row = mysql_fetch_row($query_result)) == 0) {
	TBERROR("Unknown project $project", 1);
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
	"SELECT trust from proj_memb where uid='$headuid' and pid='$project'");
    if (! $query_result) {
	TBERROR("Database Error retrieving trust for $headuid in $project", 1);
    }
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("User $headuid is not the leader of project $project.", 1);
    }

    #
    # Well, looks like everything is okay. Change the project approval
    # value appropriately.
    #
    if (strcmp($approval, "postpone") == 0) {
	echo "<p><h3>
                  Project approval for project $project (User: $headuid) was
                  postponed for later decision.
              </h3>\n";
        continue;
    }
    if ((strcmp($approval, "deny") == 0) ||
	(strcmp($approval, "destroy") == 0)) {
        #
        # Must delete the proj_memb and project records since we require a
        # new application once denied. Send the luser email to let him know. 
        #
        $query_result = mysql_db_query($TBDBNAME,
	    "delete from proj_memb where uid='$headuid' and pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error removing project membership record for ".
                    "project $project (user: $headuid) after being denied.",
                    1);
        }
        $query_result = mysql_db_query($TBDBNAME,
	    "delete from projects where pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error removing project record for project ".
                    "project $project (user: $headuid) after being denied.",
                    1);
        }

        mail("$headuid_email",
             "TESTBED: Project Denied",
	     "\n".
             "This message is to notify you that your project application\n".
             "for $project has been denied\n".
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
                        "after project $project was denied(destroyed).", 
                        1);
            }

            mail("$headuid_email",
                 "TESTBED: Account Terminated",
    	         "\n".
                 "This message is to notify you that your account has been \n".
                 "terminated because your project $project was denied\n".
                 "\n\n".
                 "Thanks,\n".
                 "Testbed Ops\n".
                 "Utah Network Testbed\n",
                 "From: $TBMAIL_CONTROL\n".
                 "Cc: $TBMAIL_CONTROL\n".
                 "Errors-To: $TBMAIL_WWW");
        }

	echo "<h3><p>
                  Project $project (User: $headuid) has been denied.
              </h3>\n";

	continue;
    }
    if (strcmp($approval, "approve") == 0) {
        #
        # Change the trust value in proj_memb to group_root, and set the
        # project "approved" field to true. 
        #
        $query_result = mysql_db_query($TBDBNAME,
	    "UPDATE proj_memb set trust='group_root' ".
            "WHERE uid='$headuid' and pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error adding $headuid to project $project.", 1);
        }

        $query_result = mysql_db_query($TBDBNAME,
	    "UPDATE projects set approved='1' WHERE pid='$project'");
        if (! $query_result) {
	    TBERROR("Database Error setting approved field for ".
                    "project $project.", 1);
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
	}

        mail("$headuid_email",
             "TESTBED: Project Approval",
	     "\n".
	     "This message is to notify you that your project $project\n".
	     "has been approved.\n".
             "\n\n".
             "Thanks,\n".
             "Testbed Ops\n".
             "Utah Network Testbed\n",
             "From: $TBMAIL_CONTROL\n".
             "Cc: $TBMAIL_CONTROL\n".
             "Errors-To: $TBMAIL_WWW");
	
	echo "<h3><p>
                  Project $project (User: $headuid) has been approved.
              </h3>\n";

	continue;
    }
    TBERROR("Invalid approval value $approval in approveproject.php3.", 1);
}

?>
</body>
</html>

