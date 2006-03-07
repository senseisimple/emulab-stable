<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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

if (!isset($verbose)) {
     $verbose = 0;
}

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
    $records = 100;
}
elseif (! TBvalid_integer($records)) {
    PAGEARGERROR("Invalid argument: $records!");
}

echo "<b>Show: <a class='static' href='showexpstats.php3'>
                  Experiment Stats</a>";
if ($isadmin) {
    echo "<a class='static' href='showsumstats.php3'>, Summary Stats</a>\n";
}
echo "</b><br>\n";

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
echo "</b><br>\n";

# Refresh option
if ($isadmin) {
    echo "<b>Refresh every: ";
    echo "<a class='static' ".
	     "href='showstats.php3?showby=$showby&refreshrate=30'>30</a>, ";
    echo "<a class='static' ".
	     "href='showstats.php3?showby=$showby&refreshrate=60'>60</a>, ";
    echo "<a class='static' ".
	     "href='showstats.php3?showby=$showby&refreshrate=120'>120</a> ";
    echo "seconds.\n";
    echo "</b><br>\n";
}
echo "<br>\n";

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
    $wclause = "where t.uid='$which'";
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
}
elseif ($showby == "expt") {
    if (!$which) {
	USERERROR("Must supply an experiment to view!", 1);
    }
    if (!TBvalid_integer($which)) {
	USERERROR("Invalid characters in $which!", 1);
    }

    #
    # We get an index. Must map that to a pid/eid to do the permission
    # check, and note that it might not be an current experiment. Not
    # sure I like this so I am not going to permit it for mere users
    # just yet.
    #
    $query_result =
	DBQueryFatal("select pid,eid from experiments where idx='$which'");
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("No such experiment index $which!", 1);
    }
    $row   = mysql_fetch_array($query_result);
    $pid   = $row[pid];
    $eid   = $row[eid];

    if (!$isadmin) {
	if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "experiment $which!", 1);
	}
    }
    $wclause = "where t.exptidx='$which'";

    #
    # When showing a single experiment, put uid,pid,eid,idx in a header.
    # Less crap to look at.
    #
    echo "<center><font size=+1>".
	 "Experiment <b>".
         "<a href='showproject.php3?pid=$pid'>$pid</a>/".
	 "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a> ".
	 "(<a href='showexpstats.php3?record=$which'>$which</a>) ".
	 "</b></font>\n";
         "</center><br>";
    echo "<br>\n";
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
}
else {
    USERERROR("Bad page arguments!", 1);
}

$query_result =
    DBQueryFatal("select t.exptidx,s.pid,s.eid,t.action,t.exitcode,t.uid, ".
                 "       r.pnodes,t.idx as statno,t.start_time,t.end_time, ".
		 "       s.archive_idx,r.archive_tag ".
		 "  from testbed_stats as t ".
		 "left join experiment_stats as s on s.exptidx=t.exptidx ".
		 "left join experiment_resources as r on r.idx=t.rsrcidx ".
		 "$wclause ".
		 "order by t.end_time desc,t.idx desc limit $records");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No testbed stats records in the system!", 1);
}

echo "<table align=center border=1>
      <tr>";
if ($verbose) {
    echo "<th>#</th>";
}
if ($EXPOSEARCHIVE) {
    echo "  <th>Run</th>";
}
if ($verbose || $showby != "expt") {
    echo "  <th>Uid</th>
            <th>Pid</th>
            <th>Eid</th>
            <th>ExptIdx</th>";
}
echo "  <th>Start</th>
        <th>End</th>
        <th>Action (Nodes)</th>
        <th>ECode</th>";
if ($EXPOSEARCHIVE) {
        echo "<th>Archive</th>";
}
echo " </tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $idx     = $row[statno];
    $exptidx = $row[exptidx];
    $pid     = $row[pid];
    $eid     = $row[eid];
    $uid     = $row[uid];
    $start   = $row[start_time];
    $end     = $row[end_time];
    $action  = $row[action];
    $ecode   = $row[exitcode];
    $pnodes  = $row[pnodes];
    $archive_idx = $row[archive_idx];
    $archive_tag = $row[archive_tag];

    if (!isset($end))
	$end = "&nbsp";
	
    echo "<tr>";
    if ($verbose)
	echo "<td>$idx</td>";
    if ($EXPOSEARCHIVE) {
	if ($archive_idx && $archive_tag &&
	    ($action == "swapout" || $action == "swapmod")) {
	    echo "  <td align=center>
                       <a href=beginexp_html.php3?copyid=$exptidx:$archive_tag>
                       <img border=0 alt=Run src=greenball.gif></a></td>";
	}
	else {
	    echo "<td>&nbsp</td>\n";
	}
    }
    if ($verbose || $showby != "expt") {
	echo "<td>$uid</td>
              <td>$pid</td>
              <td>$eid</td>
              <td><a href=showexpstats.php3?record=$exptidx>$exptidx</a></td>";
    }
    echo "  <td>$start</td>
            <td>$end</td>\n";
    if (!$ecode && (strcmp($action, "preload") &&
		    strcmp($action, "destroy"))) {
	echo "<td>$action ($pnodes)</td>\n";
    }
    else {
	echo "<td>$action</td>\n";
    }
    echo " <td>$ecode</td>\n";
    if ($EXPOSEARCHIVE) {
	if ($archive_idx && $archive_tag &&
	    (strcmp($action, "swapout") == 0 ||
	     strcmp($action, "swapmod") == 0)) {
	    echo "<td>".
		 "<a href='cvsweb/cvswebwrap.php3/$exptidx/history/".
		          "$archive_tag/?exptidx=$exptidx'>$archive_tag</a>".
		 "</td>\n";
	}
	else {
	    echo "<td>&nbsp</td>\n";
	}
    }
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
