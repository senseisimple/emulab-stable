<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Project Information List");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can see all projects, while normal users can only see
# projects for which they are the leader.
#
# XXX Should we form the list from project members instead of leaders?
#
$isadmin = ISADMIN($uid);

#
# Get the project list.
#
if ($isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects order by pid");
}
else {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects where head_uid='$uid' order by name");
}
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project list: $err\n", 1);
}

if (mysql_num_rows($query_result) == 0) {
	if ($isadmin) {
	    USERERROR("There are no projects!", 1);
	}
	else {
	    USERERROR("You are not a leader of any projects!", 1);
	}
}

if ($isadmin) {
    #
    # Process the query results for active projects so I can generate a
    # summary block (as per Jays request).
    #
    $total_projects  = mysql_num_rows($query_result);
    $active_projects = 0;

    while ($projectrow = mysql_fetch_array($query_result)) {
	$expt_count = $projectrow[expt_count];

	if ($expt_count > 0) {
	    $active_projects++;
	}
    }
    mysql_data_seek($query_result, 0);
    $never_active = $total_projects - $active_projects;

    echo "<table border=0 align=center cellpadding=0 cellspacing=3>
        <tr>
          <td align=right><b>Been Active:</b></td>
          <td align=left>$active_projects</td>
        </tr>
        <tr>
          <td align=right><b>Never Active:</b></td>
          <td align=left>$never_active</td>
        </tr>
        <tr>
          <td align=right><b>Total:</b></td>
          <td align=left>$total_projects</td>
        </tr>
      </table>\n";
}

#
# Now the project list.
#
echo "<table width=\"100%\" border=2 cellpadding=2 cellspacing=0 align=center>
      <tr>
          <td>PID</td>
          <td>Name</td>
          <td>Leader</td>
          <td align=center>Days<br>Idle</td>
          <td align=center>Expts<br>Created</td>
          <td align=center>Expts<br>Running</td>
          <td align=center>Nodes</td>
	  <td align=center>Approved?</td>\n";

#
# Admin users get a "delete" option.
# 
if ($isadmin) {
    echo "<td align=center>Delete</td>\n";
}
echo "</tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid        = $projectrow[pid];
    $headuid    = $projectrow[head_uid];
    $Pname      = $projectrow[name];
    $approved   = $projectrow[approved];
    $created    = $projectrow[created];
    $expt_count = $projectrow[expt_count];
    $expt_last  = $projectrow[expt_last];

    echo "<tr>
              <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td>$Pname</td>
              <td><A href='showuser.php3?target_uid=$headuid'>
                     $headuid</A></td>\n";

    #
    # Sleazy! Use mysql query to convert dates to days and subtract!
    #
    if (isset($expt_last)) {
	$idle_query = mysql_db_query($TBDBNAME,
		"SELECT TO_DAYS(CURDATE()) - TO_DAYS(\"$expt_last\")");
	$idle_row   = mysql_fetch_row($idle_query);
	echo "<td>$idle_row[0]</td>\n";
    }
    else {
	$idle_query = mysql_db_query($TBDBNAME,
		"SELECT TO_DAYS(CURDATE()) - TO_DAYS(\"$created\")");
	$idle_row   = mysql_fetch_row($idle_query);
	echo "<td>$idle_row[0]</td>\n";
    }

    echo "<td>$expt_count</td>\n";

    $count1 = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM experiments where pid='$pid'");
    $row1   = mysql_fetch_array($count1);
	
    $count2 = mysql_db_query($TBDBNAME,
		"SELECT count(*) FROM reserved where pid='$pid'");
    $row2   = mysql_fetch_array($count2);
	
    echo "<td>$row1[0]</td><td>$row2[0]</td>\n";

    if ($approved) {
	echo "<td align=center><img alt=\"Y\" src=\"greenball.gif\"></td>\n";
    }
    else {
	echo "<td align=center><img alt=\"N\" src=\"redball.gif\"></td>\n";
    }

    if ($isadmin) {
	echo "<td align=center><A href='deleteproject.php3?pid=$pid'>
                     <img alt=\"o\" src=\"redball.gif\"></A></td>\n";
    }
    echo "</tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
