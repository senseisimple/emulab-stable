<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2008 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("imageid_defs.php");
include_once("osinfo_defs.php");
include("osiddefs.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
#

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

if (! $isadmin) {
    USERERROR("You do not have permission to edit node types!", 1);
}

$optargs = OptionalPageArguments("submit",     PAGEARG_STRING,
				 "formfields", PAGEARG_ARRAY,

				 # Send new_type=1 to create new nodetype.
				 "new_type",   PAGEARG_STRING,
				 # Optional if new_type, required if not.
				 "node_type",  PAGEARG_STRING,

				 # Attribute creation and deletion.
				 "deletes",    PAGEARG_ARRAY,
				 "attributes", PAGEARG_ARRAY,
				 "newattribute_type",  PAGEARG_STRING,
				 "newattribute_name",  PAGEARG_STRING,
				 "newattribute_value", PAGEARG_ANYTHING);
if (!isset($node_type)) { $node_type = ""; }
if (!isset($attributes)) { $attributes = array(); }
if (!isset($deletes)) { $deletes = array(); }

$emulab_ops = Project::LookupByPid("emulab-ops");
$freebsd_mfs = OSinfo::LookupByName($emulab_ops,"FREEBSD-MFS");
$rhl_std = OSinfo::LookupByName($emulab_ops, "RHL-STD");
$fbsd_std = OSinfo::LookupByName($emulab_ops,"FBSD-STD");
$frisbee_mfs = OSinfo::LookupByName($emulab_ops,"FRISBEE-MFS");

# The default image comes from a site variable to avoid hardwiring here.
$default_imagename = TBGetSiteVar("general/default_imagename");
$default_image = Image::LookupByName($emulab_ops, $default_imagename);

if ($freebsd_mfs == null || $default_image == null ||
    $rhl_std == null || $fbsd_std == null || $frisbee_mfs == null) {
    PAGEERROR("You must add images from Utah into your database" .
              " before adding a nodetype. See installation documentation for details!",1);
}

# This belongs elsewhere!
$initial_attributes = array(
    array("attrkey" => "adminmfs_osid", "attrvalue" => $freebsd_mfs->osid(),
	  "attrtype" => "integer"),
    array("attrkey" => "bios_waittime", "attrvalue" => "60",
	  "attrtype" => "integer"),
    array("attrkey" => "bootdisk_unit", "attrvalue" => "0",
	  "attrtype" => "integer"),
    array("attrkey" => "control_interface", "attrvalue" => "ethX",
	  "attrtype" => "string"),
    array("attrkey" => "control_network", "attrvalue" => "X",
	  "attrtype" => "integer"),
    array("attrkey" => "default_imageid",
	  "attrvalue" => $default_image->imageid(),
	  "attrtype" => "string"),
    array("attrkey" => "default_osid", "attrvalue" => $rhl_std->osid(),
	  "attrtype" => "integer"),
    array("attrkey" => "delay_capacity", "attrvalue" => "2",
	  "attrtype" => "integer"),
    array("attrkey" => "delay_osid", "attrvalue" => $fbsd_std->osid(),
	  "attrtype" => "integer"),
    array("attrkey" => "diskloadmfs_osid", "attrvalue" => $frisbee_mfs->osid(),
	  "attrtype" => "integer"),
    array("attrkey" => "disksize", "attrvalue" => "0.00",
	  "attrtype" => "float"),
    array("attrkey" => "disktype", "attrvalue" => "ad",
	  "attrtype" => "string"),
    array("attrkey" => "frequency", "attrvalue" => "XXX",
	  "attrtype" => "integer"),
    array("attrkey" => "imageable", "attrvalue" => "1",
	  "attrtype" => "boolean"),
    array("attrkey" => "jail_osid", "attrvalue" =>  $fbsd_std->osid(),
	  "attrtype" => "integer"),
    array("attrkey" => "max_interfaces", "attrvalue" => "X",
	  "attrtype" => "integer"),
    array("attrkey" => "memory", "attrvalue" => "XXX",
	  "attrtype" => "integer"),
    array("attrkey" => "power_delay", "attrvalue" => "60",
	  "attrtype" => "integer"),
    array("attrkey" => "processor", "attrvalue" => "PIII",
	  "attrtype" => "string"),
    array("attrkey" => "rebootable", "attrvalue" => "1",
	  "attrtype" => "boolean"),
    array("attrkey" => "simnode_capacity", "attrvalue" => "650",
	  "attrtype" => "integer"),
    array("attrkey" => "trivlink_maxspeed", "attrvalue" => "400000",
	  "attrtype" => "integer"),
    array("attrkey" => "virtnode_capacity", "attrvalue" => "20",
	  "attrtype" => "integer"),
    );

#
# Spit the form out using the array of data.
#
function SPITFORM($node_type, $formfields, $attributes, $deletes, $errors)
{
    global $osid_result, $imageid_result, $mfsosid_result, $new_type;
    global $newattribute_name, $newattribute_value, $newattribute_type;

    #
    # Split default_imageid
    #

    if (isset($attributes["default_imageid"])) {
	$default_imageids = preg_split('/,/', $attributes["default_imageid"]);
	for ($i = 0; $i != count($default_imageids); $i++) {
	    $attributes["default_imageid_$i"] = $default_imageids[$i];
	}
	$last_default_imageid_label = "default_imageid_$i";
	$attributes["default_imageid_$i"] = 0;
	unset($attributes["default_imageid"]);
	ksort($attributes);
    } else {
	$attributes["default_imageid_0"] = 0;
	$last_default_imageid_label = "default_imageid_0";
	ksort($attributes);
    }

    #
    # Standard Testbed Header
    #
    if (! isset($new_type)) {
	PAGEHEADER("Edit Node Type");

	echo "<font size=+2>Node Type <b>".
              "<a href='shownodetype.php3?node_type=$node_type'>$node_type
                 </a></b>\n".
	       "</font>\n";

	echo "<br><br>\n";
    }
    else {
	PAGEHEADER("Create Node Type");
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


    $formargs = "node_type=$node_type";
    if (isset($new_type)) {
	$formargs .= "&new_type=1";
    }

    echo "<table align=center border=1>
          <form action=editnodetype.php3?$formargs method=post " .
	  " name=typeform>\n";

    echo "<tr>
              <td colspan=2>Type:</td>\n";
    if (isset($new_type)) {
        echo "<td class=left>
                 <input type=text
                        name=\"node_type\"
                        value=\"" . $node_type . "\"
	                size=10>
             </td>\n";
    } else {
         echo "<td class=left>$node_type</td>
          </tr>\n";
    }

    echo "<tr>
              <td colspan=2>Class:</td>\n";
    if (isset($new_type)) {
        echo "<td class=left>
                 <input type=text
                        name=\"formfields[class]\"
                        value=\"" . $formfields['class'] . "\"
	                size=10>
             </td>\n";
    } else {
	echo "<td class=left>" . $formfields["class"] . "</td>
          </tr>\n";
    }

    echo "<tr>
             <td colspan=2>isvirtnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isvirtnode]\"
                        value=\"" . $formfields["isvirtnode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isjailed:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isjailed]\"
                        value=\"" . $formfields["isjailed"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isdynamic:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isdynamic]\"
                        value=\"" . $formfields["isdynamic"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isremotenode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isremotenode]\"
                        value=\"" . $formfields["isremotenode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>issubnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issubnode]\"
                        value=\"" . $formfields["issubnode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isplabdslice:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isplabdslice]\"
                        value=\"" . $formfields["isplabdslice"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>issimnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issimnode]\"
                        value=\"" . $formfields["issimnode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isgeninode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isgeninode]\"
                        value=\"" . $formfields["isgeninode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isfednode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isfednode]\"
                        value=\"" . $formfields["isfednode"] . "\"
	                size=2>
             </td>
          </tr>\n";

    #
    # Now do attributes.
    #
    echo "<tr></tr>
          <tr></tr>
           <td align=center><font size=-1>Delete?</font></td>
           <td align=center colspan=2><b>Node Attributes</b></td></tr>\n";

    while (list ($key, $val) = each ($attributes)) {
	if (!isset($deletes[$key])) {
	    # Somehow this doesn't get initialized in the Create Node case.
	    $deletes[$key] = "";
	}
	if ($key == "default_osid" ||
	    $key == "jail_osid" ||
	    $key == "delay_osid") {
	    WRITEOSIDMENU($key, "attributes[$key]", $osid_result, $val,
			  "deletes[$key]", $deletes[$key]);
	}
	elseif ($key == "adminmfs_osid" ||
		$key == "diskloadmfs_osid") {
	    WRITEOSIDMENU($key, "attributes[$key]", $mfsosid_result, $val,
			  "deletes[$key]", $deletes[$key]);
	}
	elseif ($key == $last_default_imageid_label) {
	    WRITEIMAGEIDMENU("$key<sup><b>1</b></sup>", "attributes[$key]", $imageid_result, $val,
			     "deletes[$key]", $deletes[$key]);
	}
	elseif ($key == "default_imageid" || 
    	        substr($key, 0, strlen("default_imageid_")) == "default_imageid_") {
	    WRITEIMAGEIDMENU("$key", "attributes[$key]", $imageid_result, $val,
			     "deletes[$key]", $deletes[$key]);
	}
	else {
	    echo "<tr>
                      <td align=center>
                            <input type=checkbox value=checked
                                    name=\"deletes[$key]\" $deletes[$key]></td>
                      <td>${key}:</td>
                      <td class=left>
                          <input type=text
                                 name=\"attributes[$key]\"
                                 value=\"" . $val . "\"></td>
                  </tr>\n";
	}
    }

    #
    # Provide area for adding a new attribute.
    # 
    echo "<tr></tr>
          <tr></tr>
           <td align=center colspan=3><b>Add New Attribute</b></td></tr>\n";

    echo "<tr>
           <td colspan=2>Attribute Name:</td>
           <td class=left>
               <input type=text name=newattribute_name
                      value=\"$newattribute_name\"></td>
          </tr>\n";
    echo "<tr>
           <td colspan=2>Attribute Value:</td>
           <td class=left>
               <input type=text name=newattribute_value
                      value=\"$newattribute_value\"></td>
          </tr>\n";

    $typenames   = array();
    $typenames[] = "boolean";
    $typenames[] = "integer";
    $typenames[] = "float";
    $typenames[] = "string";
    
    echo "<tr>
           <td colspan=2>Attribute Type:</td>
           <td class=left>\n";

    foreach ($typenames as $i => $type) {
	$checked = "";

	if ($newattribute_type == $type)
	    $checked = "checked";
	
	echo "<input type=radio $checked name=newattribute_type
                     value=$type>$type\n";
    }
    echo "</td></tr>\n";

    echo "<tr>
              <td colspan=3 align=center>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";

    echo "<blockquote>
	      <ol type=1 start=1>
		 <li> To add more than one image, add the first image and 
                      submit the form.  Then add the next, etc.</li>
              </ol>
              </blockquote>";
}

if (isset($new_type)) {
    #
    # Starting a new node type - give some reasonable defaults
    #
    $defaults = array("class" => "pc", "isvirtnode" => 0,
		      "isremotenode" => 0, "issubnode" => 0,
		      "isplabdslice" => 0, "isjailed" => 0, "isdynamic" => 0,
		      "issimnode" => 0, "isgeninode" => 0, "isfednode" => 0);

    $default_attributes = array();
    $attribute_types = array();
    $attribute_deletes = array();
    
    foreach ($initial_attributes as $entry) {
	$default_attributes[$entry['attrkey']] = $entry['attrvalue'];
	$attribute_types[$entry['attrkey']] = $entry['attrtype'];
	$attribute_deletes[$entry['attrkey']] = "";
    }
}
elseif (isset($node_type)) {
    #
    # Editing an existing type - suck the current info out of the
    # database.
    #
    if (!preg_match("/^[-\w]+$/", $node_type)) {
	USERERROR("$node_type contains illegal characters!", 1);    
    }
    $query_result =
	DBQueryFatal("select * from node_types where type='$node_type'");

    if (($defaults = mysql_fetch_array($query_result)) == 0) {
	USERERROR("$node_type is not a valid node type!", 1);
    }
    
    #
    # And the attributes ... needed below.
    #
    $default_attributes = array();
    $attribute_types = array();
    $attribute_deletes = array();

    $query_result =
	DBQueryFatal("select * from node_type_attributes ".
		     "where type='$node_type'");
    while ($row = mysql_fetch_array($query_result)) {
	$default_attributes[$row['attrkey']] = $row['attrvalue'];
	$attribute_types[$row['attrkey']] = $row['attrtype'];
	$attribute_deletes[$row['attrkey']] = "";
    }
    $attribute_types["default_imageid"] = "string";
}
else {
    PAGEARGERROR("Must provide one of node_type or new_type");
    return;
}

#
# We need lists of osids and imageids for selection.
#
$osid_result =
    DBQueryFatal("select osid,osname,pid from os_info ".
		 "where (path='' or path is NULL) ".
		 "order by pid,osname");

$mfsosid_result =
    DBQueryFatal("select osid,osname,pid from os_info ".
		 "where (path is not NULL and path!='') ".
		 "order by pid,osname");

$imageid_result =
    DBQueryFatal("select imageid,imagename,pid from images ".
		 "order by pid,imagename");

#
# On first load, display initial values.
#
if (! isset($submit)) {
    SPITFORM($node_type, $defaults,
	     $default_attributes, $attribute_deletes, 0);
    PAGEFOOTER();
    return;
}

#
# We do not allow these to be changed.
#
if (!isset($new_type)) {
    $formfields["class"] = $defaults["class"];
}

#
# Otherwise, must validate and redisplay if errors.
#
$errors  = array();

#
# Combine imageids into a comma seperated list
#
$default_imagesids = array();
$default_imagesid_once_defined = 0;
for ($i = 0; isset($attributes["default_imageid_$i"]); $i++) {
    if (!(isset($deletes["default_imageid_$i"]) && $deletes["default_imageid_$i"] == "checked")
        && $attributes["default_imageid_$i"] != 0)
        array_push($default_imagesids, $attributes["default_imageid_$i"]);
    unset($attributes["default_imageid_$i"]);
    $default_imagesid_once_defined = 1;
}
if (count($default_imagesids) > 0) {
    $attributes["default_imageid"] = join(',', $default_imagesids);
} elseif ($default_imagesid_once_defined) {
    $attributes["default_imageid"] = "0";
    $deletes["default_imageid"] = "checked";
}

# Check the attributes.
while (list ($key, $val) = each ($attributes)) {
    # Skip checks if scheduled for deletion
    if (isset($deletes[$key]) && $deletes[$key] == "checked") 
	continue;

    if (!isset($attribute_types[$key])) {
	$errors[$key] = "Unknown Attribute";
	continue;
    }
    
    if ($val == "") {
	$errors[$key] = "No value provided for $key";
	continue;
    }

    # Probably redundant with the XML keyfields checking...
    $attrtype = $attribute_types[$key];
    if ($attrtype == "") {	# Shouldn't happen...
	$attrtype = $attribute_types[$key] = "integer";
    }
    if (strpos(":boolean:float:integer:string:", ":$attrtype:")===FALSE) {
	$errors[$key] = "Invalid type information: $attrtype";
	continue;
    }

    # New attributes require type and value.
    if (isset($newattribute_name) && $newattribute_name != "" &&
	!(isset($newattribute_type) && $newattribute_type != "")) {
	$errors[$newattribute_name] = "Missing type";
    }
    if (isset($newattribute_name) && $newattribute_name != "" &&
	!(isset($newattribute_value) && $newattribute_value != "")) {
	$errors[$newattribute_name] = "Missing value";
    }
}

#
# If any errors, respit the form with the current values and the
# error messages displayed. Iterate until happy.
# 
if (count($errors)) {
    SPITFORM($node_type, $formfields, $attributes, $deletes, $errors);
    PAGEFOOTER();
    return;
}

#
# Build up argument array to pass along.
#
$args = array();

# Class (only for new types.)
if (isset($new_type) &&
    isset($formfields['class']) && $formfields['class'] != "") {
    $args["new_type"] = "1";
    $args["class"] = $formfields["class"];
}

# isvirtnode
if (isset($formfields["isvirtnode"]) && $formfields["isvirtnode"] != "") {
    $args["isvirtnode"] = $formfields["isvirtnode"];
}

# isjailed
if (isset($formfields["isjailed"]) && $formfields["isjailed"] != "") {
    $args["isjailed"] = $formfields["isjailed"];
}

# isdynamic
if (isset($formfields["isdynamic"]) && $formfields["isdynamic"] != "") {
    $args["isdynamic"] = $formfields["isdynamic"];
}

# isremotenode
if (isset($formfields["isremotenode"]) && $formfields["isremotenode"] != "") {
    $args["isremotenode"] = $formfields["isremotenode"];
}

# issubnode
if (isset($formfields["issubnode"]) && $formfields["issubnode"] != "") {
    $args["issubnode"] = $formfields["issubnode"];
}

# isplabdslice
if (isset($formfields["isplabdslice"]) && $formfields["isplabdslice"] != "") {
    $args["isplabdslice"] = $formfields["isplabdslice"];
}

# issimnode
if (isset($formfields["issimnode"]) && $formfields["issimnode"] != "") {
    $args["issimnode"] = $formfields["issimnode"];
}

# isgeninode
if (isset($formfields["isgeninode"]) && $formfields["isgeninode"] != "") {
    $args["isgeninode"] = $formfields["isgeninode"];
}

# isfednode
if (isset($formfields["isfednode"]) && $formfields["isfednode"] != "") {
    $args["isfednode"] = $formfields["isfednode"];
}

# Existing attributes.
foreach ($attributes as $attr_key => $attr_val) {
    if (isset($deletes[$attr_key]) && $deletes[$attr_key] == "checked") 
	$args["delete_${attr_key}"] = "1";
    $attr_type = $attribute_types[$attr_key];
    $args["attr_${attr_type}_${attr_key}"] = $attr_val;
}

#
# Form allows for adding a single new attribute, but someday be more fancy.
#
if (isset($newattribute_name) && $newattribute_name != "" &&
    isset($newattribute_value) && $newattribute_value != "" &&
    isset($newattribute_type) && $newattribute_type != "") {

    $args["new_attr"] = $newattribute_name;
    # The following is matched by wildcards on the other side of XML,
    # including checking its type and value, just like existing attributes.
    $args["attr_${newattribute_type}_$newattribute_name"] = $newattribute_value;
}

if (! ($result = SetNodeType($node_type, $args, $errors))) {
    # Always respit the form so that the form fields are not lost.
    # I just hate it when that happens so lets not be guilty of it ourselves.
    SPITFORM($node_type, $formfields, $attributes, $deletes, $errors);
    PAGEFOOTER();
    return;
}

PAGEHEADER(isset($new_type) ? "Create" : "Edit" . " Node Type");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
#
PAGEREPLACE("editnodetype.php3?node_type=$node_type");

#
# Standard Testbed Footer
# 
PAGEFOOTER();

#
# Create or edit a nodetype.  (No class for that at present.)
#
function SetNodeType($node_type, $args, &$errors) {
    global $suexec_output, $suexec_output_array;

    #
    # Generate a temporary file and write in the XML goo.
    #
    $xmlname = tempnam("/tmp", "editnodetype");
    if (! $xmlname) {
	TBERROR("Could not create temporary filename", 0);
	$errors[] = "Transient error(1); please try again later.";
	return null;
    }
    if (! ($fp = fopen($xmlname, "w"))) {
	TBERROR("Could not open temp file $xmlname", 0);
	$errors[] = "Transient error(2); please try again later.";
	return null;
    }

    # Add these. Maybe caller should do this?
    $args["node_type"] = $node_type;
    
    fwrite($fp, "<nodetype>\n");
    foreach ($args as $name => $value) {
	fwrite($fp, "<attribute name=\"$name\">");
	fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	fwrite($fp, "</attribute>\n");
    }
    fwrite($fp, "</nodetype>\n");
    fclose($fp);
    chmod($xmlname, 0666);

    $retval = SUEXEC("nobody", "nobody", "webeditnodetype $xmlname",
		     SUEXEC_ACTION_IGNORE);

    if ($retval) {
	if ($retval < 0) {
	    $errors[] = "Transient error(3, $retval); please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	}
	else {
	    # unlink($xmlname);
	    if (count($suexec_output_array)) {
		for ($i = 0; $i < count($suexec_output_array); $i++) {
		    $line = $suexec_output_array[$i];
		    if (preg_match("/^([-\w]+):\s*(.*)$/",
				   $line, $matches)) {
			$errors[$matches[1]] = $matches[2];
		    }
		    else
			$errors[] = $line;
		}
	    }
	    else
		$errors[] = "Transient error(4, $retval); please try again later.";
	}
	return null;
    }
    # There are no return value(s) to parse at the end of the output.

    # Unlink this here, so that the file is left behind in case of error.
    # We can then create the nodetype by hand from the xmlfile, if desired.
    unlink($xmlname);
    return true;
}

?>
