<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminate Experiment List");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Lets see if the user is even part of any experiements 
#
$query_result = mysql_db_query($TBDBNAME,
	"select e.pid,eid,expt_name from experiments as e ".
	"left join proj_memb as p on p.pid=e.pid where p.uid='$uid' ".
	"order by e.pid,eid");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}
?>

<center>
<h1>Terminate Experiment Selection</h1>
<h2>Select an experiment from the list below.<br>
These are the experiments in the projects
you are a member of.</h2>
<table width="100%" border=2 cellpadding=0 cellspacing=2 align=center>
<tr>
     <td>PID</td>
     <td>EID</td>
     <td align=center>Name</td>
     <td align=center>Terminate</td>
</tr>

<?php

while ($row = mysql_fetch_array($query_result)) {
    $pid  = $row[pid];
    $eid  = $row[eid];
    $name = $row[expt_name];

    echo "<tr>
              <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td><A href='showexp.php3?exp_pideid=$pid\$\$$eid'>$eid</A></td>
              <td>$name</td>
	      <td align=center><A href='endexp.php3?exp_pideid=$pid\$\$$eid'>
                     <img alt=\"o\" src=\"redball.gif\"></A></td>
          </tr>\n";
}

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
