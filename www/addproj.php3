<html>
<head>
<title>New Project</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<H1>Create a New Project</h1>
<?php
$auth_usr = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $auth_usr=$Vals[1];
  addslashes($auth_usr);
  $query = "SELECT timeout FROM login WHERE uid=\"$auth_usr\"";
  $result = mysql_db_query("tbdb", $query);
  $n = mysql_num_rows($result);
  if ($n == 0) {
    echo "<h3>You are not logged in. Please go back to the ";
    echo "<a href=\"tbdb.html\" target=\"_top\"> Home Page </a> ";
    echo "and log in first.</h3></body></html>";
    exit;
  } else {
    $row = mysql_fetch_row($result);
    if ($row[0] < time()) { # if their login expired
      echo "<h3>You have been logged out due to inactivity.
Please log in again.</h3>\n</body></html>";
      $cmnd = "DELETE FROM login WHERE uid=\"$auth_usr\"";
      mysql_db_query("tbdb", $cmnd);
      exit;
    } else {
      $timeout = time() + 86400;
      $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$auth_usr\"";
      mysql_db_query("tbdb", $cmnd);
    }
  }
} else {
  unset($auth_usr);
}
?>
<?php
$utime = time();
$year = date("Y", $utime);
$month = date("m", $utime);
$month += 6;
if ($month > 12) {
	$month -= 12;
	$month = "0".$month;
}
$rest = date("d H:i:s", $utime);
echo "<table border=\"1\" align=\"center\">
<form action=added.php3 method=\"post\">
<tr><td colspan=\"2\">Only fields in bold red are required</td></tr>
<tr><th>Project Name:</th><td><input type=\"text\" name=\"pid\"></td></tr>
<tr><th>Group association:</th>\n";
$query = "SELECT gid FROM grp_memb WHERE uid=\"$auth_usr\"";
$result = mysql_db_query("tbdb", $query);
$n = mysql_num_rows($result);
if ($n > 1) {
	echo "<td><select name=\"grp_assoc\">\n";
	while ($row = mysql_fetch_row($result)) {
		echo "<option value=\"$row[0]\">$row[0]</option>\n";
	}
	echo "</select></td></tr>\n";
} else {
	$row = mysql_fetch_row($result);
        echo "<td><input type=\"readonly\" value=\"$row[0]\" name=\"grp_assoc\"></td></tr>\n";
}
echo "<tr><th>Expiration date:</th><td><input type=\"text\" value=\"$year:$month:$rest\" name=\"proj_expires\"></td></tr>\n";
?>
<tr><td>Project Long Name:</td><td><input type="text" name="proj_name"></td></tr>
<tr><td>Project Members:</td><td><textarea cols="20" rows="2" name="proj_memb"></textarea></td>
<tr><th colspan="2"><input type="submit" value="Submit"></th></tr>
</form>
</table>
</body>
</html>
