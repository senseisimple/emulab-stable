<html>
<head>
<title>New User Verification</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
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
<h1>New User Verification</h1>
<p> The purpose of this page is to verify, for security purposes, that
information given in your application is correct. If you never received a
key at the email address given on your application, please contact
<a href="mailto:testbed-control@flux.cs.utah.edu">
Testbed Control (testbed-control@flux.cs.utah.edu)</a>
for further assistance.
</p>
<?php
if (isset($auth_usr)) {
  echo "<table align=\"center\" border=\"1\">\n";
  echo "<form action=\"verified.php3?$auth_usr\" method=\"post\">\n";
  echo "<tr><td>Username:</td><td>\n";
  echo "<input type=\"readonly\" name=\"uid\" value=\"$auth_usr\" size=15>\n";
  echo "</td>\n";
  echo "<tr><td>Password:</td><td><input type=\"password\" name=\"pswd\" size=15></td></tr>\n";
  echo "<tr><td>Key:</td><td><input type=\"text\" name=\"key\" size=15></td></tr>\n";
  echo "<td colspan=\"2\" align=\"center\">\n";
  echo "<b><input type=\"submit\" value=\"Submit\"></b></td></tr>\n";
  echo "</form>\n";
  echo "</table>\n";
} else {
  echo "
<h3>You are not logged in. You must first log in before attempting to
use this page.</h3>
";
}
?>
</body>
</html>




