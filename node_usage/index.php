<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
require("../defs.php3");

PAGEHEADER("Testbed Node Usage Stats");

?>

<p>

These graphs show the average number of free nodes over various time
periods. Hourly graphs show the average number of free nodes in the
given hour. Daily graphs show the average number of free nodes in the
given day. Etc.

<p>

<p align=center>

<b>Last 2 Weeks (hourly)</b>
<br>
<img src="node_avail-hourly_last2weeks.png">
<br> 
<p align=center>

<b>Last 2 Months (daily)</b>
<br>
<img src="node_avail-daily_last2months.png">

<p align=center>
<hr>
<p align=center>

<b>A Typical Week (hourly)</b>
<br>
<img src="node_avail-by_hourofweek.png">

<p align=center>
<hr>
<p align=center>

<b>Since Sep 2005 (yearly)</b>

<br>
<img src="node_avail-yearly.png">

<p align=center>

<b>Since Sep 2005 (monthly)</b>

<br>
<img src="node_avail-monthly.png">

<p align=center>

<b>Since Sep 2005 (weekly)</b>

<br>
<img src="node_avail-weekly.png">

<p align=center>

Note: The gap in the last two graphs represents periods when no data
was availabe due to bugs in our system.

<?php

PAGEFOOTER();

?>
