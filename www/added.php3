<?php
echo "
<html>
<head>
<title>Adding to the database</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<h1>Adding information to the Testbed Database</h1>\n";
$my_passwd=$pswd;
$mypipe = popen(escapeshellcmd(
    "/usr/testbed/bin/checkpass $my_passwd $grp_head_uid '$usr_name:$email'"),
    "w+");
if ($mypipe) {
  $retval=fgets($mypipe,1024);
  if (strcmp($retval,"ok\n")!=0) {
    die("<h3>The password you have chosen will not work:<p>$retval</h3>");
  }
} else {
  mail("testbed-www@flux.cs.utah.edu","TESTBED: checkpass failure",
       "\n$usr_name ($grp_head_uid) just tried to set up a testbed account,\n".
       "but checkpass pipe did not open (returned '$mypipe').\n".
       "\nThanks\n");
}
$enc = crypt("$my_passwd");
array_walk($HTTP_POST_VARS, 'addslashes');
if (isset($pid)) { #add a project to the database
  if ($trust == 2) {
    $cmnd = "INSERT INTO projects VALUES ".
       "('$pid',now(),'$proj_expires','$proj_name','$PHP_AUTH_USER')";
    $result = mysql_db_query("tbdb", $cmnd);
    if (!$result) {
      $err = mysql_error();
      echo "<H3>Couldn't add project to the database: $err</h3>\n";
      exit;
    }
    $cmnd2 = "INSERT INTO proj_grps VALUES ('$pid', '$grp_assoc')";
    mysql_db_query("tbdb", $cmnd2);
    $cmnd3 = "INSERT INTO proj_memb VALUES ('$PHP_AUTH_USER', '$pid')";
    mysql_db_query("tbdb", $cmnd3);
    if ($proj_memb != "") {
      $memb = preg_split("/\s/", "$proj_memb");
      function add_memb ($item) {
	global $pid;
	trim($item);
	$insert = "INSERT INTO proj_memb VALUES ('$item', '$pid')";
	mysql_db_query("tbdb", $insert);
      }
      array_walk ($memb, "add_memb");
    }
    echo "<h1>Success</h1>";
  } else {
    mysql_db_query("tbdb","select usr_name,usr_email from groups as g ".
		   "left join users as u on g.grp_head_uid=u.uid ".
		   "where g.gid = '$grp_assoc'");
    $row = mysql_fetch_row($head);
    $grp_head = $row[0];
    $email = $row[1];
    echo "<h1>Add Project Failed:</h1>\n<h3>You are not authorized to add ".
      "projects in group '$grp_assoc'. If you feel you have reached this ".
      "message in error, please contact the group head, ".
      "'$grp_head <$email>'.</h3>";
  }
} elseif ( !empty($uid) && !empty($usr_email) &&
	   (($pswd == $pswd2) || ($enc == $pswd2)) ) {
  $query = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
  $res = mysql_db_query("tbdb", $query);
  $row = mysql_fetch_row($res);
  $unix_uid = $row[0];
  ++$unix_uid;
  $query = "SELECT usr_pswd FROM users WHERE uid=\"$uid\"";
  $result = mysql_db_query("tbdb", $query);
  if ($row = mysql_fetch_row($result)) {
    # returning user, joining new group
    $usr_pswd = $row[0];
    if ($usr_pswd != $enc) {
      die("<H3>The username that you have chosen is already in use. ".
	  "Please select another.</h3>\n");
    }
  } else { # new user
    $newuser=1;
    $cmnd = "INSERT INTO users ".
       "(uid,usr_created,usr_expires,usr_name,usr_email,usr_addr,".
       "usr_phone,usr_pswd,unix_uid,status) ".
       "VALUES ('$uid',now(),'$usr_expires','$usr_name','$usr_email',".
       "'$usr_addr','$usr_phone','$enc','$unix_uid','newuser')";
    $result = mysql_db_query("tbdb", $cmnd);
    if (!$result) {
      $err = mysql_error();
      echo "<H3>Could not add user to the database: $err</h3>\n";
      exit;
    }
    $fp = fopen("/usr/testbed/www/maillist/users.txt","a");
    fwrite($fp, "$usr_email\n");
  }
  $result=mysql_db_query("tbdb","select * from grp_memb where ".
			 "uid='$uid' and gid='$grp'");
  if (mysql_num_rows($result) > 0) {
    # Already in that group (or applied)
    echo "<h3>You have already applied for membership in that group.</h3>\n";
    echo "</body></html>\n";
    exit;
  }
  mysql_db_query("tbdb","insert into grp_memb (uid,gid,trust)".
		 "values ('$uid','$grp','none');");
  $que = "SELECT grp_head_uid FROM groups WHERE gid='$grp'";
  $res = mysql_db_query("tbdb", $que);
  $resrow = mysql_fetch_row($res);
  $ghid = $resrow[0];
  $mque = "SELECT usr_email FROM users WHERE uid='$ghid'";
  $mres = mysql_db_query("tbdb", $mque);
  $mresrow = mysql_fetch_row($mres);
  $grp_email = $mresrow[0];
  mail("$grp_email", "TESTBED: New Group Member",
       "\n$usr_name ($uid) is trying to join your group. $usr_name has the\n".
       "Testbed username $uid and email address $usr_email.\n$usr_name's ".
       "phone number is $usr_phone and address $usr_addr.\n".
       "\nPlease return ".
       "to <https://plastic.cs.utah.edu/tbdb.html>, log in,\nand select the ".
       "'New User Approval' page to enter your decision regarding\n".
       "$usr_name's membership in your group.".
       "\n\nThanks,\nTestbed Control\nUtah Network Testbed\n",
       "From: Testbed Control <testbed-control@flux.cs.utah.edu>\n".
       "Cc: Testbed WWW <testbed-www@flux.cs.utah.edu>\n".
       "Errors-To: Testbed WWW <testbed-www@flux.cs.utah.edu>");
  if ($newuser==1) {
    mail("$usr_email","TESTBED: Your New User Key",
	 "\nDear $usr_name:\n\n\tThank you for applying to use the Utah ".
	 "Network Testbed. As promised,\nhere is your key to verify your ".
	 "account. Your key is:\n\n".
	 crypt("TB_".$uid."_USR",strlen($uid)+13)."\n\n\t Please ".
	 "return to <https://plastic.cs.utah.edu/tbdb.html> and log in,\n".
	 "using the user name and password you gave us when you applied. ".
	 "You will\nthen find an option on the menu called ".
	 "'New User Verification'. Select it,\nand on that page enter in ".
	 "your user name, password, and your key,\nand you will be ".
	 "verified as a user. When you have been ".
	 "both verified and\napproved by the head of your group, you will be ".
	 "marked as an active user,\nand will be granted full access to your ".
	 "user account.\n\nThanks,\nTestbed Control\nUtah Network Testbed\n",
         "From: Testbed Control <testbed-control@flux.cs.utah.edu>\n".
         "Cc: Testbed WWW <testbed-www@flux.cs.utah.edu>\n".
         "Errors-To: Testbed WWW <testbed-www@flux.cs.utah.edu>");
    echo "
<h3> As a new user of the Testbed, for
security purposes, you will receive by e-mail a key. When you
receive it, come back to the site, and log in. When you do, you
will see a new menu option called 'New User Verification'. On
that page, enter in your username, password, and the key,
exactly as you received it in your e-mail. You will then be
marked as a verified user.</h3>
<h3>Once you have been both verified
and approved, you will be classified as an active user, and will 
be granted full access to your user account.</h3> 
";
  }
  echo "
<h3>The leader of group '$grp' has been notified of your application. He
will make a decision and either approve or deny your application, and you
will be notified as soon as a decision has been made.
 Thanks for using the Testbed! </h3>
";
} elseif (isset($eid) && isset($proj)) { #start an experiment for a project
  $cmnd = "INSERT INTO experiments VALUES ('$eid','$proj',now(),".
     "'$expt_expires','expt_name','$PHP_AUTH_USER','$expt_start','$expt_end')";
  $result = mysql_db_query("tbdb", $cmnd);
  if (!result) {
    $err = mysql_error();
    echo "<H3>Failed to add experiment: $err</h3>\n";
    exit;
  }
} else {
  echo "<H3>There was a problem with the information received, please return to the form and check to be sure you have correctly filled all required fields. </h3>\n";
}
?>
</body>
</html>
