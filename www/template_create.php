<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("template_defs.php");

#
# No PAGEHEADER since we spit out a Location header later. See below.
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
$optargs = OptionalPageArguments("create",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY); 

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $errors)
{
    global $TBDB_PIDLEN, $TBDB_GIDLEN, $TBDB_EIDLEN, $TBDOCBASE;
    global $projlist;
    global $TBVALIDDIRS_HTML;

    PAGEHEADER("Create an Experiment Template");

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

    echo "<form enctype=multipart/form-data
                action=template_create.php method=post>\n";
    echo "<table align=center border=1>\n";

    #
    # Select Project
    #
    echo "<tr>
	  <td class='pad4'>Select Project:</td>
	  <td class='pad4'><select name=\"formfields[pid]\">\n";

    # If just one project, make sure just the one option.
    if (count($projlist) != 1) {
	echo "<option value=''>Please Select &nbsp</option>\n";
    }

    while (list($project) = each($projlist)) {
	$selected = "";

	if (strcmp($formfields["pid"], $project) == 0)
	    $selected = "selected";

	echo "        <option $selected value=\"$project\">
			     $project </option>\n";
    }
    echo "       </select>";
    echo "    </td>
	  </tr>\n";

    #
    # Select a group
    #
    echo "<tr>
	      <td class='pad4'>Group:</td>
	      <td class='pad4'><select name=\"formfields[gid]\">
	          <option value=''>Default Group </option>\n";

    reset($projlist);
    while (list($project, $grouplist) = each($projlist)) {
	for ($i = 0; $i < count($grouplist); $i++) {
	    $group    = $grouplist[$i];

	    if (strcmp($project, $group)) {
		$selected = "";

		if (isset($formfields["gid"]) &&
		    isset($formfields["pid"]) &&
		    strcmp($formfields["pid"], $project) == 0 &&
		    strcmp($formfields["gid"], $group) == 0)
		    $selected = "selected";

		echo "<option $selected value=\"$group\">
			   $project/$group</option>\n";
	    }
	}
    }
    echo "     </select>
	   <font size=-1>(Must be default or correspond to selected project)
	   </font>
	  </td>
	 </tr>\n";

    #
    # TID:
    #
    echo "<tr>
              <td class='pad4'>Template ID:
              <br><font size='-1'>(alphanumeric, no blanks)</font></td>
              <td class='pad4' class=left>
                  <input type=text
                         name=\"formfields[tid]\"
                         value=\"" . $formfields["tid"] . "\"
	                 size=$TBDB_EIDLEN
                         maxlength=$TBDB_EIDLEN>
              </td>
          </tr>\n";

    #
    # NS file
    #
    if (isset($formfields["nsref"])) {
	$nsref = $formfields["nsref"];
	
	if (isset($formfields["guid"])) {
	    $guid = $formfields["guid"];
	    
	    echo "<tr>
                  <td class='pad4'>Your auto-generated NS file: &nbsp</td>
                      <input type=hidden name=\"formfields[nsref]\"
                             value=$nsref>
                      <input type=hidden name=\"formfields[guid]\"
                             value=$guid>
                  <td class='pad4'>
                      <a target=_blank
                             href=\"spitnsdata.php3?nsref=$nsref&guid=$guid\">
                      View NS File</a></td>
                  </tr>\n";
        }
	else {
	    echo "<tr>
                   <td class='pad4'>Your auto-generated NS file: &nbsp</td>
                       <input type=hidden name=\"formfields[nsref]\"
                              value=$nsref>
                   <td class='pad4'>
                       <a target=_blank href=spitnsdata.php3?nsref=$nsref>
                       View NS File</a></td>
                 </tr>\n";
        }
    }
    else {
	echo "<tr>
                  <td class='pad4'>Your NS file: </td>
                  <td><table cellspacing=0 cellpadding=0 border=0>
                    <tr>
                      <td class='pad4'>Upload<br>
			<font size='-1'>(500k&nbsp;max)</font></td>
                      <td class='pad4'>
                        <input type=hidden name=MAX_FILE_SIZE value=512000>
	                <input type=file
                               name=nsfile
                               value=\"" . $formfields["nsfile"] . "\"
	                       size=30>
                      </td>
                    </tr><tr>
                    <td>&nbsp;&nbsp;<b>or</b></td><td></td>
                    </tr><tr>
                      <td class='pad4'>On Server<br>
                              <font size='-1'>($TBVALIDDIRS_HTML)</font></td>
                      <td class='pad4'>
	                <input type=text
                               name=\"formfields[localnsfile]\"
                               value=\"" . $formfields["localnsfile"] . "\"
	                       size=40>
                      </td>
                     </tr>
                     </table>
                   </td>
               </tr>\n";
    }

    echo "<tr>
              <td colspan=2>
               Use this text area to describe your template:
              </td>
          </tr>
          <tr>
              <td colspan=2 align=center class=left>
                  <textarea name=\"formfields[description]\"
                    rows=10 cols=80>" .
	            ereg_replace("\r", "", $formfields["description"]) .
	           "</textarea>
              </td>
          </tr>\n";

    echo "<tr>
              <td class='pad4' align=center colspan=2>
                <b><input type=submit name=create value='Create Template'></b>
              </td>
         </tr>
        </form>
        </table>\n";

    echo "<blockquote><blockquote>
          <ol>
            <li> Please read this
                <a href=kb-show.php3?xref_tag=template_parameters>KB entry</a>
                to see what NS extensions are available for templates.
          </ol>
          </blockquote></blockquote>\n";
}

