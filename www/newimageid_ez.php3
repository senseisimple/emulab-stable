<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new Image Descriptor (EZ Form)");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Default features. Needs to move someplace else!
# 
$featurelist         = array();
$featurelist["ping"] = "ping";
$featurelist["ssh"]  = "ssh";
$featurelist["ipod"]  = "ipod";
$featurelist["isup"]  = "isup";

#
# Default OS strings. Needs to move someplace else!
#
$oslist            = array();
$oslist["Linux"]   = "Linux";
$oslist["FreeBSD"] = "FreeBSD";
$oslist["NetBSD"]  = "NetBSD";
$oslist["Other"]   = "Other";

#
# Default op modes. Needs to move someplace else!
#
$opmodes             = array();
$opmodes["NORMALv1"] = "NORMALv1";
$opmodes["MINIMAL"]  = "MINIMAL";
$opmodes["NORMAL"]   = "NORMAL";

#
# See what projects the uid can do this in.
#
$projlist = TBProjList($uid, $TB_PROJECT_MAKEIMAGEID);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new Image descriptors.", 1);
}

#
# Need a list of node types. We join this over the nodes table so that
# we get a list of just the nodes that currently in the testbed, not
# just in the node_types table.
#
$types_result =
    DBQueryFatal("select distinct n.type from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "where nt.imageable=1");

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $uid, $projlist, $isadmin, $types_result, $oslist, $opmodes,
      $featurelist;
    global $TBDB_IMAGEID_IMAGENAMELEN, $TBDB_NODEIDLEN;
    global $TBDB_OSID_VERSLEN, $TBBASE;

    echo "<center><b>
          See the
          <a href=tutorial/docwrapper.php3?docname=tutorial.html#CustomOS>
          tutorial</a> for more info on creating/using custom Images.
          </b></center>\n";

    if ($isadmin) {
	echo "<center>
               Administrators get to use the
               <a href='newimageid.php3'>long form</a>.
              </center>\n";
    }
    
    if ($errors) {
	echo "<table class=nogrid
                     align=center border=0 cellpadding=6 cellspacing=0>
              <tr>
                 <th align=center colspan=2>
                   <font size=+1 color=red>
                      &nbsp;Oops, please fix the following errors!&nbsp;
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right>
                       <font color=red>$name:&nbsp;</font></td>
                     <td align=left>
                       <font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<SCRIPT LANGUAGE=JavaScript>
              function SetPrefix(theform) 
              {
                  var pidx   = theform['formfields[pid]'].selectedIndex;
                  var pid    = theform['formfields[pid]'].options[pidx].value;
                  var gidx   = theform['formfields[gid]'].selectedIndex;
                  var gid    = theform['formfields[gid]'].options[gidx].value;
                  var shared = theform['formfields[shared]'].checked; 
          \n";
    if ($isadmin)
	echo     "var global = theform['formfields[global]'].checked;";
    else
	echo     "var global = 0;";

    echo         "if (pid == '') {
                      theform['formfields[path]'].value = '/proj';
                  }
                  else if (theform['formfields[imagename]'].value == '') {
		      theform['formfields[imagename]'].defaultValue = '';

                      if (global) {
    	                  theform['formfields[path]'].value =
                                  '/usr/testbed/images/';
                      }
		      else if (gid == '' || gid == pid || shared) {
    	                  theform['formfields[path]'].value =
                                  '/proj/' + pid + '/images/';
                      }
                      else {
    	                  theform['formfields[path]'].value =
                                  '/groups/' + pid + '/' + gid + '/';
                      }
                  }
                  else if (theform['formfields[imagename]'].value != '') {
                      var filename = theform['formfields[imagename]'].value +
                                     '.ndz';

                      if (global) {
    	                  theform['formfields[path]'].value =
                                  '/usr/testbed/images/' + filename;
                      }
		      else if (gid == '' || gid == pid || shared) {
    	                  theform['formfields[path]'].value =
                                  '/proj/' + pid + '/images/' + filename;
                      }
                      else {
    	                  theform['formfields[path]'].value =
                                  '/groups/' + pid + '/' + gid + '/' +
                                  filename;
                      }
                  }
              }
          </SCRIPT>\n";

    echo "<br>
          <table align=center border=1> 
          <tr>
             <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='newimageid_ez.php3' method=post name=idform>\n";

    #
    # Select Project
    #
    echo "<tr>
              <td>*Select Project:</td>
              <td><select name=\"formfields[pid]\"
                          onChange='SetPrefix(idform);'>
                      <option value=''>Please Select &nbsp</option>\n";
    
    while (list($project) = each($projlist)) {
	$selected = "";

	if ($formfields[pid] == $project)
	    $selected = "selected";
	
	echo "        <option $selected value='$project'>$project </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # Select a group
    # 
    echo "<tr>
              <td >Group:</td>
              <td><select name=\"formfields[gid]\"
                          onChange='SetPrefix(idform);'>
                    <option value=''>Default Group </option>\n";

    reset($projlist);
    while (list($project, $grouplist) = each($projlist)) {
	for ($i = 0; $i < count($grouplist); $i++) {
	    $group    = $grouplist[$i];

	    if (strcmp($project, $group)) {
		$selected = "";

		if (isset($formfields[gid]) &&
		    isset($formfields[pid]) &&
		    strcmp($formfields[pid], $project) == 0 &&
		    strcmp($formfields[gid], $group) == 0)
		    $selected = "selected";
		
		echo "<option $selected value=\"$group\">
                           $project/$group</option>\n";
	    }
	}
    }
    echo "     </select>
             </td>
          </tr>\n";

    #
    # Image Name:
    #
    echo "<tr>
              <td>*Descriptor Name (no blanks):</td>
              <td class=left>
                  <input type=text
                         onChange='SetPrefix(idform);'
                         name=\"formfields[imagename]\"
                         value=\"" . $formfields[imagename] . "\"
	                 size=$TBDB_IMAGEID_IMAGENAMELEN
                         maxlength=$TBDB_IMAGEID_IMAGENAMELEN>
              </td>
          </tr>\n";

    #
    # Description
    #
    echo "<tr>
              <td>*Description:<br>
                  (a short pithy sentence)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[description]\"
                         value=\"" . $formfields[description] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # Load Partition
    #
    echo "<tr>
              <td>*Which DOS Partition[<b>1</b>]:<br>
                  (DOS partitions are numbered 1-4)</td>
              <td><select name=\"formfields[loadpart]\">
                          <option value=X>Please Select </option>\n";

    for ($i = 1; $i <= 4; $i++) {
	$selected = "";

	if (strcmp($formfields[loadpart], "$i") == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=$i>$i </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # Select an OS
    # 
    echo "<tr>
             <td>*Operating System:<br>
                (OS that is on the partition)</td>
             <td><select name=\"formfields[os_name]\">
                   <option value=none>Please Select </option>\n";

    while (list ($os, $value) = each($oslist)) {
	$selected = "";

	if (isset($formfields[os_name]) &&
	    strcmp($formfields[os_name], $os) == 0)
	    $selected = "selected";

	echo "<option $selected value=$os>$os &nbsp </option>\n";
    }
    echo "       </select>
             </td>
          </tr>\n";

    #
    # Version String
    #
    echo "<tr>
              <td>*OS Version:<br>
                  (eg: 4.3, 7.2, etc.)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[os_version]\"
                         value=\"" . $formfields[os_version] . "\"
	                 size=$TBDB_OSID_VERSLEN maxlength=$TBDB_OSID_VERSLEN>
              </td>
          </tr>\n";

    #
    # Path to image.
    #
    echo "<tr>
              <td>Filename (full path) of Image:<br>
                  (must reside in /proj)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[path]\"
                         value=\"" . $formfields[path] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # Node to Create image from.
    #
    echo "<tr>
              <td>Node to Create Image from[<b>2</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node]\"
                         value=\"" . $formfields[node] . "\"
	                 size=$TBDB_NODEIDLEN maxlength=$TBDB_NODEIDLEN>
              </td>
          </tr>\n";

    #
    # OS Features.
    # 
    echo "<tr>
              <td>OS Features[<b>3</b>]:</td>
              <td>";

    reset($featurelist);
    while (list ($feature, $value) = each($featurelist)) {
	$checked = "";
	
	if (isset($formfields["os_feature_$feature"]) &&
	    ! strcmp($formfields["os_feature_$feature"], "checked"))
	    $checked = "checked";

	echo "<input $checked type=checkbox value=checked
                     name=\"formfields[os_feature_$feature]\">
                   $feature &nbsp\n";
    }
    echo "    </td>
          </tr>\n";

    #
    # Operational Mode
    # 
    echo "<tr>
             <td>*Operational Mode[<b>4</b>]:</td>
             <td><select name=\"formfields[op_mode]\">
                   <option value=none>Please Select </option>\n";

    while (list ($mode, $value) = each($opmodes)) {
	$selected = "";

	if (isset($formfields[op_mode]) &&
	    strcmp($formfields[op_mode], $mode) == 0)
	    $selected = "selected";

	echo "<option $selected value=$mode>$mode &nbsp </option>\n";
    }
    echo "       </select>
             </td>
          </tr>\n";

    #
    # Node Types.
    #
    echo "<tr>
              <td>Node Types[<b>5</b>]:</td>
              <td>\n";

    mysql_data_seek($types_result, 0);
    while ($row = mysql_fetch_array($types_result)) {
        $type    = $row[type];
        $checked = "";

        if (strcmp($formfields["mtype_$type"], "Yep") == 0
	    || strcmp($formfields["mtype_all"], "Yep") == 0)
	    $checked = "checked";
    
        echo "<input $checked type=checkbox
                     value=Yep name=\"formfields[mtype_$type]\">
                     $type &nbsp
              </input>\n";
    }
    echo "    </td>
          </tr>\n";

    #
    # Whole Disk Image
    #
    echo "<tr>
  	      <td>Whole Disk Image?[<b>6</b>]:</td>
              <td class=left>
                  <input type=checkbox
                         name=\"formfields[wholedisk]\"
                         value=Yep";

    if (isset($formfields[wholedisk]) &&
	strcmp($formfields[wholedisk], "Yep") == 0)
	echo "           checked";
	
    echo "                       > Yes
              </td>
          </tr>\n";

    #
    # Maxiumum concurrent loads
    #
    echo "<tr>
              <td>Maximum concurrent loads[<b>7</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[max_concurrent]\"
                         value=\"" . $formfields[max_concurrent] . "\"
	                 size=4 maxlength=4>
              </td>
          </tr>\n";

    
    #
    # Shared?
    #
    echo "<tr>
  	      <td>Shared?:<br>
                  (available to all subgroups)</td>
              <td class=left>
                  <input type=checkbox
                         onClick='SetPrefix(idform);'
                         name=\"formfields[shared]\"
                         value=Yep";

    if (isset($formfields[shared]) &&
	strcmp($formfields[shared], "Yep") == 0)
	echo "           checked";
	
    echo "                       > Yes
              </td>
          </tr>\n";

    if ($isadmin) {
        #
        # Global?
        #
	echo "<tr>
  	          <td>Global?:<br>
                      (available to all projects)</td>
                  <td class=left>
                      <input type=checkbox
                             onClick='SetPrefix(idform);'
                             name=\"formfields[global]\"
                             value=Yep";

	if (isset($formfields["global"]) &&
	    strcmp($formfields["global"], "Yep") == 0)
	    echo "           checked";
	
	echo "                       > Yes
                  </td>
              </tr>\n";

    }

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<h4><blockquote>
          <ol type=1 start=1>
             <li> If you don't know what partition you have customized,
    	          here are some guidelines:
  	         <ul>
 		    <li> if you customized one of our standard Linux
		         images (RHL-*) then it is partition 2.
	            </li>
                    <li> if you customized one of our standard BSD
 		         images (FBSD-*) then it is partition 1.
	            </li>
                    <li> otherwise, feel free to ask us!
		    </li>
                 </ul>
             </li>
             <li> If you already have a node customized, enter that node
                  name (pcXXX) and the image will be auto created for you.
                  Notification of completion will be sent to you via email. 
	     </li>
             <li> Guidelines for setting OS features for your OS:
                  (Most images should mark all four of these.)
                <ul>
                  <li> Mark ping and/or ssh if they are supported. 
		  </li>
                  <li> If you use one of our standard Linux or FreeBSD
                       kernels, or started from our kernel configs, mark ipod.
		  </li>
                  <li> If it is based on one of our standard Linux or
                       FreeBSD images (or otherwise
                       sends its own ISUP notification), mark isup.
		  </li>
                </ul>
	     </li>
             <li> Guidelines for setting Operational Mode for your OS:
                  (Most images should use NORMALv1.)
                <ul>
                  <li> If it is based on a testbed image (one of our
                       RedHat Linux or FreeBSD images)  use the same
                       op_mode as that image (should be NORMALv1,
                       or NORMAL for old images. Select it from the
                       <a href=\"$TBBASE/showosid_list.php3\"
                       >OS Descriptor List</a> to find out).
		  </li>
                  <li> If not, use MINIMAL. 
		  </li>
                </ul>
	     </li>
             <li> Specify the node types that this image will be able
                  to work on (can be loaded on and expected to work).
                  Typically, images will work on all of the \"pc\" types when
                  you are customizing one of the standard images. However,
                  if you are installing your own OS from scratch, or you are
                  using DOS partition four, then this might not be true.
                  Feel free to ask us!
	     </li>
             <li> If you need to snapshot the entire disk (including the MBR),
                  check this option. <b>Most users will not need to check this
                  option. Please ask us first to make sure</b>.
	     </li>
             <li> If your image contains software that is only licensed to run
	  	  on a limited number of nodes at a time, you can put this
		  number here. Most users will want to leave this option blank.
	     </li>
          </ol>
          </blockquote></h4>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    $defaults = array();
    $defaults[loadpart] = "X";
    $defaults[path]     = "/proj/";
    $defaults[op_mode]  = "NORMALv1";
    $defaults[os_feature_ping] = "checked";
    $defaults[os_feature_ssh]  = "checked";
    $defaults[os_feature_ipod] = "checked";
    $defaults[os_feature_isup] = "checked";

    # mtype_all is a "fake" variable which makes all
    # mtypes checked in the virgin form.
    $defaults[mtype_all] = "Yep";

    #
    # For users that are in one project and one subgroup, it is usually
    # the case that they should use the subgroup, and since they also tend
    # to be in the clueless portion of our users, give them some help.
    # 
    if (count($projlist) == 1) {
	list($project, $grouplist) = each($projlist);
	
	if (count($grouplist) <= 2) {
	    $defaults[pid] = $project;
	    if (count($grouplist) == 1 || strcmp($project, $grouplist[0]))
		$group = $grouplist[0];
	    else {
		$group = $grouplist[1];
	    }
	    $defaults[gid] = $group;
	    
	    if (!strcmp($project, $group))
		$defaults[path]     = "/proj/$project/images/";
	    else
		$defaults[path]     = "/groups/$project/$group/";
	}
	reset($projlist);
    }
    
    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors = array();

#
# Project:
# 
if (!isset($formfields[pid]) ||
    strcmp($formfields[pid], "") == 0) {
    $errors["Project"] = "Not Selected";
}
elseif (!TBValidProject($formfields[pid])) {
    $errors["Project"] = "No such project";
}
elseif (isset($formfields[gid]) &&
	strcmp($formfields[gid], "")) {
    if (!TBValidGroup($formfields[pid], $formfields[gid])) {
	$errors["Group"] = "Group '" . $formfields[gid] . "' " .
	                   "is not in project '" . $formfields[pid] ."'";
    }
    elseif (!TBProjAccessCheck($uid, $formfields[pid],
			       $formfields[gid], $TB_PROJECT_MAKEIMAGEID)) {
	$errors["Project/Group"] = "Not enough permission";
    }
}
elseif (!TBProjAccessCheck($uid, $formfields[pid],
			   $formfields[pid], $TB_PROJECT_MAKEIMAGEID)) {
    $errors["Project/Group"] = "Not enough permission";
}

#
# Image Name:
# 
if (!isset($formfields[imagename]) ||
    strcmp($formfields[imagename], "") == 0) {
    $errors["Descriptor Name"] = "Missing Field";
}
else {
    if (! ereg("^[a-zA-Z0-9][-_a-zA-Z0-9\.\+]+$", $formfields[imagename])) {
	$errors["Descriptor Name"] =
	    "Must be alphanumeric (includes _, -, +, and .)<br>".
	    "and must begin with an alphanumeric";
    }
    elseif (strlen($formfields[imagename]) > $TBDB_IMAGEID_IMAGENAMELEN) {
	$errors["Descriptor Name"] =
	    "Too long! ".
	    "Must be less than or equal to $TBDB_IMAGEID_IMAGENAMELEN";
    }
}

#
# Description
#
if (!isset($formfields[description]) ||
    strcmp($formfields[description], "") == 0) {
    $errors["Description"] = "Missing Field";
}

#
# Load Partition
#
if (!isset($formfields[loadpart]) ||
    strcmp($formfields[loadpart], "") == 0 ||
    strcmp($formfields[loadpart], "X") == 0) {
    $errors["DOS Partition"] = "Not Selected";
}
elseif (! ereg("^[0-9]+$", $formfields[loadpart]) ||
	$formfields[loadpart] <= 0 || $formfields[loadpart] > 4) {
    $errors["DOS Partition"] = "Must be 1,2,3, or 4!";
}

#
# Select an OS
# 
if (!isset($formfields[os_name]) ||
    strcmp($formfields[os_name], "") == 0 ||
    strcmp($formfields[os_name], "none") == 0) {
    $errors["OS"] = "Not Selected";
}
elseif (! isset($oslist[$formfields[os_name]])) {
    $errors["OS"] = "Invalid OS";
}

#
# Version String
#
if (!isset($formfields[os_version]) ||
    strcmp($formfields[os_version], "") == 0) {
    $errors["OS Version"] = "Missing Field";
}
elseif (! ereg("^[-_a-zA-Z0-9\.]+$", $formfields[os_version])) {
    $errors["OS Version"] = "Contains invalid characters";
}

#
# Only admin types can set the global bit for an image. Ignore silently.
#
$global = 0;
if ($isadmin &&
    isset($formfields["global"]) &&
    strcmp($formfields["global"], "Yep") == 0) {
    $global = 1;
}

#
# Image shared amongst subgroups
#
$shared = 0;
if (isset($formfields[shared]) &&
    strcmp($formfields[shared], "Yep") == 0) {
    $shared = 1;
}
# Does not make sense to do this. 
if ($global && $shared) {
    $errors["Global"] = "Image declared both shared and global";
}

#
# The path must not contain illegal chars and it must start with
# /proj/$pid/images or /groups/$pid/$gid. Admins can do whatever
# they like of course.
# 
if (!isset($formfields[path]) ||
    strcmp($formfields[path], "") == 0) {
    $errors["Path"] = "Missing Field";
}
elseif (! ereg("^[-_a-zA-Z0-9\/\.+]+$", $formfields[path])) {
    $errors["Path"] = "Contains invalid characters";
}
elseif (! $isadmin) {
    $pdef = "";
    
    if (!$shared &&
	isset($formfields[gid]) &&
	strcmp($formfields[gid], "") &&
	strcmp($formfields[gid], $formfields[pid])) {
	$pdef = "/groups/" . $formfields[pid] . "/" . $formfields[gid] . "/";
    }
    else {
	$pdef = "/proj/" . $formfields[pid] . "/images/";
    }

    if (strpos($formfields[path], $pdef) === false) {
	$errors["Path"] = "Invalid Path";
    }
}

#
# OS Features.
# 
# As a side effect of validating, form the os features set as a string
# for the insertion below. 
#
$os_features_array = array();

while (list ($feature, $value) = each($featurelist)) {
    if (isset($formfields["os_feature_$feature"]) &&
	strcmp($formfields["os_feature_$feature"], "checked") == 0) {
	$os_features_array[] = $feature;
    }
}
$os_features = join(",", $os_features_array);

#
# Operational Mode
# 
if (!isset($formfields[op_mode]) ||
    strcmp($formfields[op_mode], "") == 0 ||
    strcmp($formfields[op_mode], "none") == 0) {
    $errors["Op. Mode"] = "Not Selected";
}
elseif (! isset($opmodes[$formfields[op_mode]])) {
    $errors["Op. Mode"] = "Invalid Op. Mode";
}

#
# Node Types:
# See what node types this image will work on. Must be at least one!
# Store the valid types in a new array for simplicity.
#
$mtypes_array = array();

while ($row = mysql_fetch_array($types_result)) {
    $type = $row[type];
    $foo  = $formfields["mtype_$type"];

    #
    # Look for a post variable with name.
    # 
    if (isset($foo) &&
	strcmp($foo, "Yep") == 0) {
	$mtypes_array[] = $type;
    }
}
if (! count($mtypes_array)) {
    $errors["Node Types"] = "Must select at least one type";
}

#
# Node.
#
if (isset($formfields[node]) &&
    strcmp($formfields[node], "")) {

    if (! TBValidNodeName($formfields[node])) {
	$errors["Node"] = "Invalid node name";
    }
    
    if (! TBNodeAccessCheck($uid, $formfields[node],
			    $TB_NODEACCESS_LOADIMAGE)) {
	$errors["Node"] = "Not enough permission";
    }
    $node = $formfields[node];
}

if (isset($formfields[max_concurrent]) &&
    strcmp($formfields[max_concurrent],"")) {
    
    if (!preg_match ("/^\d+$/",$formfields[max_concurrent])) {
    	$errors["Maximum Concurrent Loads"] = "Invalid number";
    }

    $max_concurrent = "'" . $formfields[max_concurrent] . "'";
} else {
    $max_concurrent = "NULL";
}

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
# See if the user is trying anything funky.
# If so, we run this twice.
# The first time we are checking for a confirmation
# by putting up a form (we tramp their settings through
# hidden variables). The next time through the confirmation will be
# set. Or, the user can hit the 'back' button, 
# which will respit the form with their old values still filled in.
#

if ($cancelled) {
    SPITFORM($formfields, 0);    
    PAGEFOOTER();
    return;
}

$confirmationWarning = "";

#
# If user doesn't define a node to suck the image from,
# we seek confirmation.
#
if (! isset($node)) {
    $confirmationWarning .=
          "<h2>You have not defined a node from which to load the image!
           If you do not specify such a node,
           you will have to use <code>create_image</code> on <i>ops</i> to
	   load an image to the Image Descriptor you are creating.
  	   Continue only if this is what you want.</h2>";
}

#
# Generic confirmation-seeker.
#
if (!isset($confirmed) && 0 != strcmp($confirmationWarning,"")) {
    echo "<center><br />$confirmationWarning<br />";
    echo "<form action='newimageid_ez.php3' method=post>";
    #
    # tramp all of their settings along.
    #
    reset($formfields);
    while (list($key, $value) = each($formfields)) {
	echo "<input type=hidden name=\"formfields[$key]\" value=\"$value\"></input>\n";
    }
    echo "<input type=hidden name='submit' value='Submit'>\n";
    echo "<input type=submit name=confirmed value=Confirm>&nbsp;";
    echo "<input type=submit name=cancelled  value=Back>\n";
    echo "</form></center>";

    PAGEFOOTER();
    return;
}


#
# For the rest, sanitize and convert to locals to make life easier.
# 
$description = addslashes($formfields[description]);
$pid         = $formfields[pid];
$gid         = $formfields[gid];
$imagename   = $formfields[imagename];
$bootpart    = $formfields[loadpart];
$path        = $formfields[path];
$os_name     = $formfields[os_name];
$os_version  = $formfields[os_version];
$op_mode     = $formfields[op_mode];
if (!isset($gid) || !strcmp($gid, "")) {
    $gid = $pid;
}

#
# Special option. Whole disk image, but only one partition that actually
# matters. 
#
$loadlen   = 1;
$loadpart  = $bootpart;

if (isset($formfields[wholedisk]) &&
    strcmp($formfields[wholedisk], "Yep") == 0) {
    $loadlen   = 4;
    $loadpart  = 0;
}

DBQueryFatal("lock tables images write, os_info write, osidtoimageid write");

#
# Of course, the Image/OS records may not already exist in the DB.
#
if (TBValidImage($pid, $imagename) || TBValidOS($pid, $imagename)) {
    DBQueryFatal("unlock tables");

    $errors["Descriptor Name"] = "Already in use in selected project";
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Just concat them to form a unique imageid and osid.
# 
$imageid = "$pid-$imagename";
if (TBValidImageID($imageid) || TBValidOSID($imageid)) {
    DBQueryFatal("unlock tables");
    TBERROR("Could not form a unique ID for $pid/$imagename!", 1);
}

DBQueryFatal("INSERT INTO images ".
	     "(imagename, imageid, ezid, description, loadpart, loadlength, ".
	     " part" . "$bootpart" . "_osid, ".
	     " default_osid, path, pid, global, creator, created, ".
	     " max_concurrent, gid, shared) ".
	     "VALUES ".
	     "  ('$imagename', '$imageid', 1, '$description', $loadpart, ".
	     "   $loadlen, '$imageid', '$imageid', '$path', '$pid', $global, ".
             "   '$uid', now(), $max_concurrent, '$gid', $shared)");

DBQueryFatal("INSERT INTO os_info ".
	     "(osname, osid, ezid, description, OS, version, path, magic, ".
	     " osfeatures, pid, creator, shared, created, op_mode) ".
	     "VALUES ".
	     "  ('$imagename', '$imageid', 1, '$description', '$os_name', ".
	     "   '$os_version', NULL, NULL, '$os_features', '$pid', ".
             "   '$uid', $global, now(), '$op_mode')");

for ($i = 0; $i < count($mtypes_array); $i++) {
    DBQueryFatal("REPLACE INTO osidtoimageid ".
		 "(osid, type, imageid) ".
		 "VALUES ('$imageid', '$mtypes_array[$i]', '$imageid')");
}

DBQueryFatal("unlock tables");

SUBPAGESTART();
SUBMENUSTART("More Options");
if (! isset($node)) {
    $fooid = rawurlencode($imageid);
    WRITESUBMENUBUTTON("Edit this Image Descriptor",
		       "editimageid_form.php3?imageid=$fooid");
    WRITESUBMENUBUTTON("Delete this Image Descriptor",
		       "deleteimageid.php3?imageid=$fooid");
}
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_ez.php3");
if ($isadmin) {
    WRITESUBMENUBUTTON("Create a new OS Descriptor",
	  	       "newosid_form.php3");
}
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

#
# Dump os_info record.
# 
SHOWIMAGEID($imageid, 0);
SUBPAGEEND();

if (isset($node)) {
    #
    # Create the image.
    #
    # XXX There is no locking of the descriptor or the node. Not really a
    # problem for the descriptor; the script is done with it by the time
    # it returns. However, if the node is freed up, things are going to go
    # awry. 
    #
    # Grab the unix GID for running script.
    #
    TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

    echo "<br>
          Creating image using node '$node' ...
          <br><br>\n";
    flush();

    SUEXEC($uid, $unix_gid, "webcreateimage -p $pid $imagename $node", 1);

    echo "This will take 10 minutes or more; you will receive email
          notification when the image is complete. In the meantime,
          <b>PLEASE DO NOT</b> delete the imageid or the experiment
          $node is in. In fact, it is best if you do not mess with 
          the node at all!<br>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
