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
$altview     = 0;

if (isset($alternate_view) && $alternate_view) {
    echo "<b><a href='showexp_list.php3'>
                Show All Experiments</a>
          </b><br><br>\n";
    $altclause = "e.state='$TB_EXPTSTATE_ACTIVE'";
    $altview   = 1;
}
else {
    echo "<b><a href='showexp_list.php3?alternate_view=1'>
                Show Active Experiments Only</a>
          </b><br><br>\n";
}

#
# Show a menu of all experiments for all projects that this uid
# is a member of. Or, if an admin type person, show them all!
#
if ($isadmin) {
    if ($altview)
	$altclause = "where ($altclause)";
    
    $experiments_result =
	DBQueryFatal("select pid,eid,expt_head_uid,expt_name from ".
		     "experiments as e $altclause ".
		     "order by pid,eid,expt_name");
}
else {
    if ($altview)
	$altclause = "and ($altclause)";
    
    $experiments_result =
	DBQueryFatal("select distinct e.pid,eid,expt_head_uid,expt_name from ".
		     "experiments as e ".
		     "left join group_membership as g on g.pid=e.pid ".
		     "where g.uid='$uid' $altclause ".
		     "order by e.pid,eid,expt_name");
}
if (! mysql_num_rows($experiments_result)) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}

if (mysql_num_rows($experiments_result)) {
    echo "<center>
           <h2>Running Experiments</h2>
          </center>\n";
    
    echo "<table border=2 cols=0
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <td width=8%>PID</td>
              <td width=8%>EID</td>
              <td width=3%>PCs</td>\n";

    if ($isadmin)
	echo "<td width=17% align=center>Last Login</td>\n";

    echo "    <td width=60%>Name</td>
              <td width=4%>Head UID</td>
            </tr>\n";

    $total_pcs = 0;
    $total_sharks = 0;
    while ($row = mysql_fetch_array($experiments_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$huid = $row[expt_head_uid];
	$name = $row[expt_name];

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

	$total_pcs += $usage["pc"];
	$total_sharks += $usage["shark"];

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showexp.php3?pid=$pid&eid=$eid'>
                       $eid</A></td>
                <td>".$usage["pc"]." &nbsp</td>\n";

	if ($isadmin) {
	    $foo = "&nbsp";

	    if ($lastexpnodelogins = TBExpUidLastLogins($pid, $eid)) {
		$foo = $lastexpnodelogins["date"] . " " .
		 "(" . $lastexpnodelogins["uid"] . ")";
	    }

	    echo "<td>$foo</td>\n";
	}

        echo "<td>$name</td>
                <td><A href='showuser.php3?target_uid=$huid'>
                       $huid</A></td>
               </tr>\n";
    }
    echo "</table>\n";
    echo "<center>\n<h2>Total PCs in use: $total_pcs<br>\n";#</h2>\n";
    echo "Total Sharks in use: $total_sharks<br>\n";
    echo "Total Nodes in use: ",$total_sharks+$total_pcs,"</h2>\n</center>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
