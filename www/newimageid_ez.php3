<?php
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

#
# Default OS strings. Needs to move someplace else!
#
$oslist            = array();
$oslist["Linux"]   = "Linux";
$oslist["FreeBSD"] = "FreeBSD";
$oslist["NetBSD"]  = "NetBSD";
$oslist["Other"]   = "Other";

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
    global $uid, $projlist, $isadmin, $types_result, $oslist, $featurelist;
    global $TBDB_IMAGEID_IMAGENAMELEN, $TBDB_NODEIDLEN;
    global $TBDB_OSID_VERSLEN;
    
    if ($errors) {
	echo "<table align=center border=0 cellpadding=0 cellspacing=2>
              <tr>
                 <td align=center colspan=3>
                   <font size=+1 color=red>
                      Oops, please fix the following errors!
                   </font>
                 </td>
              </tr>\n";

	while (list ($name, $message) = each ($errors)) {
	    echo "<tr>
                     <td align=right><font color=red>$name:</font></td>
                     <td>&nbsp &nbsp</td>
                     <td align=left><font color=red>$message</font></td>
                  </tr>\n";
	}
	echo "</table><br>\n";
    }

    echo "<SCRIPT LANGUAGE=JavaScript>
              function SetPrefix(theform) 
              {
                  var idx = theform['formfields[pid]'].selectedIndex;
                  var pid = theform['formfields[pid]'].options[idx].value;

                  if (pid == '') {
                      theform['formfields[path]'].value = '/proj/';
                  }
                  else if (theform['formfields[imagename]'].value == '') {
		      theform['formfields[imagename]'].defaultValue = '';
  	              theform['formfields[path]'].value =
                              '/proj/' + pid + '/images/';
                  }
                  else if (theform['formfields[imagename]'].value != '') {
  	              theform['formfields[path]'].value =
                              '/proj/' + pid + '/images/' +
                              theform['formfields[imagename]'].value + '.ndz';
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
    # OS Features.
    # 
    echo "<tr>
              <td>OS Features:</td>
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
    # Node Types.
    #
    echo "<tr>
              <td>Node Types[<b>2</b>]:</td>
              <td>\n";

    mysql_data_seek($types_result, 0);
    while ($row = mysql_fetch_array($types_result)) {
        $type    = $row[type];
        $checked = "";

        if (strcmp($formfields["mtype_$type"], "Yep") == 0)
	    $checked = "checked";
    
        echo "<input $checked type=checkbox
                     value=Yep name=\"formfields[mtype_$type]\">
                     $type &nbsp
              </input>\n";
    }
    echo "    </td>
          </tr>\n";

    #
    # Node to Create image from.
    #
    echo "<tr>
              <td>Node to Create Image from[<b>3</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node]\"
                         value=\"" . $formfields[node] . "\"
	                 size=$TBDB_NODEIDLEN maxlength=$TBDB_NODEIDLEN>
              </td>
          </tr>\n";

    if ($isadmin) {
        #
        # Shared?
        #
	echo "<tr>
  	          <td>Shared?:<br>
                      (available to all projects)</td>
                  <td class=left>
                      <input type=checkbox
                             name=\"formfields[shared]\"
                             value=Yep";

	if (isset($formfields[shared]) &&
	    strcmp($formfields[shared], "Yep") == 0)
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

    echo "<h4><blockquote><blockquote><blockquote>
          <ol type=1 start=1>
             <li> If you don't know what partition you have customized,
    	          here are some guidelines:
  	         <ul>
 		    <li> if you customized one of our standard Linux
		         images (RHL-*) then it is partition 2.

                    <li> if you customized one of our standard BSD
 		         images (FBSD-*) then it is partition 1.

                    <li> otherwise, feel free to ask us!
                 </ul>
             <li> Specify the node types that this image will be able
                  to work on (can be loaded on and expected to work).
                  Typically, images will work on all of the \"pc\" types when
                  you are customizing one of the standard images. However,
                  if you are installing your own OS from scratch, or you are
                  using DOS partition four, then this might not be true.
                  Feel free to ask us!
             <li> If you already have a node customized, enter that node
                  name (pcXXX) and the image will be auto created for you.
                  Notification of completion will be sent to you via email. 
          </ol>
          </blockquote></blockquote></blockquote></h4>\n";
}

#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    $defaults = array();
    $defaults[loadpart] = "X";
    $defaults[path]     = "/proj/";
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
elseif (!TBProjAccessCheck($uid, $formfields[pid],
			   $formfields[pid], $TB_PROJECT_MAKEIMAGEID)) {
    $errors["Project"] = "No enough permission";
}

#
# Image Name:
# 
if (!isset($formfields[imagename]) ||
    strcmp($formfields[imagename], "") == 0) {
    $errors["Descriptor Name"] = "Missing Field";
}
else {
    if (! ereg("^[a-zA-Z0-9][-_a-zA-Z0-9\.]+$", $formfields[imagename])) {
	$errors["Descriptor Name"] =
	    "Must be alphanumeric (includes _ and - and .)<br>".
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
# The path must not contain illegal chars and it must be more than
# the original /proj/$pid we gave the user. We allow admins to specify
# a path outside of /proj though.
# 
if (!isset($formfields[path]) ||
    strcmp($formfields[path], "") == 0) {
    $errors["Path"] = "Missing Field";
}
elseif (! ereg("^[-_a-zA-Z0-9\/\.+]+$", $formfields[path])) {
    $errors["Path"] = "Contains invalid characters";
}
else {
    $pdef = "/proj/$formfields[pid]/";

    if (strcmp($formfields[path], "$pdef") == 0) {
	$errors["Path"] = "Incomplete Path";
    }
    elseif (!$isadmin &&
	    strcmp(substr($formfields[path], 0, strlen($pdef)), $pdef)) {
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
# Only admin types can set the shared bit for an image. Ignore silently.
#
$shared = 0;

if ($isadmin &&
    isset($formfields[shared]) &&
    strcmp($formfields[shared], "Yep") == 0) {
    $shared = 1;
}

#
# For the rest, sanitize and convert to locals to make life easier.
# 
$description = addslashes($formfields[description]);
$pid         = $formfields[pid];
$imagename   = $formfields[imagename];
$loadpart    = $formfields[loadpart];
$path        = $formfields[path];
$os_name     = $formfields[os_name];
$os_version  = $formfields[os_version];

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
	     " part" . "$loadpart" . "_osid, ".
	     " default_osid, path, pid, shared, creator, created) ".
	     "VALUES ".
	     "  ('$imagename', '$imageid', 1, '$description', $loadpart, 1, ".
	     "   '$imageid', '$imageid', '$path', '$pid', $shared, ".
             "   '$uid', now())");

DBQueryFatal("INSERT INTO os_info ".
	     "(osname, osid, ezid, description, OS, version, path, magic, ".
	     " osfeatures, pid, creator, created) ".
	     "VALUES ".
	     "  ('$imagename', '$imageid', 1, '$description', '$os_name', ".
	     "   '$os_version', NULL, NULL, '$os_features', '$pid', ".
             "   '$uid', now())");

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
		   "newimageid_explain.php3");
WRITESUBMENUBUTTON("Create a new OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("Back to Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("Back to OS Descriptor list",
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
    TBGroupUnixInfo($pid, $pid, $unix_gid, $unix_name);

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
