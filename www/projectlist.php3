<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Clients of Emulab.Net");

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
	"SELECT * FROM projects where public=1 and approved=1 order by pid");

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

echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Name</td>
          <td>URL</td>
      </tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pname  = $projectrow[name];
    $url    = $projectrow[URL];

    echo "<tr>
              <td>$pname</td>\n";

    if (!$url || strcmp($url, "") == 0) {
	echo "<td>&nbsp</td>\n";
    }
    else {
	echo "<td><A href=\"$url\">$url</A></td>\n";
    }

    echo "</tr>\n";

}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
