<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("osinfo_defs.php");
include("osiddefs.php3");
include("form_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new OS Descriptor");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$dbid      = $this_user->dbid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("submit",       PAGEARG_STRING,
				 "formfields",   PAGEARG_ARRAY);

#
# See what projects the uid can do this in.
#
$projlist = $this_user->ProjectAccessList($TB_PROJECT_MAKEOSID);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new OS Descriptors!", 1);
}
$projselection = array();
while (list($project) = each($projlist)) {
    $projselection[$project] = $project;
}

#
# Define the form.
#
$form = array('#id'	  => 'myform',
	      '#action'   => "newosid.php3");
$fields = array();

#
# Project Name:
#
$fields['pid'] = array('#type'	     => 'select',
		       '#label'      => 'Project',
		       '#options'    => $projselection,
		       '#checkslot'  => 'os_info:pid',
		       '#required'   => TRUE);
#
# OS Name
#
$fields['osname'] = array('#type'	=> 'textfield',
			  '#label'      => 'Descriptor Name',
			  '#description'=> 'alphanumeric, no spaces',
			  '#size'	=> $TBDB_OSID_OSNAMELEN,
			  '#maxlength'  => $TBDB_OSID_OSNAMELEN,
			  '#checkslot'  => 'os_info:osname',
			  '#required'   => TRUE);

#
# Description
#
$fields['description'] =
    array('#type'	=> 'textfield',
	  '#label'      => 'Description',
	  '#description'=> 'a short pithy sentence',
	  '#size'	=> 50,
	  '#checkslot'  => 'os_info:description',
	  '#required'   => TRUE);

#
# Select an OS
#
$OSselection = array();
while (list ($os, $userokay) = each($osid_oslist)) {
    if (!$userokay && !$isadmin)
	continue;
    $OSselection[$os] = $os;
}
$fields['OS'] = array('#type'	    => 'select',
		      '#label'      => 'Select OS',
		      '#options'    => $OSselection,
		      '#checkslot'  => 'os_info:OS',
		      '#required'   => TRUE);

#
# Version String
#
$fields['version'] =
    array('#type'	=> 'textfield',
	  '#label'      => 'Version',
	  '#size'	=> $TBDB_OSID_VERSLEN,
	  '#maxlength'	=> $TBDB_OSID_VERSLEN,
	  '#checkslot'  => 'os_info:version',
	  '#required'   => TRUE);

#
# Path to Multiboot image.
#
$fields['path'] =
    array('#type'	=> 'textfield',
	  '#label'      => 'Path',
	  '#checkslot'  => 'os_info:path',
	  '#size'	=> 40);

#
# Magic string
#
$fields['magic'] =
    array('#type'	=> 'textfield',
	  '#label'      => 'Magic',
	  '#description'=> 'ie: uname -r -s',
	  '#checkslot'  => 'os_info:magic',
	  '#size'	=> 30);

#
# OS Features
#
$FeatureBoxes = array();
while (list ($feature, $userokay) = each($osid_featurelist)) {
    if (!$userokay && !$isadmin)
	continue;
    $FeatureBoxes["os_feature_$feature"] = array('#return_value'=> "checked",
						 '#label'       => $feature);
}
$fields['features'] =
    array('#type'	=> 'checkboxes',
	  '#label'      => 'OS Features',
	  '#boxes'      => $FeatureBoxes);

#
# Op Mode
#
$OpmodeSelection = array();
while (list ($mode, $userokay) = each($osid_opmodes)) {
    if (!$userokay && !$isadmin)
	continue;
    $OpmodeSelection[$mode] = $mode;
}
$fields['op_mode'] =
    array('#type'	=> 'select',
	  '#label'      => 'Operational Mode',
	  '#options'    => $OpmodeSelection,
	  '#checkslot'  => 'os_info:op_mode',
	  '#required'   => TRUE);

