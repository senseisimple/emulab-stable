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
  if (($n == 0) && ($valid != 0)){
        $cmnd = "INSERT INTO login VALUES ('$PHP_AUTH_USER', '0')";
        mysql_db_query("tbdb", $cmnd);
  } else {
        $row = mysql_fetch_row($result2);
        if (($valid == 0) || ($row[0] < time())) {
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
$n = mysql_num_rows($response);
if ($n > 1) { #make a select button only if necessary.
	echo "<select name=\"uid\">\n";
}
while ($row = mysql_fetch_row($response)) {
	$gid = $row[0];
	$cmnd = "SELECT * FROM grp_memb where gid='$gid'";
	$result = mysql_db_query("tbdb", $cmnd);
	if (!$result) {
		$err = mysql_error();
		die("<H1>Failure in execution: $err</h1>\n");
	} elseif ((mysql_num_rows($result) == 1) && ($n == 1)) {
		$row = mysql_fetch_array($result);
		echo "<input type=\"readonly\" value=\"$row[uid]\" name=\"uid\">\n";
	} else {
		if ($n == 1) { #make a select button only if necessary
			echo "<select name=\"uid\">\n";
		}
		while ($row = mysql_fetch_array($result)) {
			$uid = $row[uid];
			echo "<option value=\"$uid\">$uid</option>\n";
		}
		if ($n == 1) {
			echo "</select>\n";
		}
	}
}
if ($n > 1) {
	echo "</select>\n";
}
?>

<input type="submit" value="Okay">
</td></tr>
</table>
</form>
</body>
</html>

