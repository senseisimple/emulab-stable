<?php
if (!isset($PHP_AUTH_USER) || !empty($HTTP_GET_VARS)) {
  Header("WWW-Authenticate: Basic realm=\"testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  echo ("User authentication is required to view these pages\n");
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid='$PHP_AUTH_USER' AND usr_pswd='$PSWD' AND trust_level > 0";
  $result = mysql_db_query("tbdb", $query);
  $valid = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  if (($n == 0) && ($valid != 0)){
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
<title>Modify $uid</title>
</head>
<body>
";

if (isset($HTTP_POST_VARS)) {
array_walk($HTTP_POST_VARS, "addslashes");
}
if (isset($update)) { #if the form was submitted with the update button, update the database
	echo "<H1>Updating the Database...</h1>\n";
  	$cmnd = "UPDATE users SET usr_expires=\"$usr_expires\",
                            usr_name=\"$usr_name\",
                            usr_email=\"$usr_email\",
                            usr_addr=\"$usr_addr\",
                            usr_phones=\"$usr_phones\",
                            trust_level=\"$trust_level\" WHERE uid=\"$uid\"";
  	$result = mysql_db_query("tbdb", $cmnd);
  	$succ = mysql_affected_rows($result);
  	if ($succ == 0) {
        	$err = mysql_error();
        	echo "<H3>Could not query database: $err</h3>\n";
        	exit;
  	} elseif (($old_pw != $new_pw) && ($new_pw == $new_pw2)) {
        	$enc = crypt("$new_pw", strlen($uid));
        	$pwcom = "UPDATE users SET usr_pswd=\"$enc\" WHERE uid=\"$uid\"";
        	$pres = mysql_db_query("tbdb", $pwcom);
        	if (!$pres) {
                	$err = mysql_error();
                	die ("<H3>Failed to change password: $err</h3>");
        	}
   	}
   	echo "<H3>$uid UPDATED</h3>";
} elseif (isset($delete)) { #if the form was submitted with the delete button, delete the user
  	$cmnd = "DELETE FROM users WHERE uid=\"$uid\"";
  	$result = mysql_db_query("tbdb", $cmnd);
  	$succ = mysql_affected_rows($result);
  	if ($succ == 0) {
       		$err = mysql_error();
       		die ("<H3 color=red>Could not query database: $err</h3>\n");
 	}
  	$cmnd2 = "DELETE FROM grp_memb WHERE uid=\"$uid\"";
  	mysql_db_query("tbdb", $cmnd2);
 	$cmnd3 = "DELETE FROM proj_memb WHERE uid=\"$uid\"";
 	mysql_db_query("tbdb", $cmnd3);
 	echo "<H3>$uid DELETED</h3>";
} elseif (isset($uid)) { #when coming from usrs.php3, display user info in a form to be altered
	echo "<H3>Modify only those entries you wish to change</h3>
	<table border = \"1\" summary=\"Modify entries in the table and submit it to change the databse\">
	<form action=\"usrmod.php3\" method=\"post\">\n";
        $cmnd = "SELECT * FROM users WHERE uid=\"$uid\"";
        $result = mysql_db_query("tbdb", $cmnd);
        $row = mysql_fetch_array($result);
        print "<tr><th>Username</th>
                <td><input type=\"text\" name=\"uid\" value=$uid></td></tr>\n";
        print "<tr><th>Full Name</th>
                <td><input type=\"text\" name=\"usr_name\" value=\"$row[usr_name]\"></td></tr>\n";
        print "<tr><th>Email</th>
                <td><input type=\"text\" name=\"usr_email\" value=$row[usr_email]></td></tr>\n";
        print "<tr><th>Mailing Address</th>
                <td><input type=\"text\" name=\"usr_addr\" value=\"$row[usr_addr]\"></td></tr>\n";
        print "<tr><th>Phone Number</th>
                <td><input type=\"text\" name=\"usr_phones\" value=$row[usr_phones]></td></tr>\n";
        print "<tr><th>User Expires</th>
                <td><input type=\"text\" name=\"usr_expires\" value=\"$row[usr_expires]\"></td></tr>\n";
        print "<tr><th>Trust Level</th>
                <td><input type=\"text\" name=\"trust_level\" value=$row[trust_level]></td></tr>
        <tr><th>Old Password</th><td><input type=\"password\" name=\"old_pw\"></td></tr>
        <tr><th>New Password</th><td><input type=\"password\" name=\"new_pw\"></td></tr>
        <tr><th>Retype New Password</th><td><input type=\"password\" name=\"new_pw2\"></td></tr>
        </table>
        <p>
        <input type=\"submit\" value=\"Update\" name=\"update\">
        <input type=\"submit\" value=\"Delete User\" name=\"delete\">
        </p>
	</form>
	<form action=usrmod.php3 method=\"post\">
	<input type=\"submit\" value=\"Cancel\">
	</form>\n";
} else { #when no variable are passed to the form, ask for some
	echo "<H1>Please provide a testbed username</h1>";
}
echo "
</body>
</html>
";
?>