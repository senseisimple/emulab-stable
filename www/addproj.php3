<?php
if (!isset($PHP_AUTH_USER)) {
  Header("WWW-Authenticate: Basic realm=\"testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  die("User authenication is required to view these pages\n");
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid=\"$PHP_AUTH_USER\" AND usr_pswd=\"$PSWD\" AND trust_level > 0";
  $result = mysql_db_query("tbdb", $query);
  $numusers = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  if (($n == 0) && ($numusers != 0)){
        $cmnd = "INSERT INTO login VALUES ('$PHP_AUTH_USER', '0')";
        mysql_db_query("tbdb", $cmnd);
  } else {
        $row = mysql_fetch_row($result2);
        if (($numusers == 0) || ($row[0] < time())) {
                $cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
                mysql_db_query("tbdb", $cmnd);
		Header("WWW-Authenticate: Basic realm=\"testbed\"");
                Header("HTTP/1.0 401 Unauthorized");
                die ("Authorization Failed\n");
        }
  }
  $timeout = time() + 1800;
  $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$PHP_AUTH_USER\"";
  mysql_db_query("tbdb", $cmnd);
}
?>

<html>
<head>
<title>New Project</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<H1>Begin a project</h1>

<?php
addslashes($PHP_AUTH_USER);
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
$query = "SELECT gid FROM grp_memb WHERE uid=\"$PHP_AUTH_USER\"";
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