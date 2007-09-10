<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("osinfo_defs.php");
include("osiddefs.php3");

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

#
# Spit the form out using the array of data. 
# 
function SPITFORM($formfields, $errors)
{
    global $this_user, $projlist, $isadmin;
    global $osid_opmodes, $osid_oslist, $osid_featurelist;
    global $TBDB_OSID_OSNAMELEN, $TBDB_OSID_OSNAMELEN;
    global $TBDB_OSID_VERSLEN, $TBDB_OSID_VERSLEN;

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

    echo "<br>
          <table align=center border=1> 
          <tr>
             <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='newosid.php3' method=post name=idform>\n";

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

	if ($formfields["pid"] == $project)
	    $selected = "selected";
	
	echo "        <option $selected value='$project'>$project </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # OS Name
    #
    echo "<tr>
              <td>*Descriptor Name (no blanks):</td>
              <td><input type=text
                         name=\"formfields[osname]\"
                         value=\"" . $formfields["osname"] . "\"
                         size=$TBDB_OSID_OSNAMELEN
                         maxlength=$TBDB_OSID_OSNAMELEN>
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
                         value=\"" . $formfields["description"] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # Select an OS
    # 
    echo "<tr>
              <td>*Select OS:</td>
              <td><select name=\"formfields[OS]\">\n";

    while (list ($os, $userokay) = each($osid_oslist)) {
        if (!$userokay && !$isadmin)
	    continue;

	$selected = "";
	if ($formfields["OS"] == $os)
	    $selected = "selected";

	echo "<option $selected value=$os>$os &nbsp; </option>\n";
    }
    echo "       </select>
              </td>
          </tr>\n";

    #
    # Version String
    #
    echo "<tr>
              <td>*Version:</td>
              <td><input type=text
                         name=\"formfields[version]\"
                         value=\"" . $formfields["version"] . "\"
                         size=$TBDB_OSID_VERSLEN maxlength=$TBDB_OSID_VERSLEN>
              </td>
          </tr>\n";

    #
    # Path to Multiboot image.
    #
    echo "<tr>
              <td>Path:</td>
              <td><input type=text
                         name=\"formfields[path]\"
                         value=\"" . $formfields["path"] . "\"
                         size=40>
              </td>
          </tr>\n";

    #
    # Magic string?
    #
    echo "<tr>
              <td>Magic (ie: uname -r -s):</td>
              <td><input type=text
                         name=\"formfields[magic]\"
                         value=\"" . $formfields["magic"] . "\"
                         size=30>
              </td>
          </tr>\n";

    echo "<tr>
              <td>OS Features:</td>
              <td>";

    reset($osid_featurelist);
    while (list ($feature, $userokay) = each($osid_featurelist)) {
	if (!$userokay && !$isadmin)
	    continue;

	$checked = "";
	    
	if (isset($formfields["os_feature_$feature"]) &&
	    ! strcmp($formfields["os_feature_$feature"], "checked"))
	    $checked = "checked";

	echo "<input $checked type=checkbox value=checked
	 	 name=\"formfields[os_feature_$feature]\">$feature &nbsp\n";
    }
    echo "<p>Guidelines for setting os_features for your OS:
              <ol>
                <li> Mark ping and/or ssh if they are supported.
                <li> If you use a testbed kernel, or are based on a
                     testbed kernel config, mark the ipod box.
                <li> If it is based on a testbed image or sends its own
                     isup, mark isup. 
              </ol>
            </td>
         </tr>\n";

    
    #
    # Op Mode
    #
    echo "<tr>
	      <td>*Operational Mode[<b>4</b>]:</td>
	      <td><select name=\"formfields[op_mode]\">
		         <option value=none>Please Select </option>\n";

    while (list ($mode, $userokay) = each($osid_opmodes)) {
	$selected = "";

	if (!$userokay && !$isadmin)
	    continue;

	if (isset($formfields["op_mode"]) &&
	    strcmp($formfields["op_mode"], $mode) == 0)
	    $selected = "selected";

	echo "<option $selected value=$mode>$mode &nbsp; </option>\n";
    }
    echo "       </select>
             <p>
              Guidelines for setting op_mode for your OS:
              <ol>
                <li> If it is based on a testbed image (one of our
                     Linux, Fedora, FreeBSD or Windows images) use the same
                     op_mode as that image. Select it from the
                     <a href=\"$TBBASE/showosid_list.php3\"
                     >OS Descriptor List</a> to find out).
                <li> If not, use MINIMAL.
              </ol>
	     </td>
	  </tr>\n";

    if ($isadmin) {
        #
        # Shared?
        #
	echo "<tr>
	          <td>Global?:<br>
                      (available to all projects)</td>
                  <td><input type=checkbox
			     name=\"formfields[shared]\"
			     value=Yep";
	
	if (isset($formfields["shared"]) &&
	    strcmp($formfields["shared"], "Yep") == 0)
	    echo "           checked";
	    
	echo "                       > Yes
		  </td>
	      </tr>\n";

        #
        # Mustclean?
        #
	echo "<tr>
	          <td>Clean?:<br>
                  (no disk reload required)</td>
                  <td><input type=checkbox
			     name=\"formfields[mustclean]\"
			     value=Yep";
	
	if (isset($formfields["mustclean"]) &&
	    strcmp($formfields["mustclean"], "Yep") == 0)
	    echo "           checked";
	    
	echo "                       > Yes
		  </td>
	      </tr>\n";
	
        #
        # Reboot Waittime. 
        #
	echo "<tr>
	          <td>Reboot Waittime (seconds)</td>
                  <td><input type=text
                             name=\"formfields[reboot_waittime]\"
                             value=\"" . $formfields["reboot_waittime"] ."\"
                             size=6>
                  </td>
              </tr>\n";
    
	$osid_result =
	    DBQueryFatal("select * from os_info ".
			 "where (path='' or path is NULL) and ".
			 "      version!='' and version is not NULL ".
			 "order by pid,osname");
    
	WRITEOSIDMENU("NextOsid", "formfields[nextosid]", $osid_result,
		      $formfields["nextosid"]);
    }

    echo "<tr>
              <td align=center colspan=2>
                  <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";
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
    $defaults["shared"]         = "No";
    $defaults["mustclean"]      = "No";
    $defaults["path"]           = "";
    $defaults["op_mode"]             = TBDB_DEFAULT_OSID_OPMODE;
    $defaults["os_feature_ping"]     = "checked";
    $defaults["os_feature_ssh"]      = "checked";
    $defaults["os_feature_ipod"]     = "checked";
    $defaults["os_feature_isup"]     = "checked";
    $defaults["os_feature_linktest"] = "checked";
    $defaults["reboot_waittime"]     = "";
    $defaults["nextosid"]            = "none";

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

