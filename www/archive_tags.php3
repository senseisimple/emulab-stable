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

# An experiment idx.
if (! isset($exptidx) || $exptidx == "") {
    USERERROR("Must supply an experiment index!", 1);
}
if (!TBvalid_integer($exptidx)) {
    USERERROR("Invalid characters in $exptidx!", 1);
}

#
# We get an index. Must map that to a pid/gid to do a group level permission
# check, since it might not be an current experiment.
#
unset($pid);
unset($eid);
unset($gid);
if (TBExptidx2PidEid($exptidx, $pid, $eid, $gid) < 0) {
    USERERROR("No such experiment index $exptidx!", 1);
}

if (!$isadmin &&
    !TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_READINFO)) {
    USERERROR("You do not have permission to view tags for ".
	      "archive in $pid/$gid ($exptidx)!", 1);
}

if (TBCurrentExperiment($exptidx)) {
    echo "<center><font size=+1>".
	"Experiment <b>".
	"<a href='showproject.php3?pid=$pid'>$pid</a>/".
        "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a> ".
        "(<a href='showstats.php3?showby=expt&exptidx=$exptidx'>$exptidx</a>) ".
        "</b></font>\n";
        "</center><br>";
}
else {
    echo "<center><font size=+1>".
	"Experiment ".
        "<a href='showstats.php3?showby=expt&exptidx=$exptidx'>$exptidx</a> ".
        "</b></font>\n";
        "</center><br>";
}

echo "<br>\n";

#
# We need the archive index so we can find its view.
#
$query_result =
    DBQueryFatal("select s.archive_idx from experiment_stats as s ".
		 "where s.exptidx='$exptidx'");
if (mysql_num_rows($query_result) == 0) {
    TBERROR("Could not get archive index for experiment $exptidx", 1);
}
$row   = mysql_fetch_array($query_result);
$archive_idx = $row["archive_idx"];
		    
#
# Grab all the (commit/user) tags.
#
$query_result =
    DBQueryFatal("select *,FROM_UNIXTIME(date_created) as date_created ".
		 "  from archive_tags ".
		 "where archive_idx='$archive_idx' and view='$exptidx' and ".
		 "      (tagtype='user' or tagtype='commit') ".
		 "order by date_created desc");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No tags for experiment $pid/$eid", 1);
}

echo "<table align=center border=1>
      <tr>";
echo "  <th>Run</th>";
echo "  <th>Tag (Click to visit archive)</th>";
echo "  <th>Date</th>";
echo "  <th>Description</th>";
echo "</tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $archive_tag = $row["tag"];
    $date_tagged = $row["date_created"];
    $description = $row["description"];

    echo "<tr>";
    echo "  <td align=center>
                <a href=beginexp_html.php3?copyid=$exptidx:$archive_tag>
                    <img border=0 alt=Run src=greenball.gif></a></td>";
    echo "  <td>".
	     "<a href='archive_view.php3/$exptidx/history/".
		    "$archive_tag/?exptidx=$exptidx'>$archive_tag</a>".
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
