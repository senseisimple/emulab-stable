<?php
include("defs.php3");

PAGEHEADER("Start a New Project");

#
# Get current user.
# 
$uid = GETLOGIN();

#
# If a uid came in, then we check to see if the login is valid.
# If the login is not valid, then quit cause we don't want to display the
# personal information for some random ?uid argument.
#
if ($uid) {
    if (CHECKLOGIN($uid) != 1) {
        USERERROR("You are not logged in. Please log in and try again.", 1);
    }
    $query_result = mysql_db_query($TBDBNAME,
		"SELECT * FROM users WHERE uid=\"$uid\"");
    if (! $query_result) {
        $err = mysql_error();
	TBERROR("Database Error getting info for $uid: $err\n", 1);
    }
    $row = mysql_fetch_array($query_result);
}
else {
    #
    # No uid, so must be new.
    #
    $row = 0;
}

$expiretime = date("Y:m:d", time() + (86400 * 120));

?>
<table align="center" border="1"> 
<tr>
    <td colspan="2" align="center">
        <h1>Apply to Use the Utah Network Testbed</h1>
        <font size="+1">
               If you are a student
               <font color="red">(undergrad or graduate)</font>, please
               <a href="auth.html">read this first</a>!
        </font>
    </td>
</tr>
<tr></tr>
<tr></tr>

<tr>
    <td align="center" colspan="2">
        Fields marked with * are required;
        those marked + are highly recommended.</td>
</tr>

<form action=newproject.php3 method="post">
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
          <td>*Username (no blanks, lowercase):</td>
          <td class=\"left\">
              <input name=\"proj_head_uid\"";
if ($row) {
    echo     "type=\"readonly\" value=\"$row[uid]\">";
}
else {
    echo     "type=\"text\" size=$TBDB_UIDLEN maxlength=$TBDB_UIDLEN>";
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
    echo "           type=\"text\" size=\"30\">";
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
    echo "          type=\"text\" size=\"30\">";
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
    echo "          type=\"text\" size=\"40\">";
}

echo "     </td>
      </tr>\n";

#
# User URL
#
echo "<tr>
         <td>Home Page URL:</td>
         <td class=\"left\">
             <input name=\"usr_url\"";
if ($row) {
    echo "          type=\"readonly\" value=\"$row[usr_URL]\">";
} else {
    echo "          value=\"http://\" type=\"text\" size=\"45\">";
}

echo "     </td>
      </tr>\n";

#
# Email:
#
echo "<tr>
         <td>*Email<br>Address:</td>
         <td class=\"left\">
             <input name=\"usr_email\"";
if ($row) {
    echo "          type=\"readonly\" value=\"$row[usr_email]\">";
} else {
    echo "          type=\"text\" size=\"30\">";
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
    echo "           type=\"text\" size=\"40\">";
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
    echo "           type=\"text\" size=\"15\">";
}
echo "    </td>
      </tr>\n";

#
# If a new usr, then provide a second password confirmation field.
# Otherwise, a blank spot.
#
if (! $row) {
echo "<tr>
         <td>*Password:</td>
         <td><input type=\"password\" name=\"password1\" size=\"8\"></td>
      </tr>\n";

echo "<tr>
          <td>*Retype Password:</td>
          <td class=\"left\">
              <input type=\"password\" name=\"password2\" size=\"8\"></td>
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
          <td><input type=\"text\" name=\"pid\"
                     size=$TBDB_PIDLEN maxlength=$TBDB_PIDLEN></td>
      </tr>\n";

#
#  Long Name
#
echo "<tr>
          <td>*Long name:</td>
          <td><input type=\"text\" name=\"proj_name\"
                     size=\"40\"></td>
      </tr>\n";

#
#  URL
#
echo "<tr>
         <td>*URL:</td>
         <td><input type=\"text\" name=\"proj_URL\"
                    value=\"http://\" size=\"45\"></td>
      </tr>\n";

#
# Nodes and PCs and Users
# 
echo "<tr>
         <td>*Estimated #of Project Members:</td>
         <td><input type=\"text\" name=\"proj_members\" size=\"4\"></td>
      </tr>\n";

echo "<tr>
         <td>*Estimated #of PCs:</td>
         <td><input type=\"text\" name=\"proj_pcs\" size=\"4\"></td>
      </tr>\n";

echo "<tr>
         <td>*Estimated #of Sharks:</td>
         <td><input type=\"text\" name=\"proj_sharks\" size=\"4\"
                    value=\"0\"></td>
      </tr>\n";

#
#  Expires
#
echo "<tr>
          <td>When do you expect to be done<br>
              using the testbed (YYYY:MM:DD):</td>
          <td><input type=\"text\" name=\"proj_expires\" size=\"10\"
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
        <textarea name="proj_why" rows="10" cols="60"></textarea></td>
</tr>

<tr>
    <td colspan="2" align="center">
        <b><input type="submit" value="Submit"></b></td>
</tr>
</form>
</table>

<?php
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
