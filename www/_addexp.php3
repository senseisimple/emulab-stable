<?php
if (!isset($PHP_AUTH_USER)) {
  Header("WWW-Authenticate: Basic realm=\"testbed\"");
  Header("HTTP/1.0 401 Unauthorized");
  echo "User authenication is required to view these pages\n";
  exit;
} else {
  addslashes($PHP_AUTH_USER);
  $PSWD = crypt("$PHP_AUTH_PW", strlen($PHP_AUTH_USER));
  $query = "SELECT * FROM users WHERE uid=\"$PHP_AUTH_USER\" AND usr_pswd=\"$PSWD\"";
  $result = mysql_db_query("tbdb", $query);
  $numusers = mysql_num_rows($result);
  $query2 = "SELECT timeout FROM login WHERE uid=\"$PHP_AUTH_USER\"";
  $result2 = mysql_db_query("tbdb", $query2);
  $n = mysql_num_rows($result2);
  $row = mysql_fetch_row($result2);
  if (($n == 0) || ($numusers == 0) || ($row[0] < time())) {
     	$cmnd = "DELETE FROM login WHERE uid=\"$PHP_AUTH_USER\"";
        mysql_db_query("tbdb", $cmnd);
	Header("WWW-Authenticate: Basic realm=\"testbed\"");
        Header("HTTP/1.0 401 Unauthorized");
        die ("Authorization Failed\n");
  }
  $timeout = time() + 1800;
  $cmnd = "UPDATE login SET timeout=\"$timeout\" where uid=\"$PHP_AUTH_USER\"";
  mysql_db_query("tbdb", $cmnd);
}
?>

<html>
<head>
<title>New Experiment</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<H1>Begin an experiment on the testbed</h1>
<table border="1" align="center">
<tr><td colspan="2">Only those fields in bold, red type are required.</td></tr>
<form action=added.php3 method="post">
	<tr><th>Experiment Name:</th><td><input type="text" name="eid"></td></tr>

	<?php
	addslashes($PHP_AUTH_USER);
	$query = "SELECT pid FROM proj_memb WHERE uid=\"$PHP_AUTH_USER\"";
	$result = mysql_db_query("tbdb", $query);
	$n = mysql_num_rows($result);
	if ($n == 1) {
		echo "<tr><th>Project ID:</th>";
		$row = mysql_fetch_row($result);
		echo "<td><input type=\"readonly\" value=\"$row[0]\" name=\"proj\"></td></tr>\n";
	} elseif ($n > 1) {
		echo "<tr><th>Project ID:</th><td><select name=\"proj\">\n";
		while ($row = mysql_fetch_row($result)) {
			echo "<option value=\"$row[0]\">$row[0]</option>\n";
		}
		echo "</select></td></tr>\n";
	} else {
		echo "<tr><th colspan=\"2\">You must be part of a project if you wan to run an experiment</th></tr>";
	}
	$utime = time();
	$year = date("Y", $utime);
	$month = date("m", $utime);
	$thismonth = $month++;
	if ($month > 12) {
		$month -= 12;
		$month = "0".$month;
	}
	$rest = date("d H:i:s", $utime);
	echo "<tr><th>Expiration date:</th><td><input type=\"text\" value=\"$year:$month:$rest\" name=\"expt_expires\"></td></tr>
	<tr><td>Experiment long name:</td><td><input type=\"text\" name=\"expt_name\"></td></tr>
	<tr><td>Experiment starts:</td><td><input type=\"text\" value=\"$year:$thismonth:$rest\" name=\"expt_start\"></td></tr>
	<tr><td>Experiment ends:</td><td><input type=\"text\" value=\"$year:$month:$rest\" name=\"expt_end\"></td></tr>
  	<tr><th colspan=\"2\"><input type=\"submit\" value=\"Submit\"></th></tr>\n";
	?>

  </form>
  </table>
  </body>
  </html>