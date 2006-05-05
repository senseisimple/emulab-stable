<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

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

function CheckArguments($guid, $version) {
    global $TB_EXPT_READINFO;
    global $uid, $pid, $eid, $gid;

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
    if (! TBValidExperimentTemplate($guid, $version)) {
	USERERROR("The experiment template $guid/$version is not a ".
		  "valid experiment template!", 1);
    }
    TBTemplatePidEid($guid, $version, $pid, $eid);
  
    #
    # Verify Permission.
    #
    if (! TBExptTemplateAccessCheck($uid, $guid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view experiment ".
		  "template $guid/$version!", 1);
    }
}

/**
 * Get the template versions associated with the given GUID.
 *
 * @param $guid The template GUID.
 * @return An array of template "objects".
 */
function GetTemplates($guid) {
    CheckArguments($guid, 1); /* XXX bogus version number. */

    $query_result =
	DBQueryFatal("SELECT vers,parent_vers,tid,hidden ".
		     "FROM experiment_templates ".
		     "WHERE guid='$guid' ".
		     "ORDER BY vers");

    $retval = array();
    while ($row = mysql_fetch_array($query_result)) {
	$metadata_result =
	    DBQueryFatal("SELECT name,value ".
			 "FROM experiment_template_metadata ".
			 "WHERE template_guid='$guid' and ".
			 "      template_vers='$row[vers]' ".
			 "ORDER BY created DESC");

	$metadata = array();
	while ($mrow = mysql_fetch_array($metadata_result)) {
	    $metadata[$mrow[name]] = $mrow[value];
	}

	if (count($metadata) > 0) {
	    $row[metadata] = $metadata;
	}
	
	if (TBTemplatePidEid($guid, $row[vers], &$pid, &$eid)) {
	    $parameter_result =
		DBQueryFatal("SELECT name,value FROM virt_parameters ".
			     "WHERE pid='$pid' and eid='$eid'");
	    
	    $parameters = array();
	    while ($prow = mysql_fetch_array($parameter_result)) {
		$parameters[$prow[name]] = $prow[value];
	    }
	    
	    if (count($parameters) > 0) {
		$row[parameters] = $parameters;
	    }
	}
	
	$retval[] = $row;
    }
    
    return $retval;
}

function queryFilter($v) {
    return ($v != "");
}

function SearchTemplates($guid, $terms) {
    /* XXX */
    CheckArguments($guid, 1);

    $matches = array();
    $retval = array();

    $terms = split(" ", $terms);
    $terms = array_filter($terms, "queryFilter");

    foreach ($terms as $term) {
	$term = "'%" . mysql_real_escape_string($term) . "%'";

	$query_result =
	    DBQueryFatal("SELECT template_vers ".
			 "FROM experiment_template_metadata ".
			 "WHERE template_guid='$guid' and ".
			 "      (name like ". $term . " or " .
			 "       value like ". $term . ") " .
			 "GROUP BY template_vers");

	while ($row = mysql_fetch_array($query_result)) {
	    if (!isset($matches[$row[template_vers]])) {
		$matches[$row[template_vers]] = 0;
	    }
	    $matches[$row[template_vers]] += 1;
	}
    }

    foreach ($matches as $key => $match) {
	if ($match == count($terms)) {
	    $retval[] = $key;
	}
    }

    return $retval;
}

CheckArguments($guid, $version);

