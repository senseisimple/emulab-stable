<html>
<head>
<title>New User Approval</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
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
    echo "<h3>You are not logged in. Please go back to the ";
    echo "<a href=\"tbdb.html\" target=\"_top\"> Home Page </a> ";
    echo "and log in first.</h3></body></html>";
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
echo "
<h1>Approve new users in your group</h1>
<h3><p>
This page will let you approve new members of your group. Once approved,
they will be able to log into machines in your group's experiments.</p>
<p> If you desire, you may set their trust/privilege levels to give them
more or less access to your nodes:
<ol>
<li>User - Can log into machines in your experiments.
<li>Local Root - Can have root access on machines, can create new experiments.
";
#echo "<li>Group Root - Can approve users, create projects, and update any group info or personal info for group members.";
echo "</ol>
</p></h3>\n";
$query="SELECT gid FROM grp_memb WHERE uid='$auth_usr' and trust='group_root'";
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
if ($select=="SELECT") {
  echo "<h3>You do not have Group Root permissions in any group.</h3>";
  echo "</body></html>\n";
  exit;
}
$selected = mysql_db_query("tbdb", $select);
$find = "SELECT";
while ($row = mysql_fetch_row($selected)) {
  $uid = $row[0];
  if ($find == "SELECT") {
    $find .= " DISTINCT uid,usr_name,usr_email,usr_addr,usr_phone FROM users WHERE (status='newuser' OR status='unapproved') AND (uid='$uid'";
  } else {
    $find .= " OR uid='$uid'";
  }
}
$find .= ")";
$found = mysql_db_query("tbdb", $find);
if ( mysql_num_rows($found) == 0 ) {
  echo "<h3>You have no new group members who need approval</h3>\n";
} else {
  echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2 align='center'>
<tr>
<td>Action</td>
<td>Trust Level</td>
<td>User</td>
<td>Name</td>
<td>E-mail</td>
<td>Addr</td>
<td>Ph&nbsp;#</td>
</tr>
<form action='approved.php3?$auth_usr' method='post'>\n";
  while ($row = mysql_fetch_row($found)) {
    $uid = $row[0];
    $name= $row[1];
    $email=$row[2];
    $addr= $row[3];
    $phone=$row[4];
    echo "<tr><td><select name=\"$uid\">
<option value='approve'>Approve</option>
<option value='deny'>Deny</option>
<option value='later'>Postpone</option></select></td>
<td><select name=\"$uid-trust\">
<option value='user'>User</option>
<option value='local_root'>Local Root</option>";    
    #echo "<option value='group_root'>Group Root</option>";
    echo "</select></td>
<td>&nbsp;$uid&nbsp;</td><td>&nbsp;$name&nbsp;</td><td>&nbsp;$email&nbsp;</td>
<td>&nbsp;$addr&nbsp;</td><td>&nbsp;$phone&nbsp;</td>
</tr>\n";
  }
  echo "
<tr><td colspan=7><b><input type='submit' value='Submit' name='OK'></td></tr>
</form>
</table>\n";
}
echo "
</body>
</html>";
?>
