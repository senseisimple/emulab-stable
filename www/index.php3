<html>
<head>
<title>Utah Network Testbed</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
<base href='https://plastic.cs.utah.edu/' target='dynamic'>
</head>
<body>
<a href="welcome.html"><h3>Utah Network Testbed</h3></a>
<?php
if (isset($login)) {
  unset($login);
  if (isset($auth_usr)) {
    addslashes($auth_usr);
    $query = "SELECT usr_pswd FROM users WHERE uid=\"$auth_usr\"";
    $result = mysql_db_query("tbdb", $query);
    $row = mysql_fetch_row($result);
    $usr_pswd = $row[0];
    #print "Got $usr_pswd from Database\n<br>";
    $salt = substr($usr_pswd,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    #print "Got $salt for salt\n<br>";
    $PSWD = crypt("$auth_passwd",$salt);
    #echo "<pre>GOT PWD $PSWD</pre>";
    if ($PSWD == $usr_pswd) {
      $query2 = "SELECT timeout FROM login WHERE uid=\"$auth_usr\"";
      $result2 = mysql_db_query("tbdb", $query2);
      $exists = mysql_num_rows($result2);
      $timeout = time() + 86400;
      if ($exists) {
	$cmnd="update login set timeout='$timeout' where uid='$auth_usr'";
	mysql_db_query("tbdb", $cmnd);
      } else {
	$c="insert into login (uid,timeout) values ('$auth_usr','$timeout')";
	mysql_db_query("tbdb", $c);
      }
      echo "$auth_usr Logged in!";  
    } else {
      echo "Login Failed\n";
      unset($auth_usr);
    }
  } else {
    echo "Login Failed\n";
    unset($auth_usr);
  }
} elseif (isset($logout)) { # a logout clause 
  unset($logout);
  addslashes($auth_usr);
  $cmnd = "delete from login WHERE uid=\"$auth_usr\"";
  $result = mysql_db_query("tbdb", $cmnd);
  if (!$result) {
    $err = mysql_error();
    echo "Logout failed: $err";
  } else {
    echo "<p>You have been logged out, $auth_usr.</p>";
    unset($auth_usr);
  }
} elseif (isset($auth_usr)) {
  #Check login...
  addslashes($auth_usr);
  $query = "SELECT timeout FROM login WHERE uid=\"$auth_usr\"";
  $result = mysql_db_query("tbdb", $query);
  $n = mysql_num_rows($result);
  if ($n == 0) {
    echo "<h3>You are not logged in. Please go back to the
<a href=\"tbdb.html\"> Home Page </a> and log in first.\n";
  } else {
    $row = mysql_fetch_row($result);
    if ($row[0] < time()) { # if their login expired
      echo "<h3>You have been logged out due to inactivity.
Please log in again.</h3>\n";
      $cmnd = "DELETE FROM login WHERE uid=\"$auth_usr\"";
      mysql_db_query("tbdb", $cmnd);
    } else {
      $timeout = time() + 86400;
      $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$auth_usr\"";
      mysql_db_query("tbdb", $cmnd);
    }
  }
}
if (isset($auth_usr)) {
  echo "<hr>";
  $query="SELECT status FROM users WHERE uid='$auth_usr'";
  $result = mysql_db_query("tbdb", $query);
  $status_row = mysql_fetch_row($result);
  $status = $status_row[0];
  $query="SELECT trust FROM grp_memb WHERE uid='$auth_usr'";
  $result = mysql_db_query("tbdb", $query);
  $row = mysql_fetch_row($result);
  $trust = $row[0];
  if ($status == "active") {
    if ($trust == "group_root") {
      # Only group leaders can do these options
      echo "<A href='approval.php3?$auth_usr'>New User Approval</A>\n";
      echo "<p>Add a New User";
      #echo "<p><A href='addusr.php3?$auth_usr'>Add a New User</A>";
      echo "<p>Begin a Project";
      #echo "<p><A href='addproj.php3?$auth_usr'>Begin a Project</A>\n";
      echo "</p>";
    }
    if (($trust == "group_root") || ($trust == "local_root")) {
      # Only local root people can do these options
      echo "Begin an Experiment";
      #echo "<p><A href='addexp.php3?$auth_usr'>Begin an Experiment</A>\n";
      echo "</p>";
    }
    # Every active user can do these options
    echo "<b>Update user information";
    #echo "<p><A href='modify.html'>Update user information</A>\n";
    echo "</p>\n";
  } elseif ($status == "unapproved") {
    echo "Your account has not been approved yet. Please try back ";
    echo "later. Contact ";
    echo "<a href=\"mailto:testbed-control@flux.cs.utah.edu\">";
    echo "Testbed Control (testbed-control@flux.cs.utah.edu)</a>";
    echo " for further assistance.\n";  
  } elseif (($status == "newuser") || ($status == "unverified")) {
    echo "<A href='verify.php3?$auth_usr'>New User Verification</A>\n";
  } elseif (($status == "frozen") || ($status == "other")) {
    echo "Your account has been changed to status $status, and is ";
    echo "currently unusable. Please contact your group leader to find out ";
    echo "why. If you need further help, contact ";
    echo "<a href=\"mailto:testbed-control@flux.cs.utah.edu\">";
    echo "Testbed Control (testbed-control@flux.cs.utah.edu)</a>.";
  }
}
?>
<hr>
<?php
echo "<A href='addgrp.php3";
if (isset($auth_usr)) { echo "?$auth_usr"; }
echo "'>Start a Project</A>\n";
echo "<p><A href='addusr.php3";
if (isset($auth_usr)) { echo "?$auth_usr"; }
echo "'>Join a Project</A>";
?>
<hr><A href='faq.html'>Testbed FAQ</a></p>
<table cellpadding='0' cellspacing='0' width="100%">
<form action="index.php3" method='post' target='fixed'>
<?php
if (!isset($auth_usr)) {
  echo "<tr><td>Username:<input type='text' name='auth_usr' size=8></td></tr><tr><td>Password:<input type='password' name='auth_passwd' size=8></td></tr><tr><td align='center'><b><input type='submit' value='Login' name='login'></td></tr></b>";
} else {
  echo "<tr><td><input type='hidden' name='auth_usr' value='$auth_usr'</td><td align='center'><b><input type='submit' value='Logout' name='logout'></b></td></tr>";
}
?>
</form>
</table>
</body>
</html>



