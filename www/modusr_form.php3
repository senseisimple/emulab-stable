<html>
<head>
<title>Modify User Information</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can modify info.
#
$uid = "";
if (ereg("php3\?([[:alnum:]]+)", $REQUEST_URI, $Vals)) {
    $uid=$Vals[1];
    addslashes($uid);
}
else {
    unset($uid);
}
LOGGEDINORDIE($uid);

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
$usr_URL     = $row[usr_URL];
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
      </tr>\n";

echo "<tr>
          <td>*Full Name:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_name\" size=\"30\"
                     value=\"$usr_name\"></td>
      </tr>\n";

echo "<tr>
          <td>*Email Address:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_email\" size=\"30\"
                     value=\"$usr_email\"></td>
      </tr>\n";

echo "<tr>
         <td>Home Page URL:</td>
         <td class=\"left\">
             <input type=\"text\" name=\"usr_url\" size=\"45\"
                    value=\"$usr_URL\"></td>
      </tr>\n";

echo "<tr>
          <td>Expiration date:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_expires\"
                     value=\"$usr_expires\"></td>
      </tr>\n";

echo "<tr>
          <td>*Mailing Address:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_addr\" size=\"40\"
                     value=\"$usr_addr\"></td>
      </tr>\n";

echo "<tr>
          <td>*Phone #:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_phone\" size=\"15\"
                     value=\"$usr_phone\"></td>
      </tr>\n";

echo "<tr>
          <td>*Title/Position:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_title\" size=\"30\"
                     value=\"$usr_title\"></td>
     </tr>\n";

echo "<tr>
          <td>*Institutional Affiliation:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"usr_affil\" size=\"40\"
                     value=\"$usr_affil\"></td>
      </tr>\n";

echo "<tr>
          <td>New Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"new_password1\" size=\"8\"></td>
     </tr>\n";

echo "<tr>
          <td>Retype<br>New Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"new_password2\" size=\"8\"></td>
     </tr>\n";
?>
<td colspan="4" align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</center>
</body>
</html>
