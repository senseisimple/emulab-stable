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
<h1>Approve new users in your Project</h1>
Use this page to approve new members of your Project.  Once approved,
they will be able to log into machines in your Project's experiments.</p>
<p> If you desire, you may set their trust/privilege levels to give them
more or less access to your nodes:
<ul>
	<li>User - Can log into machines in your experiments.
	<li>Local Root - Granted root access on your project's machines; can create new experiments.
";
#echo "<li>Group Root - Can approve users, create projects, and update any project info or personal info for project members.";
echo "</ul>
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
    $find .= " DISTINCT uid,usr_name,usr_email,usr_title,usr_affil,usr_addr,usr_addr2,usr_city,usr_state,usr_zip,usr_phone FROM users WHERE (status='newuser' OR status='unapproved') AND (uid='$uid'";
  } else {
    $find .= " OR uid='$uid'";
  }
}
$find .= ")";
$found = mysql_db_query("tbdb", $find);
if ( mysql_num_rows($found) == 0 ) {
  echo "<h3>You have no new project members who need approval</h3>\n";
} else {
  echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2 align='center'>
<tr>
<td rowspan=2>Action</td>
<td rowspan=2>Trust Level</td>
<td rowspan=2>User</td>
<td>Name</td>
<td>Title</td>
<td>Affil.</td>
<td>E-mail</td>
<td>Phone</td>
</tr><tr>
<td>Addr</td>
<td>Addr2</td>
<td>City</td>
<td>State</td>
<td>Zip</td>
</tr>
<form action='approved.php3?$auth_usr' method='post'>\n";
  while ($row = mysql_fetch_row($found)) {
    $uid = $row[0];
    $name= $row[1];
    $email=$row[2];
    $title=$row[3];
    $affil=$row[4];
    $addr= $row[5];
    $addr2=$row[6];
    $city= $row[7];
    $state=$row[8];
    $zip=  $row[9];
    $phone=$row[10];
    echo "
<tr><td colspan=8>&nbsp;</td></tr>
<tr><td rowspan=2><select name=\"$uid\">
<option value='approve'>Approve</option>
<option value='deny'>Deny</option>
<option value='later'>Postpone</option></select></td>
<td rowspan=2><select name=\"$uid-trust\">
<option value='user'>User</option>
<option value='local_root'>Local Root</option>";    
    #echo "<option value='group_root'>Group Root</option>";
    echo "</select></td>
<td rowspan=2>&nbsp;$uid&nbsp;</td><td>&nbsp;$name&nbsp;</td><td>&nbsp;$title&nbsp;</td><td>&nbsp;$affil&nbsp;</td><td>&nbsp;$email&nbsp;</td><td>&nbsp;$phone&nbsp;</td></tr>
<tr><td>&nbsp;$addr&nbsp;</td><td>&nbsp;$addr2&nbsp;</td><td>&nbsp;city&nbsp;</td><td>&nbsp;$state&nbsp;</td><td>&nbsp;$zip&nbsp;</td>
</tr>\n";
  }
  echo "
<tr><td align=center colspan=8><b><input type='submit' value='Submit' name='OK'></td></tr>
</form>
</table>\n";
}
echo "
</body>
</html>";
?>
