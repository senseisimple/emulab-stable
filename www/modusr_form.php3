<html>
<head>
<title>Modify User Information</title>
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
# Only known and logged in users can modify info.
#
if (!isset($uid)) {
    USERERROR("You must be logged in to change your user information!", 1);
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
?>

<center>
<h1>Modify Your User Information</h1>
<table align="center" border="1">
  <tr><td align="center" colspan="4">
          Only fields marked with * are required
      </td>
  </tr>

<?php

#
# Suck the current info out of the database and break it apart.
#
$info_result = mysql_db_query($TBDBNAME,
	"select * from users where uid='$uid'");
if (! $info_result) {
    $err = mysql_error();
    TBERROR("Database Error getting user info for user $uid: $err\n", 1);
}

$row = mysql_fetch_array($info_result);
$usr_expires = $row[usr_expires];
$usr_email   = $row[usr_email];
$usr_addr    = $row[usr_addr];
$usr_name    = $row[usr_name];
$usr_phone   = $row[usr_phone];
$usr_passwd  = $row[usr_pswd];
$usr_title   = $row[usr_title];
$usr_affil   = $row[usr_affil];

#
# Generate the form.
# 
echo "<form action=\"modusr_process.php3?$uid\" method=\"post\">\n";
echo "<tr>
          <td>Username:</td>
          <td class=\"left\"> 
              <input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>

          <td>Expiration date:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_expires\"
                     value=\"$usr_expires\"></td>
      </tr>\n";

echo "<tr>
          <td>*Email Address:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_email\"
                     value=\"$usr_email\"></td>

          <td>*Mailing Address:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_addr\" 
                     value=\"$usr_addr\"></td>
      </tr>\n";

echo "<tr>
          <td>*Full Name:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_name\"
                     value=\"$usr_name\"></td>

          <td>*Phone #:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_phone\"
                     value=\"$usr_phone\"></td>
      </tr>\n";

echo "<tr>
          <td>*Old Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"old_password\"></td>

          <td>*Title/Position:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_title\"
                     value=\"$usr_title\"></td>
     </tr>\n";

echo "<tr>
          <td>New Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"new_password1\"></td>

          <td>*Institutional<br>Affiliation:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_affil\"
                     value=\"$usr_affil\"></td>
     </tr>\n";

echo "<tr>
          <td>Retype<br>New Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"new_password2\"></td>
     </tr>\n";
?>
<td colspan="4" align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</center>
</body>
</html>
