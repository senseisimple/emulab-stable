<?php
if (!isset($PHP_AUTH_USER)) {
  Header("WWW-Authenticate: Basic realm=\"testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  die("User authentication is required to view these pages\n");
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid=\"$PHP_AUTH_USER\" AND usr_pswd=\"$PSWD\" AND trust_level > 0";
  $result = mysql_db_query("tbdb", $query);
  $valid = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  $row = mysql_fetch_row($result2);
  if (($n == 0) || ($valid == 0) || ($row[0] < time())) {
     	$cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
        mysql_db_query("tbdb", $cmnd);
	Header("WWW-Authenticate: Basic realm=\"testbed\"");
       	Header("HTTP/1.0 401 Unauthorized");
        die ("Authorization Failed\n");
  }
  $timeout = time() + 1800;
  $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$PHP_AUTH_USER\"";
  mysql_db_query("tbdb", $cmnd);
}
echo "
<html>
<head>
<title>Users</title>
</head>
<body>
<form action=\"usrmod.php3\" target=\"modify\" method=\"post\">
<table border=\"1\"><tr><th>Select the user to be modified</th></tr>
<tr><td>\n";
$query = "SELECT gid FROM grp_memb where uid=\"$PHP_AUTH_USER\"";
$response = mysql_db_query("tbdb", $query);
$select = "SELECT";
while ($row = mysql_fetch_row($response)) {
	$gid = $row[0];
	if ($select == "SELECT") {
		$select .= " DISTINCT uid FROM grp_memb WHERE gid='$gid'";
	} else {
		$select .= " OR gid='$gid'";
	}
}
$selected = mysql_db_query("tbdb", $select);
if (!$selected) die("Failure in execution of database query</td></tr></table></body></html>");
$n = mysql_num_rows($selected);
if ($n == 1) {
	$uid_row = mysql_fetch_row($selected);
	echo "<input type='readonly' value='$uid_row[0]' name='uid'>\n";
} else {
	echo "<select name='uid'>\n";
	while ($uid_row = mysql_fetch_row($selected)) {
		echo "<option value='$uid_row[0]'>$uid_row[0]</option>\n";
	}
	echo "</select>\n";
}
?>

<input type="submit" value="Okay">
</td></tr>
</table>
</form>
</body>
</html>

