<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("showby",	      PAGEARG_STRING,
				 "target_user",	      PAGEARG_USER,
				 "target_project",    PAGEARG_PROJECT,
				 "exptidx",           PAGEARG_INTEGER,
				 "records",           PAGEARG_INTEGER,
				 "verbose",           PAGEARG_BOOLEAN,
				 "startwith",         PAGEARG_INTEGER);

#
# Standard Testbed Header
#
PAGEHEADER("Testbed Wide Stats");

if (! isset($verbose)) {
     $verbose = 0;
}
if (! isset($showby)) {
    if ($isadmin) 
	$showby = "all";
    else
	$showby = "user";
}
# Show just the last N records unless request is different.
if (!isset($records) || $records == 0) {
    $records = 20;
}
if (!isset($startwith)) {
    $startwith    = 0;
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
echo "<br>\n";

# Determine what to do.
if ($showby == "user") {
    if (!isset($target_user)) {
	$target_user = $this_user;
    }
    elseif (! $target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
	USERERROR("You do not have permission to view ${user}'s stats!", 1);
    }
    $wclause = "where (t.uid_idx='" . $target_user->uid_idx() . "')";
}
elseif ($showby == "project") {
    if (!isset($target_project)) {
	USERERROR("Must supply a project to view!", 1);
    }
    $pid = $target_project->pid();
    
    if (!$target_project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You do not have permission to view stats for ".
		  "project $pid!", 1);
    }
    $wclause = "where (s.pid='$pid')";
}
elseif ($showby == "expt") {
    #
    # We get an index, which may or may not map to a current experiment.
    # If not a current experiment, then need to check in the stats table
    # so we can map it to a pid/eid to do the permission check. Not
    # sure I like this so I am not going to permit it for mere users
    # just yet.
    #
    if (!isset($exptidx)) {
	USERERROR("Must supply an experiment to view!", 1);
    }
    if (!TBvalid_integer($exptidx)) {
	USERERROR("Invalid characters in $exptidx!", 1);
    }
    if (! ($experiment = Experiment::Lookup($exptidx))) {
	if (! ($stats = ExperimentStats::Lookup($exptidx))) {
	    USERERROR("No such experiment index $exptidx!", 1);
	}
	if (!$isadmin) {
	    # XXX
	    USERERROR("You do not have permission to view stats for ".
		      "historical experiment $exptidx!", 1);
	}
    }
    elseif (! $experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view stats for ".
		  "experiment $exptidx!", 1);
    }
    $wclause = "where (t.exptidx='$exptidx')";

    echo "<center>";
    if ($experiment) {
	echo $experiment->PageHeader();
    }
    echo "<font size=+2>";
    echo "(<a href='showexpstats.php3?record=$exptidx'>$exptidx</a>) ";
    echo "</font>";
    echo "</center>\n";
    echo "<br>\n";
}
elseif ($showby == "all") {
    if (isset($target_project)) {
	$pid = $target_project->pid();
    
	if (!$target_project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	    USERERROR("You do not have permission to view stats for ".
		      "project $pid!", 1);
	}
	$wclause = "where (s.pid='$pid')";
    }
    elseif (!$isadmin) {
        #
        # Get a project list for which the user has read permission.
        #
	$orclause = "";
        $projlist = $this_user->ProjectAccessList($TB_PROJECT_READINFO);
	if (! count($projlist)) {
	    USERERROR("You do not have permission to view stats for any ".
		      "project!", 1);
	}
	while (list($project, $grouplist) = each($projlist)) {
	    $orclause .= "s.pid='$project' or ";
	}
	$wclause = "where ($wclause 0)";
    }
}
else {
    USERERROR("Bad page arguments!", 1);
}

# Do the endwith to ensure the query does not take too long
$startclause = ($startwith ? "(t.idx<$startwith)" : "");

if (strlen($wclause)) {
    if (strlen($startclause)) {
	$wclause = "$wclause and $startclause";
    }
}
else if (strlen($startclause)) {
    $wclause = "where ($startclause)";
}
else {
    $wclause = "";
}

$query_result =
    DBQueryFatal("select t.exptidx,s.pid,s.eid,t.action,t.exitcode,t.uid, ".
                 "       r.pnodes,t.idx as statno,t.start_time,t.end_time, ".
		 "       s.archive_idx,r.archive_tag,t.uid_idx ".
		 "  from testbed_stats as t force index (end_time, idx) ".
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
    $idx     = $row["statno"];
    $eidx    = $row["exptidx"];
    $pid     = $row["pid"];
    $eid     = $row["eid"];
    $uid     = $row["uid"];
    $uid_idx = $row["uid_idx"];
    $start   = $row["start_time"];
    $end     = $row["end_time"];
    $action  = $row["action"];
    $ecode   = $row["exitcode"];
    $pnodes  = $row["pnodes"];
    $archive_idx = $row["archive_idx"];
    $archive_tag = $row["archive_tag"];
    $startwith   = $idx;

    if (!isset($end))
	$end = "&nbsp";
	
    echo "<tr>";
    if ($verbose)
	echo "<td>$idx</td>";
    if ($EXPOSEARCHIVE) {
	if ($archive_idx && $archive_tag &&
	    ($action == "swapout" || $action == "swapmod")) {
	    echo "  <td align=center>
                       <a href=beginexp_html.php3?copyid=$eidx:$archive_tag>
                       <img border=0 alt=Run src=greenball.gif></a></td>";
	}
	else {
	    echo "<td>&nbsp</td>\n";
	}
    }
    if ($verbose || $showby != "expt") {
	echo "<td>$uid ($uid_idx)</td>
              <td>$pid</td>
              <td>$eid</td>
              <td><a href=showexpstats.php3?record=$eidx>$eidx</a></td>";
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
		 "<a href='cvsweb/cvswebwrap.php3/$eidx/history/".
		          "$archive_tag/?exptidx=$eidx'>$archive_tag</a>".
		 "</td>\n";
	}
	else {
	    echo "<td>&nbsp</td>\n";
	}
    }
    echo "</tr>\n";
}
echo "</table>\n";

if (1) {
    $args  = "?showby=$showby&startwith=$startwith&records=$records";
    if (isset($target_user)) {
	$args .= "&user=" . $target_user->uid_idx();
    }
    if (isset($target_project)) {
	$args .= "&project=" . $target_project->pid_idx();
    }
    if (isset($exptidx)) {
	$args .= "&exptidx=$exptidx";
    }
    if ($verbose) {
	$args .= "&verbose=$verbose";
    }
    echo "<center>".
	"<a href='showstats.php3${args}'>".
	"More Entries</a></center><br>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
