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
# We do not allow the default group to be edited. Never ever!
#
if (strcmp($gid, $pid) == 0) {
    USERERROR("You are not allowed to modify a project's default group!", 1);
}

#
# Verify permission.
#
if (! TBProjAccessCheck($uid, $pid, 0, $TB_PROJECT_EDITGROUP)) {
    USERERROR("You do not have permission to edit group $gid in ".
	      "project $pid!", 1);
}

#
# Grab the user list for the group. Provide a button selection of people
# that can be removed. The group leader cannot be removed! Neither can
# the project leader.
#
$curmembers_result =
    DBQueryFatal("select m.uid from group_membership as m ".
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

echo "<form action='editgroup.php3?pid=$pid&gid=$gid' method=post>
      <table align=center border=1>\n";

if (mysql_num_rows($curmembers_result)) {
    echo "<tr><td align=center>
          <b>These are the current group members.<br>
              Deselect the ones you would like to remove.</b>
          </td></tr>\n";
    
    echo "<tr><td align=center>\n";
    
    while ($row = mysql_fetch_array($curmembers_result)) {
	echo "<input checked type=checkbox value=permit name='delete_$row[0]'>
                     $row[0] &nbsp\n";
    }
    echo "</td></tr>\n";
}

if (mysql_num_rows($nonmembers_result)) {
    echo "<tr><td align=center>
          <b>These are project members who are not in the group.<br>
              Select the ones you would like to add.</b>
          </td></tr>\n";
    
    echo "<tr><td align=center>\n";
    
    while ($row = mysql_fetch_array($nonmembers_result)) {
	echo "<input type=checkbox value=permit name='add_$row[0]'>
                     $row[0] &nbsp\n";
    }
    echo "</td></tr>\n";
}

echo "<tr>
         <td align=center>
             <b><input type=submit value=Submit></b>
         </td>
      </tr>\n";

echo "</table>
      </form>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
