<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("New Project Approval List");

#
# Only known and logged in users can do this. uid came in with the URI.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Of course verify that this uid has admin privs!
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
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

echo "Below is the list of projects waiting for approval or denial. Click
      on a particular project to act on it, and you will be zapped to a
      page with more information about the project, and your options menu.
      <p>\n";
      
echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td rowspan=2>Act</td>
          <td rowspan=2>Project Info</td>
          <td rowspan=2>User</td>
          <td>User Name</td>
          <td>Title</td>
          <td>E-mail</td>
      </tr>
      <tr>
          <td>Proj Name</td>
          <td>User Affil</td>
          <td>Phone</td>
      </tr>\n";

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
              <td height=15 colspan=6></td>
          </tr>
          <tr>
              <td align=center rowspan=2>
                  <A href='approveproject_form.php3?pid=$pid'>
                     <img alt=\"o\" src=\"redball.gif\"></A></td>
              <td rowspan=2>
                  <A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td rowspan=2>
                  <A href='showuser.php3?target_uid=$headuid'>
                     $headuid</A></td>
              <td>$name</td>
              <td>$title</td>
              <td>$email</td>
          </tr>\n";
    echo "<tr>
              <td>$Pname</td>
              <td>$affil</td>
              <td>$phone</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>

