<html>
<head>
  <title>New Group</title>
  <link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
  <H1 align="center">Apply to use the University of Utah's network testbed</h1>
  <table align="center" width="80%" border="1"> 
  <tr><td colspan="4">Only fields in bold, red type are required</td></tr>
  <form action=grpadded.php3 method="post">
	<tr><th colspan=2>Group Information</th>
	<th colspan=2>Group Head Information</th></tr>
	<tr><th>Group Name:</th><td><input type="text" name="gid"></td>
	<th>Username</th><td>
	<?php
	if (isset($PHP_AUTH_USER)) {
/* if this person is logged into the database, 
fill the user info fields with info from the database */
		$uid = addslashes($PHP_AUTH_USER);
		$query = "SELECT * FROM users WHERE uid=\"$uid\"";
		$result = mysql_db_query("tbdb", $query);
		$row = mysql_fetch_array($result);
		echo "<input type=\"readonly\" value=\"$row[uid]\" name=\"grp_head_uid\"></td></tr>\n";
	} else {
		echo "<input type=\"text\" name=\"grp_head_uid\"></td></tr>\n";
	}
	echo "<tr><td>Group long name:</td><td><input type=\"text\" name=\"grp_name\"></td>
	<th>Full Name:</th><td>";
	if (isset($row)) {
		echo "<input type=\"readonly\" value=\"$row[usr_name]\"";
	} else {
		echo "<input type=\"text\"";
	}
	echo "name=\"usr_name\"></td></tr>
	<tr><td>Group URL:</td><td><input type=\"text\" name=\"grp_URL\"></td>
	<th>Email Address:</th><td>";
	if (isset($row)) {
		echo "<input type=\"readonly\" value=\"$row[usr_email]\" ";
	} else {
		echo "<input type=\"text\" ";
	}
	echo "name=\"email\"></td></tr>
	<tr><th>When do you expect to be done using the testbed?</th>
	<td><input type=\"text\" value=";  #set a default expiration date 
		$time = time(); 
		$year = date("Y", $time); 
		++$year; 
		$mytime = date("m:d H:i:s", $time); 
		echo "\"$year:$mytime\""; 
		echo "name=\"grp_expires\"></td>
	<th>Mailing Address:</th><td>";
	if (isset($row)) {
		echo "<input type=\"readonly\" value=\"$row[usr_addr]\" name=\"usr_addr\">";
	} else {
		echo "<input type=\"text\"  name=\"usr_addr\">";
	}
	echo "</td></tr>
	<tr><td>Group Affiliation:</td><td><input type=\"text\" name=\"grp_affil\"></td>
	<th>Phone #:</th><td><input ";
	if (isset($row)) {
		echo "type=\"readonly\" value=\"$row[usr_phones]\"";
	} else {
		echo "type=\"text\"";
	} 
	echo "name=\"usr_phones\"></td></tr>\n";
	?>
	<tr><th>Password:</th><td><input type="password" name="password1"></td>
	<th>Retype Password:</th><td><input type="password" name="password2"></td></tr>
	<tr><th colspan="4">Please describe how and why you plan to use the testbed</th></tr> 
	<tr><td colspan="4" align="center"><textarea name="why" rows="10" cols="70"></textarea></td></tr>
  <tr><th colspan="4" align="center"><input type="submit" value="Submit"></th></tr>
  </form>
  </table>
</body>
</html>

