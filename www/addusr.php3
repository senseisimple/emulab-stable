<?php
if (!isset($PHP_AUTH_USER)) {
  Header("WWW-Authenticate: Basic realm=\"Testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  echo "User authentication is required to view these pages\n";
  exit;
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid=\"$PHP_AUTH_USER\" AND usr_pswd=\"$PSWD\" AND trust_level > 0";
  $result = mysql_db_query("tbdb", $query);
  $numusers = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  if (($n == 0) && ($numusers != 0)) {
        $cmnd = "INSERT INTO login VALUES ('$PHP_AUTH_USER', '0')";
        mysql_db_query("tbdb", $cmnd);
  } else {
        $row = mysql_fetch_row($result2);
	if (($numusers  == 0) || ($row[0] < time())) {
                $cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
                mysql_db_query("tbdb", $cmnd);
  		Header("WWW-Authenticate: Basic realm=\"Testbed\"");
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
<title>New User</title>
<link rel=\"stylesheet\" href=\"tbstyle.css\" type=\"text/css\">
</head>
<body>
<H1>Add a new user to the testbed database</h1>\n";
$utime = time();
$year = date("Y", $utime);
++$year;
$time = date("m:d H:i:s", $utime);
echo "<table align=\"center\" border=\"1\">
<tr><td colspan=\"4\">Only fields in bold, red type are required</td></tr>
<form enctype=\"multipart/form-data\" action=\"added.php3\" method=\"post\">
	<tr><th>Username:</th><td><input type=\"text\" name=\"uid\"></td>
	<td>Expiration date:</td><td><input type=\"text\" name=\"usr_expires\" value=\"$year:$time\"></td></tr>	
	<tr><th>email:</th><td><input type=\"text\" name=\"usr_email\"></td>
	<td>Mailing Address:</td><td><input type\"text\" name=\"usr_addr\"></td></tr>
	<tr><th>Full Name:</th><td><input type=\"text\" name=\"usr_name\"></td>
 	<td>Phone #:</td><td><input type=\"text\" name=\"usr_phones\"></td></tr>
	<tr><th>Password:</th><td><input type=\"password\" name=\"pswd\"></td>
	<td rowspan=\"3\" colspan=\"2\">Or enter the name of a file containing a list of users to be added.  Users should be separated by a newline.  Each field of information should be separated by a tab.  The second field should be \"now\".  Each of these users will be given the password specified in the form above and assigned to the specified group.</td></tr>
	<tr><th>Retype Password:</th><td><input type=\"password\" name=\"pswd2\"></td></tr>
	<tr><th>Group:</th><th>\n";
                $query = "SELECT gid FROM grp_memb WHERE uid=\"$PHP_AUTH_USER\"";
                $result = mysql_db_query("tbdb", $query);
		$n = mysql_num_rows($result);
 		if ($n == 1) { # if only one option make a readonly field
			$row = mysql_fetch_row($result);
			echo "<input type=\"readonly\" value=\"$row[0]\" name=\"grp\"></th></tr>\n";
		} elseif ($n > 1) { # if more than one option make a select button
			echo "<select name=\"grp\">\n";
			while ($row = mysql_fetch_row($result)) {
                        	$gid = $row[gid];
                        	echo "<option value=$gid>$gid</option>\n";
                	}
			echo "</select></th></tr>\n";
		} else { # if no options say this
			echo "You don't seem to belong to any group.  This may be a problem.</th></tr>\n";
		}
?>
	<tr><th colspan="2" align="center"><input type="submit" value="Submit"></th>
	<td>Filename:</td><td><input type="file" name="filename"></td></tr>
</form>
</table>
</body>

</html>




