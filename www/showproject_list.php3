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
	"SELECT pid,head_uid,name,approved FROM projects order by pid");
}
else {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects where head_uid='$uid' order by pid");
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

echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Project Info</td>
          <td>Name</td>
          <td>Leader</td>
          <td>Expts</td>
          <td>Nodes</td>
	  <td align=center>Approved?</td>\n";

#
# Admin users get a "delete" option.
# 
if ($isadmin) {
    echo "<td align=center>Delete</td>\n";
}
echo "</tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid      = $projectrow[pid];
    $headuid  = $projectrow[head_uid];
    $Pname    = $projectrow[name];
    $approved = $projectrow[approved];
    
    echo "<tr>
              <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td>$Pname</td>
              <td><A href='showuser.php3?target_uid=$headuid'>
                     $headuid</A></td>\n";

    if ($isadmin) {
      $count1 = mysql_db_query($TBDBNAME,
      "SELECT count(*) FROM experiments where pid='$pid'");
      $row1 = mysql_fetch_array($count1);
      $count2 = mysql_db_query($TBDBNAME,
      "SELECT count(*) FROM reserved where pid='$pid'");
      $row2 = mysql_fetch_array($count2);
      if ($row1[0] > 0) {
	echo "<td>$row1[0]</td><td>$row2[0]</td>\n";
      } else { echo "<td>&nbsp;</td><td>&nbsp;</td>\n"; }
    }

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
