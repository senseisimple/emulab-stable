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
PAGEHEADER("Experiment Tags");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Show just the last N records unless request is different.
if (!isset($records) || !strcmp($records, "")) {
    $records = 100;
}

if (! isset($which) || $which == "") {
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
$pid   = $row["pid"];
$eid   = $row["eid"];

if (!$isadmin) {
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "experiment $pid/$eid ($which)!", 1);
    }
}

echo "<center><font size=+1>".
     "Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a> ".
     "(<a href='showstats.php3?showby=expt&which=$which'>$which</a>) ".
     "</b></font>\n";
     "</center><br>";
echo "<br>\n";

#
# We need the archive index so we can find its view.
#
$query_result =
    DBQueryFatal("select s.archive_idx from experiments as e ".
		 "left join experiment_stats as s on s.exptidx=e.idx ".
		 "where e.pid='$pid' and e.eid='$eid'");
if (mysql_num_rows($query_result) == 0) {
    TBERROR("Could not get archive index for experiment $pid/$eid", 1);
}
$row   = mysql_fetch_array($query_result);
$archive_idx = $row["archive_idx"];
		    
#
# Grab all the (commit/user) tags.
#
$query_result =
    DBQueryFatal("select * from archive_tags ".
		 "where archive_idx='$archive_idx' and view='$which' and ".
		 "      (tagtype='user' or tagtype='commit')");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No tags for experiment $pid/$eid", 1);
}

echo "<table align=center border=1>
      <tr>";
echo "  <th>Run</th>";
echo "  <th>Tag</th>";
echo "  <th>Date</th>";
echo "  <th>Description</th>";
echo "</tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $archive_tag = $row["tag"];
    $date_tagged = $row["date_created"];
    $description = $row["description"];

    echo "<tr>";
    echo "  <td align=center>
                <a href=beginexp_html.php3?copyid=$which:$archive_tag>
                    <img border=0 alt=Run src=greenball.gif></a></td>";
    echo "  <td>".
	     "<a href='cvsweb/cvswebwrap.php3/$which/history/".
		    "$archive_tag/?exptidx=$which'>$archive_tag</a>".
	 "  </td>";
    
    echo "  <td>$date_tagged</td>";
    echo "  <td>$description</td>";
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
