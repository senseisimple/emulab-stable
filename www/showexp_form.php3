<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information Form");

#
# Only known and logged in users can end experiments.
#
$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
    $uid=$Vals[1];
    addslashes($uid);
} else {
    unset($uid);
}
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Show a menu of all experiments for all projects that this uid
# is a member of. Or, if an admin type person, show them all!
#
if ($isadmin) {
    $projmemb_result = mysql_db_query($TBDBNAME,
	"SELECT DISTINCT pid FROM proj_memb");
    if (mysql_num_rows($projmemb_result) == 0) {
        USERERROR("There are no experiments to ".
                  "show any experiment information", 1);
    }
}
else {
    $projmemb_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\"");
    if (mysql_num_rows($projmemb_result) == 0) {
        USERERROR("You are not a member of any Projects, so you cannot ".
                  "show any experiment information", 1);
    }
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

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
