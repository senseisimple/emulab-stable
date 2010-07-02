<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");
include_once("pub_defs.php");

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
		   "editgroup.php3?pid=$pid&gid=$pid");
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

# Gather up the html sections.
ob_start();
$project->Show();
$profile_html = ob_get_contents();
ob_end_clean();

ob_start();
$group->ShowMembers();
$members_html = ob_get_contents();
ob_end_clean();

ob_start();
$project->ShowGroupList();
$groups_html = ob_get_contents();
ob_end_clean();

# Project wide Templates.
$templates_html = null;
if ($EXPOSETEMPLATES) {
    $templates_html = SHOWTEMPLATELIST("PROJ", 0, $uid, $pid, "", TRUE);
}

ob_start();
ShowExperimentList("PROJ", $this_user, $project);
$experiments_html = ob_get_contents();
ob_end_clean();

$stats_html = null;
if ($isadmin) {
    ob_start();
    $project->ShowStats();
    $stats_html = ob_get_contents();
    ob_end_clean();
}

$papers_html = null;
if ($PUBSUPPORT) {
    #
    # List papers for this project if any
    #
    $query_result = GetPubs("`project` = \"$pid\"");
    if (mysql_num_rows($query_result)) {
	$papers_html = MakeBibList($this_user, $isadmin, $query_result);
    }
}

$vis_html = null;
$whocares = null;
if ($EXP_VIS && CHECKURL("http://$USERNODE/proj-vis/$pid/", $whocares)) {
  $vis_html = "<iframe src=\"http://$USERNODE/proj-vis/$pid/\" width=\"100%\" height=600 id=\"vis-iframe\"></iframe>";
}

#
# Show number of PCS
#
$numpcs = $project->PCsInUse();

if ($numpcs) {
    echo "<center><font color=Red size=+2>\n";
    echo "Project $pid is using $numpcs PCs!\n";
    echo "</font></center><br>\n";
}

#
# Function to change what is being shown.
#
echo "<script type='text/javascript' language='javascript'>
        var li_current = 'li_profile';
        var div_current = 'div_profile';
        function Show(which) {
	    li = getObjbyName(li_current);
            li.style.backgroundColor = '#DDE';
            li.style.borderBottom = '1px solid #778';
            div = getObjbyName(div_current);
            div.style.display = 'none';

            li_current = 'li_' + which;
	    li = getObjbyName(li_current);
            li.style.backgroundColor = 'white';
            li.style.borderBottom = '1px solid white';
            div_current = 'div_' + which;
            div = getObjbyName(div_current);
            div.style.display = 'block';

            return false;
        }
        function Setup(which) {
            li_current = 'li_' + which;
            div_current = 'div_' + which;
	    li = getObjbyName(li_current);
            li.style.backgroundColor = 'white';
            li.style.borderBottom = '1px solid white';
            div = getObjbyName(div_current);
            div.style.display = 'block';
        }
      </script>\n";

#
# This is the topbar
#
echo "<div width=\"100%\" align=center>\n";
echo "<ul id=\"topnavbar\">\n";
if ($templates_html) {
    echo "<li>
           <a href=\"#A\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
               "id=\"li_templates\" onclick=\"Show('templates');\">".
               "Templates</a></li>\n";
}
if ($experiments_html) {
     echo "<li>
            <a href=\"#B\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
               "id=\"li_experiments\" onclick=\"Show('experiments');\">".
               "Experiments</a></li>\n";
}
if ($groups_html) {
    echo "<li>
          <a href=\"#C\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	      "id=\"li_groups\" onclick=\"Show('groups');\">".
              "Groups</a></li>\n";
}
if ($members_html) {
    echo "<li>
          <a href=\"#D\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	      "id=\"li_members\" onclick=\"Show('members');\">".
              "Members</a></li>\n";
}
echo "<li>
      <a href=\"#E\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
           "id=\"li_profile\" onclick=\"Show('profile');\">".
           "Profile</a></li>\n";

if ($isadmin && $stats_html) {
    echo "<li>
          <a href=\"#F\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	      "id=\"li_stats\" onclick=\"Show('stats');\">".
              "Project Stats</a></li>\n";
}
if ($papers_html) {
    echo "<li>
          <a href=\"#G\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	      "id=\"li_papers\" onclick=\"Show('papers');\">".
              "Publications</a></li>\n";
}
if ($vis_html) {
    echo "<li>
          <a href=\"#G\" class=topnavbar onfocus=\"this.hideFocus=true;\" ".
	      "id=\"li_vis\" onclick=\"Show('vis');\">".
              "Visualization</a></li>\n";
}
echo "</ul>\n";
echo "</div>\n";
echo "<div align=center id=topnavbarbottom>&nbsp</div>\n";

if ($templates_html) {
     echo "<div class=invisible id=\"div_templates\">$templates_html</div>";
}
if ($experiments_html) {
     echo "<div class=invisible id=\"div_experiments\">$experiments_html</div>";
}
if ($groups_html) {
     echo "<div class=invisible id=\"div_groups\">$groups_html</div>";
}
if ($members_html) {
     echo "<div class=invisible id=\"div_members\">$members_html</div>";
}
echo "<div class=invisible id=\"div_profile\">$profile_html</div>";
if ($isadmin && $stats_html) {
    echo "<div class=invisible id=\"div_stats\">$stats_html</div>";
}
if ($papers_html) {
     echo "<div class=invisible id=\"div_papers\">$papers_html</div>";
}
if ($vis_html) {
     echo "<div class=invisible id=\"div_vis\">$vis_html</div>";
}
SUBPAGEEND();

#
# Get the active tab to look right.
#
echo "<script type='text/javascript' language='javascript'>
      Setup(\"profile\");
      </script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
