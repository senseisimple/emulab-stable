<html>
<head>
<title>Show Experiment Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can end experiments.
#
#
# Only known and logged in users can end experiments.
#
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
	"SELECT pid FROM projects");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting project list: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no projects!", 1);
}

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid      = $projectrow[pid];
    $headuid  = $projectrow[head_uid];
    $Purl     = $projectrow[URL];
    $Pname    = $projectrow[name];
    $Paffil   = $projectrow[affil];

}

#
# Lets see if the user is even part of any experiements before
# presenting a bogus option list.
#
$experiments = "";
while ($projrow = mysql_fetch_array($projmemb_result)) {
    $pid = $projrow[pid];
    $exp_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM experiments WHERE pid=\"$pid\"");
    while ($exprow = mysql_fetch_array($exp_result)) {
        $eid = $exprow[eid];
        $experiments = "$experiments " .
                      "<option value=\"$pid\$\$$eid\">$pid/$eid</option>\n";
    }
}
if (strcmp($experiments, "") == 0) {
    USERERROR("There are no experiments running in any of the projects ".
              "you are a member of.", 1);
}

?>

<center>
<h1>Experiment Information Selection</h1>
<h2>Select an experiment from the list below.<br>
These are the experiments in the projects
you are a member of.</h2>
<table align="center" border="1">

<?php
echo "<form action=\"showexp.php3?$uid\" method=\"post\">";
echo "<tr>
          <td align='center'>Project/Experiment</td>
      </tr>\n";
echo "<tr></tr>";
echo "<tr></tr>";

#
# Suck the current info out of the database and display a list of
# experiments as an option list.
#
echo "<tr>";
echo "    <td><select name=\"exp_pideid\">";
echo "        $experiments";
echo "        </select>";
echo "    </td>
      </tr>\n";

?>
<td align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</center>
</body>
</html>
