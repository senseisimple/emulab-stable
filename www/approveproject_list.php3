<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
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
    USERERROR("You do not have admin privileges to approve projects!", 1);
}

#
# Look in the projects table to see which projects have not been approved.
# Present a menu of options to either approve or deny the projects.
# Approving a project implies approving the project leader. Denying a project
# implies denying the project leader account, when there is just a single
# project pending for that project leader. 
#
$query_result = DBQueryFatal("SELECT * from projects where approved='0'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There are no projects to approve!", 1);
}

echo "<p>Below is the list of projects waiting for approval or denial. Click
      on a particular project to act on it, and you will be zapped to a
      page with more information about the project, and your options menu.
      </p>\n";
      
echo "<table width=\"100%\" border=2 cellpadding=0 cellspacing=2
       >\n";

echo "<tr>
          <th rowspan=2>Act</th>
          <th rowspan=2>Project Info</th>
          <th rowspan=2>User</th>
          <th>User Name</th>
          <th>Title</th>
          <th>E-mail</th>
      </tr>
      <tr>
          <th>Proj Name</th>
          <th>User Affil</th>
          <th>Phone</th>
      </tr>\n";

while ($projectrow = mysql_fetch_array($query_result)) {
    $pid      = $projectrow[pid];
    $headuid  = $projectrow[head_uid];
    $Purl     = $projectrow[URL];
    $Pname    = $projectrow[name];

    $userinfo_result =
	DBQueryFatal("SELECT * from users where uid='$headuid'");

    $row	= mysql_fetch_array($userinfo_result);
    $name	= $row[usr_name];
    $email	= $row[usr_email];
    $title	= $row[usr_title];
    $affil	= $row[usr_affil];
    $phone	= $row[usr_phone];

    echo "<tr>
              <td height=15 colspan=6></td>
          </tr>
          <tr>
              <td align=center valign=center rowspan=2>
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

