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

#
# Only admins
#
if (! $isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

echo "Show <a class='static' href='showexpstats.php3'>
              Experiment Stats</a><br><br>\n";

#
# Right now we show just the last 200 records entered. 
#
$query_result =
    DBQueryFatal("select e.*,t.*,t.idx as statno from testbed_stats as t ".
		 "left join experiment_stats as e on e.idx=t.exptidx ".
		 "order by t.idx desc limit 200");

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
        <th>Action</th>
        <th>ExitCode</th>
      </tr>\n";

while ($row = mysql_fetch_assoc($query_result)) {
    $idx     = $row[statno];
    $exptidx = $row[exptidx];
    $pid     = $row[pid];
    $eid     = $row[eid];
    $when    = $row[tstamp];
    $action  = $row[action];
    $ecode   = $row[exitcode];
	
    echo "<tr>
            <td>$idx</td>
            <td>$pid</td>
            <td>$eid</td>
            <td><a href=showexpstats.php3?record=$exptidx>$exptidx</a></td>
            <td>$when</td>
            <td>$action</td>
            <td>$ecode</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
