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
PAGEHEADER("Testbed Summary Stats");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Summary data for admins only.
if (!$isadmin) {
    USERERROR("You are not allowed to view this page!", 1);
}

# Page args,
if (! isset($showby)) {
    $showby = "users";
}
if (! isset($sortby)) {
    $sortby = "pdays";
}
if (! isset($range)) {
    $range = "epoch";
}

echo "<b>Show: <a class='static' href='showexpstats.php3'>
                  Experiment Stats</a>,
               <a class='static' href='showstats.php3'>
                  Testbed Stats</a>.
      </b><br>\n";

echo "<b>Show by: ";
# By Users.
if ($showby != "users") {
    echo "<a class='static' href='showsumstats.php3?showby=users'>
             Users</a>, ";
}
else {
    echo "Users, ";
}
# By Projects
if ($showby != "projects") {
    echo "<a class='static' href='showsumstats.php3?showby=projects'>
             Projects</a>, ";
}
else {
    echo "Projects. ";
}
echo "</b><br>\n";

echo "<b>Range: ";
if ($range != "epoch") {
    echo "<a class='static'
            href='showsumstats.php3?showby=$showby&sortby=$sortby&range=epoch'>
            Epoch</a>, ";
}
else {
    echo "Epoch, ";
}
if ($range != "day") {
    echo "<a class='static'
            href='showsumstats.php3?showby=$showby&sortby=$sortby&range=day'>
            Day</a>, ";
}
else {
    echo "Day, ";
}
if ($range != "week") {
    echo "<a class='static'
            href='showsumstats.php3?showby=$showby&sortby=$sortby&range=week'>
            Week</a>, ";
}
else {
    echo "Week, ";
}
if ($range != "month") {
    echo "<a class='static'
            href='showsumstats.php3?showby=$showby&sortby=$sortby&range=month'>
            Month</a>, ";
}
else {
    echo "Month. ";
}
echo "</b><br><br>\n";


#
# This version prints out the simple summary info for the entire table.
# No ranges, just ordered. 
#
function showsummary ($showby, $sortby) {
    switch ($showby) {
        case "projects":
	    $which = "pid";
	    $table = "project_stats";
	    $title = "Project Summary Stats (Epoch)";
	    $link  = "showproject.php3?pid=";
	    break;
        case "users":
	    $which = "uid";
	    $table = "user_stats";
	    $title = "User Summary Stats (Epoch)";
	    $link  = "showuser.php3?target_uid=";
	    break;
        default:
	    USERERROR("Invalid showby argument: $showby!", 1);
    }
    switch ($sortby) {
        case "pid":
	    $order = "pid";
	    break;
        case "uid":
	    $order = "uid";
	    break;
        case "pnodes":
	    $order = "allexpt_pnodes desc";
	    break;
        case "pdays":
	    $order = "pnode_days desc";
	    break;
        case "edays":
	    $order = "expt_days desc";
	    break;
        default:
	    USERERROR("Invalid sortby argument: $sortby!", 1);
    }

    $query_result =
	DBQueryFatal("select $which, allexpt_pnodes, ".
		     "allexpt_pnode_duration / (24 * 3600) as pnode_days, ".
		     "allexpt_duration / (24 * 3600) as expt_days ".
		     "from $table where allexpt_pnodes!=0 ".
		     "order by $order");

    echo "<center><b>$title</b></center><br>\n";
    echo "<table align=center border=1>
          <tr>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=$which'>
                    $which</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=pnodes'>
                    Pnodes</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=pdays'>
                    Pnode Days</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=edays'>
                    Expt Days</th>
          </tr>\n";

    while ($row = mysql_fetch_assoc($query_result)) {
	$heading = $row[$which];
	$pnodes  = $row["allexpt_pnodes"];
	$phours  = $row["pnode_days"];
	$ehours  = $row["expt_days"];

	echo "<tr>
                <td><A href='$link${heading}'>$heading</A></td>
                <td>$pnodes</td>
                <td>$phours</td>
                <td>$ehours</td>
              </tr>\n";
    }
    echo "</table>\n";
}

#
# COmparison functions for sort.
#
function intcmp ($a, $b) {
    if ($a == $b) return 0;
    return ($a > $b) ? -1 : 1;
}
function pnodecmp ($a, $b) {
    return intcmp($a["pnodes"], $b["pnodes"]);
}
function pdaycmp ($a, $b) {
    return intcmp($a["pseconds"], $b["pseconds"]);
}
function edaycmp ($a, $b) {
    return intcmp($a["eseconds"], $b["eseconds"]);
}

