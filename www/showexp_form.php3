<html>
<head>
<title>Show Experiment Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $uid=$Vals[1];
  addslashes($uid);
} else {
  unset($uid);
}

#
# Only known and logged in users can do this.
#
if (!isset($uid)) {
    USERERROR("You must be logged in to show experiment information!", 1);
}

#
# Verify that the uid is known in the database.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid='$uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error confirming user $uid: $err\n", 1);
}
if (($row = mysql_fetch_row($query_result)) == 0) {
    USERERROR("You do not appear to have an account!", 1);
}


#
# Show a menu of all experiments for all projects that this uid
# is a member of.
#
# XXX Split across grp_memb and proj_memb. grp_memb needs to be flushed, but
# right now that has all the info we need. 
#
$groupmemb_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM grp_memb WHERE uid=\"$uid\"");
if (mysql_num_rows($groupmemb_result) == 0) {
  USERERROR("You are not a member of any Projects, so you cannot ".
            "show any experiment information", 1);
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

#
# Suck the current info out of the database and display a list of
# experiments as an option list.
#
echo "<tr>";
echo "    <td><select name=\"exp_eid\">";
               while ($grprow = mysql_fetch_array($groupmemb_result)) {
                  $pid = $grprow[gid];
		  $exp_result = mysql_db_query($TBDBNAME,
			"SELECT eid FROM experiments WHERE pid=\"$pid\"");
                  while ($exprow = mysql_fetch_array($exp_result)) {
                      $eid = $exprow[eid];
                      echo "<option value=\"$eid\">$eid</option>\n";
                  }
             }
echo "       </select>";
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
