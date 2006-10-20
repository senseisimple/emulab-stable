<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");
include("osiddefs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Edit Image Descriptor");

#
# Only known and logged in users!
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Must get the imageid as a page argument.
#
if (!isset($imageid) || $imageid == "") {
    PAGEARGERROR("Must supply an imageid!");
}
if (!TBvalid_imageid($imageid)) {
    PAGEARGERROR("Invalid characters in imageid!");
}
if (!TBValidImageID($imageid)) {
    PAGEARGERROR("No such imageid!");
}
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_MODIFYINFO)) {
    USERERROR("You do not have permission to access ImageID $imageid!", 1);
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
		 "      a.attrvalue!='0'");

#
# Spit the form out using the array of data. 
# 
function SPITFORM($imageid, $formfields, $errors)
{
    global $uid, $isadmin, $types_result;
    global $TBDB_IMAGEID_IMAGENAMELEN, $TBDB_NODEIDLEN;
    
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

    # Must encode the imageid since Rob started using plus signs in
    # the names.
    $url = rawurlencode($imageid);
    echo "<br>
          <table align=center border=1> 
          <form action='editimageid.php3?imageid=$url'
                method=post name=idform>\n";

    #
    # Imagename
    #
    echo "<tr>
              <td>ImageID:</td>
              <td class=left>" . $formfields[imagename] . "</td>
          </tr>\n";

    #
    # Project
    #
    echo "<tr>
              <td>Project:</td>
              <td class=left>" . $formfields[pid] . "</td>
          </tr>\n";

    #
    # Group
    # 
    echo "<tr>
              <td>Group:</td>
              <td class=left>" . $formfields[gid] . "</td>
          </tr>\n";

    #
    # Image Name:
    #
    echo "<tr>
              <td>Descriptor Name:</td>
              <td class=left>" . $formfields[imagename] . "</td>
          </tr>\n";

    #
    # Description
    #
    echo "<tr>
              <td>Description:</td>
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
              <td>Load Partition:</td>
              <td class=left>" . $formfields[loadpart] . "</td>
          </tr>\n";

    #
    # Load Length
    #
    echo "<tr>
              <td>Load Partition:</td>
              <td class=left>" . $formfields[loadlength] . "</td>
          </tr>\n";

    echo "<tr>
             <td>Partition 1 OS: </td>
             <td class=\"left\">";
    if (isset($formfields[part1_osid]))
	SPITOSINFOLINK($formfields[part1_osid]);
    else
	echo "No OS";
    echo "   </td>
          </tr>\n";
    
    echo "<tr>
             <td>Partition 2 OS: </td>
             <td class=\"left\">";
    if (isset($formfields[part2_osid]))
	SPITOSINFOLINK($formfields[part2_osid]);
    else
	echo "No OS";
    echo "   </td>
          </tr>\n";

    echo "<tr>
             <td>Partition 3 OS: </td>
             <td class=\"left\">";
    if (isset($formfields[part3_osid]))
	SPITOSINFOLINK($formfields[part3_osid]);
    else
	echo "No OS";
    echo "   </td>
          </tr>\n";

    echo "<tr>
             <td>Partition 4 OS: </td>
             <td class=\"left\">";
    if (isset($formfields[part4_osid]))
	SPITOSINFOLINK($formfields[part4_osid]);
    else
	echo "No OS";
    echo "   </td>
          </tr>\n";
    
    echo "<tr>
             <td>Boot OS: </td>
             <td class=\"left\">";
    if (isset($formfields[default_osid]))
	SPITOSINFOLINK($formfields[default_osid]);
    else
	echo "No OS";
    echo "   </td>
          </tr>\n";
    
    #
    # Path to image.
    #
    echo "<tr>
              <td>Filename</td>
              <td class=left>
                  <input type=text
                         name=\"formfields[path]\"
                         value=\"" . $formfields[path] . "\"
	                 size=50>
              </td>
          </tr>\n";

    #
    # Node Types.
    #
    echo "<tr>
              <td>Node Types:</td>
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
    # Shared?
    #
    echo "<tr>
  	      <td>Shared?:</td>
              <td class=left>" . ($formfields[shared] ? "Yes" : "No") . "</td>
          </tr>\n";

    #
    # Global?
    #
    echo "<tr>
  	      <td>Global?:</td>
              <td class=left>" . ($formfields[shared] ? "Yes" : "No") . "</td>
          </tr>\n";

    echo "<tr>
            <td>Load Address: </td>
            <td class=left>\n";

    if ($isadmin) {
	echo "  <input type=text
                       name=\"formfields[load_address]\"
                       value=\"" . $formfields[load_address] . "\"
	               size=20 maxlength=256>";
    }
    else {
	echo $formfields[load_address];
    }
    echo "  </td>
          </tr>\n";

    echo "<tr>
            <td>Frisbee pid: </td>
            <td class=left>\n";

    if ($isadmin) {
	echo "  <input type=text
                       name=\"formfields[frisbee_pid]\"
                       value=\"" . $formfields[frisbee_pid] . "\"
	               size=6 maxlength=10>";
    }
    else {
	echo $formfields[frisbee_pid];
    }
    echo "  </td>
          </tr>\n";

    echo "<tr>
              <td align=center colspan=2>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";
}

