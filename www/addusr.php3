<html>
<head>
<title>New User</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $uid=$Vals[1];
  addslashes($uid);
} else {
  unset($uid);
}
echo "<h1>Apply for Group Membership</h1>\n";
echo "<table align=\"center\" border=\"1\">\n";
echo "<tr><td align='center' colspan=\"4\">\n";
echo "Only fields in bold, red type are required</td></tr>\n";
if (isset($uid)) {
  echo "<form action=\"added.php3?$uid\" method=\"post\">\n";
  echo "<input type=\"hidden\" name=\"logged_in\" value=\"true\">";
  echo "<tr><td>Username:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>";
  $query = mysql_db_query("tbdb","select usr_expires,usr_email,usr_addr,usr_name,usr_phone,usr_pswd from users where uid='$uid'");
  $row = mysql_fetch_row($query);
  echo "<td>Expiration date:</td>";
  echo "<td class=\"left\"><input type=\"readonly\" name=\"usr_expires\" ";
  echo "value=\"$row[0]\"</td></tr>\n";
  echo "<tr><td>email:</td><td class=\"left\"><input type=\"readonly\" ";
  echo "name=\"usr_email\" value=\"$row[1]\"></td>";
  echo "<td>Mailing Address:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_addr\" ";
  echo "value=\"$row[2]\"></td></tr>";
  echo "<tr><td>Full Name:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_name\" ";
  echo "value=\"$row[3]\"></td>";
  echo "<td>Phone #:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_phone\" ";
  echo "value=\"$row[4]\"></td></tr>";
  echo "<tr><td>Password:</td><td>";
  echo "<input type=\"password\" name=\"pswd\"></td>";
  echo "<td>Retype Password:</td><td>";
  echo "<input type=\"hidden\" name=\"pswd2\" ";
  echo "value=\"$row[5]\">&nbsp;</td></tr>";
} else {
  echo "<form action=\"added.php3\" method=\"post\">\n";
  echo "<tr><td>Username:</td><td><input type=\"text\" name=\"uid\"></td>";
  echo "<td>Expiration date:</td>";
  echo "<td><input type=\"text\" name=\"usr_expires\"";
  $time = date("m/d/Y", time() + (86400 * 90)); #add 90 days
  echo "value=\"$time\"></td></tr>\n";
  echo "<tr><td>email:</td><td><input type=\"text\" name=\"usr_email\"></td>";
  echo "<td>Mailing Address:</td><td>";
  echo "<input type\"text\" name=\"usr_addr\"></td></tr>";
  echo "<tr><td>Full Name:</td><td>";
  echo "<input type=\"text\" name=\"usr_name\"></td>";
  echo "<td>Phone #:</td><td>";
  echo "<input type=\"text\" name=\"usr_phone\"></td></tr>";
  echo "<tr><td>Password:</td><td>";
  echo "<input type=\"password\" name=\"pswd\"></td>";
  echo "<td>Retype Password:</td><td>";
  echo "<input type=\"password\" name=\"pswd2\"></td></tr>";
}  
echo "<tr><td>Group:</td><td><b>";
$query = "SELECT gid FROM groups";
$result = mysql_db_query("tbdb", $query);
$n = mysql_num_rows($result);
if ($n == 1) { # if only one option make a readonly field
  $row = mysql_fetch_row($result);
  echo "<input type=\"readonly\" value=\"$row[0]\" name=\"grp\"></td>\n";
} elseif ($n > 1) { # if more than one option make a select button
  echo "<select name=\"grp\">\n";
  while ($row = mysql_fetch_row($result)) {
    $gid = $row[0];
    echo "<option value=$gid>$gid</option>\n";
  }
  echo "</select></td>\n";
} else { # if no options say this
  echo "There don't seem to be any groups in the database</td>\n";
}
?>
<td colspan="2" align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</body>
</html>