#
# Otherwise, must validate and redisplay if errors
#
$errors  = array();
$project = null;
$osname  = "";

#
# Project:
#
if (!isset($formfields["pid"]) || $formfields["pid"] == "") {
    $errors["Project"] = "Not Selected";
}
elseif (!TBvalid_pid($formfields["pid"])) {
    $errors["Project"] = "Invalid project name";
}
elseif (! ($project = Project::Lookup($formfields["pid"]))) {
    $errors["Project"] = "Invalid project name";
}
elseif (!$project->AccessCheck($this_user, $TB_PROJECT_MAKEOSID)) {
    $errors["Project"] = "Not enough permission";    
}
# Osname obviously needs to exist.
if (! isset($formfields["osname"]) || $formfields["osname"] == "") {
    $errors["Descriptor Name"] = "Required value not provided";
}
else {
    $osname = $formfields["osname"];
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
# Build up argument array to pass along.
#
$args = array();

if (isset($formfields["OS"]) &&
    $formfields["OS"] != "none" && $formfields["OS"] != "") {
    $args["OS"]		= $formfields["OS"];
}
if (isset($formfields["path"]) && $formfields["path"] != "") {
    $args["path"]	= $formfields["path"];
}
if (isset($formfields["version"]) && $formfields["version"] != "") {
    $args["version"]	= $formfields["version"];
}
if (isset($formfields["description"]) && $formfields["description"] != "") {
    $args["description"]= $formfields["description"];
}
if (isset($formfields["magic"]) && $formfields["magic"] != "") {
    $args["magic"]	= $formfields["magic"];
}
if (isset($formfields["shared"]) && $formfields["shared"] == "Yep") {
    $args["shared"]	= 1;
}
if (isset($formfields["mustclean"]) && $formfields["mustclean"] == "Yep") {
    $args["mustclean"]	= 1;
}
if (isset($formfields["op_mode"]) &&
    $formfields["op_mode"] != "none" && $formfields["op_mode"] != "") {
    $args["op_mode"]	= $formfields["op_mode"];
}
if (isset($formfields["nextosid"]) &&
    $formfields["nextosid"] != "" && $formfields["nextosid"] != "none") {
    $args["nextosid"] = $formfields["nextosid"];
}
if (isset($formfields["reboot_waittime"]) &&
    $formfields["reboot_waittime"] != "") {
    $args["reboot_waittime"] = $formfields["reboot_waittime"];
}
#
# Form comma separated list of osfeatures.
#
$os_features_array = array();

while (list ($feature, $userokay) = each($osid_featurelist)) {
    if (isset($formfields["os_feature_$feature"]) &&
	$formfields["os_feature_$feature"] == "checked") {
	$os_features_array[] = $feature;
    }
}
$args["features"] = join(",", $os_features_array);

if (! ($osinfo = OSinfo::NewOSID($this_user, $project,
				 $osname, $args, $errors))) {
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