# Need this below.
$query_result =
   DBQueryFatal("select * from images where imageid='$imageid'");
$defaults = mysql_fetch_array($query_result);
   
#
# On first load, display a virgin form and exit.
#
if (! $submit) {
    # Generate the current types array.
    $image_types =
	DBQueryFatal("select type from osidtoimageid ".
		     "where imageid='$imageid'");
    while ($row = mysql_fetch_array($image_types)) {
	$type = $row['type'];

	$defaults["mtype_${type}"] = "Yep";
    }

    SPITFORM($imageid, $defaults, 0);
    PAGEFOOTER();
    return;
}

#
# Otherwise, must validate and redisplay if errors
#
$errors     = array();
$updates    = array();
$osid_array = array();

if (!isset($formfields[description]) ||
    strcmp($formfields[description], "") == 0) {
    $errors["Description"] = "Missing Field";
}
elseif (! TBvalid_description($formfields[description])) {
    $errors["Description"] = TBFieldErrorString();
}
else {
    $updates[] = "description='" . addslashes($formfields[description]) . "'";
}

if (!isset($formfields[path]) ||
    strcmp($formfields[path], "") == 0) {
    $errors["Path"] = "Missing Field";
}
elseif (! ereg("^[-_a-zA-Z0-9\/\.+]+$", $formfields[path])) {
    $errors["Path"] = "Contains invalid characters";
}
elseif ($isadmin) {
    $updates[] = "path='" . $formfields[path] . "'";
}    
else {
    $pdef    = "";
    $shared = $defaults["shared"];
    $pid    = $defaults["pid"];
    $gid    = $defaults["gid"];
	
    if (!$shared && strcmp($gid, $pid)) {
	$pdef = "$TBGROUP_DIR/" . $pid . "/" . $gid . "/";
    }
    else {
	$pdef = "$TBPROJ_DIR/" . $pid . "/";
    }

    if (strpos($formfields[path], $pdef) === false) {
	$errors["Path"] = "Must reside in $pdef";
    }
    $updates[] = "path='" . $formfields[path] . "'";
}

