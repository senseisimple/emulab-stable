<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit Group Membership");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

ignore_user_abort(1);

#
# First off, sanity check page args.
#
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("Must provide a Project ID!", 1);
}
if (!isset($gid) ||
    strcmp($gid, "") == 0) {
    USERERROR("Must privide a Group ID!", 1);
}

#
# The default group membership cannot be changed, but the trust levels can.
#
$defaultgroup = 0;
if (strcmp($gid, $pid) == 0) {
    $defaultgroup = 1;
}

#
# Verify permission. 
#
if (! TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_EDITGROUP)) {
    USERERROR("You do not have permission to edit group $gid in ".
	      "project $pid!", 1);
}

#
# Grab the current user list for the group. The group leader cannot be
# removed! Neither can the project leader.
#
$curmembers_result =
    DBQueryFatal("select distinct m.uid from group_membership as m ".
		 "left join groups as g on g.pid=m.pid and g.gid=m.gid ".
		 "left join projects as p on p.pid=m.pid ".
		 "where m.pid='$pid' and m.gid='$gid' and ".
		 "      m.uid!=g.leader and m.uid!=p.head_uid");

#
# Grab the user list from the project. These are the people who can be
# added. Do not include people in the above list, obviously!
# 
$nonmembers_result =
    DBQueryFatal("select m.uid from group_membership as m ".
		 "left join group_membership as a on ".
		 "     a.uid=m.uid and a.pid=m.pid and a.gid='$gid' ".
		 "where m.pid='$pid' and m.gid=m.pid and a.uid is NULL");

function TBCheckTrustConsistency($user, $pid, $gid, $newtrust)
{
    #
    # If changing default group trust level, then compare levels.
    # A user may not have greater permission in the default group than
    # in a subgroup.
    #
    if (strcmp($pid, $gid)) {
	$projtrust = TBProjTrust($user, $pid);
	if ($projtrust > TBTrustConvert($newtrust)) {
	    USERERROR("User $user may not have a higher trust level in ".
		      "the default group of $pid, than in subgroup $gid!", 1);
	}
    }
    else
	$projtrust = TBTrustConvert($newtrust);

    #
    # Get all the subgroups not equal to the subgroup being changed.
    # 
    $query_result =
	DBQueryFatal("select trust,gid from group_membership ".
		     "where uid='$user' and pid='$pid' and trust!='none' ".
		     " and gid!=pid and gid!='$gid'");

    while ($row = mysql_fetch_array($query_result)) {
	$grptrust = $row[0];
	$ogid     = $row[1];

	if ($projtrust > TBTrustConvert($grptrust)) {
	    USERERROR("User $user may not have a higher trust level in ".
		      "the default group of $pid, than in subgroup $ogid!", 1);
	}

	if (strcmp($pid, $gid)) {
            #
            # Check to make sure new trust is same as all other subgroup trust.
            # 
	    if (strcmp($newtrust, $grptrust)) {
		USERERROR("User $user may not have different trust levels in ".
			  "different subgroups of $pid!", 1);
	    }
	}
    }
    return 1;
}

#
# First pass does checks. Second pass does the real thing. 
#

#
# Go through the list of current members. For each one, check to see if
# the checkbox for that person was checked. If not, delete the person
# from the group membership. Otherwise, look to see if the trust level
# has been changed.
# 
if (mysql_num_rows($curmembers_result)) {
    while ($row = mysql_fetch_array($curmembers_result)) {
	$user = $row[0];
	$foo  = "change_$user";

	#
	# Is member to be deleted?
	# 
	if (!$defaultgroup && !isset($$foo)) {
	    # Yes.
	    continue;
	}

        #
        # There should be a corresponding trust variable in the POST vars.
        # Note that we construct the variable name and indirect to it.
        #
        $foo      = "$user\$\$trust";
	$newtrust = $$foo;
	
	if (!$newtrust || strcmp($newtrust, "") == 0) {
	    TBERROR("Error finding trust for $user in editgroup.php3", 1);
	}

	if (strcmp($newtrust, "user") &&
	    strcmp($newtrust, "local_root") &&
	    strcmp($newtrust, "group_root")) {
	    TBERROR("Invalid trust $newtrust for $user in editgroup.php3.", 1);
	}

	TBCheckTrustConsistency($user, $pid, $gid, $newtrust);
    }
}

