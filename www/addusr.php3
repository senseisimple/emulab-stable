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
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid, then quit cause we don't want to display the
# personal information for some random ?uid argument.
#
if (isset($uid)) {
    if (CHECKLOGIN($uid) != 1) {
        USERERROR("You are not logged in. Please log in and try again.", 1);
    }
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

    $returning = 1;
}
else {
    $returning = 0;
}
?>

<table align="center" border="1"> 
<tr>
    <td colspan="2">
        <h1 align="center">Apply for Project Membership</h1></td>
</tr>

<tr>
    <td align="center" colspan="2">
        Fields marked with * are required.</td>
</tr>
<form action="usradded.php3" method="post">

<?php
if ($returning) {
    echo "<tr>
              <td>*Username:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"uid\" value=\"$uid\"></td>
          </tr>\n";

    echo "<tr>
              <td>*Full Name:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_name\"
                         value=\"$usr_name\"></td>
          </tr>\n";

    echo "<tr>
              <td>*Title/Position:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_title\"
                         value=\"$usr_title\"></td>
          </tr>\n";
    
    echo "<tr>
              <td>*Institutional<br>Affiliation:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_affil\"
                         value=\"$usr_affil\"></td>
          </tr>\n";

    echo "<tr>
              <td>*Email Address:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_email\"
                         value=\"$usr_email\"></td>
          </tr>\n";

    echo "<tr>
              <td>Mailing Address:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_addr\"
                         value=\"$usr_addr\"></td>
          </tr>\n";

    echo "<tr>
              <td>Phone #:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_phone\"
                         value=\"$usr_phone\"></td>
          </tr>\n";

    echo "<tr>
              <td>Expiration date:</td>
              <td class=\"left\">
                  <input type=\"readonly\" name=\"usr_expires\"
                         value=\"$usr_expires\"</td>
          </tr>\n";
}
else {
    echo "<tr>
              <td>*Username:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"uid\" size=8 maxlength=8></td>
          </tr>\n";

    echo "<tr>
              <td>*Full Name:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_name\" size=30></td>
          </tr>\n";

    echo "<tr>
              <td>*Title/Position:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_title\" size=30></td>
          </tr>\n";

    echo "<tr>
              <td>*Institutional<br>Affiliation:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_affil\" size=40></td>
          </tr>\n";

    echo "<tr>
              <td>*Email Address:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_email\" size=30></td>
          </tr>\n";

    echo "<tr>
              <td>Mailing Address:</td>
              <td class=\"left\">
                  <input type\"text\" name=\"usr_addr\" size=40></td>
          </tr>\n";

    echo "<tr>
              <td>Phone #:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_phone\" size=16></td>
          </tr>\n";

    $expiretime = date("m/d/Y", time() + (86400 * 90)); #add 90 days
    echo "<tr>
              <td>Expiration date:</td>
              <td class=\"left\">
                  <input type=\"text\" name=\"usr_expires\" size=10
                         value=\"$expiretime\"></td>
          </tr>\n";

    echo "<tr>
              <td>*Password:</td>
              <td><input type=\"password\" name=\"password1\" size=12></td>
          </tr>
          
          <tr>
              <td>*Retype Password:</td>
              <td><input type=\"password\" name=\"password2\" size=12></td>
          </tr>\n";
}

#
# The only common field!
#
# XXX Note CONSTANT size in expression: PID is 12 chars max.
# 
echo "<tr>
          <td>*Project:</td>
          <td class=\"left\">
              <input type=\"text\" name=\"pid\" size=12></td>
      </tr>\n";

?>
<td colspan="2" align="center">
<b><input type="submit" value="Submit"></b></td></tr>
</form>
</table>
</body>
</html>




