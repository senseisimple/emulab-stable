<html>
<head>
<title>New User</title>
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
# If a uid came in, verify that it is known in the database
#
if (isset($uid)) {
    $query_result = mysql_db_query($TBDBNAME,
	    "SELECT * FROM users WHERE uid='$uid'");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error confirming user $uid: $err\n", 1);
    }
    if (($row = mysql_fetch_array($query_result)) == 0) {
        USERERROR("You do not appear to have an account!", 1);
    }
    $usr_expires = $row[usr_expires];
    $usr_email   = $row[usr_email];
    $usr_addr    = $row[usr_addr];
    $usr_name    = $row[usr_name];
    $usr_phone   = $row[usr_phone];
    $usr_passwd  = $row[usr_pswd];
    $usr_title   = $row[usr_title];
    $usr_affil   = $row[usr_affil];
}

echo "<h1>Apply for Project Membership</h1>\n";
echo "<table align=\"center\" border=\"1\">\n";
echo "<tr><td align='center' colspan=\"4\">\n";
echo "Only fields marked with * are required</td></tr>\n";
if (isset($uid)) {
  echo "<form action=\"usradded.php3?$uid\" method=\"post\">\n";
  echo "<input type=\"hidden\" name=\"logged_in\" value=\"true\">";
  echo "<tr><td>*Username:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>";
  echo "<td>Expiration date:</td>";
  echo "<td class=\"left\"><input type=\"readonly\" name=\"usr_expires\" ";
  echo "value=\"$usr_expires\"</td></tr>\n";
  echo "<tr><td>*Email Address:</td><td class=\"left\"><input type=\"readonly\" ";
  echo "name=\"usr_email\" value=\"$usr_email\"></td>";
  echo "<td>Mailing Address:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_addr\" ";
  echo "value=\"$usr_addr\"></td></tr>";
  echo "<tr><td>*Full Name:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_name\" ";
  echo "value=\"$usr_name\"></td>";
  echo "<td>Phone #:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_phone\" ";
  echo "value=\"$usr_phone\"></td></tr>";
  echo "<tr><td>*Password:</td><td>";
  echo "<input type=\"password\" name=\"password1\"></td>";
  echo "<td>*Title/Position:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_title\"";
  echo "value=\"$usr_title\"></td>";
  echo "</tr>";
  echo "<tr><td></td><td></td>";
  echo "<td>*Institutional<br>Affiliation:</td><td class=\"left\">";
  echo "<input type=\"readonly\" name=\"usr_affil\"";
  echo "value=\"$usr_affil\"></td>";
  echo "</tr>";
} else {
  echo "<form action=\"usradded.php3\" method=\"post\">\n";
  echo "<tr><td>*Username:</td><td><input type=\"text\" name=\"uid\"></td>";
  echo "<td>Expiration date:</td>";
  echo "<td><input type=\"text\" name=\"usr_expires\"";
  $time = date("m/d/Y", time() + (86400 * 90)); #add 90 days
  echo "value=\"$time\"></td></tr>\n";
  echo "<tr><td>*Email Address:</td><td><input type=\"text\" name=\"usr_email\"></td>";
  echo "<td>Mailing Address:</td><td>";
  echo "<input type\"text\" name=\"usr_addr\"></td></tr>";
  echo "<tr><td>*Full Name:</td><td>";
  echo "<input type=\"text\" name=\"usr_name\"></td>";
  echo "<td>Phone #:</td><td>";
  echo "<input type=\"text\" name=\"usr_phone\"></td></tr>";
  echo "<tr><td>*Password:</td><td>";
  echo "<input type=\"password\" name=\"password1\"></td>";
  echo "<td>*Title/Position:</td>";
  echo "<td><input type=\"text\" name=\"usr_title\"></td>";
  echo "</tr>";
  echo "<tr><td>*Retype<br>Password:</td><td>";
  echo "<input type=\"password\" name=\"password2\"></td>";
  echo "<td>*Institutional<br>Affiliation:</td>";
  echo "<td><input type=\"text\" name=\"usr_affil\"></td>";
  echo "</tr>";
}
echo "<tr><td>*Project:</td><td>";
echo "<input type=\"text\" name=\"grp\"></td>";
echo "</tr>";
# This used to give the selection box with all the groups...
#$query = "SELECT gid FROM groups";
#$result = mysql_db_query("tbdb", $query);
#$n = mysql_num_rows($result);
#if ($n == 1) { # if only one option make a readonly field
#  $row = mysql_fetch_row($result);
#  echo "<input type=\"readonly\" value=\"$row[0]\" name=\"grp\"></td>\n";
#} elseif ($n > 1) { # if more than one option make a select button
#  echo "<select name=\"grp\">\n";
#  while ($row = mysql_fetch_row($result)) {
#    $gid = $row[0];
#    echo "<option value=$gid>$gid</option>\n";
#  }
#  echo "</select></td>\n";
#} else { # if no options say this
#  echo "There don't seem to be any groups in the database</td>\n";
#}
?>
<td colspan="4" align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</body>
</html>




