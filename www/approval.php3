<?php
if (!isset($PHP_AUTH_USER) || !empty($HTTP_GET_VARS)) {
  Header("WWW-Authenticate: Basic realm=\"Testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  echo "User authentication is required to view these pages\n";
  exit;
} else {
  $USER = addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($USER));
  $query = "SELECT * FROM users WHERE uid=\"$USER\" AND usr_pswd=\"$PSWD\" AND trust_level > 1";
  $result = mysql_db_query("tbdb", $query);
  $numusers = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  $row = mysql_fetch_row($result2);
  if (($n == 0) && ($numusers != 0)) {
        $cmnd = "INSERT INTO login VALUES ('$USER', '0')";
        mysql_db_query("tbdb", $cmnd);
  } elseif (($numusers  == 0) || ($row[0] < time())) {
        $cmnd = "DELETE FROM login WHERE uid=\"$USER\"";
        mysql_db_query("tbdb", $cmnd);
        Header("WWW-Authenticate: Basic realm=\"Testbed\"");
        Header("HTTP/1.0 401 Unauthorized");
        die ("Authorization Failed\n");
  }
  $timeout = time() + 1800;
  $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$USER\"";
  mysql_db_query("tbdb", $cmnd);
}
echo "
<html>
<head>
<title>Approve users</title>";
#<link rel='stylesheet' href='tbstyle.css' type='text/css'>
echo "</head>
<body>\n";
if (isset($OK)) {
	while ($elem = each($HTTP_POST_VARS)) {
		$uid = $elem[0];
		$act = $elem[1];
		if ($act == "reject") {
			$cmnd1 = "DELETE FROM users WHERE uid='$uid'";
			$cmnd2 = "DELETE FROM grp_memb WHERE uid='$uid'";
			$cmnd3 = "DELETE FROM proj_memb WHERE uid='$uid'";
			mysql_db_query("tbdb", $cmnd1);
			mysql_db_query("tbdb", $cmnd2);
			mysql_db_query("tbdb", $cmnd3);
			echo "<h3 color='red'>$uid DELETED</h3>\n";
		} elseif ($act == "accept") {
			$cmnd = "UPDATE users SET trust_level=1 where uid='$uid'";
			mysql_db_query("tbdb", $cmnd);
			echo "<h3 color='blue'>$uid APPROVED</h3>\n";
		} elseif ($act == "postpone") {
			echo "<h3>$uid is waiting</h3>\n";
		} else {
			echo "<h1>Something is Wrong: $uid, $act</h1>\n";
		}
	}
	die("</body></html>");
}
echo "
<h1>Approve new users in your group</h1>
<table border=1 cellpadding=3 cellspacing=0>
<tr><th cellvalign>Reject</th>
<th cellvalign>Postpone Judgement</th>
<th cellvalign>Accept</th>
<th cellvalign>User</th></tr>
<form action='approval.php3' method='post'>\n";

$query = "SELECT gid FROM grp_memb WHERE uid='$USER'";
$result = mysql_db_query("tbdb", $query);
$select = "SELECT";
while ($row = mysql_fetch_row($result)) {
	$gid = $row[0];
	if ($select == "SELECT") {
		$select .= " DISTINCT uid FROM grp_memb WHERE gid='$gid'";
	} else {
		$select .= " OR gid='$gid'";
	}
}
$selected = mysql_db_query("tbdb", $select);
$find = "SELECT";
while ($row = mysql_fetch_row($selected)) {
	$uid = $row[0];
	if ($find = "SELECT") {
		$find .= " DISTINCT uid FROM users WHERE trust_level=0 AND uid='$uid'";
	} else {
		$find .= " OR uid='$uid'";
	}
}
$found = mysql_db_query("tbdb", $find);
while ($row = mysql_fetch_row($found)) {
	$uid = $row[0];
	echo "<tr><th><input type='radio' name='$uid' value='reject'></th>
	      	 <th><input type='radio' name='$uid' value='postpone'></th>
	         <th><input type='radio' name='$uid' value='accept'></th>
		 <th>$uid</th></tr>\n";
}
echo "
<tr><th colspan=4><input type='submit' value='Okay' name='OK'></th></tr>
</form>
</table>
</body>
</html>";
?>