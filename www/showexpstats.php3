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
PAGEHEADER("Show Experiment Information");

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

#
# Right now we show just the last 50 records entered. 
#
$query_result =
    DBQueryFatal("select * from experiment_stats order by idx desc limit 100");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No experiment records in the system!", 1);
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
