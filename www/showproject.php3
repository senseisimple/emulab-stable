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
PAGEHEADER("Show Project Information");


#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a project ID.", 1);
}

#
# Check to make sure thats this is a valid PID.
#
if (! TBValidProject($pid)) {
    USERERROR("The project '$pid' is not a valid project.", 1);
}

#
# Verify that this uid is a member of the project being displayed.
#
if (! TBProjAccessCheck($uid, $pid, $pid, $TB_PROJECT_READINFO)) {
    USERERROR("You are not a member of Project $pid.", 1);
}

#
# A list of project experiments.
#
$query_result =
    DBQueryFatal("select e.*,count(r.node_id) from experiments as e ".
		 "left join reserved as r on e.pid=r.pid and e.eid=r.eid ".
		 "where e.pid='$pid' ".
		 "group by e.eid order by e.state,e.eid");

if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Experiments</h3>
          </center>
          <table align=center border=1 cellpadding=2 cellspacing=2>\n";

    echo "<tr>
              <th>EID</th>
              <th>State</th>
              <th>Nodes</th>
              <th>Description</th>
          </tr>\n";

    while ($projrow = mysql_fetch_array($query_result)) {
	$eid  = $projrow[eid];
	$state= $projrow[state];
	$nodes= $projrow["count(r.node_id)"];
	$name = stripslashes($projrow[expt_name]);
	if ($projrow[swap_requests] > 0) {
	  $state .= "&nbsp;(idle)";
	}

        echo "<tr>
                 <td><A href='showexp.php3?pid=$pid&eid=$eid'>$eid</A></td>
		 <td>$state</td>
                 <td>$nodes</td>
                 <td>$name</td>
             </tr>\n";
    }
    echo "</table>\n";
}

#
# A list of project members (from the default group).
#
SHOWGROUPMEMBERS($pid, $pid, 0);

#
# A list of project Groups (if more than just the default).
#
$query_result =
    DBQueryFatal("SELECT * FROM groups WHERE pid='$pid'");
if (mysql_num_rows($query_result)) {
    echo "<center>
          <h3>Project Groups</h3>
          </center>
          <table align=center border=1>\n";

    echo "<tr>
              <th>GID</th>
              <th>Desription</th>
              <th>Leader</th>
          </tr>\n";

    while ($row = mysql_fetch_array($query_result)) {
        $gid      = $row[gid];
        $desc     = stripslashes($row[description]);
	$leader   = $row[leader];

        echo "<tr>
                  <td>
                      <A href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a>
                      </td>

                  <td>$desc</td>

	          <td><A href='showuser.php3?target_uid=$leader'>$leader</A>
                      </td>
              </tr>\n";
    }

    echo "</table>\n";
}

echo "<p><center>
       <A href='newgroup_form.php3?pid=$pid'>Create</a> a new Group?
      </center>\n";

SHOWPROJECT($pid, $uid);

if ($isadmin) {
    echo "<p>
          <A href='deleteproject.php3?pid=$pid'>
             <font color=Red>Delete this project?</font></a>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