if ($isadmin) {
    #
    # Shared?
    #
    $fields['shared'] =
	array('#type'        => 'checkbox',
	      '#return_value'=> 1,
	      '#label'       => 'Global?',
	      '#checkslot'   => 'os_info:shared',
	      '#description' => 'available to all projects');
    #
    # Mustclean?
    #
    $fields['mustclean'] =
	array('#type'        => 'checkbox',
	      '#return_value'=> 1,
	      '#label'       => 'Clean?',
	      '#checkslot'   => 'os_info:mustclean',
	      '#description' => 'no disk load required');
    
    #
    # Reboot Waittime. 
    #
    $fields['reboot_waittime'] =
	array('#type'	     => 'textfield',
	      '#label'       => 'Reboot Waittime',
	      '#description' => 'seconds',
	      '#checkslot'   => 'os_info:reboot_waittime',
	      '#size'	     => 6);

    #
    # NextOsid
    #
    $osid_result =
	DBQueryFatal("select * from os_info ".
		     "where (path='' or path is NULL) and ".
		     "      version!='' and version is not NULL ".
		     "order by pid,osname");

    $NextOsidSelection = array();
    while ($row = mysql_fetch_array($osid_result)) {
	$osid   = $row["osid"];
	$osname = $row["osname"];
	$pid    = $row["pid"];

	$NextOsidSelection["$osid"] = "$pid - $osname";
    }
    $fields['nextosid'] =
	array('#type'	    => 'select',
	      '#label'      => 'NextOSid',
	      '#checkslot'  => 'os_info:nextosid',
	      '#options'    => $NextOsidSelection);
}

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $form, $fields;
    
    $fields['submit'] = array('#type'  => 'submit',
			      '#value' => "Submit");

    echo "<center>";
    FormRender($form, $errors, $fields, $formfields);
    echo "</center>";
}

#
# On first load, display a virgin form and exit.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["pid"]            = "";
    $defaults["osname"]         = "";
    $defaults["description"]    = "";
    $defaults["OS"]             = "";
    $defaults["version"]        = "";
    $defaults["path"]           = "";
    $defaults["magic"]          = "";
    $defaults["shared"]         = 0;
    $defaults["mustclean"]      = 0;
    $defaults["path"]           = "";
    $defaults["op_mode"]             = TBDB_DEFAULT_OSID_OPMODE;
    $defaults["os_feature_ping"]     = "checked";
    $defaults["os_feature_ssh"]      = "checked";
    $defaults["os_feature_ipod"]     = "checked";
    $defaults["os_feature_isup"]     = "checked";
    $defaults["os_feature_linktest"] = "checked";
    $defaults["reboot_waittime"]     = "";
    $defaults["nextosid"]            = "";

    #
    # For users that are in one project and one subgroup, it is usually
    # the case that they should use the subgroup, and since they also tend
    # to be in the clueless portion of our users, give them some help.
    # 
    if (count($projlist) == 1) {
	list($project, $grouplist) = each($projlist);
	
	if (count($grouplist) <= 2) {
	    $defaults["pid"] = $project;
	}
	reset($projlist);
    }
 
    SPITFORM($defaults, null);
    PAGEFOOTER();
    return;
}
# Form submitted. Make sure we have a formfields array.
if (!isset($formfields)) {
    PAGEARGERROR("Invalid form arguments.");
}

#
# Otherwise, must validate and redisplay if errors
#
$errors  = array();
$project = null;

#
# Project:
#
if (!isset($formfields["pid"]) ||
    strcmp($formfields["pid"], "") == 0) {
    $errors["Project"] = "Not Selected";
}
elseif (!TBvalid_pid($formfields["pid"])) {
    $errors["Project"] = "Invalid project name";
}
elseif (!($project = Project::Lookup($formfields["pid"]))) {
    $errors["Project"] = "Invalid project name";
}
elseif (!$project->AccessCheck($this_user, $TB_PROJECT_MAKEOSID)) {
    $errors["Project"] = "Not enough permission";
}

FormValidate($form, $errors, $fields, $formfields);

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along. We pass only those form fields that
# have actual values and let the backend decide the rest.
#
$args = array();

while (list ($name, $attributes) = each ($fields)) {
    # features special. see below
    if ($name == "features")
	continue;
    # pid and osname are handled in NewOSID
    if ($name == "pid" || $name == "osname")
	continue;
    if (isset($formfields[$name]) && $formfields[$name] != "") {
	$args[$name] = $formfields[$name];
    }
}

#
# Form comma separated list of osfeatures.
#
$os_features_array = array();

reset($osid_featurelist);
while (list ($feature, $userokay) = each($osid_featurelist)) {
    if (isset($formfields["os_feature_$feature"]) &&
	$formfields["os_feature_$feature"] == "checked") {
	$os_features_array[] = $feature;
    }
}
$args["features"] = join(",", $os_features_array);

if (! ($osinfo = OSinfo::NewOSID($this_user, $project,
				 $formfields["osname"], $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

echo "<center><h3>Done!</h3></center>\n";
PAGEREPLACE(CreateURL("showosinfo", $osinfo));

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
