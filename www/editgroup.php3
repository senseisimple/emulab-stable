<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit Group Membership");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("group", PAGEARG_GROUP);
$optargs = OptionalPageArguments("submit", PAGEARG_STRING);

#
# The default group membership cannot be changed, but the trust levels can.
#
$defaultgroup = $group->IsProjectGroup();

# Need these below;
$pid = $group->pid();
$gid = $group->gid();

#
# Verify permission. 
#
if (! $group->AccessCheck($this_user, $TB_PROJECT_EDITGROUP)) {
    USERERROR("You do not have permission to edit group $gid in ".
	      "project $pid!", 1);
}

#
# See if user is allowed to add non-members to group.
# 
$grabusers = 0;
if ($group->AccessCheck($this_user, $TB_PROJECT_GROUPGRABUSERS)) {
    $grabusers = 1;
}

#
# See if user is allowed to bestow group_root upon members of group.
# 
$bestowgrouproot = 0;
if ($group->AccessCheck($this_user, $TB_PROJECT_BESTOWGROUPROOT)) {
    $bestowgrouproot = 1;
}

#
# Grab the current user list for the group. The group leader cannot be
# removed! Do not include members that have not been approved to main
# group either! This will force them to go through the approval page first.
#
$curmembers = $group->MemberList();

#
# Grab the user list from the project. These are the people who can be
# added. Do not include people in the above list, obviously! Do not
# include members that have not been approved to main group either! This
# will force them to go through the approval page first.
# 
$nonmembers = $group->NonMemberList();

#
# First pass does checks. Second pass does the real thing. 
#

#
# Go through the list of current members. For each one, check to see if
# the checkbox for that person was checked. If not, delete the person
# from the group membership. Otherwise, look to see if the trust level
# has been changed.
# 
if (count($curmembers)) {
    foreach ($curmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$oldtrust   = $target_user->GetTempData();
	$foo        = "change_$target_idx";

	#
	# Is member to be deleted?
	# 
	if (!$defaultgroup && !isset($HTTP_POST_VARS[$foo])) {
            # Yes.
	    continue;
	}

        #
        # There should be a corresponding trust variable in the POST vars.
        # Note that we construct the variable name and indirect to it.
        #
        $foo      = "U${target_idx}\$\$trust";
	$newtrust = $HTTP_POST_VARS[$foo];
	
	if (!$newtrust || strcmp($newtrust, "") == 0) {
	    TBERROR("Error finding trust for $target_uid in editgroup", 1);
	}

	if (strcmp($newtrust, TBDB_TRUSTSTRING_USER) &&
	    strcmp($newtrust, TBDB_TRUSTSTRING_LOCALROOT) &&
	    strcmp($newtrust, TBDB_TRUSTSTRING_GROUPROOT)) {
	    TBERROR("Invalid trust $newtrust for $target_uid in editgroup", 1);
	}

	#
	# If the user is attempting to bestow group_root on a user who 
	# did not previously have group_root, check to see if the operation is
	# permitted.
	#
	if (strcmp($newtrust, $oldtrust) &&
	    !strcmp($newtrust, TBDB_TRUSTSTRING_GROUPROOT) && 
	    !$bestowgrouproot) {
	    USERERROR("You do not have permission to bestow group root".
		      "trust to users in $pid/$gid!", 1 );
	}

	$group->CheckTrustConsistency($target_user, $newtrust, 1);
    }
    reset($curmembers);
}

#
# Go through the list of non members. For each one, check to see if
# the checkbox for that person was checked. If so, add the person
# to the group membership, with the trust level specified.
# Only do this if user has permission to grab users. 
#

if ($grabusers && !$defaultgroup && count($nonmembers)) {
    foreach ($nonmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$foo        = "add_$target_idx";    
	
	if (isset($HTTP_POST_VARS[$foo]) && $HTTP_POST_VARS[$foo] == "permit"){
	    #
	    # There should be a corresponding trust variable in the POST vars.
	    # Note that we construct the variable name and indirect to it.
	    #
	    $bar      = "U${target_idx}\$\$trust";
	    $newtrust = $HTTP_POST_VARS[$bar];
	    
	    if (!$newtrust || strcmp($newtrust, "") == 0) {
		TBERROR("Error finding trust for $target_uid", 1);
	    }
	    
	    if (strcmp($newtrust, TBDB_TRUSTSTRING_USER) &&
		strcmp($newtrust, TBDB_TRUSTSTRING_LOCALROOT) &&
		strcmp($newtrust, TBDB_TRUSTSTRING_GROUPROOT)) {
		TBERROR("Invalid trust $newtrust for $target_uid", 1);
	    }

	    if (!strcmp($newtrust, TBDB_TRUSTSTRING_GROUPROOT)
		&& !$bestowgrouproot) {
		USERERROR("You do not have permission to bestow group root".
			  "trust to users in $pid/$gid!", 1 );
	    }
	    $group->CheckTrustConsistency($target_user, $newtrust, 1);
	}
    }
    reset($nonmembers);
}

#
# Now do the second pass, which makes the changes. 
#
# Grab the unix GID for running scripts.
#
$unix_gid = $group->unix_gid();

STARTBUSY("Applying group membership changes");

#
# Go through the list of current members. For each one, check to see if
# the checkbox for that person was checked. If not, delete the person
# from the group membership. Otherwise, look to see if the trust level
# has been changed.
#
if (count($curmembers)) {
    foreach ($curmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$oldtrust   = $target_user->GetTempData();
	$foo        = "change_$target_idx";    

	if (!$defaultgroup && !isset($HTTP_POST_VARS[$foo])) {
	    SUEXEC($uid, $unix_gid, "webmodgroups -r $pid:$gid $target_uid",
		   SUEXEC_ACTION_DIE);
	    continue;
	}
        #
        # There should be a corresponding trust variable in the POST vars.
        # Note that we construct the variable name and indirect to it.
        #
        $foo      = "U${target_idx}\$\$trust";
	$newtrust = $HTTP_POST_VARS[$foo];
	
	if (strcmp($oldtrust,$newtrust)) {
	    SUEXEC($uid, $unix_gid,
		   "webmodgroups -m $pid:$gid:$newtrust $target_uid",
		   SUEXEC_ACTION_DIE);
	}
    }
}

#
# Go through the list of non members. For each one, check to see if
# the checkbox for that person was checked. If so, add the person
# to the group membership, with the trust level specified.
# 

if ($grabusers && !$defaultgroup && count($nonmembers)) {
    foreach ($nonmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$foo        = "add_$target_idx";    
	
	if (isset($HTTP_POST_VARS[$foo]) && $HTTP_POST_VARS[$foo] == "permit"){
	    #
	    # There should be a corresponding trust variable in the POST vars.
	    # Note that we construct the variable name and indirect to it.
	    #
	    $bar      = "U${target_idx}\$\$trust";
	    $newtrust = $HTTP_POST_VARS[$bar];

	    SUEXEC($uid, $unix_gid,
		   "webmodgroups -a $pid:$gid:$newtrust $target_uid",
		   SUEXEC_ACTION_DIE);
	}
    }
}

STOPBUSY();

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
# 
PAGEREPLACE(CreateUrl("showgroup", $group));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
