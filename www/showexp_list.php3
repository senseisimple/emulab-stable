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

$isadmin = ISADMIN($uid);

#
# Show a menu of all experiments for all projects that this uid
# is a member of. Or, if an admin type person, show them all!
#
if ($isadmin) {
    $experiments_result = mysql_db_query($TBDBNAME,
	"select pid,eid,expt_name from experiments ".
	"order by pid,eid,expt_name");

    $batch_result = mysql_db_query($TBDBNAME,
	"select pid,eid,name from batch_experiments ".
	"order by pid,eid,name");
}
else {
    $experiments_result = mysql_db_query($TBDBNAME,
	"select e.pid,eid,expt_name from experiments as e ".
	"left join proj_memb as p on p.pid=e.pid ".
	"where p.uid='$uid' ".
	"order by e.pid,eid,expt_name");

    $batch_result = mysql_db_query($TBDBNAME,
	"select e.pid,eid,name from batch_experiments as e ".
	"left join proj_memb as p on p.pid=e.pid ".
	"where p.uid='$uid' ".
	"order by e.pid,eid,name");
}
if (mysql_num_rows($experiments_result) == 0 &&
    mysql_num_rows($batch_result) == 0) {
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
              <td width=10%>PID</td>
              <td width=10%>EID</td>
              <td width=3%>PCs</td>
              <td width=3%>Sharks</td>
              <td width=5% align=center>Terminate</td>
              <td width=65%>Name</td>
            </tr>\n";

    while ($row = mysql_fetch_array($experiments_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$name = $row[expt_name];

	$usage_query = mysql_db_query($TBDBNAME,
	  "select n.type, count(*) from reserved as r left join nodes as n ".
	  "on r.node_id=n.node_id where r.pid='$pid' and r.eid='$eid' ".
	  "group by n.type;");

	$usage["pc"]="";
	$usage["shark"]="";
	while ($n = mysql_fetch_array($usage_query)) {
	  $usage[$n[0]] = $n[1];
	}

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showexp.php3?exp_pideid=$pid\$\$$eid'>
                       $eid</A></td>
                <td>".$usage["pc"]." &nbsp;</td>
                <td>".$usage["shark"]." &nbsp;</td>
	        <td align=center>
                    <A href='endexp.php3?exp_pideid=$pid\$\$$eid'>
                       <img alt=\"o\" src=\"redball.gif\"></A></td>
                <td>$name</td>
               </tr>\n";
    }
    echo "</table>\n";
}

if (mysql_num_rows($batch_result)) {
    echo "<center>
           <h2>Batch Mode Experiments</h2>
          </center>\n";
    
    echo "<table border=2
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <td width=10%>PID</td>
              <td width=20%>EID</td>
              <td width=5% align=center>Terminate</td>
              <td width=65%>Name</td>
            </tr>\n";

    while ($row = mysql_fetch_array($batch_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$name = $row[name];

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showbatch.php3?exp_pideid=$pid\$\$$eid'>
                       $eid</A></td>
	        <td align=center>
                    <A href='endbatch.php3?exp_pideid=$pid\$\$$eid'>
                       <img alt=\"o\" src=\"redball.gif\"></A></td>
                <td>$name</td>
               </tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
