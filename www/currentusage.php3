<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Get current user.
# 
$uid = GETLOGIN();

# Force the page to reload periodically.
$autorefresh = 90;

#
# For anonymous users, show experiment stats.
#
function SHOWSTATS()
{
    $query_result =
	DBQueryFatal("select count(*) from experiments as e " .
	     "left join experiment_stats as s on s.exptidx=e.idx " .
	     "left join experiment_resources as rs on rs.idx=s.rsrcidx ".
	     "where state='active' and rs.pnodes>0 and " .
		     "      e.pid!='emulab-ops' and ".
		     "      not (e.pid='ron' and e.eid='all')");
    
    if (mysql_num_rows($query_result) != 1) {
	$active_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$active_expts = $row[0];
    }

    $query_result =
	DBQueryFatal("select count(*) from experiments where ".
		     "state='swapped' and pid!='emulab-ops' and ".
		     "pid!='testbed'");
    if (mysql_num_rows($query_result) != 1) {
	$swapped_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$swapped_expts = $row[0];
    }

    $query_result =
	DBQueryFatal("select count(*) from experiments where ".
		     "swap_requests > 0 and idle_ignore=0 ".
		     "and pid!='emulab-ops' and pid!='testbed'");
    if (mysql_num_rows($query_result) != 1) {
	$idle_expts = "ERR";
    }
    else {
	$row = mysql_fetch_array($query_result);
	$idle_expts = $row[0];
    }
    $freepcs = TBFreePCs();

    PAGEBEGINNING("Current Usage", 1, 1);
    ?>
     <table valign=top align=center width=100% height=100% border=1>
     <tr><td nowrap colspan=2 class="contentheader"
	     bgcolor="#E1EAEA" align="center">
	   <font size=-1>Current Experiments</font></td></tr>
     <tr><td class="menuoptusage" align=right>
          <?php echo $active_expts ?></td> 
         <td align="left" class="menuoptusage">
             <a target=_parent href=explist.php3#active>Active</a>
          </td></tr>
     <tr><td align="right" class="menuoptusage"><?php echo $idle_expts ?></td>
         <td align="left" class="menuoptusage">Idle</td></tr>
     <tr><td align="right" class="menuoptusage">
	     <?php echo $swapped_expts ?></td>
         <td align="left" class="menuoptusage">
            <a target=_parent href=explist.php3#swapped>Swapped</a>
          </td></tr>
     <tr><td align="right" class="menuoptusage">
	     <b><?php echo $freepcs ?></b></td>
         <td align="left" class="menuoptusage"><b>Free PCs</b></td>
     </tr>
     </table>
   <?php
}

#
# Logged in users, show free node counts.
#
function SHOWFREENODES()
{
    # Get free totals by type.
    $query_result =
	DBQueryFatal("select n.eventstate,n.type,count(*) from nodes as n ".
		     "left join node_types as nt on n.type=nt.type ".
		     "left join reserved as r on r.node_id=n.node_id ".
		     "where (role='testnode') and class='pc' ".
		     "      and r.pid is null ".
		     "group BY n.eventstate,n.type");

    while ($row = mysql_fetch_array($query_result)) {
	$type  = $row[1];
	$count = $row[2];
        # XXX Yeah, I'm a doofus and can't figure out how to do this in SQL.
	if (($row[0] == TBDB_NODESTATE_ISUP) ||
	    ($row[0] == TBDB_NODESTATE_PXEWAIT) ||
	    ($row[0] == TBDB_NODESTATE_ALWAYSUP) ||
	    ($row[0] == TBDB_NODESTATE_POWEROFF)) {
	    $freecounts[$type] = $count;
	}
    }
    PAGEBEGINNING("Free Node Summary", 1, 1);

    echo "<table valign=top align=center width=100% height=100% border=1>
          <tr><td nowrap colspan=4 class=contentheader
	          bgcolor='#E1EAEA' align=center>
 	       <font size=-1>Free PCs</font></td></tr>\n";

    $newrow = 1;
    foreach($freecounts as $key => $value) {
	$freecount = $freecounts[$key];

	if ($newrow) 
	    echo "<tr>\n";
	$newrow = ($newrow ? 0 : 1);
	
	echo "<td class=menuoptusage align=right>
                  <a target=_parent href=shownodetype.php3?node_type=$key>
                      $key</a></td>
              <td class=menuoptusage align=left>${freecount}</td>\n";

	if ($newrow) {
	    echo "</tr>\n";
	}
    }
    if (! $newrow) {
	echo "<td></td><td></td></tr>\n";
    }
    $freepcs = TBFreePCs();
    echo "<tr>
          <td class=menuoptusage colspan=4 align=center>
               <b>$freepcs Free PCs</b></td>
          </tr>\n";
    echo "</table>";
}
     
#
# If user is anonymous, show experiment stats.
# 
if (CHECKLOGIN($uid) & CHECKLOGIN_LOGGEDIN) {
    SHOWFREENODES();
}
else {
    SHOWSTATS();
}

echo "</body></html>";

