<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Tags");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "exptidx", PAGEARG_INTEGER,
				 "records", PAGEARG_INTEGER);

#
# If we got a current experiment, great. Otherwise we have to lookup
# data for a historical experiment.
#
if ($experiment) {
    # Need these below.
    $pid = $experiment->pid();
    $eid = $experiment->eid();
    $gid = $experiment->gid();

    # Permission
    if (!$isadmin &&
	!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
elseif (isset($exptidx)) {
    $stats = ExperimentStats::Lookup($exptidx);
    if (!$stats) {
	PAGEARGERROR("Invalid experiment index: $exptidx");
    }

    # Need these below.
    $pid = $stats->pid();
    $eid = $stats->eid();
    $gid = $stats->gid();

    # Permission
    if (!$isadmin &&
	!$stats->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
	USERERROR("You do not have permission to view tags for ".
		  "archive in $pid/$eid!", 1);
    }
}
else {
    PAGEARGERROR("Must provide a current or former experiment index");
}

# Show just the last N records unless request is different.
if (!isset($records)) {
    $records = 100;
}

echo "<center>\n";
if ($experiment) {
    echo $experiment->PageHeader();
}
else {
    echo $stats->PageHeader();
}
echo "</center>\n";
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
