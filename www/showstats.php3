<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Testbed Wide Stats");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Page args,
if (! isset($showby)) {
    if ($isadmin) 
	$showby = "all";
    else
	$showby = "user";
}
if (! isset($which))
    $which = 0;
# Show just the last N records unless request is different.
if (!isset($records) || !strcmp($records, "")) {
    $records = 200;
}

echo "<b>Show: <a class='static' href='showexpstats.php3'>
              Experiment Stats</a></b><br>\n";

# Set up where clause and menu.
$wclause = "";

echo "<b>Show by: ";

# By User. A specific user, or the current uid.
if ($showby != "user") {
    echo "<a class='static' href='showstats.php3?showby=user'>User</a>, ";
}
else {
    echo "User, ";
}
# Or All. For mere users, "all" would be confusing, so change the tag
# to "projects" to indicate its really just projects they are members of.
if ($showby != "all") {
    echo "<a class='static' href='showstats.php3?showby=all'>";
    if ($isadmin)
	echo "All</a>.";
    else
	echo "Project</a>.";
}
else {
    if ($isadmin)
	echo "All</a>.";
    else
	echo "Project</a>.";
}
echo "</b><br><br>\n";

# Determine what to do.
if ($showby == "user") {
    if ($which) {
	if ($which != $uid &&
	    ! TBUserInfoAccessCheck($uid, $which, $TB_USERINFO_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "user $which!", 1);
	}
    }
    else
	$which = $uid;
    $wclause = "where s.creator='$which'";
    $records = 400;
}
elseif ($showby == "project") {
    if (! $which) {
	USERERROR("Must supply a project to view!", 1);
    }
    if (! TBProjAccessCheck($uid, $which, $which, $TB_PROJECT_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "project $which!", 1);
    }
    $wclause = "where s.pid='$which'";
    $records = 400;
}
elseif ($showby == "expt") {
    if (!$which) {
	USERERROR("Must supply an experiment to view!", 1);
    }

    #
    # We get an index. Must map that to a pid/eid to do the permission
    # check, and note that it might not be an current experiment. Not
    # sure I like this so I am not going to permit it for mere users
    # just yet.
    #
    if (!$isadmin) {
	$query_result =
	    DBQueryFatal("select pid,eid from experiments where idx='$which'");
	if (mysql_num_rows($query_result) == 0) {
	    USERERROR("No such experiment index $which!", 1);
	}
	$row   = mysql_fetch_array($query_result);
	$pid   = $row[pid];
	$eid   = $row[eid];
	
	if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "experiment $which!", 1);
	}
    }
    $wclause = "where t.exptidx='$which'";
    $records = 100;
}
elseif ($showby == "all") {
    if ($which) {
	if (! TBProjAccessCheck($uid, $which, $which, $TB_PROJECT_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "project $which!", 1);
	}
	$wclause = "where s.pid='$which'";
    }
    elseif (!$isadmin) {
        #
        # Get a project list for which the user has read permission.
        #
        $projlist = TBProjList($uid, $TB_PROJECT_READINFO);
	if (! count($projlist)) {
	    USERERROR("You do not have permission to view stats for any ".
		      "project!", 1);
	}
	while (list($project, $grouplist) = each($projlist)) {
	    $wclause .= "s.pid='$project' or ";
	}
	$wclause = "where $wclause 0";
    }
    $records = 400;
}
else {
    USERERROR("Bad page arguments!", 1);
}

$query_result =
    DBQueryFatal("select s.*,t.*,r.*,t.idx as statno,t.tstamp as ttstamp ".
		 "  from testbed_stats as t ".
		 "left join experiment_stats as s on s.exptidx=t.exptidx ".
		 "left join experiment_resources as r on r.idx=t.rsrcidx ".
		 "$wclause ".
		 "order by t.tstamp desc,t.idx desc limit $records");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No testbed stats records in the system!", 1);
}

echo "<table align=center border=1>
      <tr>
        <th>#</th>
        <th>Pid</th>
        <th>Eid</th>
        <th>ExptIdx</th>
        <th>Time</th>
        <th>Action (Nodes)</th>
        <th>ECode</th>
      </tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $idx     = $row[statno];
    $exptidx = $row[exptidx];
    $pid     = $row[pid];
    $eid     = $row[eid];
    $when    = $row[ttstamp];
    $action  = $row[action];
    $ecode   = $row[exitcode];
    $pnodes  = $row[pnodes];
	
    echo "<tr>
            <td>$idx</td>
            <td>$pid</td>
            <td>$eid</td>
            <td><a href=showexpstats.php3?record=$exptidx>$exptidx</a></td>
            <td>$when</td>\n";
    if (!$ecode && (strcmp($action, "preload") &&
		    strcmp($action, "destroy"))) {
	echo "<td>$action ($pnodes)</td>\n";
    }
    else {
	echo "<td>$action</td>\n";
    }
    echo " <td>$ecode</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
