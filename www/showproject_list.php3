<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information List");

#
# Only known and logged in users can end experiments.
#
#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can do this.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges!", 1);
}

#
# Show a menu of all projects.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project list: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no projects!", 1);
}

echo "<center><h3>
      Project List
      </h3></center>\n";

echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Project Info</td>
          <td>Name</td>
          <td>Leader</td>
          <td>Affiliation</td>
      </tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid      = $projectrow[pid];
    $headuid  = $projectrow[head_uid];
    $Pname    = $projectrow[name];
    $Paffil   = $projectrow[affil];

    echo "<tr>
              <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td>$Pname</td>
              <td><A href='showuser.php3?target_uid=$headuid'>
                     $headuid</A></td>
	      <td>$Paffil</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
