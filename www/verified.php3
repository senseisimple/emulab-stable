<html>
<head>
<title>Confirming Verification</title>
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
    echo "<h3>You are not logged in. Please go back to the
<a href=\"tbdb.html\">Home Page</a> and log in first.</h3></body></html>";
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
<h1>Confirming Verification...</h1>
<?php
if (isset($uid) && isset($pswd) && isset($key)) {
  $match = crypt("TB_".$uid."_USR",strlen($uid)+13);
  if ($key==$match) {
    $cmd = "select usr_pswd from users where uid='$uid'";
    $result = mysql_db_query("tbdb", $cmd);
    $row = mysql_fetch_row($result);
    $usr_pswd = $row[0];
    $salt = substr($usr_pswd,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    $PSWD = crypt("$pswd",$salt);
    if ($PSWD == $usr_pswd) {
      $cmd = "select status from users where uid='$uid'";
      $result = mysql_db_query("tbdb", $cmd);
      $row = mysql_fetch_row($result);
      $status = $row[0];
      if ($status=="unverified") {
	$cmd = "update users set status='active' where uid='$uid'";
	mysql_db_query("tbdb", $cmd);
	echo "<h3>Because your group leader has already approved ".
	  "your membership in the group, you are now an active user ".
	  "of the Testbed. Reload this page, and any options that are ".
	  "now available to you will appear. ".
	  "Thanks for using the Testbed!</h3>\n";
      } elseif ($status=="newuser") {
	$cmd = "update users set status='unapproved' where uid='$uid'";
	mysql_db_query("tbdb", $cmd);
	echo "<h3>You have now been verified. However, your application ".
	  "has not yet been approved by the group leader. You will receive ".
	  "email when that decision has been made. Thanks for ".
	  "using the Testbed!</h3>\n";
      } else {
	echo "<h3>You have already been verified, $uid. If you did not ".
	  "perform this verification, please notify ".
	  "<a href=\"mailto:testbed-control@flux.cs.utah.edu\">".
	  "Testbed Control (testbed-control@flux.cs.utah.edu)</a> immediately.</h3>\n";
      }
    } else {
      echo "<h3>The given password is incorrect. Please go back to ".
	"<a href=\"verify.php3?$uid\">New User Verification</a> and ".
	"enter the correct password and key.</h3>\n";
    }
  } else {
    echo "<h3>The given key is incorrect. Please go back to ".
      "<a href=\"verify.php3?$uid\">New User Verification</a> and ".
      "enter the correct password and key.</h3>\n";
  }
} else {
  echo "<h3>The username, password or key are invalid. Please go back to ".
    "<a href=\"verify.php3?$uid\">New User Verification</a> and ".
    "enter the correct password and key.</h3>\n";
}
?>
<p>Please contact 
<a href="mailto:testbed-control@flux.cs.utah.edu">Testbed Control
(testbed-control@flux.cs.utah.edu)</a> 
if you need further assistance.
</p>
</body>
</html>




