<?php
$contents = "<html><head>
		<title>TBDB Online Index</title>
		<link rel='stylesheet' href='tbstyle.css' type='text/css'>
		</head>
		<body>
		<H3 align='center'>Guide to the testbed database</h3>\n";
if (!isset($PHP_AUTH_USER)) {
  $contents .= "<p><A href='addgrp.php3'>Apply for a group</A></p>\n";
  $contents .= "<p><A href='addusr.php3'>Apply for group membership</A></p>\n";
} elseif (isset($logout)) { # a logout clause 
  addslashes($PHP_AUTH_USER);
  $cmnd = "UPDATE login SET timeout=\"0\" WHERE uid=\"$PHP_AUTH_USER\"";
  $result = mysql_db_query("tbdb", $cmnd);
  if (!$result) {
        $err = mysql_error();
        echo "Logout failed: $err";
  }
  echo "<html><head><title>Testbed Logout</title></head></html>\n";
  exit;
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen("$PHP_AUTH_USER"));
  $query = "SELECT trust_level FROM users WHERE uid='$PHP_AUTH_USER' AND usr_pswd='$PSWD'";
  $result = mysql_db_query("tbdb", $query);
  $valid = mysql_num_rows($result);
  if ($valid != 0) {
	$trust_row = mysql_fetch_row($result);
  	$trust = $trust_row[0];
  	$query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  	$result2 = mysql_db_query("tbdb", $query2);
  	$n = mysql_num_rows($result2);
  	if ($n == 0) {
        	$cmnd = "INSERT INTO login VALUES ('$PHP_AUTH_USER', '0')";
        	mysql_db_query("tbdb", $cmnd);
  	} else {
        	$row = mysql_fetch_row($result2);
        	if ($row[0] < time()) {
                	$cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
			mysql_db_query("tbdb", $cmnd);
			Header("WWW-Authenticate: Basic realm=\"testbed\"");
                	Header("HTTP/1.0 401 Unauthorized");
                	die ("Authorization Failed\n");
        	}
	}	
  	$timeout = time() + 600;
  	$cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$PHP_AUTH_USER\"";
  	mysql_db_query("tbdb", $cmnd);
	if ($trust == 1) {
		$contents .= "<p><A href='addgrp.php3'>Apply for a group</A></p>\n";
		$contents .= "<p><A href='modify.html'>Update user information</A></p>\n";
		$contents .= "<p><A href='addusr.php3'>Apply for membership with a different group</A></p>\n";
	} else {
		$contents .= "<p><A href='modify.html'>Update user information</A></p>\n";
		$contents .= "<p><A href='addusr.php3'>Add a user to your group</A></p>\n";
		$contents .= "<p><A href='addproj.php3'>Begin a project</A></p>\n";
		$contents .= "<p><A href='addexp.php3'>Begin an experiment</A></p>\n";
	}
	$contents .= "<table cellpadding='0' cellspacing='0' width='100%'>
  		<form action=index.php3 method='post' target='dynamic'>
    		<tr><th><input type='submit' value='Logout' name='logout'></th></tr>
  		</form></table>\n";
  }
}
$contents .= "</body></html>";
echo $contents;
?>

