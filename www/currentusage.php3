<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

# Get some stats about current experiments

$query_result =
    DBQueryFatal("select count(*) from experiments as e " .
	"left join experiment_stats as s on s.exptidx=e.idx " .
	"left join experiment_resources as rs on rs.idx=s.rsrcidx ".
	"where state='active' and rs.pnodes>0 and " .
        "      e.pid!='emulab-ops' and ".
	"      not (e.pid='ron' and e.eid='all')");

if (mysql_num_rows($query_result) != 1) {
    $active_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $active_expts = $row[0];
}

$query_result = DBQueryFatal("select count(*) from experiments where ".
	"state='swapped' and pid!='emulab-ops' and pid!='testbed'");
if (mysql_num_rows($query_result) != 1) {
    $swapped_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $swapped_expts = $row[0];
}

$query_result = DBQueryFatal("select count(*) from experiments where ".
			     "swap_requests > 0 and idle_ignore=0 ".
			     "and pid!='emulab-ops' and pid!='testbed'");
if (mysql_num_rows($query_result) != 1) {
    $idle_expts = "ERR";
} else {
    $row = mysql_fetch_array($query_result);
    $idle_expts = $row[0];
}
$freepcs = TBFreePCs();

# Force the page to reload periodically.
$autorefresh = 90;

PAGEBEGINNING("Current Usage", 1, 1);
?>
<table valign=top align=center width=100% height=100% border=1>
<tr><td nowrap colspan=2 class="contentheader" bgcolor="#E1EAEA" align="center">
	<font size=-1>Current Experiments</font></td></tr>
<tr><td class="menuoptusage" align=right>
       <?php echo $active_expts ?></td> 
    <td align="left" class="menuoptusage">
        <a target=_parent href=explist.php3#active>Active</a>
    </td></tr>
<tr><td align="right" class="menuoptusage"><?php echo $idle_expts ?></td>
    <td align="left" class="menuoptusage">Idle</td></tr>
<tr><td align="right" class="menuoptusage"><?php echo $swapped_expts ?></td>
    <td align="left" class="menuoptusage">
        <a target=_parent href=explist.php3#swapped>Swapped</a>
    </td></tr>
<tr><td align="right" class="menuoptusage"><b><?php echo $freepcs ?></b></td>
    <td align="left" class="menuoptusage"><b>Free PCs</b></td>
</tr>
</table></body></html>
