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
periods. Nodes are broken down into two classes: The first class are the 
<a href="http://users.emulab.net/trac/emulab/wiki/pc3000">pc3000</a>s,
which are the most numerous, and are fairly modern. They are
most experimenters' nodes of choice. For many experimenters, the number of
free pc3000s limits the size of the experiments they can run.
The second class includes other, older PCs (along with the pc3000s) to give
a sense for how many nodes are available if one is willing to use some
slower nodes.

</p>

<p>
Hourly graphs show the average number of free nodes in the
given hour. Daily graphs show the average number of free nodes in the
given day, etc. Note that because some data in these graphs is averaged
over a very long time period (up to four years), it may not reflect recent
trends.
</p>



<h2 align=center>Recent Availability</h2>

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

<h2 align=center>Diurnal</h2>

<p align=center>
<b>A Typical Week (hourly, average since 2005)</b>
<br>
<img src="node_avail-by_hourofweek.png">

<p align=center>
<hr>

<h2 align=center>Long-Term Trends</h2>

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
