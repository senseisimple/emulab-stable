<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# This page needs more work to make user friendly and flexible.
# Its a hack job at the moment.
# 

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Right now we show just the last N records entered, unless the user
# requested a specific record. 
#
if (isset($record) && strcmp($record, "")) {
    $wclause = "";
    if (! $isadmin) {
	$wclause = "and s.creator='$uid'";
    }
    $query_result =
	DBQueryFatal("select s.*,r.* from experiment_stats as s ".
		     "left join experiment_resources as r on ".
		     " r.exptidx=s.exptidx ".
		     "where s.exptidx=$record $wclause ".
		     "order by r.idx desc");

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("No such experiment record $record in the system!", 1);
    }
}
else {
    $wclause = "";
    if (! $isadmin) {
	$wclause = "where s.creator='$uid'";
    }
    $query_result =
	DBQueryFatal("select s.*,r.* from experiment_stats as s ".
		     "left join experiment_resources as r on ".
		     " r.exptidx=s.exptidx ".
		     "$wclause ".
		     "order by s.exptidx desc,r.idx asc limit 200");

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("No experiment records in the system!", 1);
    }
}

#
# Use first row to get the column headers (no pretty printing yet).
# 
$row = mysql_fetch_assoc($query_result);

echo "<table align=center border=1>\n";
echo "<tr>\n";
foreach($row as $key => $value) {
    $key = str_replace("_", " ", $key);
    
    echo "<th><font size=-1>$key</font></th>\n";
}
echo "</tr>\n";

mysql_data_seek($query_result, 0);

while ($row = mysql_fetch_assoc($query_result)) {
    echo "<tr>\n";
    foreach($row as $key => $value) {
	echo "<td nowrap>$value</td>\n";
    }
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
