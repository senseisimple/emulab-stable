<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("template_defs.php");

require("Sajax.php");
sajax_init();
sajax_export("GetTemplates", "SearchTemplates");

# If this call is to client request function, then turn off interactive mode.
# All errors will go to above function and get reported back through the
# Sajax interface.
if (sajax_client_request()) {
    $session_interactive = 0;
}

#
# Only known and logged in users ...
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Need these below, set in CheckArguments.
$pid = "";
$eid = "";

function CheckArguments($guid, $version) {
    global $TB_EXPT_READINFO;
    global $uid, $pid, $eid;
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

# See if this request is to one of the above functions. Does not return
# if it is. Otherwise return and continue on.
sajax_handle_client_request();

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
}

#
# Standard Testbed Header after argument checking.
#
PAGEHEADER("Experiment Template: $guid/$version");

?>

<head>
<style>
@import url("timetree/timetree.css");
@import url("logger.css");
@import url("dragview.css");

#fvlogger {
  position: fixed;
  height: 25%;
  bottom: 5px;
}
</style>
</head>

<body onunload="ClientStateFunctions.saveState()">

<div id="fvlogger2"></div>

<script language="javascript" type="text/javascript" src="logger.js">
</script>
<script language="javascript" type="text/javascript" src="json.js">
</script>
<script language="javascript" type="text/javascript" src="js/cookieLib.js">
</script>
<script language="javascript" type="text/javascript" src="js/clientstate.js">
</script>
<script language="javascript" type="text/javascript">
ClientStateFunctions.loadState();
</script>

<script language="javascript" type="text/javascript" src="js/prototype.js">
</script>
<script language="javascript" type="text/javascript" src="timetree/timetree.js">
</script>
<script language="javascript" type="text/javascript" src="js/behaviour.js">
</script>
<script language="javascript" type="text/javascript" src="js/dragview.js">
</script>
<script language="javascript" type="text/javascript" src="js/template.js">
</script>
<script language="javascript" type="text/javascript" src="js/delaybox.js">
</script>
<script language="javascript" type="text/javascript" src="js/activity.js">
</script>

<div id="ttmenucontainer">
<div id="viewcontrols">
<label id="showall"><input id="ttshowall" type="checkbox">Show All</label>
<label id="zoom"><input id="ttzoom" type="checkbox">Zoom</label>
</div>
<label id="search">Search:<input id="ttsearch" type="text"></label>
<span id="matchdisp"><span id="matchcount">0</span> matches</span>
</div>

<div id="ttviewport" class="dragviewport">
  <div id="ttsubstrate" class="dragsubstrate nodrag">
    <div id="templatetree"></div>
  </div>
</div>

<script language="javascript" type="text/javascript">

<?php sajax_show_javascript(); ?>

$("ttshowall").checked = ClientState.Template.SHOW_ALL;

Template.GUID = "<?php echo $guid ?>";
Template.vers = "<?php echo $version ?>";
tds = new Template.DataSource();

sai = new Activity.Indicator($("search"), "url('spinner.gif')");

ttv = new TimeTree.View($("templatetree"), tds);
ttv.delegate = new Template.Delegate();
ttv.updateDOM(0, 0);

dvc = new DragView.Controller($("ttviewport"), $("ttsubstrate"), ttv.bounds);
sc = new DelayBox.Controller();
sc.callback = function(matches) {
    $("matchcount").innerHTML = "" + matches.length;
    Template.updateSearch(matches);
    ttv.reloadData();
    ttv.updateDOM(0, 0);
    sai.decrement();
};
sc.handler = function() {
    sai.increment();
    ClientState.SEARCH_VALUE = $F("ttsearch");
    if (ClientState.SEARCH_VALUE == "") {
        this.callback([]);
    }
    else {
        x_SearchTemplates(Template.GUID, ClientState.SEARCH_VALUE, this.callback);
    }
};

var myrules = {
    "#ttsearch" : sc.behaviour,
    "#ttshowall" : function(element) {
	element.onclick = function() {
	    ClientState.Template.SHOW_ALL = $("ttshowall").checked;
	    ttv.reloadData();
	    ttv.updateDOM(0, 0);
	};
    },
    "#ttzoom" : function(element) {
	element.onclick = function() {
	    ClientState.ZOOMED = $("ttzoom").checked;
	    dvc.zoom(ClientState.ZOOMED);
	};
    }
};

Behaviour.register(myrules);

x_GetTemplates(Template.GUID, function(dbstate) {
    Template.DBtoNode(dbstate);
    Template.tracePath(Template.vers2node[<?php echo $version ?>]);
    ttv.reloadData();
    ttv.updateDOM(0, 0);
    sc.handler();
});

if ("SEARCH_VALUE" in ClientState && ClientState.SEARCH_VALUE != "") {
    $("ttsearch").value = ClientState.SEARCH_VALUE;
}
else {
    ClientState.SEARCH_VALUE = "";
}

if (!("ZOOMED" in ClientState))
    ClientState.ZOOMED = false;
$("ttzoom").checked = ClientState.ZOOMED;
dvc.zoom(ClientState.ZOOMED);

</script>

<?php

SUBPAGESTART();

SUBMENUSTART("Template Options");

WRITESUBMENUBUTTON("Show NS File &nbsp &nbsp",
		   "spitnsdata.php3?guid=$guid&version=$version");

WRITESUBMENUBUTTON("Modify Template",
		   "template_modify.php?guid=$guid&version=$version");

if (TBIsExperimentTemplateHidden($guid, $version)) {
    WRITESUBMENUBUTTON("Show Template",
		       "template_show.php?guid=$guid&version=$version&action=show");
}
else {
    WRITESUBMENUBUTTON("Hide Template",
		       "template_show.php?guid=$guid&version=$version&action=hide");
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

SUBMENUEND_2B();

SHOWTEMPLATE($guid, $version);
SHOWTEMPLATEMETADATA($guid, $version);
SUBPAGEEND();

if (SHOWTEMPLATEPARAMETERS($guid, $version)) {
    echo "<br>\n";
}
SHOWTEMPLATEINSTANCES($guid, $version);

SHOWTEMPLATEGRAPH($guid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
