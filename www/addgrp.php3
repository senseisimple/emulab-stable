<html>
<head>
<title>Start a New Project</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

$auth_usr = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
  $auth_usr=$Vals[1];
  addslashes($auth_usr);
} else {
  unset($auth_usr);
}

$row = 0;
if (isset($auth_usr)) {
    $uid = addslashes($auth_usr);
    $query_result = mysql_db_query($TBDBNAME,
		"SELECT * FROM users WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
	TBERROR("Database Error getting info for $uid: $err\n", 1);
    }
    $row = mysql_fetch_array($query_result);
}

$expiretime = date("m/d/Y", time() + (86400 * 90));

?>
<table align="center" border="1"> 
<tr>
    <td colspan="2">
        <h1 align="center">Apply to Use the Utah Network Testbed</h1>
    </td>
</tr>

<tr>
    <td align="center" colspan="2">
        Fields marked with * are required;
        those marked + are highly recommended.
    </td>
</tr>

<form action=grpadded.php3 method="post">
<tr></tr>
<tr></tr>
<?php

#
# User information.
#
echo "<tr>
          <td colspan=2>
              Project Head Information
          </td>
      </tr>\n";

#
# UserName:
#
echo "<tr>
          <td>*Username:</td>
          <td class=\"left\">
              <input name=\"grp_head_uid\"";
if ($row) {
    echo     "type=\"readonly\" value=\"$row[uid]\">";
}
else {
    echo     "type=\"text\">";
}
echo "     </td>
      </tr>\n";

#
# Full Name
#
echo "<tr>
          <td>*Full Name:</td>
          <td class=\"left\">
              <input name=\"usr_name\"";
if ($row) {
    echo "           type=\"readonly\" value=\"$row[usr_name]\">";
} else {
    echo "           type=\"text\">";
}
echo "     </td>
      </tr>\n";

#
# Title/Position:
# 
echo "<tr>
         <td>*Title/Position:</td>
         <td class=\"left\">
             <input name=\"usr_title\"";
if ($row) {
    echo "          type=\"readonly\" value=\"$row[usr_title]\">";
} else {
    echo "          type=\"text\" value=\"Professor Emeritus\">";
}

echo "     </td>
      </tr>\n";

#
# Affiliation:
#
echo "<tr>
         <td>*Institutional<br>Affiliation:</td>
         <td class=\"left\">
             <input name=\"usr_affil\"";
if ($row) {
    echo "          type=\"readonly\" value=\"$row[usr_affil]\">";
} else {
    echo "          type=\"text\" value=\"UCB Networks Group\">";
}

echo "     </td>
      </tr>\n";


#
# Email:
#
echo "<tr>
         <td>*Email<br>Address:</td>
         <td class=\"left\">
             <input name=\"email\"";
if ($row) {
    echo "          type=\"readonly\" value=\"$row[usr_email]\">";
} else {
    echo "          type=\"text\">";
}

echo "    </td>
      </tr>\n";

#
# Postal Address
#
echo "<tr>
         <td>*Postal<br>Address:</td>
         <td class=\"left\">
              <input name=\"usr_addr\"";
if ($row) {
    echo "           type=\"readonly\" value=\"$row[usr_addr]\">";
} else {
    echo "           type=\"text\">";
}
echo "    </td>
      </tr>\n";

#
# Phone
#
echo "    <td>*Phone #:</td>
          <td class=\"left\">
              <input name=\"usr_phones\"";
if ($row) {
    echo "           type=\"readonly\" value=\"$row[usr_phone]\">";
} else {
    echo "           type=\"text\">";
}
echo "    </td>
      </tr>\n";

#
# Password
#
echo "<tr>
         <td>*Password:</td>
         <td><input type=\"password\" name=\"password1\"></td>
      </tr>\n";

#
# If a new usr, then provide a second password confirmation field.
# Otherwise, a blank spot.
#
if (! $row) {
echo "<tr>
          <td>*Retype<br>New Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"password2\"></td>
      </tr>\n";
}


#
# Project information
#
echo "<tr><td colspan='2'><hr></td></tr>\n";

echo "<tr>
         <td colspan='2'>
             Project Information <em>(replace the example entries)</em>
         </td>
      </tr>\n";

#
#  Project Name:
#
echo "<tr>
          <td>*Name (no blanks):</td>
          <td><input type=\"text\" name=\"gid\" value=\"ucb-omcast\"></td>
      </tr>\n";

#
#  Long Name
#
echo "<tr>
          <td>*Long name:</td>
          <td><input type=\"text\" name=\"grp_name\"
                     value=\"UCB Overlay Multicast\"></td>
      </tr>\n";

#
#  URL
#
echo "<tr>
         <td>+URL:</td>
         <td><input type=\"text\" name=\"grp_URL\"
                    value=\"http://www.cs.berkeley.edu/netgrp/omcast/\"></td>
      </tr>\n";

#
# Nodes and PCs
# 
echo "<tr>
         <td>*Estimated #of PCs:</td>
         <td><input type=\"text\" name=\"grp_pcs\"></td>
      </tr>\n";

echo "<tr>
         <td>*Estimated #of Sharks:</td>
         <td><input type=\"text\" name=\"grp_sharks\" value=\"0\"></td>
      </tr>\n";

#
#  Expires
#
echo "<tr>
          <td>When do you expect to be<br>done using the testbed</td>
          <td><input type=\"text\" name=\"grp_expires\"
                     value=\"$expiretime\"></td>
      </tr>\n";


?>

<tr>
    <td colspan="2">
        *Please describe how and why you'd like to use the testbed.<br>
         If the research is sponsored (funded), list the sponsors.</td>
</tr>

<tr>
    <td colspan="2" align="center" class="left">
        <textarea name="why" rows="10" cols="60"></textarea></td>
</tr>

<tr>
    <td colspan="2" align="center">
        <b><input type="submit" value="Submit"></b></td>
</tr>
</form>
</table>
</body>
</html>