#
# See what projects the uid can create experiments in. Must be at least one.
#
$projlist = $this_user->ProjectAccessList($TB_PROJECT_CREATEEXPT);

if (! count($projlist)) {
    USERERROR("You do not appear to be a member of any Projects in which ".
	      "you have permission to create new experiment templates.", 1);
}

#
# On first load, display virgin form and exit.
#
if (!isset($create)) {
    $defaults = array();

    $defaults["pid"]               = "";
    $defaults["gid"]               = "";
    $defaults["tid"]               = "";
    $defaults["pid"]               = "";
    $defaults["description"]       = "";
    $defaults["localnsfile"]       = "";
    $defaults["nsfile"]            = ""; # Multipart data.

    #
    # For users that are in one project and one subgroup, it is usually
    # the case that they should use the subgroup, and since they also tend
    # to be in the clueless portion of our users, give them some help.
    #
    if (count($projlist) == 1) {
	list($project, $grouplist) = each($projlist);

	if (count($grouplist) <= 2) {
	    $defaults['pid'] = $project;
	    if (count($grouplist) == 1 || strcmp($project, $grouplist[0]))
		$defaults['gid'] = $grouplist[0];
	    else
		$defaults['gid'] = $grouplist[1];
	}
	reset($projlist);
    }

    #
    # Allow formfields that are already set to override defaults
    #
    if (isset($formfields)) {
	while (list ($field, $value) = each ($formfields)) {
	    $defaults[$field] = $formfields[$field];
	}
    }

    SPITFORM($defaults, 0);
    PAGEFOOTER();
    return;
}

# Make sure we got that formfields array!
if (! isset($formfields)) {
    PAGEARGERROR();
}

#
# Okay, validate form arguments.
#
$errors = array();

# Some local variables.
$nsfilelocale    = 0;
$thensfile       = 0;
$deletensfile    = 0;

#
# Project:
#
if (!isset($formfields["pid"]) || $formfields["pid"] == "") {
    $errors["Project"] = "Not Selected";
}
elseif (!TBvalid_pid($formfields["pid"])) {
    $errors["Project"] = TBFieldErrorString();
}
elseif (! ($project = Project::Lookup($formfields["pid"]))) {
    $errors["Project"] = "No such project";
}
else {
    #
    # Group: If none specified, then use default group (see below).
    #
    if (isset($formfields["gid"]) && $formfields["gid"] != "") {
	if (!TBvalid_gid($formfields["gid"])) {
	    $errors["Group"] = TBFieldErrorString();
	}
	elseif (! ($group = Group::LookupByPidGid($formfields["pid"],
						  $formfields["gid"]))) {
	    $errors["Group"] = "No such group in project'";
	}
    }
    else {
	$group = $project->DefaultGroup();
    }
}

#
# TID
#
if (!isset($formfields["tid"]) || $formfields["tid"] == "") {
    $errors["Template ID"] = "Missing Field";
}
elseif (!TBvalid_eid($formfields["tid"])) {
    $errors["Template ID"] = TBFieldErrorString();
}

#
# Description:
# 
if (!isset($formfields["description"]) || $formfields["description"] == "") {
    $errors["Description"] = "Missing Field";
}
elseif (!TBvalid_template_description($formfields["description"])) {
    $errors["Description"] = TBFieldErrorString();
}

#
# The NS file. There is a bunch of stuff here for Netbuild.
#
$formfields["nsfile"] = "";

