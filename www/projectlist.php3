<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Clients who have actively used Emulab.net");

#
# We let anyone access this page. Its bascailly a pretty printed version of the
# current testbed clients, who have not opted out from this display.
#
# Complete information is better viewed with the "Project Information" link.
# That requires a logged in user though. 
#

#
# Get the project list.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid,name,URL,usr_affil FROM projects ".
	"left join users on projects.head_uid=users.uid ".
	"where public=1 and approved=1 order by pid");

if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project list: $err\n", 1);
}

# Not likely to happen!
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no projects!", 1);
}

echo "<center><h3>
      Here is a list of research groups using Emulab.Net
      </h3></center>\n";

echo "<table width=\"100%\" border=0 cellpadding=2 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td><h4>Name</td>
          <td><h4>Institution</td>
      </tr>\n";
echo "<tr></tr>\n";
echo "<tr></tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pname  = $projectrow[name];
    $url    = $projectrow[URL];
    $affil  = $projectrow[usr_affil];

    echo "<tr>\n";

    if (!$url || strcmp($url, "") == 0) {
	echo "<td>$pname</td>\n";
    }
    else {
	echo "<td><A href=\"$url\">$pname</A></td>\n";
    }

    echo "<td>$affil</td>\n";

    echo "</tr>\n";

}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
