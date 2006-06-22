<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# Only known and logged in users ...
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Need these below, set in CheckArguments.
$pid = "";
$eid = "";
$gid = "";
$template = NULL;
$exptidx  = 0;

function CheckArguments($guid, $version) {
    global $TB_EXPT_READINFO;
    global $uid, $pid, $eid, $gid, $template, $exptidx;

    #
    # Verify page arguments.
    # 
    if (!isset($guid) ||
	strcmp($guid, "") == 0) {
	USERERROR("You must provide a template GUID.", 1);
    }
    
    if (!isset($version) ||
	strcmp($version, "") == 0) {
	USERERROR("You must provide a template version number", 1);
    }
    if (!TBvalid_guid($guid)) {
	PAGEARGERROR("Invalid characters in GUID!");
    }
    if (!TBvalid_integer($version)) {
	PAGEARGERROR("Invalid characters in version!");
    }
    
    #
    # Check to make sure this is a valid template.
    #
    if (($template = new Template($guid, $version)) == NULL) {
	USERERROR("The experiment template $guid/$version is not a ".
		  "valid experiment template!", 1);
    }
    $pid = $template->pid();
    $gid = $template->gid();
    $eid = $template->eid();
    $exptidx = TBExptIndex($pid, $eid);
    
    #
    # Verify Permission.
    #
    if (! $template->AccessCheck($uid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view experiment ".
		  "template $guid/$version!", 1);
    }
}

CheckArguments($guid, $version);

if (isset($showtemplate) || isset($hidetemplate) ||
    isset($showhidden) || isset($hidehidden) ||
    isset($zoomin) || isset($zoomout)) {

    # Need this for scripts.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    # Hide or show templates.
    if (isset($showtemplate) || isset($hidetemplate) ||
	isset($showhidden) || isset($hidehidden)) {

	$optarg  = ((isset($recursive) && $recursive == "Yep") ? "-r" : "");
	$reqarg  = "-a ";

	if (isset($showtemplate)) {
	    $reqarg .= "show";
	}
	elseif (isset($hidetemplate)) {
	    $reqarg .= "hide";
	}
	elseif (isset($showhidden)) {
	    $reqarg .= "showhidden";
	}
	elseif (isset($hidehidden)) {
	    $reqarg .= "hidehidden";
	}
	$reqarg .= " $guid/";

	if (isset($showtemplate) || isset($hidetemplate)) {
	    $reqarg .= $version;
	}
	else {
	    # Applies only to root template
	    $reqarg .= 1;
	}
	
	SUEXEC($uid, "$pid,$unix_gid",
	       "webtemplate_control $reqarg $optarg",
	       SUEXEC_ACTION_DIE);
    }
    else {
	$optarg = "";
	
	if (isset($zoomin)) {
	    $optarg = "-z in";
	}
	elseif (isset($zoomout)) {
	    $optarg = "-z out";
	}

        # Need to update the template graph.
	SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $optarg $guid",
	       SUEXEC_ACTION_DIE);
    }
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template: $guid/$version");

SUBPAGESTART();

SUBMENUSTART("Template Options");

if (!isset($show) || $show == "graph") {
    WRITESUBMENUBUTTON("Show NS File &nbsp &nbsp",
		       "template_show.php?guid=$guid".
		       "&version=$version&show=nsfile");
}
else {
    WRITESUBMENUBUTTON("Show Graph &nbsp &nbsp",
		       "template_show.php?guid=$guid".
		       "&version=$version&show=graph");
}

WRITESUBMENUBUTTON("Modify Template",
		   "template_modify.php?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Instantiate Template",
		   "template_swapin.php?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Add Metadata",
		   "template_metadata.php?action=add&".
		   "guid=$guid&version=$version");

# We show the user the datastore for the template; the rest of it is not important.
WRITESUBMENUBUTTON("Template Archive",
		   "archive_view.php3/$exptidx/trunk?exptidx=$exptidx");

WRITESUBMENUBUTTON("Template Record",
		   "template_history.php?guid=$guid&version=$version");

SUBMENUEND_2A();

#
# Ick.
#
$rsrcidx = TBrsrcIndex($pid, $eid);

echo "<br>
      <img border=1 alt='template visualization'
           src='showthumb.php3?idx=$rsrcidx'>";

if ($template->InstanceCount()) {
    $template->ShowInstances();
}

SUBMENUEND_2B();

if (!isset($show) || $show == "graph") {
    $template->ShowGraph();

    #
    # Define the control buttons.
    #
    echo "<center>\n";
    echo "<form action='template_show.php?guid=$guid&version=$version'
                method=post>\n";

    if (! $template->IsRoot()) {
	if ($template->IsHidden()) {
	    echo "<button name=showtemplate type=submit value=Show>";
	    echo " Show Template</button></a>&nbsp ";
	}
	else {
	    echo "<button name=hidetemplate type=submit value=Hide>";
	    echo " Hide Template</button></a>&nbsp ";
	}
	echo "<input type=checkbox name=recursive value=Yep>Recursive? &nbsp ";
	echo "&nbsp &nbsp &nbsp ";
    }
    $root = Template::LookupRoot($guid);

    # We overload the hidden bit on the root.
    if ($root->IsHidden()) {
	echo "<button name=showhidden type=submit value=showhidden>";
	echo " Show Hidden Templates</button></a>&nbsp &nbsp &nbsp &nbsp ";
    }
    else {
	echo "<button name=hidehidden type=submit value=hidehidden>";
	echo " Hide Hidden Templates</button></a>&nbsp &nbsp &nbsp &nbsp ";
    }
    echo "<button name=zoomout type=submit value=zoomout>";
    echo " Zoom Out</button>\n";
    echo "<button name=zoomin type=submit value=zoomin>";
    echo "Zoom In</button>\n";
    echo "</form>\n";
    echo "</center>\n";
}
else {
    $template->ShowNS();
}

SUBPAGEEND();

$paramcount = $template->ParameterCount();
$metacount  = $template->MetadataCount();
$rowspan    = (($paramcount && $metacount) ? 2 : 1);

echo "<center>\n";
echo "<table border=0 bgcolor=#000 color=#000 class=stealth ".
     " cellpadding=0 cellspacing=0>\n";
echo "<tr valign=top><td rowspan=$rowspan class=stealth align=center>\n";

$template->Show();

echo "</td>\n";

if ($paramcount || $metacount) {
    echo "<td align=center class=stealth> &nbsp &nbsp &nbsp </td>\n";
    echo "<td align=center class=stealth> \n";
    
    if ($paramcount && $metacount) {
	$template->ShowParameters();
	echo "</td>\n";
	echo "</tr>\n";
	echo "<tr valign=top>";
	echo "<td align=center class=stealth> &nbsp &nbsp &nbsp </td>\n";
	echo "<td class=stealth align=center>\n";
	$template->ShowMetadata();
    }
    elseif ($paramcount) {
	$template->ShowParameters();
    }
    else {
	$template->ShowMetadata();
    }
    echo "</td>\n";
}

echo "</tr>\n";
echo "</table>\n";
echo "</center>\n";

echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
