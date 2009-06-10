<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#
require("../defs.php3");

PAGEHEADER("Testbed Node Usage Stats");

?>

$avail_header

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

$avail_footer

<?php

PAGEFOOTER();

?>