if (isset($action)) {
    if ($action == "hide") {
	DBQueryFatal("UPDATE experiment_templates SET hidden=1 ".
		     "WHERE guid='$guid' and vers='$version'");
    }
    else if ($action == "show") {
	DBQueryFatal("UPDATE experiment_templates SET hidden=0 ".
		     "WHERE guid='$guid' and vers='$version'");
    }
    else if ($action == "showall") {
	DBQueryFatal("UPDATE experiment_templates SET hidden=0 ".
		     "WHERE guid='$guid'");
    }

    if (! TBGuid2PidGid($guid, $pid, $gid)) {
	TBERROR("Could not get template pid,gid for template $guid", 1);
    }
    # Need to update the template graph.
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    SUEXEC($uid, "$pid,$unix_gid", "webtemplate_graph $guid",
	   SUEXEC_ACTION_DIE);
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template: $guid/$version");

SUBPAGESTART();

SUBMENUSTART("Template Options");

WRITESUBMENUBUTTON("Show NS File &nbsp &nbsp",
		   "spitnsdata.php3?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Modify Template",
		   "template_modify.php?guid=$guid&version=$version");

if (! TBIsRootTemplate($guid, $version)) {
    $hidden = TBIsExperimentTemplateHidden($guid, $version);
    
    if ($hidden) {
	WRITESUBMENUBUTTON("Show Template",
			   "template_show.php?guid=$guid".
			   "&version=$version&action=show");
    }
    else {
	WRITESUBMENUBUTTON("Hide Template",
			   "template_show.php?guid=$guid".
			   "&version=$version&action=hide");
    }
}

WRITESUBMENUBUTTON("Instantiate Template",
		   "template_swapin.php?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Add Metadata",
		   "template_metadata.php?action=add&".
		   "guid=$guid&version=$version");

WRITESUBMENUBUTTON("Template Archive",
		   "archive_view.php3?pid=$pid&eid=$eid");

WRITESUBMENUBUTTON("Template History",
		   "template_history.php?guid=$guid&version=$version");

SUBMENUEND_2A();

#
# Ick.
#
$rsrcidx = TBrsrcIndex($pid, $eid);

echo "<br>
      <img border=1 alt='template visualization'
           src='showthumb.php3?idx=$rsrcidx'>";

if (TemplateInstanceCount($guid, $version)) {
    SHOWTEMPLATEINSTANCES($guid, $version);
}

SUBMENUEND_2B();

echo "<script type='text/javascript' src='js/wz_dragdrop.js'></script>";

SHOWTEMPLATEGRAPH($guid);

#
# Define the zoom buttons. This should go elsewhere.
#
echo "<script type=text/javascript>
      function ZoomOut() {
      }
      function ZoomIn() {
      }
      function ShowAll() {
          open('template_show.php?guid=$guid&version=$version&action=showall',
               '_self');
      }
      </script>\n";

echo "<center>\n";
echo "<button name=showall type=button onClick='ShowAll();'>";
echo " Show All Templates</button></a>&nbsp &nbsp ";
echo "<button name=zoomout type=button onClick='ZoomOut();'>";
echo " Zoom Out</button>\n";
echo "<button name=zoomin type=button onClick='ZoomIn();'>";
echo "Zoom In</button>\n";
echo "</center>\n";

SUBPAGEEND();

echo "<script type='text/javascript' src='js/wz_tooltip.js'></script>";

$paramcount = TemplateParameterCount($guid, $version);
$metacount  = TemplateMetadataCount($guid, $version);
$rowspan    = (($paramcount && $metacount) ? 2 : 1);

echo "<center>\n";
echo "<table border=0 bgcolor=#000 color=#000 class=stealth ".
     " cellpadding=0 cellspacing=0>\n";
echo "<tr valign=top><td rowspan=$rowspan class=stealth align=center>\n";

SHOWTEMPLATE($guid, $version);

echo "</td>\n";

if ($paramcount || $metacount) {
    echo "<td align=center class=stealth> &nbsp &nbsp &nbsp </td>\n";
    echo "<td align=center class=stealth> \n";
    
    if ($paramcount && $metacount) {
	SHOWTEMPLATEPARAMETERS($guid, $version);
	echo "</td>\n";
	echo "</tr>\n";
	echo "<tr valign=top>";
	echo "<td align=center class=stealth> &nbsp &nbsp &nbsp </td>\n";
	echo "<td class=stealth align=center>\n";
	SHOWTEMPLATEMETADATA($guid, $version);
    }
    elseif ($paramcount) {
	SHOWTEMPLATEPARAMETERS($guid, $version);
    }
    else {
	SHOWTEMPLATEMETADATA($guid, $version);
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
