<html>
<head>
<title>New Project Approval</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can do this.
#
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges to approve projects!", 1);
}

echo "<center><h1>Approve a Project</h1></center>\n";

#
# Check to make sure thats this is a valid PID.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM projects WHERE pid=\"$pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The project $pid is not a valid project.", 1);
}
$row = mysql_fetch_array($query_result);


echo "<center><h3>You have the following choices:</h3></center>
      <table align=center border=0>
        <tr>
            <td>Deny</td>
            <td>-</td>
            <td>Deny project application (kills project records)</td>
        </tr>

        <tr>
            <td>Destroy</td>
            <td>-</td>
            <td>Deny project application, and kill the user account</td>
        </tr>

        <tr>
            <td>Approve</td>
            <td>-</td>
            <td>Approve the project</td>
        </tr>

        <tr>
            <td>More Info</td>
            <td>-</td>
            <td>Ask for more info</td>
        </tr>

        <tr>
            <td>Postpone</td>
            <td>-</td>
            <td>Twiddle your thumbs some more</td>
        </tr>
      </table>\n";

echo "<center>
      <h3>Project Information</h3>
      </center>
      <table align=center border=1>\n";

$proj_created	= $row[created];
$proj_expires	= $row[expires];
$proj_name	= $row[name];
$proj_URL	= $row[URL];
$proj_head_uid	= $row[head_uid];
$proj_pcs       = $row[num_pcs];
$proj_sharks    = $row[num_sharks];
$proj_why       = $row[why];
$control_node	= $row[control_node];

#
# Generate the table.
# 
echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$pid</td>
      </tr>\n";

echo "<tr>
          <td>Long Name: </td>
          <td class=\"left\">$proj_name</td>
      </tr>\n";

echo "<tr>
          <td>Project Head: </td>
          <td class=\"left\">$proj_head_uid</td>
      </tr>\n";

echo "<tr>
          <td>URL: </td>
          <td class=\"left\">
              <A href='$proj_URL'>$proj_URL</A></td>
      </tr>\n";

echo "<tr>
          <td>#PCs: </td>
          <td class=\"left\">$proj_pcs</td>
      </tr>\n";

echo "<tr>
          <td>#Sharks: </td>
          <td class=\"left\">$proj_sharks</td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$proj_created</td>
      </tr>\n";

echo "<tr>
          <td colspan='2'>Why?</td>
      </tr>\n";

echo "<tr>
          <td colspan='2' width=600>$proj_why</td>
      </tr>\n";

echo "</table>\n";


$userinfo_result = mysql_db_query($TBDBNAME,
	"SELECT * from users where uid=\"$proj_head_uid\"");

$row	= mysql_fetch_array($userinfo_result);
$usr_expires = $row[usr_expires];
$usr_email   = $row[usr_email];
$usr_addr    = $row[usr_addr];
$usr_name    = $row[usr_name];
$usr_phone   = $row[usr_phone];
$usr_passwd  = $row[usr_pswd];
$usr_title   = $row[usr_title];
$usr_affil   = $row[usr_affil];

echo "<center>
      <h3>Project Leader Information</h3>
      </center>
      <table align=center border=1>\n";

echo "<tr>
          <td>Username:</td>
          <td>$proj_head_uid</td>
      </tr>\n";

echo "<tr>
          <td>Full Name:</td>
          <td>$usr_name</td>
      </tr>\n";

echo "<tr>
          <td>Email Address:</td>
          <td>$usr_email</td>
      </tr>\n";

echo "<tr>
          <td>Expiration date:</td>
          <td>$usr_expires</td>
      </tr>\n";

echo "<tr>
          <td>Mailing Address:</td>
          <td>$usr_addr</td>
      </tr>\n";

echo "<tr>
          <td>Phone #:</td>
          <td>$usr_phone</td>
      </tr>\n";

echo "<tr>
          <td>Title/Position:</td>
          <td>$usr_title</td>
     </tr>\n";

echo "<tr>
          <td>Institutional Affiliation:</td>
          <td>$usr_affil</td>
      </tr>\n";

echo "</table>\n";

#
# Now put up the menu choice along with a text box for an email message.
#
echo "<center>
      <h3>What would you like to do?</h3>
      </center>
      <table align=center border=1>
      <form action='approveproject.php3?uid=$uid&pid=$pid' method='post'>\n";

echo "<tr>
          <td align=center>
              <select name=approval>
                      <option value='postpone'>Postpone</option>
                      <option value='approve'>Approve</option>
                      <option value='moreinfo'>More Info</option>
                      <option value='deny'>Deny</option>
                      <option value='destroy'>Destroy</option>
              </select>
          </td>
       </tr>\n";


echo "<tr>
          <td>Use the text box to add a message to the email notification.</td>
      </tr>\n";

echo "<tr>
         <td align=center class=left>
             <textarea name=message rows=5 cols=60></textarea>
         </td>
      </tr>\n";

echo "<tr>
          <td align=center colspan=2>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";
?>
</body>
</html>
