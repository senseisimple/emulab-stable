<html>
<head>
<title>New Project</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
$auth_usr = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $auth_usr=$Vals[1];
  addslashes($auth_usr);
} else {
  unset($auth_usr);
}
?>
<table align="center" border="1"> 
<tr><td colspan="4">
  <H1 align="center">Apply to Use the Utah&nbsp;Network&nbsp;Testbed</h1>
  </td></tr>
<tr><td align="center" colspan="4">
Only fields marked with * are required</td></tr>
  <form action=grpadded.php3 method="post">
	<tr><td colspan=2>Project Information</td>
  <td colspan=2>Project Head Information</td></tr>
  <tr><td>*Name:</td><td><input type="text" name="gid" value="TestNet-One">
      </td>
  <td>*Username:</td><td class="left">
  <?php
  if (isset($auth_usr)) {
    /* if this person is logged into the database, 
       fill the user info fields with info from the database */
    $uid = addslashes($auth_usr);
    $query = "SELECT * FROM users WHERE uid=\"$uid\"";
    $result = mysql_db_query("tbdb", $query);
    $row = mysql_fetch_array($result);
    echo "<input type=\"readonly\" value=\"$row[uid]\" name=\"grp_head_uid\"></td></tr>\n";
	} else {
    echo "<input type=\"text\" name=\"grp_head_uid\"></td></tr>\n";
	}
echo "<tr><td>*Long name:</td><td>
      <input type=\"text\" name=\"grp_name\" value=\"Test Networks One\">
      </td>
      <td>*Full Name:</td><td class=\"left\">";
if (isset($row)) {
  echo "<input type=\"readonly\" value=\"$row[usr_name]\"";
} else {
  echo "<input type=\"text\"";
}
echo "name=\"usr_name\"></td></tr>
<tr><td>URL:</td><td><input type=\"text\" name=\"grp_URL\"
                 value=\"http://www.testnetworks.org\">
                 </td>
<td>*Email<br>Address:</td><td class=\"left\">";
if (isset($row)) {
  echo "<input type=\"readonly\" value=\"$row[usr_email]\" ";
} else {
  echo "<input type=\"text\" ";
}
echo "name=\"email\"></td></tr>
<tr><td>When&nbsp;do&nbsp;you<br>expect&nbsp;to&nbsp;be&nbsp;done<br>using&nbsp;the&nbsp;testbed?</td>
<td><input type=\"text\" value=";  #set a default expiration date 
$mytime = date("m/d/Y", time() + (86400 * 90)); #add 30 days 
echo "\"$mytime\""; 
echo "name=\"grp_expires\"></td>
	<td>Mailing<br>Address:</td><td class=\"left\">";
if (isset($row)) {
  echo "<input type=\"readonly\" value=\"$row[usr_addr]\" name=\"usr_addr\">";
} else {
		echo "<input type=\"text\"  name=\"usr_addr\">";
}
echo "</td></tr>
<tr><td>*Your Research<br>Affiliation:</td><td><input type=\"text\" name=\"grp_affil\" value=\"UofX Networks Group\"></td>
	<td>*Phone #:</td><td class=\"left\"><input ";
if (isset($row)) {
  echo "type=\"readonly\" value=\"$row[usr_phone]\"";
} else {
  echo "type=\"text\"";
} 
echo "name=\"usr_phones\"></td></tr>\n";
?>
<tr>
<td>*Password:</td><td><input type="password" name="password1"></td>
</tr>
<tr>
<td>*Retype<br>Password:</td><td><input 
<?php
if (isset($row)) {
  echo "type=\"hidden\" value=\"$row[usr_pswd]\"";
} else {
  echo "type=\"password\"";
}
?>
name="password2">&nbsp;</td></tr>
<tr><td colspan="4">*Please describe how and why you plan 
to use the Testbed:</td></tr> 
<tr><td colspan="4" class="left"><textarea name="why"
rows="10" cols="70"></textarea></td></tr>
  <tr><td colspan="4" align="center"><b><input type="submit" 
value="Submit"></b></td></tr>
</form>
  </table>
</body>
</html>

