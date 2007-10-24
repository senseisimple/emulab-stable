<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Note the difference with which this page gets it arguments!
# I invoke it using GET arguments, so uid and pid are are defined
# without having to find them in URI (like most of the other pages
# find the uid).
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs  = RequiredPageArguments("project", PAGEARG_PROJECT);
$project  = $reqargs["project"];
$group    = $project->Group();
$pid      = $project->pid();

#
# Standard Testbed Header
#
PAGEHEADER("Project $pid");

#
# Verify that this uid is a member of the project being displayed.
#
if (! $project->AccessCheck($this_user, $TB_PROJECT_READINFO)) {
    USERERROR("You are not a member of Project $pid.", 1);
}

SUBPAGESTART();
SUBMENUSTART("Project Options");
WRITESUBMENUBUTTON("Create Subgroup",
		   "newgroup.php3?pid=$pid");
WRITESUBMENUBUTTON("Edit User Privs",
		   "editgroup_form.php3?pid=$pid&gid=$pid");
WRITESUBMENUBUTTON("Remove Users",
		   "showgroup.php3?pid=$pid&gid=$pid");
WRITESUBMENUBUTTON("Show Project History",
		   "showstats.php3?showby=project&pid=$pid");
WRITESUBMENUBUTTON("Free Node Summary",
		   "nodecontrol_list.php3?showtype=summary&bypid=$pid");
if ($isadmin) {
    WRITESUBMENUDIVIDER();
    WRITESUBMENUBUTTON("Delete this project",
		       "deleteproject.php3?pid=$pid");
    WRITESUBMENUBUTTON("Resend Approval Message",
		       "resendapproval.php?pid=$pid");
}

SUBMENUEND();

#
# Show number of PCS
#
$numpcs = $project->PCsInUse();

if ($numpcs) {
    echo "<center><font color=Red size=+2>\n";
    echo "Project $pid is using $numpcs PCs!\n";
    echo "</font></center>\n";
}

$project->Show();
SUBPAGEEND();

echo "<center>\n";
echo "<table border=0 bgcolor=#000 color=#000 class=stealth>\n";
echo "<tr valign=top><td class=stealth align=center>\n";

#
# A list of project members (from the default group).
#
$group->ShowMembers();

echo "</td><td align=center class=stealth>\n";

#
# A list of project Groups
#
$project->ShowGroupList();

echo "</td></table>\n";
echo "</center>\n";

# Project wide Templates.
if ($EXPOSETEMPLATES) {
    SHOWTEMPLATELIST("PROJ", 0, $uid, $pid);
}

#
# A list of project experiments.
#
ShowExperimentList("PROJ", $this_user, $project);

if ($isadmin) {
    echo "<center>
          <h3>Project Stats</h3>
         </center>\n";

    $project->ShowStats();
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