#
# Go through the list of non members. For each one, check to see if
# the checkbox for that person was checked. If so, add the person
# to the group membership, with the trust level specified.
# 
if (!$defaultgroup && mysql_num_rows($nonmembers_result)) {
    while ($row = mysql_fetch_array($nonmembers_result)) {
	$user = $row[0];
	$foo  = "add_$user";
	
	if (isset($$foo)) {
            #
            # There should be a corresponding trust variable in the POST vars.
            # Note that we construct the variable name and indirect to it.
            #
            $bar      = "$user\$\$trust";
	    $newtrust = $$bar;
	
	    if (!$newtrust || strcmp($newtrust, "") == 0) {
		TBERROR("Error finding trust for $user in editgroup.php3",
			1);
	    }

	    if (strcmp($newtrust, "user") &&
		strcmp($newtrust, "local_root") &&
		strcmp($newtrust, "group_root")) {
		TBERROR("Invalid trust $newtrust for $user in editgroup.php3.",
			1);
	    }
	
	    TBCheckTrustConsistency($user, $pid, $gid, $newtrust);
	}
    }
}

#
# Now do the second pass, which makes the changes. Record the user IDs
# that are changed so that we can pass that to setgroups below.
#
$modusers = "";

#
# Go through the list of current members. For each one, check to see if
# the checkbox for that person was checked. If not, delete the person
# from the group membership. Otherwise, look to see if the trust level
# has been changed.
#
if (mysql_num_rows($curmembers_result)) {
    mysql_data_seek($curmembers_result, 0);
    
    while ($row = mysql_fetch_array($curmembers_result)) {
	$user = $row[0];
	$foo  = "change_$user";

	if (!$defaultgroup && !isset($$foo)) {
	    DBQueryFatal("delete from group_membership ".
			 "where pid='$pid' and gid='$gid' and uid='$user'");

	    $modusers = "$modusers $user";
	    continue;
	}
        #
        # There should be a corresponding trust variable in the POST vars.
        # Note that we construct the variable name and indirect to it.
        #
        $foo      = "$user\$\$trust";
	$newtrust = $$foo;
	
	DBQueryFatal("update group_membership set trust='$newtrust' ".
		     "where pid='$pid' and gid='$gid' and uid='$user'");
    }
}

#
# Go through the list of non members. For each one, check to see if
# the checkbox for that person was checked. If so, add the person
# to the group membership, with the trust level specified.
# 
if (!$defaultgroup && mysql_num_rows($nonmembers_result)) {
    mysql_data_seek($nonmembers_result, 0);

    while ($row = mysql_fetch_array($nonmembers_result)) {
	$user = $row[0];
	$foo  = "add_$user";
	
	if (isset($$foo)) {
            #
            # There should be a corresponding trust variable in the POST vars.
            # Note that we construct the variable name and indirect to it.
            #
            $bar      = "$user\$\$trust";
	    $newtrust = $$bar;
	
	    DBQueryFatal("insert into group_membership ".
			 "(uid, pid, gid, trust, ".
			 " date_applied,date_approved) ".
			 "values ('$user','$pid','$gid', '$newtrust', ".
			 "        now(), now())");
	    
	    $modusers = "$modusers $user";
	}
    }
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

echo "<br>
      Group '$gid' in project '$pid' is being updated!<br><br>
      This will take a minute or two. <b>Please</b> do not click the Stop
      button during this time. If you do not receive notification within
      a reasonable amount of time, please contact $TBMAILADDR.\n";
flush();

#
# Run the script. This will do the account stuff for all the people
# in the group. This is the same script that gets run when the group
# is first created.
#
SUEXEC($uid, $unix_gid, "websetgroups -a -p $pid $modusers", 1);

echo "<br><br>
      <b>Done!</b>
      <br>\n";

#
# Show it again!
#
SHOWGROUP($pid, $gid);

SHOWGROUPMEMBERS($pid, $gid);

#
# An edit option.
# 
echo "<br><center>
       <A href='editgroup_form.php3?pid=$pid&gid=$gid'>Edit</a> this Group?
      </center>\n";

#
# A delete option, but not for the default group!
#
if (! $defaultgroup) {
    echo "<p>
           <A href='deletegroup.php3?pid=$pid&gid=$gid'>Delete</a> this Group?
          \n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
