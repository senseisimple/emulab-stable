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
    DBQueryFatal("select m.uid,m.trust from group_membership as m ".
		 "left join group_membership as a on ".
		 "     a.uid=m.uid and a.pid=m.pid and a.gid='$gid' ".
		 "where m.pid='$pid' and m.gid=m.pid and a.uid is NULL");

#
# Go through the list of current members. For each one, check to see if
# the checkbox for that person was checked. If not, delete the person
# from the group membership.
# 
if (mysql_num_rows($curmembers_result)) {
    while ($row = mysql_fetch_array($curmembers_result)) {
	$deluid = $row[0];
	$foo    = "delete_$row[0]";
	
	if (! isset($$foo)) {
	    DBQueryFatal("delete from group_membership ".
			 "where pid='$pid' and gid='$gid' and uid='$deluid'");
	}
    }
}

#
# Go through the list of non members. For each one, check to see if
# the checkbox for that person was checked. If so, add the person
# to the group membership. For now, they get the same permission they
# already have in the default group. At some point provide a way to
# do this on the page.
# 
if (mysql_num_rows($nonmembers_result)) {
    while ($row = mysql_fetch_array($nonmembers_result)) {
	$adduid = $row[0];
	$trust  = $row[1];
	$foo    = "add_$row[0]";
	
	if (isset($$foo)) {
	    DBQueryFatal("insert into group_membership ".
			 "(uid, pid, gid, trust, date_applied,date_approved) ".
			 "values ('$adduid','$pid','$gid', '$trust', ".
			 "        now(), now())");
	}
    }
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

#
# Run the script. This will do the account stuff for all the people
# in the group. This is the same script that gets run when the group
# is first created.
#
SUEXEC($uid, $unix_gid, "webgroupupdate $pid $gid", 1);

#
# No show it again.
#
SHOWGROUP($pid, $gid);

#
# An edit option.
# 
echo "<p><center>
       Do you want to edit this Group?
       <A href='editgroup_form.php3?pid=$pid&gid=$gid'>Yes</a>
      </center>\n";

SHOWGROUPMEMBERS($pid, $gid);

#
# A delete option
#
echo "<p><center>
       Do you want to delete this Group?
       <A href='deletegroup.php3?pid=$pid&gid=$gid'>Yes</a>
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
