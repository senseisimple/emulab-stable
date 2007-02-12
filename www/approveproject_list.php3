<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Of course verify that this uid has admin privs!
#
if (! $isadmin) {
    USERERROR("You do not have admin privileges to approve projects!", 1);
}

#
# The reason for this call is to make sure that globals are set properly.
#
$reqargs = RequiredPageArguments();

#
# Look in the projects table to see which projects have not been approved.
# Present a menu of options to either approve or deny the projects.
# Approving a project implies approving the project leader. Denying a project
# implies denying the project leader account, when there is just a single
# project pending for that project leader. 
#
$projlist = Project::PendingProjectList();

if (count($projlist) == 0) {
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

foreach ($projlist as $project) {
    $pid_idx  = $project->pid_idx();
    $Pcreated = $project->GetTempData();

    if (! ($leader = $project->GetLeader())) {
	TBERROR("Could not get leader for project $pid_idx", 1);
    }
    $pid        = $project->pid();
    $Purl       = $project->URL();
    $Pname      = $project->name();
    $headuid    = $leader->uid();
    $name	= $leader->name();
    $email	= $leader->email();
    $title	= $leader->title();
    $affil	= $leader->affil();
    $phone	= $leader->phone();
    $status     = $leader->status();

    $apprproj_url = CreateURL("approveproject_form", $project);
    $showproj_url = CreateURL("showproject", $project);
    $showuser_url = CreateURL("showuser", $leader);

    echo "<tr>
              <td height=15 colspan=6></td>
          </tr>
          <tr>
              <td align=center valign=center rowspan=2>
                  <A href='$apprproj_url'>
                     <img alt=\"o\" src=\"redball.gif\"></A></td>
              <td rowspan=2>
                  <A href='$showproj_url'>$pid</A>
                  <br>$Pcreated</td>
              <td rowspan=2>
                  <A href='$showuser_url'>$headuid</A></td>
              <td>$name";
    if ($status == TBDB_USERSTATUS_NEWUSER) {
	echo " (<font color=red>unverified</font>)";
    }
    echo "         </td>";
    echo "    <td>$title</td>
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

