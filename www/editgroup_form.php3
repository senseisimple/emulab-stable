<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
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
# Grab the user list for the group. Provide a button selection of people
# that can be removed. The group leader cannot be removed! Neither can
# the project leader.
#
$curmembers_result =
    DBQueryFatal("select m.uid,m.trust from group_membership as m ".
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

#
# We do not allow the actual group info to be edited. Just the membership.
#
SHOWGROUP($pid, $gid);

echo "<br>
      <form action='editgroup.php3?pid=$pid&gid=$gid' method=post>
      <table align=center border=1>\n";

if (mysql_num_rows($curmembers_result)) {
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

    while ($row = mysql_fetch_array($curmembers_result)) {
	$user  = $row[0];
	$trust = $row[1];

	if ($defaultgroup) {
	    echo "<tr>
                     <td>
                       <input type=hidden name='change_$user' value=permit>
                          <A href='showuser.php3?target_uid=$user'>
                             $user &nbsp</A>
                     </td>\n";
	}
	else {
	    echo "<tr>
                     <td>   
                       <input checked type=checkbox value=permit
                              name='change_$user'>
                          <A href='showuser.php3?target_uid=$user'>
                             $user &nbsp</A>
                     </td>\n";
	}

	echo "   <td align=center>
                    <select name='$user\$\$trust'>\n";

	#
	# We want to have the current trust value selected in the menu.
	#
	echo "<option value='user' " .
	     ((strcmp($trust, "user") == 0) ? "selected" : "") .
	     ">User </option>\n";

	echo "<option value='local_root' " .
	     ((strcmp($trust, "local_root") == 0) ? "selected" : "") .
	     ">Local Root </option>\n";
	    
	echo "<option value='group_root' " .
	     ((strcmp($trust, "group_root") == 0) ? "selected" : "") .
	     ">Group Root </option>\n";

	echo "        </select>
                 </td>\n";
    }
    echo "</tr>\n";
}

if (mysql_num_rows($nonmembers_result)) {
    echo "<tr><td align=center colspan=2 nowrap=1>
          <br>
          <font size=+1><b>Add Group Members.</b></font>
             <br>
             Select the ones you would like to add.<br>
             Be sure to select the appropriate trust level.
          </td></tr>\n";
    
    while ($row = mysql_fetch_array($nonmembers_result)) {
	$user  = $row[0];
	$trust = $row[1];
	
	echo "<tr>
                 <td>
                   <input type=checkbox value=permit name='add_$user'>
                      <A href='showuser.php3?target_uid=$user'>$user &nbsp</A>
                 </td>\n";

	echo "   <td align=center>
                   <select name='$user\$\$trust'>
                      <option value='user'>User</option>
                      <option value='local_root'>Local Root</option>
	              <option value='group_root'>Group Root</option>\n";

	echo "        </select>
                 </td>\n";
    }
    echo "</tr>\n";
}

echo "<tr>
         <td align=center colspan=2>
             <b><input type=submit value=Submit></b>
         </td>
      </tr>\n";

echo "</table>
      </form>\n";

echo "<br><center>
       Important <a href='docwrapper.php3?docname=groups.html#SECURITY'>
       security issues</a> are discussed in the
       <a href='docwrapper.php3?docname=groups.html'>Groups Tutorial</a>.
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
