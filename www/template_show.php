<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");
require("Sajax.php");
sajax_init();
sajax_export("Show", "GraphChange");

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
    $template = Template::Lookup($guid, $version);    
    if (!$template) {
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

#
# For the Sajax Interface
#
function Show($which, $zoom, $detail)
{
    global $pid, $eid, $uid, $TBSUEXEC_PATH, $TBADMINGROUP;
    global $template, $isadmin;
    $html = "";

    if ($which == "vis") {
	if ($zoom == 0) {
            # Default is whatever we have; to avoid regen of the image.
	    list ($zoom, $detail) = $template->CurrentVisDetails();	    
	}
	else {
            # Sanity check but lets not worry about throwing an error.
	    if (!TBvalid_float($zoom))
		$zoom = 1.25;
	    if (!TBvalid_integer($detail))
		$detail = 1;
    	}

	ob_start();
	$template->ShowVis($zoom, $detail);
	$html = ob_get_contents();
	ob_end_clean();

	$zoomout = sprintf("%.2f", $zoom / 1.25);
	$zoomin  = sprintf("%.2f", $zoom * 1.25);

	$html .= "<button name=viszoomout type=button value=$zoomout";
	$html .= " onclick=\"VisChange('$zoomout', $detail);\">";
	$html .= "Zoom Out</button>\n";
	$html .= "<button name=viszoomin type=button value=$zoomin";
	$html .= " onclick=\"VisChange('$zoomin', $detail);\">";
	$html .= "Zoom In</button>\n";

	if ($detail) {
	    $html .= "<button name=hidedetail type=button value=0";
	    $html .= " onclick=\"VisChange('$zoom', 0);\">";
	    $html .= "Hide Details</button>\n";
	}
	else {
	    $html .= "<button name=showdetail type=button value=1";
	    $html .= " onclick=\"VisChange('$zoom', 1);\">";
	    $html .= "Show Details</button>\n";
	}
    }
    elseif ($which == "graph") {
	ob_start();
	$template->ShowGraph();
	$html = ob_get_contents();
	ob_end_clean();

	if (! $template->IsRoot()) {
	    if ($template->IsHidden()) {
		$html .= "<button name=showtemplate type=button value=Show";
		$html .= " onclick=\"GraphChange('showtemplate');\">";
		$html .= "Show Template</button>&nbsp";
	    }
	    else {
		$html .= "<button name=hidetemplate type=button value=Hide";
		$html .= " onclick=\"GraphChange('hidetemplate');\">";
		$html .= "Hide Template</button>&nbsp";
	    }
	    $html .= "<input id=showexp_recursive type=checkbox value=Yep> ";
	    $html .= "Recursive? &nbsp &nbsp &nbsp &nbsp ";
	}
	$root = Template::LookupRoot($template->guid());

        # We overload the hidden bit on the root.
	if ($root->IsHidden()) {
	    $html .= "<button name=showhidden type=button value=showhidden";
	    $html .= " onclick=\"GraphChange('showhidden');\">";
	    $html .= "Show Hidden Templates</button>&nbsp &nbsp &nbsp &nbsp ";
	}
	else {
	    $html .= "<button name=hidehidden type=button value=hidehidden";
	    $html .= " onclick=\"GraphChange('hidehidden');\"> ";
	    $html .= "Hide Hidden Templates</button>&nbsp &nbsp &nbsp &nbsp ";
	}
	$html .= "<button name=zoomout type=button value=out";
	$html .= " onclick=\"GraphChange('zoomout');\">Zoom Out</button>\n";
	$html .= "<button name=zoomin type=button value=in";
	$html .= " onclick=\"GraphChange('zoomin');\">Zoom In</button>\n";

	# A delete button with a confirm box right there.
	if ($isadmin) {
	    $html .= "<br><br>\n";
	    $html .= "<button name=deletetemplate type=button value=Delete";
	    $html .= " onclick=\"DeleteTemplate();\">";
	    $html .= "<font color=red>Delete</font></button>&nbsp";	
	    $html .= "<input id=confirm_delete type=checkbox value=Yep> ";
	    $html .= "Confirm";
	}
    }
    elseif ($which == "nsfile") {
	$nsdata = "";

	$input_list = $template->InputFiles();

	for ($i = 0; $i < count($input_list); $i++) {
	    $nsdata .= htmlentities($input_list[$i]);
	    $nsdata .= "\n\n";
	}
	$html = "<pre><div align=left class=\"showexp_codeblock\">".
	    "$nsdata</div></pre>\n";

	$html .= "<button name=savens type=button value=1";
	$html .= " onclick=\"SaveNS();\">";
	$html .= "Save</button>\n";
    }
    return $html;
}

#
# Sajax callback for operating on the template graph.
#
function GraphChange($action, $recursive = 0, $no_output = 0)
{
    global $pid, $gid, $eid, $uid, $guid, $TBSUEXEC_PATH, $TBADMINGROUP;
    global $template;
    $html = "";

    # Need this for scripts.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);
    
    $reqarg  = "-a ";
    $versarg = $template->vers();

    if ($action == "zoomout" || $action == "zoomin") {
	$optarg = "";
	
	if ($action == "zoomin") {
	    $optarg = "-z in";
	}
	else {
	    $optarg = "-z out";
	}

        # Need to update the template graph.
	SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $optarg $guid",
	       SUEXEC_ACTION_DIE);
    }
    else {
	$optarg  = ($recursive ? "-r" : "");
	
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
    $template->Refresh();

    $html = "";
    if (! $no_output)
	$html = Show("graph", 0, 0);
    
    return $html;
}

#
# See if this request is to the above function. Does not return
# if it is. Otherwise return and continue on.
#
sajax_handle_client_request();

#
# Active/Inactive is a plain menu link.
#
if (isset($action) && ($action == "activate" || $action == "inactivate")) {
    GraphChange($action, 0, 1);
}

# Delete is just plain special!
if (isset($action) && $action == "deletetemplate" &&
    isset($confirmed) && $confirmed == "yep") {

    # Need this for scripts.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    PAGEHEADER("Delete Template: $guid/$version");
    STARTBUSY("Deleting template $guid/$version recursively");

    # Pass recursive option all the time.
    $retval = SUEXEC($uid, "$pid,$unix_gid",
		     "webtemplate_delete -r $guid/$version",
		     SUEXEC_ACTION_IGNORE);

    CLEARBUSY();
    
    #
    # Fatal Error. Report to the user, even though there is not much he can
    # do with the error. Also reports to tbops.
    # 
    if ($retval < 0) {
	SUEXECERROR(SUEXEC_ACTION_CONTINUE);
    }

    # User error. Tell user and exit.
    if ($retval) {
	SUEXECERROR(SUEXEC_ACTION_USERERROR);
	PAGEFOOTER();
	return;
    }
    #
    # Okay, lets zap back to the root, unless this was the root.
    #
    if ($template->IsRoot()) {
	PAGEREPLACE("showuser.php3?target_uid=$uid");
    }
    else {
	PAGEREPLACE("template_show.php?guid=$guid&version=1");
    }
    return;
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

if ($template->EventCount() > 0) {
    WRITESUBMENUBUTTON("Edit Template Events",
		       "template_editevents.php?guid=$guid&version=$version");
}

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
#
echo "<script type='text/javascript' src='template_sup.js'></script>\n";
echo "<script type='text/javascript' language='javascript'>
        var li_current = 'li_vis';
        function Show(which) {
	    li = getObjbyName(li_current);
            li.style.backgroundColor = '#DDE';
            li.style.borderBottom = 'none';

            li_current = 'li_' + which;
	    li = getObjbyName(li_current);
            li.style.backgroundColor = 'white';
            li.style.borderBottom = '1px solid white';

            x_Show(which, 0, 0, Show_cb);
            return false;
        }
        function Show_cb(html) {
	    visarea = getObjbyName('showexp_visarea');
            if (visarea) {
                visarea.innerHTML = html;
            }
        }
        function ShowVisInit() {
            ADD_DHTML(\"myvisdiv\");
        }
        function ShowGraphInit() {
 	    ADD_DHTML(\"mygraphdiv\");
  	    SetActiveTemplate(\"mygraphimg\", \"CurrentTemplate\", 
			      \"Tarea${version}\");
            tt_Init();
        }
        function VisChange(zoom, detail) {
            x_Show('vis', zoom, detail, Show_cb);
            return false;
        }
        function DeleteTemplate() {
            confirm_flag = 0;
            confirm_box  = getObjbyName('confirm_delete');

	    if (confirm_box) {
                confirm_flag = ((confirm_box.checked == true) ? 1 : 0);
            }
            if (confirm_flag == 0) {
                return false;
            }
	    window.location.replace('template_show.php?guid=$guid" .
                  "&version=$version&action=deletetemplate&confirmed=yep');
            return false;
        }
        function GraphChange(action) {
            recursive_flag = 0;

	    recursive = getObjbyName('showexp_recursive');
            if (recursive) {
                recursive_flag = ((recursive.checked == true) ? 1 : 0);
            }

            x_GraphChange(action, recursive_flag, Show_cb);
            return false;
        }
        function SaveNS() {
            window.open('spitnsdata.php3?guid=$guid&version=$version',
                        'Save NS File','width=650,height=400,toolbar=no,".
                        "resizeable=yes,scrollbars=yes,status=yes,".
	                "menubar=yes');
        }\n\n";
sajax_show_javascript();
echo "</script>\n";
echo "<script type='text/javascript' src='js/wz_dragdrop.js'></script>";

#
# This has to happen for dragdrop to work.
#
$bodyclosestring = "<script type='text/javascript'>SET_DHTML();</script>\n";

#
# This is the topbar
#
echo "<div width=\"100%\" align=center>\n";
echo "<ul id=\"topnavbar\">\n";
echo "<li>
          <a href=\"#A\" style=\"background-color:white\" ".
               "id=\"li_vis\" onclick=\"Show('vis');\">".
               "Visualization</a></li>\n";
echo "<li>
          <a href=\"#B\" id=\"li_nsfile\" onclick=\"Show('nsfile');\">".
              "NS File</a></li>\n";
echo "<li>
          <a href=\"#C\" id=\"li_graph\" onclick=\"Show('graph');\">".
              "Graph</a></li>\n";
echo "</ul>\n";

#
# Start out with Visualization ...
#
echo "<div align=center width=\"100%\" id=\"showexp_visarea\">\n";
echo Show("vis", 0, 0);
echo "</div>\n";
echo "</div>\n";

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

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
