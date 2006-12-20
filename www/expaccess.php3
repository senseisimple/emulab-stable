<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Manage Shared Access to your Experiment");

#
# Only known and logged in users can do this. 
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# We must get a valid PID/EID.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
  USERERROR("You must provide a valid project ID!", 1);
}
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
  USERERROR("You must provide a valid Experiment ID!", 1);
}

$query_result = mysql_db_query($TBDBNAME,
	"select e.shared,e.expt_head_uid,p.head_uid from experiments as e, ".
	"projects as p ".
	"where e.eid='$eid' and e.pid='$pid' and p.pid='$pid'");

if (!$query_result) {
    TBERROR("DB error getting experiment record $pid/$eid", 1);
}
if (($row = mysql_fetch_array($query_result)) == 0) {
    USERERROR("No such experiment $eid in project $pid!", 1);
}
if (!$row[0]) {
    USERERROR("Experiment $eid in project $pid is not shared", 1);
}

#
# Admin users get to do anything they want, of course. Otherwise, only
# the experiment creator or the project leader has permission to mess
# with this. 
#
if (! $isadmin) {
    #
    # One of them better be you!
    #
    if (strcmp($row[1], $uid) && strcmp($row[2], $uid)) {
	USERERROR("You must be either the experiment creator or the project ".
		  "leader under which the experiment was created, to change ".
		  "experiment/project access permissions!", 1);
    }
}
    
#
# Get the current set of projects that have been granted share access to
# this experiment.
#
$query_result = mysql_db_query($TBDBNAME,
	"select pid from exppid_access ".
	"where exp_eid='$eid' and exp_pid='$pid'");

if (mysql_num_rows($query_result)) {
    #
    # For each one, make sure the checkbox was checked. If not, delete
    # it from the table.
    # 
    while ($row = mysql_fetch_array($query_result)) {
	$foo = "access_$row[0]";
	
	if (! isset($$foo)) {
	    $update_result = mysql_db_query($TBDBNAME,
		"delete from exppid_access ".
		"where exp_eid='$eid' and exp_pid='$pid' and pid='$row[0]'");
	    if (! $update_result) {
		TBERROR("DB Error deleting $row[0] from experiment access ".
			"for eid=$eid and pid=$pid!", 1);
	    }
	}
    }
}

#
# Add the new PID (if provided), but only if not already a member.
#
if (isset($addpid) && strcmp($addpid, "")) {
    #
    # Make sure a valid PID.
    #
    $query_result = mysql_db_query($TBDBNAME,
	   "select pid from projects where pid='$addpid'");
    if (! $query_result) {
	TBERROR("DB Error getting project info for $addpid!", 1);
    }
    if (! mysql_num_rows($query_result)) {
	USERERROR("No such project $addpid!", 1);
    }
    
    $query_result = mysql_db_query($TBDBNAME,
	"replace into exppid_access (exp_pid, exp_eid, pid) ".
	"VALUES ('$pid', '$eid', '$addpid')");

    if (! $query_result) {
	TBERROR("DB Error adding experiment access for project $addpid ".
		"to eid=$eid and pid=$pid!", 1);
    }
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

SUEXEC($uid, $unix_gid, "webnodeupdate -b $pid $eid", 1);

echo "<center>
      <h3>Access Permissions Changed.</h3>
      </center>\n";

echo "<p>
      You will be notified via email when the update has completed on
      all of the nodes in your experiment. Since this involves rebuilding
      the accounts database on each node, it should be just a few minutes
      before you receive notification.\n";

#
# Back links for convenience.
#
echo "<p>
      <a href='expaccess_form.php3?pid=$pid&eid=$eid'>
         Back to access management page</a>\n";

echo "<p>
      <a href='showexp.php3?pid=$pid&eid=$eid'>
         Back to experiment information page</a>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
