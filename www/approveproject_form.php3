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
$uid = "";
if (ereg("php3\?([[:alnum:]]+)", $REQUEST_URI, $Vals)) {
    $uid=$Vals[1];
    addslashes($uid);
}
else {
    unset($uid);
}
LOGGEDINORDIE($uid);

echo "<center><h1>Approve New Projects</h1></center>\n";

#
# Of course verify that this uid has admin privs!
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT admin from users where uid='$uid' and admin='1'" );
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting admin status for $uid: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("You do not have admin privledges to approve projects!", 1);
}

#
# Look in the projects table to see which projects have not been approved.
# Present a menu of options to either approve or deny the projects.
# Approving a project implies approving the project leader. Denying a project
# implies denying the project leader account, when there is just a single
# project pending for that project leader. 
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * from projects where approved='0'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error getting unapproved project list: $err\n", 1);
}
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no projects to approve!", 1);
}

echo "For each project waiting to be approved, you may select on of the
      following choices:
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
            <td>Postpone</td>
            <td>-</td>
            <td>Twiddle your thumbs some more</td>
        </tr>
      </table>\n";

#
# Now build a table with a bunch of selections. The thing to note about the
# form inside this table is that the selection fields are constructed with
# name= on the fly, from the uid of the user to be approved. In other words:
#
#             project   menu 
#	name=testbed$$approval value=approve,deny,murder,postpone
#
# so that we can go through the entire list of post variables, looking
# for these. The alternative is to work backwards, and I don't like that.
# 
echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td rowspan=2>Project</td>
          <td rowspan=2>User</td>
          <td rowspan=2>Action</td>
          <td>User Name</td>
          <td>Title</td>
          <td>User Affil</td>
          <td>E-mail</td>
      </tr>
      <tr>
          <td>Proj Name</td>
          <td>URL</td>
          <td>Proj Affil</td>
          <td>Phone</td>
      </tr>\n";

echo "<form action='approveproject.php3?$uid' method='post'>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid      = $projectrow[pid];
    $headuid  = $projectrow[head_uid];
    $Purl     = $projectrow[URL];
    $Pname    = $projectrow[name];
    $Paffil   = $projectrow[affil];

    $userinfo_result = mysql_db_query($TBDBNAME,
	"SELECT * from users where uid=\"$headuid\"");

    $row	= mysql_fetch_array($userinfo_result);
    $name	= $row[usr_name];
    $email	= $row[usr_email];
    $title	= $row[usr_title];
    $affil	= $row[usr_affil];
    $addr	= $row[usr_addr];
    $addr2	= $row[usr_addr2];
    $city	= $row[usr_city];
    $state	= $row[usr_state];
    $zip	= $row[usr_zip];
    $phone	= $row[usr_phone];

    echo "<tr>
              <td colspan=7> </td>
          </tr>
          <tr>
              <td rowspan=2>$pid</td>
              <td rowspan=2>$headuid</td>
              <td rowspan=2>
                  <select name=\"$pid\$\$approval\">
                          <option value='postpone'>Postpone</option>
                          <option value='approve'>Approve</option>
                          <option value='deny'>Deny</option>
                          <option value='destroy'>Destroy</option>
                  </select>
              </td>\n";

    echo "    <td>$name</td>
              <td>$title</td>
              <td>$affil</td>
              <td>$email</td>
          </tr>\n";
    echo "<tr>
              <td>$Pname</td>
              <td>$Purl</td>
              <td>$Paffil</td>
              <td>$phone</td>
          </tr>\n";
}
echo "<tr>
          <td align=center colspan=7>
              <b><input type='submit' value='Submit' name='OK'></td>
      </tr>
      </form>
      </table>\n";
?>
</body>
</html>

