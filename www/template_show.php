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
if (!isset($show)) {
    $show = "vis";
}

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

if ((isset($action) && $action != "" && $action != "none") ||
    ($show == "graph" && isset($zoom) && $zoom != "none")) {

    # Need this for scripts.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    # Hide or show templates.
    if (isset($action) && $action != "" && $action != "none") {
	$optarg  = ((isset($recursive) && $recursive == "Yep") ? "-r" : "");
	$reqarg  = "-a ";
	$versarg = "$version";

	if ($action == "showtemplate") {
	    $reqarg .= "show";
	}
	elseif ($action == "hidetemplate") {
	    $reqarg .= "hide";
	}
	elseif ($action == "showhidden") {
	    $reqarg .= "showhidden";
	    # Applies only to root template
	    $versarg = "1";
	}
	elseif ($action == "hidehidden") {
	    $reqarg .= "hidehidden";
	    # Applies only to root template
	    $versarg = "1";
	}
	elseif ($action == "activate") {
	    $reqarg .= "activate";
	}
	elseif ($action == "inactivate") {
	    $reqarg .= "inactivate";
	}
	else {
	    PAGEARGERROR("Invalid action $action");
	    return;
	}
	$reqarg .= " $guid/$versarg";
	
	SUEXEC($uid, "$pid,$unix_gid",
	       "webtemplate_control $reqarg $optarg",
	       SUEXEC_ACTION_DIE);
    }
    elseif (isset($zoom) && ($zoom == "out" || $zoom == "in")) {
	$optarg = "";
	
	if ($zoom == "in") {
	    $optarg = "-z in";
	}
	else {
	    $optarg = "-z out";
	}

        # Need to update the template graph.
	SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $optarg $guid",
	       SUEXEC_ACTION_DIE);
    }
    $template->Refresh();
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template: $guid/$version");

SUBPAGESTART();

SUBMENUSTART("Template Options");

if ($template->IsActive()) {
    WRITESUBMENUBUTTON("InActivate Template &nbsp &nbsp",
		       "template_show.php?guid=$guid".
		       "&version=$version&action=inactivate");
}
else {
    WRITESUBMENUBUTTON("Activate Template &nbsp &nbsp",
		       "template_show.php?guid=$guid".
		       "&version=$version&action=activate");
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

#
# The center area is a form that can show NS file, Template Graph, or Vis.
# IE complicates this, although in retrospect, I could have used plain
# input buttons instead of the fancy rendering kind of buttons, which do not
# work as expected (violates the html spec) in IE. 
#
echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";
echo "<script type='text/javascript' language='javascript'>
        function Show(which) {
            document.form1['show'].value = which;
            document.form1.submit();
            return false;
        }
        function Zoom(howmuch) {
            document.form1['zoom'].value = howmuch;
            document.form1.submit();
            return false;
        }
        function DoAction(action) {
            document.form1['action'].value = action;
            document.form1.submit();
            return false;
        }
      </script>\n";

echo "<center>\n";
echo "<form action='template_show.php?guid=$guid&version=$version'
            name=form1 method=post>\n";
echo "<input type=hidden name=show   value=$show>\n";
echo "<input type=hidden name=zoom   value='none'>\n";
echo "<input type=hidden name=action value='none'>\n";

echo "<button name=showns type=button value=ns onclick=\"Show('ns');\"
        style='float:center; width:15%;'>NS File</button>\n";
echo "<button name=showvis type=button value=vis onclick=\"Show('vis');\"
        style='float:center; width:15%;'>Visualization</button>\n";
echo "<button name=showgraph type=button value=graph onclick=\"Show('graph');\"
        style='float:center; width:15%;'>Graph</button>\n";

if ($show == "graph") {
    $template->ShowGraph();

    if (! $template->IsRoot()) {
	if ($template->IsHidden()) {
	    echo "<button name=showtemplate type=button value=Show";
	    echo " onclick=\"DoAction('showtemplate');\">Show Template";
	    echo "</button>&nbsp";
	}
	else {
	    echo "<button name=hidetemplate type=button value=Hide";
	    echo " onclick=\"DoAction('hidetemplate');\">Hide Template";
	    echo "</button>&nbsp";
	}
	echo "<input type=checkbox name=recursive value=Yep>Recursive? &nbsp ";
	echo "&nbsp &nbsp &nbsp ";
    }
    $root = Template::LookupRoot($guid);

    # We overload the hidden bit on the root.
    if ($root->IsHidden()) {
	echo "<button name=showhidden type=button value=showhidden";
	echo " onclick=\"DoAction('showhidden');\">Show Hidden Templates ";
	echo "</button>&nbsp &nbsp &nbsp &nbsp ";
    }
    else {
	echo "<button name=hidehidden type=button value=hidehidden";
	echo " onclick=\"DoAction('hidehidden');\">Hide Hidden Templates ";
	echo "</button>&nbsp &nbsp &nbsp &nbsp ";
    }
    echo "<button name=zoomout type=button value=out";
    echo " onclick=\"Zoom('out');\">Zoom Out</button>\n";
    echo "<button name=zoomin type=button value=in";
    echo " onclick=\"Zoom('in');\">Zoom In</button>\n";
}
elseif ($show == "ns") {
    $template->ShowNS();
}
elseif ($show == "vis") {
    # Default is whatever we have; to avoid regen of the image.
    list ($newzoom, $newdetail) = $template->CurrentVisDetails();
    
    if (isset($zoom) && $zoom != "none")
	$newzoom = $zoom;

    # Sanity check but lets not worry about throwing an error.
    if (!TBvalid_float($newzoom))
	$newzoom = 1.25;
    if (!TBvalid_integer($newdetail))
	$newdetail = 1;
    
    $template->ShowVis($newzoom, $newdetail);

    $zoomout = sprintf("%.2f", $newzoom / 1.25);
    $zoomin  = sprintf("%.2f", $newzoom * 1.25);

    echo "<button name=viszoomout type=button value=$zoomout";
    echo " onclick=\"Zoom('$zoomout');\">Zoom Out</button>\n";
    echo "<button name=viszoomin type=button value=$zoomin";
    echo " onclick=\"Zoom('$zoomin');\">Zoom In</button>\n";
}
echo "</form>\n";
echo "</center>\n";

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

print_r($HTTP_POST_VARS);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
