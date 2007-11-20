<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("imageid_defs.php");
include_once("osinfo_defs.php");
include_once("node_defs.php");
include("osiddefs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Create a new Image Descriptor (long form)");

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
$optargs = OptionalPageArguments("submit",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY);

#
# See what projects the uid can do this in.
#
$projlist = $this_user->ProjectAccessList($TB_PROJECT_MAKEIMAGEID);

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
		 "left join node_type_attributes as a on a.type=n.type ".
		 "where a.attrkey='imageable' and ".
		 "      a.attrvalue!='0' and n.role='testnode'");

#
# Spit the form out using the array of data.
#
function SPITFORM($formfields, $errors)
{
    global $this_user, $projlist, $isadmin, $types_result;
    global $TBDB_IMAGEID_IMAGENAMELEN, $TBDB_NODEIDLEN;
    global $TBPROJ_DIR, $TBGROUP_DIR;

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

    #
    # Get the OSID list that the user has access to. Do not allow shared OSIDs,
    # since we want to force mere users to create thier own versions.
    #
    if ($isadmin) {
	$osid_result =
	    DBQueryFatal("select * from os_info ".
			 "where (path='' or path is NULL) and ".
			 "      version!='' and version is not NULL ".
			 "order by pid,osname");
    }
    else {
	$uid_idx = $this_user->uid_idx();
	
	$osid_result =
	    DBQueryFatal("select distinct o.* from os_info as o ".
			 "left join group_membership as m ".
			 " on m.pid=o.pid ".
			 "where m.uid_idx='$uid_idx' and ".
			 "      (path='' or path is NULL) and ".
			 "      version!='' and version is not NULL ".
			 "order by o.pid,o.osname");
    }
    if (! mysql_num_rows($osid_result)) {
	USERERROR("There are no OS Descriptors that you are able to use!", 1);
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
                      theform['formfields[path]'].value = '$TBPROJ_DIR';
                  }
                  else if (theform['formfields[imagename]'].value == '') {
		      theform['formfields[imagename]'].defaultValue = '';

                      if (global) {
    	                  theform['formfields[path]'].value =
                                  '/usr/testbed/images/';
                      }
		      else if (gid == '' || gid == pid || shared) {
    	                  theform['formfields[path]'].value =
                                  '$TBPROJ_DIR/' + pid + '/images/';
                      }
                      else {
    	                  theform['formfields[path]'].value =
                                  '$TBGROUP_DIR/' + pid + '/' + gid + '/images/';
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
                                  '$TBPROJ_DIR/' + pid + '/images/' + filename;
                      }
                      else {
    	                  theform['formfields[path]'].value =
                                  '$TBGROUP_DIR/' + pid + '/' + gid + '/images/' +
                                  filename;
                      }
                  }
              }
          </SCRIPT>\n";

    $url = CreateURL("newimageid");
    echo "<br>
          <table align=center border=1>
          <tr>
             <td align=center colspan=2>
                 <em>(Fields marked with * are required)</em>
             </td>
          </tr>
          <form action='$url' method=post name=idform>\n";

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
             </td>
          </tr>\n";

    #
    # Image Name
    #
    echo "<tr>
              <td>*Descriptor Name (no blanks):</td>
              <td class=left>
                  <input type=text
                         onChange='SetPrefix(idform);'
                         name=\"formfields[imagename]\"
                         value=\"" . $formfields["imagename"] . "\"
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
                         value=\"" . $formfields["description"] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # Load Partition
    #
    echo "<tr>
              <td>*Starting DOS Partition:<br>
                  (1-4, 0 means entire disk)</td>
              <td><select name=\"formfields[loadpart]\">
                          <option value=X>Please Select </option>\n";

    for ($i = 0; $i <= 4; $i++) {
	$selected = "";

	if (strcmp($formfields["loadpart"], "$i") == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=$i>$i </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    #
    # Load length
    #
    echo "<tr>
              <td>*Number of DOS Partitions[<b>1</b>]:<br>
                  (1, 2, 3, or 4)</td>
              <td><select name=\"formfields[loadlength]\">
                          <option value=X>Please Select </option>\n";

    for ($i = 0; $i <= 4; $i++) {
	$selected = "";

	if (strcmp($formfields["loadlength"], "$i") == 0)
	    $selected = "selected";
	
	echo "        <option $selected value=$i>$i </option>\n";
    }
    echo "       </select>";
    echo "    </td>
          </tr>\n";

    WRITEOSIDMENU("Partition 1 OS[<b>2</b>]", "formfields[part1_osid]",
		  $osid_result, $formfields["part1_osid"]);
    WRITEOSIDMENU("Partition 2 OS", "formfields[part2_osid]",
		  $osid_result, $formfields["part2_osid"]);
    WRITEOSIDMENU("Partition 3 OS", "formfields[part3_osid]",
		  $osid_result, $formfields["part3_osid"]);
    WRITEOSIDMENU("Partition 4 OS", "formfields[part4_osid]",
		  $osid_result, $formfields["part4_osid"]);
    WRITEOSIDMENU("Boot OS[<b>3</b>]", "formfields[default_osid]",
		  $osid_result, $formfields["default_osid"]);

    #
    # Path to image.
    #
    echo "<tr>
              <td>Filename (full path) of Image[<b>4</b>]:<br>
                  (must reside in $TBPROJ_DIR)</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[path]\"
                         value=\"" . $formfields["path"] . "\"
	                 size=50>
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
        $type    = $row["type"];
        $checked = "";

        if (isset($formfields["mtype_$type"]) &&
	    $formfields["mtype_$type"] == "Yep") {
	    $checked = "checked";
	}
    
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
              <td>Node to Create Image from[<b>6</b>]:</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[node_id]\"
                         value=\"" . $formfields["node_id"] . "\"
	                 size=$TBDB_NODEIDLEN maxlength=$TBDB_NODEIDLEN>
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

    if (isset($formfields["shared"]) &&
	strcmp($formfields["shared"], "Yep") == 0)
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

        #
        # Create mapping for this image.
        #
	echo "<tr>
  	          <td>Create Mapping?:<br>
                      (insert osid to imageid mappings)</td>
                  <td class=left>
                      <input type=checkbox
                             name=\"formfields[makedefault]\"
                             value=Yep";

	if (isset($formfields["makedefault"]) &&
	    strcmp($formfields["makedefault"], "Yep") == 0)
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
             <li>Only single slices or partial disks are allowed.
  	         If you specify a non-zero starting load partition, the load
	         load length must be one. If you specify zero
	         for the starting load partition, then you can include any or
	         all of the slices (1-4). Note that '0' means to start at the
	         boot sector, while '1' means to start at the beginning of the
	         first partition (typically starting at sector 63).
             <li>If you are using a custom OS, you must
                  <a href='newosid.php3'>create the OS Descriptor
                  first!</a>
             <li>The OS (partition) that is active when the node boots up.
             <li>The image file must reside in the project directory.
             <li>Specify the node types that this image will be able
                  to work on (ie: can be loaded on and expected to work).
                  Typically, images will work on all of the \"pc\" types when
                  you are customizing one of the default images. However,
                  if you are installing your own OS from scratch, or you are
                  using DOS partition four, then this might not be true.
                  Feel free to ask us!
             <li>If you already have a node customized, enter that node
                 name (pcXXX) and the image will be auto created for you.
                 Notification of completion will be sent to you via email. 
          </ol>
          </blockquote></blockquote></blockquote></h4>\n";
}
    
#
# On first load, display a virgin form and exit.
#
if (!isset($submit)) {
    $defaults = array();
    $defaults["pid"]         = "";
    $defaults["gid"]         = "";
    $defaults["imagename"]   = "";
    $defaults["description"] = "";
    $defaults["loadpart"]    = "X";
    $defaults["loadlength"]  = "X";
    $defaults["part1_osid"]  = "";
    $defaults["part2_osid"]  = "";
    $defaults["part3_osid"]  = "";
    $defaults["part4_osid"]  = "";
    $defaults["default_osid"]= "";
    $defaults["path"]        = "$TBPROJ_DIR/";
    $defaults["node_id"]     = "";
    $defaults["shared"]      = "No";
    $defaults["global"]      = "No";
    $defaults["makedefault"] = "No";

    #
    # For users that are in one project and one subgroup, it is usually
    # the case that they should use the subgroup, and since they also tend
    # to be in the naive portion of our users, give them some help.
    # 
    if (count($projlist) == 1) {
	list($project, $grouplist) = each($projlist);

	if (count($grouplist) <= 2) {
	    $defaults["pid"] = $project;
	    if (count($grouplist) == 1 || strcmp($project, $grouplist[0]))
		$group = $grouplist[0];
	    else {
		$group = $grouplist[1];
	    }
	    $defaults["gid"] = $group;
	    
	    if (!strcmp($project, $group))
		$defaults["path"]     = "$TBPROJ_DIR/$project/images/";
	    else
		$defaults["path"]     = "$TBGROUP_DIR/$project/$group/images/";
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
$errors  = array();

# Be friendly about the required form field names.
if (!isset($formfields["pid"]) ||
    strcmp($formfields["pid"], "") == 0) {
    $errors["Project"] = "Missing Field";
}

if (!isset($formfields["imagename"]) ||
    strcmp($formfields["imagename"], "") == 0) {
    $errors["Descriptor Name"] = "Missing Field";
}

if (!isset($formfields["loadpart"]) ||
    strcmp($formfields["loadpart"], "X") == 0) {
    $errors["Starting DOS Partion"] = "Missing Field";
}

if (!isset($formfields["loadlength"]) ||
    strcmp($formfields["loadlength"], "X") == 0) {
    $errors["Number of DOS Partitions"] = "Missing Field";
}

if (!isset($formfields["default_osid"]) ||
    strcmp($formfields["default_osid"], "none") == 0) {
    $errors["Boot OS"] = "Missing Field";
}

#
# Build up argument array to pass along.
#
$args = array();

if (isset($formfields["pid"]) && $formfields["pid"] != "") {
    $args["pid"] = $pid = $formfields["pid"];
}

if (isset($formfields["gid"]) && $formfields["gid"] != "") {
    $args["gid"] = $gid = $formfields["gid"];
}

if (isset($formfields["imagename"]) && $formfields["imagename"] != "") {
    $args["imagename"] = $formfields["imagename"];
}

if (isset($formfields["description"]) && $formfields["description"] != "") {
    $args["description"] = $formfields["description"];
}

if (isset($formfields["loadpart"]) &&
    $formfields["loadpart"] != "X" && $formfields["loadpart"] != "") {
    $args["loadpart"] = $formfields["loadpart"];
}

if (isset($formfields["loadlength"]) &&
    $formfields["loadlength"] != "none" && $formfields["loadlength"] != "") {
    $args["loadlength"] = $formfields["loadlength"];
}

if (isset($formfields["part1_osid"]) &&
    $formfields["part1_osid"] != "none" && $formfields["part1_osid"] != "") {
    $args["part1_osid"] = $formfields["part1_osid"];
}

if (isset($formfields["part2_osid"]) &&
    $formfields["part2_osid"] != "none" && $formfields["part2_osid"] != "") {
    $args["part2_osid"] = $formfields["part2_osid"];
}

if (isset($formfields["part3_osid"]) &&
    $formfields["part3_osid"] != "none" && $formfields["part3_osid"] != "") {
    $args["part3_osid"] = $formfields["part3_osid"];
}

if (isset($formfields["part4_osid"]) &&
    $formfields["part4_osid"] != "none" && $formfields["part4_osid"] != "") {
    $args["part4_osid"] = $formfields["part4_osid"];
}

if (isset($formfields["default_osid"]) &&
    $formfields["default_osid"] != "none" && $formfields["default_osid"] != "") {
    $args["default_osid"] = $formfields["default_osid"];
}

if (isset($formfields["path"]) && $formfields["path"] != "") {
    $args["path"] = $formfields["path"];
}

if (isset($formfields["node_id"]) && $formfields["node_id"] != "") {
    $args["node_id"] = $node_id = $formfields["node_id"];
}

# Filter booleans from checkboxes to 0 or 1.
if (isset($formfields["shared"])) {
   $args["shared"] = strcmp($formfields["shared"], "Yep") ? 0 : 1;
}
if (isset($formfields["global"])) {
   $args["global"] = strcmp($formfields["global"], "Yep") ? 0 : 1;
}
$makedefault = 0;
if (isset($formfields["makedefault"])) {
   $args["makedefault"] = $makedefault = 
       strcmp($formfields["makedefault"], "Yep") ? 0 : 1;
}

#
# See what node types this image will work on. Must be at least one!
# Store the valid types in a new array for simplicity.
#
$mtypes_array = array();
mysql_data_seek($types_result, 0);
while ($row = mysql_fetch_array($types_result)) {
    $type = $row["type"];

    #
    # Look for a post variable with name.
    # 
    if (isset($formfields["mtype_$type"]) &&
	$formfields["mtype_$type"] == "Yep") {
	$mtypes_array[] = $type;
    }
}
if (! count($mtypes_array)) {
    $errors["Node Types"] = "Must select at least one type";
}

# The mtype_* checkboxes are dynamically generated.
foreach ($mtypes_array as $type) {

    # Filter booleans from checkbox values.
    $checked = isset($formfields["mtype_$type"]) &&
	strcmp($formfields["mtype_$type"], "Yep") == 0;
    $args["mtype_$type"] = $checked ? "1" : "0";
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
# Mereusers are not allowed to create more than one osid/imageid mapping
# for each machinetype. They cannot actually do that through the EZ form
# since the osid/imageid has to be unique, but it can happen by mixed
# use of the long form and the short form, or with multiple uses of the
# long form.

# Can't check this unless we have at least one mtype!
if (!count($mtypes_array)) {
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}
    
$typeclause = "type=" . "'$mtypes_array[0]'";
for ($i = 1; $i < count($mtypes_array); $i++) {
    $typeclause = "$typeclause or type=" . "'$mtypes_array[$i]'";
}

unset($osidclause);
for ($i = 1; $i <= 4; $i++) {
    # Local variable dynamically created.    
    $foo      = "part${i}_osid";

    if (isset($formfields[$foo])) {
	if (isset($osidclause))
	    $osidclause = "$osidclause or osid='" . $formfields[$foo] . "' ";
	else 
	    $osidclause = "osid='" . $formfields[$foo] . "' ";

	$osid_array[] = $formfields[$foo];
    }
}
    
DBQueryFatal("lock tables images write, os_info write, osidtoimageid write");
$query_result =
    DBQueryFatal("select osidtoimageid.*,images.pid,images.imagename ".
		 " from osidtoimageid ".
		 "left join images on ".
		 " images.imageid=osidtoimageid.imageid ".
		 "where ($osidclause) and ($typeclause)");
DBQueryFatal("unlock tables");

if (mysql_num_rows($query_result)) {
    if (!$isadmin || $makedefault) {
	DBQueryFatal("unlock tables");

	echo "<center>
              There are other image descriptors that specify the 
	      same OS descriptors for the same node types.<br>
              There must be a
	      unique mapping of OS descriptor to Image descriptor for
              each node type! Perhaps you need to delete one of the
              images below, or create a new OS descriptor to use in
              this new Image descriptor.
              </center><br>\n";

	echo "<table border=1 cellpadding=2 cellspacing=2 align='center'>\n";

	echo "<tr>
                  <td align=center>OS ID/name</td>
                  <td align=center>Node Type</td>
                  <td align=center>Image PID/ID/name</td>
             </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $osid      = $row["osid"];
            $osinfo    = OSinfo::Lookup($osid);
            $osname    = $osinfo->osname();
	    $type      = $row["type"];
	    $imageid   = $row['imageid'];
	    $url       = CreateURL("showimageid", URLARG_IMAGEID, $imageid);
	    $pid       = $row['pid'];
	    $imagename = $row["imagename"];
	    
	    echo "<tr>
                      <td>$osid/$osname</td>
	              <td>$type</td>
                      <td>$pid/$imageid/<A href='$url'>$imagename</A></td>
	          </tr>\n";
	}
	echo "</table><br><br>\n";
    
	USERERROR("Please check the other Image descriptors and make the ".
		  "necessary changes!", 1);
    }
}

# Send to the backend for more checking, and eventually, to update the DB.
$imagename = $args["imagename"];
if (! ($image = Image::NewImageId(0, $imagename, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($formfields, $errors);
    PAGEFOOTER();
    return;
}

$pid = $image->pid();
$gid_idx = $image->gid_idx();
$group = Group::Lookup($gid_idx);

SUBPAGESTART();
SUBMENUSTART("More Options");
if (! isset($node_id)) {
    $imageid = $image->imageid();
    $fooid = rawurlencode($imageid);
    WRITESUBMENUBUTTON("Edit this Image Descriptor",
		       "editimageid.php3?imageid=$fooid");
    WRITESUBMENUBUTTON("Delete this Image Descriptor",
		       "deleteimageid.php3?imageid=$fooid");
}
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_ez.php3");
WRITESUBMENUBUTTON("Create a new OS Descriptor",
		   "newosid.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

#
# Dump os_info record.
#
$image->Show();
SUBPAGEEND();

if (isset($node_id)) {
    #
    # Create the image.
    #
    # XXX There is no locking of the descriptor or the node. Not really a
    # problem for the descriptor; the script is done with it by the time
    # it returns. However, if the node is freed up, things are going to go
    # awry. 
    #

    $node = Node::Lookup($node_id); # Already been checked.
    $node_id = $node->node_id();    # XXX Why?

    #
    # Grab the unix GID for running script.
    #
    $unix_gid  = $group->unix_gid();
    $safe_name = escapeshellarg($imagename);

    echo "<br>
          Creating image using node '$node_id'.
          <br><br>\n";
    flush();

    SUEXEC($uid, "$pid,$unix_gid",
	   "webcreate_image -p $pid $safe_name $node_id",
	   SUEXEC_ACTION_DUPDIE);

    echo "This will take 10 minutes or more; you will receive email
          notification when the image is complete. In the meantime,
          <b>PLEASE DO NOT</b> delete the imageid or the experiment
          $node_id is in. In fact, it is best if you do not mess with 
          the node or the experiment at all until you receive email.<br>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