#
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
# Only admins can edit the load_address or the frisbee pid.
# 
if ($isadmin) {
    if (isset($formfields[load_address]) && $formfields[load_address] != "") {
	$foo = addslashes($formfields[load_address]);

	if (strcmp($formfields[load_address], $foo)) {
	    $errors["Load Address"] = "Contains	illegal characters!";
	}
	$updates[] = "load_address='$foo'";
    }
    else {
	$updates[] = "load_address=NULL";
    }

    if (isset($formfields[frisbee_pid]) && $formfields[frisbee_pid] != "") {
	if (! TBvalid_integer($formfields[frisbee_pid])) {
	    $errors["Frisbee PID"] = "Must must be a valid integer!";
	}
	$updates[] = "frisbee_pid='" . $formfields[frisbee_pid] . "'";
    }
    else {
	$updates[] = "frisbee_pid=NULL";
    }
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($imageid, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Mereusers are not allowed to create more than one osid/imageid mapping
# for each machinetype. They cannot actually do that through the EZ form
# since the osid/imageid has to be unique, but it can happen by mixed
# use of the long form and the short form, or with multiple uses of the
# long form. 
#
$typeclause = "type=" . "'$mtypes_array[0]'";

for ($i = 1; $i < count($mtypes_array); $i++) {
    $typeclause = "$typeclause or type=" . "'$mtypes_array[$i]'";
}

unset($osidclause);

for ($i = 1; $i <= 4; $i++) {
    # Local variable dynamically created.    
    $foo      = "part${i}_osid";

    if (isset($defaults[$foo])) {
	if (isset($osidclause))
	    $osidclause = "$osidclause or osid='" . $defaults[$foo] . "' ";
	else 
	    $osidclause = "osid='" . $defaults[$foo] . "' ";

	$osid_array[] = $defaults[$foo];
    }
}
    
DBQueryFatal("lock tables images write, os_info write, osidtoimageid write");

$query_result =
    DBQueryFatal("select osidtoimageid.*,images.pid,images.imagename ".
		 " from osidtoimageid ".
		 "left join images on ".
		 " images.imageid=osidtoimageid.imageid ".
		 "where ($osidclause) and ($typeclause) and ".
		 "      images.imageid!='$imageid'");

if (mysql_num_rows($query_result)) {
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
                  <td align=center>OSID</td>
                  <td align=center>Type</td>
                  <td align=center>ImageID</td>
             </tr>\n";

	while ($row = mysql_fetch_array($query_result)) {
	    $imageid   = $row['imageid'];
	    $url       = rawurlencode($imageid);
	    $osid      = $row[osid];
	    $type      = $row[type];
	    $imagename = $row[imagename];
	    
	    echo "<tr>
                      <td>$osid</td>
	              <td>$type</td>
                      <td><A href='showimageid.php3?&imageid=$url'>
                             $imagename</A></td>
	          </tr>\n";
	}
	echo "</table><br><br>\n";
    
	USERERROR("Please check the other Image descriptors and make the ".
		  "necessary changes!", 1);
}

#
# Update the images table.
# 
DBQueryFatal("update images set ".
	     implode(",", $updates) . " ".
	     "where imageid='$imageid'");

#
# And the osidtoimageid table.
# 
# Must delete old entries first.
DBQueryFatal("delete from osidtoimageid ".
	     "where imageid='$imageid'");
    
for ($i = 0; $i < count($mtypes_array); $i++) {
    for ($j = 0; $j < count($osid_array); $j++) {
	DBQueryFatal("REPLACE INTO osidtoimageid ".
		     "(osid, type, imageid) ".
		     "VALUES ('$osid_array[$j]', '$mtypes_array[$i]', ".
		     "        '$imageid')");
    }
}
DBQueryFatal("unlock tables");

SUBPAGESTART();
SUBMENUSTART("More Options");
$fooid = rawurlencode($imageid);
WRITESUBMENUBUTTON("Edit this Image Descriptor",
		   "editimageid.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Delete this Image Descriptor",
		   "deleteimageid.php3?imageid=$fooid");
WRITESUBMENUBUTTON("Create a new Image Descriptor",
		   "newimageid_ez.php3");
WRITESUBMENUBUTTON("Create a new OS Descriptor",
		   "newosid_form.php3");
WRITESUBMENUBUTTON("Image Descriptor list",
		   "showimageid_list.php3");
WRITESUBMENUBUTTON("OS Descriptor list",
		   "showosid_list.php3");
SUBMENUEND();

#
# Dump record.
# 
SHOWIMAGEID($imageid, 0);
SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