function showrange ($showby, $sortby, $range) {
    switch ($range) {
        case "day":
	    $wclause = "(3600 * 24 * 1)";
	    break;
        case "week":
	    $wclause = "(3600 * 24 * 7)";
	    break;
        case "month":
	    $wclause = "(3600 * 24 * 31)";
	    break;
        default:
	    USERERROR("Invalid range argument: $range!", 1);
    }

    $query_result =
	DBQueryFatal("select s.*,t.*,r.*,UNIX_TIMESTAMP(t.tstamp) as ttstamp ".
		     "  from testbed_stats as t ".
		     "left join experiment_stats as s on s.exptidx=t.exptidx ".
		     "left join experiment_resources as r on r.idx=t.rsrcidx ".
		     "where t.exitcode = 0 && ".
		     "     ((UNIX_TIMESTAMP(now())-UNIX_TIMESTAMP(t.tstamp)) ".
		     "      < $wclause) ".
		     "order by t.tstamp");

    # Experiment start time, indexed by pid:eid.
    $expt_start = array();

    # Summary info, indexed by pid and uid. Each entry is an array of the
    # summary info.
    $pid_summary = array();
    $uid_summary = array();

    while ($row = mysql_fetch_assoc($query_result)) {
	$pid     = $row["pid"];
	$eid     = $row["eid"];
	$uid     = $row["creator"];
	$tstamp  = $row["ttstamp"];
	$action  = $row["action"];
	$pnodes  = $row["pnodes"];

	if ($pnodes == 0)
	    continue;

	if (!isset($pid_summary[$pid])) {
	    $pid_summary[$pid] = array('pnodes'   => 0,
				       'pseconds' => 0,
				       'eseconds' => 0);
	    $uid_summary[$uid] = array('pnodes'   => 0,
				       'pseconds' => 0,
				       'eseconds' => 0);
	}

	#echo "$pid $eid $uid $tstamp $action $pnodes<br>\n";

	switch ($action) {
        case "start":
        case "swapin":
	    $expt_start["$pid:$eid"] = array('pnodes' => $pnodes,
					     'uid'    => $uid,
					     'pid'    => $pid,
					     'stamp'  => $tstamp);
	    break;
        case "swapout":
        case "swapmod":
	    if (isset($expt_start["$pid:$eid"])) {
		# Use the original data, expecially pnodes since if this
		# was a swapmod, the nodes are for the new config, not
		# the old config. 
		$uid    = $expt_start["$pid:$eid"]["uid"];
		$pnodes = $expt_start["$pid:$eid"]["pnodes"];
		$diff = $tstamp - $expt_start["$pid:$eid"]["stamp"];

		$pid_summary[$pid]["pnodes"]   += $pnodes;
		$pid_summary[$pid]["pseconds"] += $pnodes * $diff;
		$pid_summary[$pid]["eseconds"] += $diff;
		$uid_summary[$uid]["pnodes"]   += $pnodes;
		$uid_summary[$uid]["pseconds"] += $pnodes * $diff;
		$uid_summary[$uid]["eseconds"] += $diff;
		unset($expt_start["$pid:$eid"]);
	    }
	    # Basically, start the clock ticking again with the new
	    # number of pnodes.
	    if ($action == "swapmod") {
		$expt_start["$pid:$eid"] = array('pnodes' => $pnodes,
						 'uid'    => $uid,
						 'pid'    => $pid,
						 'stamp'  => $tstamp);
	    }
	    break;
	case "preload":
	case "destroy":
	    break;
        default:
	    TBERROR("Invalid action: $action!", 1);
	}
    }
    #
    # Anything still in the expt_start arrary is obviously still running, so
    # count those up to the current time. 
    #
    $now = time();
    foreach ($expt_start as $pideid => $value) {
	$pid     = $value["pid"];
	$uid     = $value["uid"];
	$pnodes  = $value["pnodes"];
	$stamp   = $value["stamp"];
	$diff    = $now - $stamp;

	$pid_summary[$pid]["pnodes"]   += $pnodes;
	$pid_summary[$pid]["pseconds"] += $pnodes * $diff;
	$pid_summary[$pid]["eseconds"] += $diff;
	$uid_summary[$uid]["pnodes"]   += $pnodes;
	$uid_summary[$uid]["pseconds"] += $pnodes * $diff;
	$uid_summary[$uid]["eseconds"] += $diff;
    }
    
    switch ($showby) {
        case "projects":
	    $which = "pid";
	    $table = $pid_summary;
	    $title = "Project Summary Stats ($range)";
	    $link  = "showproject.php3?pid=";
	    break;
        case "users":
	    $which = "uid";
	    $table = $uid_summary;
	    $title = "User Summary Stats ($range)";
	    $link  = "showuser.php3?target_uid=";
	    break;
        default:
	    USERERROR("Invalid showby argument: $showby!", 1);
    }
    switch ($sortby) {
        case "pid":
        case "uid":
	    ksort($table);
	    break;
        case "pnodes":
	    uasort($table, "pnodecmp");
	    break;
        case "pdays":
	    uasort($table, "pdaycmp");
	    break;
        case "edays":
	    uasort($table, "edaycmp");
	    break;
        default:
	    USERERROR("Invalid sortby argument: $sortby!", 1);
    }

    echo "<center><b>$title</b></center><br>\n";
    echo "<table align=center border=1>
          <tr>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=$which&range=$range'>
                    $which</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=pnodes&range=$range'>
                    Pnodes</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=pdays&range=$range'>
                    Pnode Days</th>
             <th><a class='static'
                    href='showsumstats.php3?showby=$showby&sortby=edays&range=$range'>
                    Expt Days</th>
          </tr>\n";

    foreach ($table as $key => $value) {
	$heading = $key;
	$pnodes  = $value["pnodes"];
	$phours  = sprintf("%.2f", $value["pseconds"] / (3600 * 24));
	$ehours  = sprintf("%.2f", $value["eseconds"] / (3600 * 24));

	# We caught a swapout, where the swapin was before the interval
	# being looked at.
	if (!$pnodes)
	    continue;

	echo "<tr>
                <td><A href='$link${heading}'>$heading</A></td>
                <td>$pnodes</td>
                <td>$phours</td>
                <td>$ehours</td>
              </tr>\n";
    }
    echo "</table>\n";
}

if ($range == "epoch") {
    showsummary($showby, $sortby);
}
else {
    showrange($showby, $sortby, $range);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