if (isset($formfields["guid"])) {
    if ($formfields["guid"] == "" ||
	!preg_match("/^\d+$/", $formfields["guid"])) {
	$errors["NS File GUID"] = "Invalid characters";
    }
    $nsfilelocale = "nsref";
}
elseif (isset($formfields["nsref"])) {
    if ($formfields["nsref"] == "" ||
	!preg_match("/^\d+$/", $formfields["nsref"])) {
	$errors["NS File Reference"] = "Invalid characters";
    }
    $nsfilelocale = "nsref";
}
elseif (isset($formfields["localnsfile"]) &&
	$formfields["localnsfile"] != "") {
    if (!preg_match("/^([-\@\w\.\/]+)$/", $formfields["localnsfile"])) {
	$errors["Server NS File"] = "Pathname includes illegal characters";
    }
    elseif (! VALIDUSERPATH($formfields["localnsfile"])) {
	$errors["Server NS File"] =
		"Must reside in one of: $TBVALIDDIRS";
    }
    $nsfilelocale = "local";
}
elseif (isset($_FILES['nsfile']) && $_FILES['nsfile']['size'] != 0) {
    if ($_FILES['nsfile']['size'] > (1024 * 500)) {
	$errors["Local NS File"] = "Too big!";
    }
    elseif ($_FILES['nsfile']['name'] == "") {
	$errors["Local NS File"] =
	    "Local filename does not appear to be valid";
    }
    elseif ($_FILES['nsfile']['tmp_name'] == "") {
	$errors["Local NS File"] =
	    "Temp filename does not appear to be valid";
    }
    elseif (!preg_match("/^([-\@\w\.\/]+)$/",
			$_FILES['nsfile']['tmp_name'])) {
	$errors["Local NS File"] = "Temp path includes illegal characters";
    }
    #
    # For the benefit of the form. Remember to pass back actual filename, not
    # the php temporary file name. Note that there appears to be some kind
    # of breakage, at least in opera; filename has no path.
    #
    $formfields[nsfile] = $_FILES['nsfile']['name'];
    $nsfilelocale = "remote";
}
else {
    $errors["NS File"] = "You must provide an NS file";
}

if (count($errors)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Okay, we can run the backend script.
#
$description  = escapeshellarg($formfields["description"]);
$pid          = $formfields["pid"];
$gid          = $formfields["gid"];
$tid          = $formfields["tid"];

# Default to pid=gid;
if (!isset($gid) || $gid == "") {
    $gid = $pid;
}

#
# Verify permissions. We do this here since pid/eid/gid could be bogus above.
#
if (! $group->AccessCheck($this_user, $TB_PROJECT_CREATEEXPT)) {
    $errors["Project/Group"] = "Not enough permission to create template";
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    exit(1);
}

#
# Figure out the NS file to give to the script. 
#
if ($nsfilelocale == "local") {
    #
    # No way to tell from here if this file actually exists, since
    # the web server runs as user nobody. The startexp script checks
    # for the file so the user will get immediate feedback if the filename
    # is bogus.
    #
    $thensfile = $formfields["localnsfile"];
}
elseif ($nsfilelocale == "nsref") {
    $nsref = $formfields["nsref"];
    
    if (isset($formfields["guid"])) {
	$guid      = $formfields["guid"];
	$thensfile = "/tmp/$guid-$nsref.nsfile";
    }
    else {
	$thensfile = "/tmp/$uid-$nsref.nsfile";
    }
    if (! file_exists($thensfile)) {
	$errors["NS File"] = "Temp file no longer exists on server";
	SPITFORM($formfields, $errors);
	PAGEFOOTER();
	exit(1);
    }
    $deletensfile = 1;
}
elseif ($nsfilelocale == "remote") {
    #
    # The NS file was uploaded to a temp file.
    #
    $thensfile = $_FILES['nsfile']['tmp_name'];

    if (! file_exists($thensfile)) {
	$errors["NS File"] = "Temp file no longer exists on server";
	SPITFORM($formfields, $errors);
	PAGEFOOTER();
	exit(1);
    }
    chmod($thensfile, 0666);
}

#
# Grab the unix GID for running scripts.
#
$unix_gid = $group->unix_gid();

# Okay, we can spit back a header now that there is no worry of redirect.
PAGEHEADER("Create an Experiment Template");

echo "<script type='text/javascript' language='javascript' ".
     "        src='template_sup.js'>\n";
echo "</script>\n";

STARTBUSY("Starting template creation!");

# And run that script!
$retval = SUEXEC($uid, "$pid,$unix_gid",
		 "webtemplate_create -w -q -E $description ".
		 "-g $gid $pid $tid $thensfile",
		 SUEXEC_ACTION_IGNORE);

if ($deletensfile) {
    unlink($thensfile);
}

/* Clear the various 'loading' indicators. */
STOPBUSY();

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
    return;
}

#
# Parse the last line of output. Ick.
#
if (preg_match("/^Template\s+(\w+)\/(\w+)\s+/",
	       $suexec_output_array[count($suexec_output_array)-1],
	       $matches)) {
    $guid = $matches[1];
    $vers = $matches[2];

    if (($template = Template::Lookup($guid, $vers))) {
	PAGEREPLACE(CreateURL("template_show", $template));
    }
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
