<?php
if (!empty($HTTP_GET_VARS)) {
	die("This form does not accept variables sent by get");
}
?>
<html>
<head>
<title>Group Request</title>
</head>
<body>
<?php
array_walk($HTTP_POST_VARS, 'addslashes');
if (isset($gid) && isset($password1) && isset($email) && ($password1 == $password2)) {
        $salt = strlen("$grp_head_uid");
	$enc = crypt("$password1", "$salt");
	$query = "SELECT usr_pswd FROM users WHERE uid=\"$grp_head_uid\"";
        $result = mysql_db_query("tbdb", $query);
        $query2 = "SELECT gid FROM groups WHERE gid=\"$gid\"";
	$result2 = mysql_db_query("tbdb", $query2);
	if ($row = mysql_fetch_row($result2)) {
		die("The group name you have chosen is already in use. Please select another");
	} elseif ($row = mysql_fetch_row($result)) {
		$usr_pswd = $row[0];
                if ($usr_pswd != $enc) {
                        die("<H3>The username that you have chosen is already in use.</h3>\n");
		}
	} else { #The uid and gid are not already in use
                $query3 = "SELECT unix_uid FROM users ORDER BY unix_uid DESC";
		$result3 = mysql_db_query("tbdb", $query2);
		$row = mysql_fetch_row($result3);
		$unix_uid = $row[0];
		++$unix_uid;
		$cmnd1 = "INSERT INTO users VALUES ('$grp_head_uid',
							now(),
							'$grp_expires',
							'$usr_name',
							'$email',
							'$usr_addr',
							'$usr_phones',
							'0',
							'$enc',
							'$unix_uid')";
                $cmndres1 = mysql_db_query("tbdb", $cmnd1);
		if (!$cmndres1) {
			$err = mysql_error();
			echo "<H3>Failed to add you to the database: $err</h3>\n";
			exit;
		}
        }
        $fp = fopen("/usr/local/share/apache/htdocs/testbed/maillist/leaders.txt", "a");
	$fp2 = fopen("/usr/local/share/apache/htdocs/testbed/maillist/users.txt", "a");
        fwrite($fp, "$email\n");   #Writes the email address to mailing lists
	fwrite($fp2, "$email\n");
        mail("axon@cs.utah.edu", "New Group", "$usr_name wants to start $gid. $why", "From $grp_head_uid <$email>");
	$ques = "SELECT unix_gid FROM groups ORDER BY unix_gid DESC";
        $resp = mysql_db_query("tbdb", $ques);
        $row = mysql_fetch_row($resp);
        $unix_gid = $row[0];
        ++$unix_gid;
	$cmnd2 = "INSERT INTO groups VALUES ('$gid',
					    now(), 
					    '$grp_expires',
					    '$grp_name',
					    '$grp_URL',
					    '$grp_affil',
					    '$grp_addr',
					    '$grp_head_uid', '',
					    '$unix_gid')";
	$cresult = mysql_db_query("tbdb", $cmnd2);
	if (!cresult) {
		$err = mysql_error();
		echo "<H3>Failed to add group: $err</h3>\n";
		exit;
	}
	$cmnd3 = "INSERT INTO grp_memb VALUES ('$grp_head_uid', '$gid')";
	mysql_db_query("tbdb", $cmnd3);
	echo "<H3>Information succesfully added</h3>\n";
} else { #if not enough information was given
	echo "<H3>Please return to <A href=\"addgrp.php3\">the form</A> and enter your user ID and/or email address and/or password.</h3>\n";
} 
?>
</body>
</html>



