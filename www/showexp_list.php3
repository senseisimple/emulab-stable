<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information Form");

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
	"where p.uid='$uid' and (trust='local_root' or trust='group_root') ".
	"order by e.pid,eid,expt_name");

    $batch_result = mysql_db_query($TBDBNAME,
	"select e.pid,eid,name from batch_experiments as e ".
	"left join proj_memb as p on p.pid=e.pid ".
	"where p.uid='$uid' and (trust='local_root' or trust='group_root') ".
	"order by e.pid,eid,name");
}
if (mysql_num_rows($experiments_result) == 0 &&
    mysql_num_rows($batch_result) == 0) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}

echo "<center>
       <h1>Experiment Information Listing</h1>
      </center>\n";

if (mysql_num_rows($experiments_result)) {
    echo "<center>
           <h2>Running Experiments</h2>
          </center>\n";
    
    echo "<table width=\"100%\" border=2
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <td>PID</td>
              <td>EID</td>
              <td>Name</td>
              <td align=center>Terminate</td>
            </tr>\n";

    while ($row = mysql_fetch_array($experiments_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$name = $row[expt_name];

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showexp.php3?exp_pideid=$pid\$\$$eid'>
                       $eid</A></td>
                <td>$name</td>
	        <td align=center>
                    <A href='endexp.php3?exp_pideid=$pid\$\$$eid'>
                       <img alt=\"o\" src=\"redball.gif\"></A></td>
               </tr>\n";
    }
    echo "</table>\n";
}

if (mysql_num_rows($batch_result)) {
    echo "<center>
           <h2>Batch Mode Experiments</h2>
          </center>\n";
    
    echo "<table width=\"100%\" border=2
                 cellpadding=0 cellspacing=2 align=center>
            <tr>
              <td>PID</td>
              <td>EID</td>
              <td>Name</td>
              <td align=center>Terminate</td>
            </tr>\n";

    while ($row = mysql_fetch_array($batch_result)) {
	$pid  = $row[pid];
	$eid  = $row[eid];
	$name = $row[name];

	echo "<tr>
                <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
                <td><A href='showbatch.php3?exp_pideid=$pid\$\$$eid'>
                       $eid</A></td>
                <td>$name</td>
	        <td align=center>
                    <A href='endbatch.php3?exp_pideid=$pid\$\$$eid'>
                       <img alt=\"o\" src=\"redball.gif\"></A></td>
               </tr>\n";
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
