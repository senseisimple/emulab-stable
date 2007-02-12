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

# Need these below;
$pid = $group->pid();
$gid = $group->gid();

#
# The default group membership cannot be changed, but the trust levels can.
#
$defaultgroup = $group->IsProjectGroup();

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
# Grab the user list for the group. Provide a button selection of people
# that can be removed. The group leader cannot be removed!
# Do not include members that have not been approved
# to main group either! This will force them to go through the approval
# page first.
#
$curmembers =& $group->MemberList();

#
# Grab the user list from the project. These are the people who can be
# added. Do not include people in the above list, obviously! Do not
# include members that have not been approved to main group either! This
# will force them to go through the approval page first.
#
$nonmembers = $group->NonMemberList();

#
# We do not allow the actual group info to be edited. Just the membership.
#
$group->Show();

echo "<br><center>
       Important <a href='docwrapper.php3?docname=groups.html#SECURITY'>
       security issues</a> are discussed in the
       <a href='docwrapper.php3?docname=groups.html'>Groups Tutorial</a>.
      </center>\n";

if (count($curmembers) ||
    ($grabusers && count($nonmembers))) {
    $url = CreateURL("editgroup", $group);
    echo "<br>
          <form action='$url' method=post>
          <table align=center border=1>\n";
}

if (count($curmembers)) {
    if ($defaultgroup) {
	echo "<tr><td align=center colspan=2 nowrap=1>
              <br>
              <font size=+1><b>Edit Trust Level</b></font>
              <br>
              You may edit trust level in the default group,<br>
                but you are not allowed to remove members.
              </td></tr>\n";
    }
    else {
	echo "<tr><td align=center colspan=2 nowrap=1>
              <br>
              <font size=+1><b>Remove/Edit Group Members.</b></font>
              <br>
              Deselect the ones you would like to remove,<br>
                   or edit their trust value.
              </td></tr>\n";
    }

    foreach ($curmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$trust      = $target_user->GetTempData();
	$showurl    = CreateURL("showuser", $target_user);

	if ($defaultgroup) {
	    echo "<tr>
                     <td>
                       <input type=hidden name='change_$target_idx'
                               value=permit>
                          <A href='$showurl'>$target_uid &nbsp</A>
                     </td>\n";
	}
	else {
	    echo "<tr>
                     <td>   
                       <input checked type=checkbox value=permit
                              name='change_$target_idx'>
                          <A href='$showurl'>$target_uid &nbsp</A>
                     </td>\n";
	}

	echo "   <td align=center>
                    <select name='U${target_idx}\$\$trust'>\n";

	#
	# We want to have the current trust value selected in the menu.
	#
	if ($group->CheckTrustConsistency($target_user,
					  TBDB_TRUSTSTRING_USER, 0)) {
	    echo "<option value='user' " .
		((strcmp($trust, "user") == 0) ? "selected" : "") .
		    ">User </option>\n";
	}
	if ($group->CheckTrustConsistency($target_user,
					  TBDB_TRUSTSTRING_LOCALROOT, 0)) {
	    echo "<option value='local_root' " .
		((strcmp($trust, "local_root") == 0) ? "selected" : "") .
		    ">Local Root </option>\n";

	    #
	    # If group_root is already selected, or we have permission to set
	    # it, show it. Otherwise do not.
	    #
	    if (strcmp($trust, "group_root") == 0 || $bestowgrouproot) {
		echo "<option value='group_root' " .
		    ((strcmp($trust, "group_root") == 0) ? "selected" : "") .
			">Group Root </option>\n";
	    }
	}
	echo "        </select>
                   </td>\n";
    }
    echo "</tr>\n";
    reset($curmembers);
}

if ($grabusers && count($nonmembers)) {
    echo "<tr><td align=center colspan=2 nowrap=1>
          <br>
          <font size=+1><b>Add Group Members</b></font>[<b>1</b>].
             <br>
             Select the ones you would like to add.<br>
             Be sure to select the appropriate trust level.
          </td></tr>\n";

    foreach ($nonmembers as $target_user) {
	$target_uid = $target_user->uid();
	$target_idx = $target_user->uid_idx();
	$trust      = $target_user->GetTempData();
	$showurl    = CreateURL("showuser", $target_user);

	echo "<tr>
                 <td>
                   <input type=checkbox value=permit name='add_$target_idx'>
                      <A href='$showurl'>$target_uid &nbsp</A>
                 </td>\n";

	echo "   <td align=center>
                   <select name='U${target_idx}\$\$trust'>\n";

	if ($group->CheckTrustConsistency($target_user,
					  TBDB_TRUSTSTRING_USER, 0)) {
	    echo "<option value='user' " .
		((strcmp($trust, "user") == 0) ? "selected" : "") .
		    ">User</option>\n";
	}
	if ($group->CheckTrustConsistency($target_user,
					  TBDB_TRUSTSTRING_LOCALROOT, 0)) {
	    echo "<option value='local_root' " .
		((strcmp($trust, "local_root") == 0) ? "selected" : "") .
		    ">Local Root</option>\n";

	    if ($bestowgrouproot) {
		echo "<option value='group_root' " .
		    ((strcmp($trust, "group_root") == 0) ? "selected" : "") .
			">Group Root</option>\n";
	    }
	}
	echo "        </select>
	    </td>\n";
    }
    echo "</tr>\n";
    reset($nonmembers);
}

if (count($curmembers) ||
    ($grabusers && count($nonmembers))) {
    echo "<tr>
             <td align=center colspan=2>
                 <b><input type=submit value=Submit></b>
             </td>
          </tr>\n";

    echo "</table>
          </form>\n";
}
else {
    echo "<br><center>
           <em>There are no project members who are eligible to be added
               or removed from this group[<b>1</b>].</em>
             </center>\n";
}

echo "<h4><blockquote><blockquote><blockquote>
      <ol>
       <li> Only members who have already been approved to the main
            project will be listed. If a project member is missing, please
            go to <a href=approveuser_form.php3>New User Approval</a>
            and approve the user to the main project group. Then you can
            reload this page and add those members to other groups in your
            project.\n";
echo "</ol>
      </blockquote></blockquote></blockquote>
      </h4>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
