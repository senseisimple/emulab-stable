<?php
if (!isset($PHP_AUTH_USER) || !empty($HTTP_GET_VARS)) {
  Header("WWW-Authenticate: Basic realm=\"testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  die ("User authentication is required to view these pages\n");
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid=\"$PHP_AUTH_USER\" AND usr_pswd=\"$PSWD\" AND trust_level!=\"0\"";
  $result = mysql_db_query("tbdb", $query);
  $valid = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  if (($n == 0) && ($valid != 0)) {
        $cmnd = "INSERT INTO login VALUES ('$PHP_AUTH_USER', '0')";
        mysql_db_query("tbdb", $cmnd);
  } else {
        $row = mysql_fetch_row($result2);
        if (($valid == 0) || ($row[0] < time())) {
                $cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
                mysql_db_query("tbdb", $cmnd);
		Header("WWW-Authenticate: Basic realm=\"testbed\"");
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
<title>Altering TBDB</title>
</head>
<body>
<h1>Adding information to the testbed database...</h1>\n";

array_walk($HTTP_POST_VARS, 'addslashes');
if (isset($pid)) { #add a project to the database
	$cmnd = "INSERT INTO projects VALUES ('$pid', 
					      now(), 
					      '$proj_expires',
					      '$proj_name', 
					      '$PHP_AUTH_USER')";
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
} elseif (!empty($uid) && ($pswd == $pswd2)) { #add a user to the database
	$query = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
	$res = mysql_db_query("tbdb", $query);
	$row = mysql_fetch_row($res);
	$unix_uid = $row[0];
	++$unix_uid;
	$enc = crypt("$pswd", strlen($uid));
	$cmnd = "INSERT INTO users VALUES (
					'$uid',
					now(),
					'$usr_expires', 
					'$usr_name', 
					'$usr_email', 
					'$usr_addr', 
					'$usr_phones', 
					'1',
					'$enc',
					'$unix_uid')";
   	$result = mysql_db_query("tbdb", $cmnd); #put the user into users
   	if (!$result) {
		$err = mysql_error();
		echo "<H3>Could not add user to the database: $err</h3>\n";
		exit;
   	}
   	$cmnd2 = "INSERT INTO grp_memb VALUES ('$uid', '$grp')"; 
   	mysql_db_query("tbdb", $cmnd2); #put the user and group into grp_memb
	$fp = fopen("/usr/local/share/apache/htdocs/testbed/maillist/users.txt", "a");
	fwrite($fp, "$usr_email\n");  #write the user's email to the mail list
} elseif ($filename != "" && isset($filename) && ($pswd == $pswd2)) {
/* if a file of user information is specified, load the file into the database. Then update all the entries just made, and add them to grp_memb. */
	$cmnd = "LOAD DATA LOCAL INFILE '$filename' INTO TABLE users";
	$result = mysql_db_query("tbdb", $cmnd);
   	if (!$result) {
		$err = mysql_error();
		echo "<H3>Could not query database: $err</h3>\n";
		exit;
   	}
   	$time = mysql_db_query("tbdb", "SELECT now()");
   	$row = mysql_fetch_row($time);
   	$now = $row[0]; #get the current time in mysql format
   	$query = "UPDATE users SET usr_created='$now' where usr_created=\"0000-00-00 00:00:00\"";
   	mysql_db_query("tbdb", $query); #set the time of creation of the users to now
   	$query2 = "SELECT uid, usr_email FROM users WHERE usr_created=\"$now\"";
   	$result2 = mysql_db_query("tbdb", $query2); #get all the users just added
	$query3 = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
	$result3 = mysql_db_query("tbdb", $query3);
	$row3 = mysql_fetch_row($result3);
	$unix_uid = $row3[0];
   	while ($row = mysql_fetch_array($result2)) {
		++$unix_uid;
		$uid = $row[uid];
		$enc = crypt("$pswd", strlen($uid));
		$email = $row[usr_email];
		$cmnd2 = "INSERT INTO grp_memb VALUES ('$uid', '$grp')";
		mysql_db_query("tbdb", $cmnd2); #add users to grp_memb
		$cmnd3 = "UPDATE users SET usr_pswd='$enc', unix_uid='$unix_uid' where uid='$uid'";
		mysql_db_query('tbdb', $cmnd3); #give users unix_uids
		$fp = fopen("/usr/local/share/apache/htdocs/testbed/maillist/users.txt", "a");
		fwrite($fp, "$email\n"); #write email addresses to the mail list
   	}
} elseif (isset($eid) && isset($proj)) { #start an experiment for a project
	$cmnd = "INSERT INTO experiments VALUES ('$eid',
						 '$proj',
						 now(),
						 '$expt_expires',
						 'expt_name',
						 '$PHP_AUTH_USER',
						 '$expt_start',
						 '$expt_end')";
	$result = mysql_db_query("tbdb", $cmnd);
	if (!result) {
		$err = mysql_error();
		echo "<H3>Failed to add experiment: $err</h3>\n";
		exit;
	}
} else {
	echo "<H3>There was a problem with the information recieved, please return to the form and check to be sure you have correctly filled all required fields. </h3>\n";
}
?>
</body>
</html>