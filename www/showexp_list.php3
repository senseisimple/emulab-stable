<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Information Listing");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin     = ISADMIN($uid);
$altclause   = "";
$active      = 0;
$idle        = 0;

if (! isset($showtype))
    $showtype="active";
if (! isset($sortby))
    $sortby = "normal";
if (! isset($minidledays))
    $minidledays = "3";

echo "<b>Show:
         <a href='showexp_list.php3?showtype=active&sortby=$sortby'>active</a>,
         <a href='showexp_list.php3?showtype=batch&sortby=$sortby'>batch</a>,";
if ($isadmin) 
     echo "\n<a href='showexp_list.php3?showtype=idle&sortby=$sortby".
       "&minidledays=$minidledays'>idle</a>,";

echo "\n       <a href='showexp_list.php3?showtype=all&sortby=$sortby'>all</a>.
      </b><br><br>\n";

#
# Handle showtype
# 
if (! strcmp($showtype, "all")) {
    $clause = 0;
    $title  = "All";
}
elseif (! strcmp($showtype, "active")) {
    $clause = "e.state='$TB_EXPTSTATE_ACTIVE'";
    $title  = "Active";
    $active = 1;
}
elseif (! strcmp($showtype, "batch")) {
    $clause = "e.batchmode=1";
    $title  = "Batch";
}
elseif ((!strcmp($showtype, "idle")) && $isadmin ) {
    $clause = "e.state='$TB_EXPTSTATE_ACTIVE') having (lastswap>=$minidledays";
    $title  = "Idle";
    $idle = 1;
}
else {
    $clause = "e.state='$TB_EXPTSTATE_ACTIVE'";
    $title  = "Active";
    $active = 1;
}

#
# Handle sortby.
# 
if (! strcmp($sortby, "normal") ||
    ! strcmp($sortby, "pid"))
    $order = "e.pid,e.eid";
elseif (! strcmp($sortby, "eid"))
    $order = "e.eid,e.expt_head_uid";
elseif (! strcmp($sortby, "uid"))
    $order = "e.expt_head_uid,e.pid,e.eid";
elseif (! strcmp($sortby, "name"))
    $order = "e.expt_name";
else 
    $order = "e.pid,e.eid";

#
# Show a menu of all experiments for all projects that this uid
# is a member of. Or, if an admin type person, show them all!
#
if ($isadmin) {
    if ($clause)
	$clause = "where ($clause)";
    else
        $clause = "";
    
    $experiments_result =
	DBQueryFatal("select pid,eid,expt_head_uid,expt_name, ".
		     "date_format(expt_swapped,\"%Y-%m-%d\") as d, ".
		     "(to_days(now())-to_days(expt_swapped)) as lastswap ".
		     "from experiments as e $clause ".
		     "order by $order");
}
else {
    if ($clause)
	$clause = "and ($clause)";
    else
        $clause = "";
    
    $experiments_result =
	DBQueryFatal("select distinct e.pid,eid,expt_head_uid,expt_name, ".
		     "date_format(expt_swapped,\"%Y-%m-%d\") as d ".
		     "from experiments as e ".
		     "left join group_membership as g on g.pid=e.pid ".
		     "where g.uid='$uid' $clause ".
		     "order by $order");
}
if (! mysql_num_rows($experiments_result)) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}

if (mysql_num_rows($experiments_result)) {
    echo "<center>
           <h2>$title Experiments</h2>
          </center>\n";

    if ($idle) {
      echo "<center><b>Experiments that have been idle at least $minidledays days</b></center>\n";
      echo "<center><h3>This will change soon - Slothd is on the way</h3></center>\n";
      echo "<b>\"Days idle\" is defined as:<br>
a) number of days since last login (as reported in last login column)<br>
b) number of days it has been swapped in (if nobody has logged in)<br>
</b><br>
Note that it is not reliable in the case where they're using a special
kernel or logging into the nodes in a way that lastlogins can't detect.<p>\n";
    }
    
    echo "<table border=2 cols=0
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <td width=8%>
               <a href='showexp_list.php3?showactive=$showactive&sortby=pid'>
                  PID</td>
              <td width=8%>
               <a href='showexp_list.php3?showactive=$showactive&sortby=eid'>
                  EID</td>
              <td width=3%>PCs</td>\n";

    if ($isadmin)
	echo "<td width=17% align=center>Last Login</td>\n";
    if ($idle)
      echo "<td width=4% align=center>Days Idle</td>
<td width=4% align=center>Swap Req.</td>\n";

    echo "    <td width=60%>
               <a href='showexp_list.php3?showactive=$showactive&sortby=name'>
                  Name</td>
              <td width=4%>
               <a href='showexp_list.php3?showactive=$showactive&sortby=uid'>
                  Head UID</td>
            </tr>\n";

    $total_pcs = 0;
    $total_sharks = 0;
    while ($row = mysql_fetch_array($experiments_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$huid = $row[expt_head_uid];
	$name = $row[expt_name];
	$date = $row[d];
	$daysidle=0;
	
	if ($isadmin) {
	    $foo = "&nbsp;";
	    if ($lastexpnodelogins = TBExpUidLastLogins($pid, $eid)) {
	        $daysidle=$lastexpnodelogins["daysidle"];
	        if ($idle && $daysidle<$minidledays)
		  continue;
		$foo = $lastexpnodelogins["date"] . " " .
		 "(" . $lastexpnodelogins["uid"] . ")";
	    } elseif (TBExptState($pid,$eid)=="active") {
	        $daysidle=$row[lastswap];
	        $foo = "$date Swapped In";
	    }
	}

	if ($idle) $foo .= "</td><td align=center>$daysidle</td>
<td align=center valign=center>
<a href=\"request_swapexp.php3?pid=$pid&eid=$eid\">
<img src=\"redball.gif\"></a>";
	
	$usage_query =
	    DBQueryFatal("select nt.class, count(*) from reserved as r ".
			 "left join nodes as n ".
			 " on r.node_id=n.node_id ".
			 "left join node_types as nt ".
			 " on n.type=nt.type ".
			 "where r.pid='$pid' and r.eid='$eid' ".
			 "group by nt.class;");

	$usage["pc"]="";
	$usage["shark"]="";
	while ($n = mysql_fetch_array($usage_query)) {
	  $usage[$n[0]] = $n[1];
	}

	# in idle or active, skip experiments with no nodes (for now, node==pc)
	if (($idle || $active) && $usage["pc"]==0) continue;

	$total_pcs += $usage["pc"];
	$total_sharks += $usage["shark"];

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showexp.php3?pid=$pid&eid=$eid'>
                       $eid</A></td>
                <td>".$usage["pc"]." &nbsp;</td>\n";

	if ($isadmin) echo "<td>$foo</td>\n";

        echo "<td>$name</td>
                <td><A href='showuser.php3?target_uid=$huid'>
                       $huid</A></td>
               </tr>\n";
    }
    echo "</table>\n";
    echo "<center>\n<h2>Total PCs in $title experiments: $total_pcs<br>\n";#</h2>\n";
    echo "Total Sharks in $title experiments: $total_sharks<br>\n";
    echo "Total Nodes in $title experiments: ",$total_sharks+$total_pcs,"</h2>\n</center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
