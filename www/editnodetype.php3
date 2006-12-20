<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
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

if (!$new_type && (!isset($node_type) || !strcmp($node_type,""))) {
    USERERROR("No type given!", 1);
}

# This belongs elsewhere!
$initial_attributes = array(
    array("attrkey" => "adminmfs_osid", "attrvalue" => "FREEBSD-MFS",
	  "attrtype" => "string"),
    array("attrkey" => "bios_waittime", "attrvalue" => "60",
	  "attrtype" => "integer"),
    array("attrkey" => "bootdisk_unit", "attrvalue" => "0",
	  "attrtype" => "integer"),
    array("attrkey" => "control_interface", "attrvalue" => "ethX",
	  "attrtype" => "string"),
    array("attrkey" => "control_network", "attrvalue" => "X",
	  "attrtype" => "integer"),
    array("attrkey" => "default_imageid", "attrvalue" => "",
	  "attrtype" => "string"),
    array("attrkey" => "default_osid", "attrvalue" => "RHL-STD",
	  "attrtype" => "string"),
    array("attrkey" => "delay_capacity", "attrvalue" => "2",
	  "attrtype" => "integer"),
    array("attrkey" => "delay_osid", "attrvalue" => "FBSD-STD",
	  "attrtype" => "string"),
    array("attrkey" => "diskloadmfs_osid", "attrvalue" => "FRISBEE-MFS",
	  "attrtype" => "string"),
    array("attrkey" => "disksize", "attrvalue" => "0.00",
	  "attrtype" => "float"),
    array("attrkey" => "disktype", "attrvalue" => "ad",
	  "attrtype" => "string"),
    array("attrkey" => "frequency", "attrvalue" => "XXX",
	  "attrtype" => "integer"),
    array("attrkey" => "imageable", "attrvalue" => "1",
	  "attrtype" => "boolean"),
    array("attrkey" => "jail_osid", "attrvalue" => "FBSD-STD",
	  "attrtype" => "string"),
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
    global $osid_result, $imageid_result, $new_type;
    global $newattribute_name, $newattribute_value, $newattribute_type;

    #
    # Standard Testbed Header
    #
    if (! $new_type) {
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
    if ($new_type) {
	$formargs .= "&new_type=1";
    }

    echo "<table align=center border=1>
          <form action=editnodetype.php3?$formargs method=post " .
	  " name=typeform>\n";

    echo "<tr>
              <td colspan=2>Type:</td>\n";
    if ($new_type) {
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
    if ($new_type) {
        echo "<td class=left>
                 <input type=text
                        name=\"formfields[class]\"
                        value=\"" . $formfields['class'] . "\"
	                size=10>
             </td>\n";
    } else {
	echo "<td class=left>$formfields[class]</td>
          </tr>\n";
    }

    echo "<tr>
             <td colspan=2>isvirtnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isvirtnode]\"
                        value=\"" . $formfields[isvirtnode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isjailed:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isjailed]\"
                        value=\"" . $formfields[isjailed] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isdynamic:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isdynamic]\"
                        value=\"" . $formfields[isdynamic] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isremotenode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isremotenode]\"
                        value=\"" . $formfields[isremotenode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>issubnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issubnode]\"
                        value=\"" . $formfields[issubnode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>isplabdslice:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isplabdslice]\"
                        value=\"" . $formfields[isplabdslice] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td colspan=2>issimnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issimnode]\"
                        value=\"" . $formfields[issimnode] . "\"
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
	if ($key == "default_osid" ||
	    $key == "jail_osid" ||
	    $key == "delay_osid") {
	    WRITEOSIDMENU($key, "attributes[$key]", $osid_result, $val,
			  "deletes[$key]", $deletes[$key]);
	}
	elseif ($key == "default_imageid") {
	    WRITEIMAGEIDMENU($key, "attributes[$key]", $imageid_result, $val,
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
}

if ($new_type) {
    #
    # Starting a new node type - give some reasonable defaults
    #
    $defaults = array("class" => "pc", "isvirtnode" => 0,
		      "isremotenode" => 0, "issubnode" => 0,
		      "isplabdslice" => 0, "isjailed" => 0, "isdynamic" => 0,
		      "issimnode" => 0);

    $default_attributes = array();
    $attribute_types = array();
    $attribute_deletes = array();
    
    foreach ($initial_attributes as $entry) {
	$default_attributes[$entry['attrkey']] = $entry['attrvalue'];
	$attribute_types[$entry['attrkey']] = $entry['attrtype'];
	$attribute_deletes[$entry['attrkey']] = "";
    }
}
else {
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
}

#
# We need a list of osids and imageids.
#
$osid_result =
    DBQueryFatal("select osid,osname,pid from os_info ".
		 "where (path='' or path is NULL) ".
		 "order by pid,osid");

$imageid_result =
    DBQueryFatal("select imageid,imagename,pid from images ".
		 "order by pid,imageid");

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
if (!$new_type) {
    $formfields["class"] = $defaults["class"];
}

#
# Otherwise, must validate and redisplay if errors. Build up a DB insert
# string as we go. 
#
$errors  = array();
$inserts = array();

# Class (only for new types)
if ($new_type && isset($formfields['class']) && $formfields['class'] != "") {
    if (! TBvalid_description($formfields['class'])) {
	$errors["Class"] = TBFieldErrorString();
    }
    else {
	$inserts["class"] = addslashes($formfields["class"]);
    }
}

# isvirtnode
if (isset($formfields[isvirtnode]) && $formfields[isvirtnode] != "") {
    if (! TBvalid_boolean($formfields[isvirtnode])) {
	$errors["isvirtnode"] = TBFieldErrorString();
    }
    else {
	$inserts["isvirtnode"] = $formfields[isvirtnode];
    }
}

# isjailed
if (isset($formfields[isjailed]) && $formfields[isjailed] != "") {
    if (! TBvalid_boolean($formfields[isjailed])) {
	$errors["isjailed"] = TBFieldErrorString();
    }
    else {
	$inserts["isjailed"] = $formfields[isjailed];
    }
}

# isdynamic
if (isset($formfields[isdynamic]) && $formfields[isdynamic] != "") {
    if (! TBvalid_boolean($formfields[isdynamic])) {
	$errors["isdynamic"] = TBFieldErrorString();
    }
    else {
	$inserts["isdynamic"] = $formfields[isdynamic];
    }
}

# isremotenode
if (isset($formfields[isremotenode]) && $formfields[isremotenode] != "") {
    if (! TBvalid_boolean($formfields[isremotenode])) {
	$errors["isremotenode"] = TBFieldErrorString();
    }
    else {
	$inserts["isremotenode"] = $formfields[isremotenode];
    }
}

# issubnode
if (isset($formfields[issubnode]) && $formfields[issubnode] != "") {
    if (! TBvalid_boolean($formfields[issubnode])) {
	$errors["issubnode"] = TBFieldErrorString();
    }
    else {
	$inserts["issubnode"] = $formfields[issubnode];
    }
}

# isplabdslice
if (isset($formfields[isplabdslice]) && $formfields[isplabdslice] != "") {
    if (! TBvalid_boolean($formfields[isplabdslice])) {
	$errors["isplabdslice"] = TBFieldErrorString();
    }
    else {
	$inserts["isplabdslice"] = $formfields[isplabdslice];
    }
}

# issimnode
if (isset($formfields[issimnode]) && $formfields[issimnode] != "") {
    if (! TBvalid_boolean($formfields[issimnode])) {
	$errors["issimnode"] = TBFieldErrorString();
    }
    else {
	$inserts["issimnode"] = $formfields[issimnode];
    }
}

# Check the attributes
while (list ($key, $val) = each ($attributes)) {
    # Skip checks if scheduled for deletion
    if (isset($deletes[$key]) && $deletes[$key] == "checked") 
	continue;

    if (!isset($attribute_types[$key])) {
	$errors[$key] = "Unknown Attribute";
	continue;
    }
    
    $attrtype = $attribute_types[$key];
    $valid    = 1;

    if ($val == "") {
	$errors[$key] = "No value provided for $key";
	continue;
    }
    elseif ($attrtype == "boolean") {
	$valid = TBvalid_boolean($val);
    }
    elseif ($attrtype == "float") {
	$valid = TBvalid_float($val);
    }
    elseif ($attrtype == "integer") {
	$valid = TBvalid_integer($val);
    }
    elseif ($attrtype == "string") {
	$valid = TBvalid_description($val);
    }
    else {
	$errors[$key] = "Invalid type information: $attrtype";
	continue;
    }
    if (!$valid) {
	$errors[$key] = TBFieldErrorString();
    }
}

#
# Spit any errors now.
#
if (count($errors)) {
    SPITFORM($node_type, $formfields, $attributes, $deletes, $errors);
    PAGEFOOTER();
    return;
}

#
# Form allows for a single new attribute, but someday be more fancy.
#
if (isset($newattribute_name) && $newattribute_name != "" &&
    isset($newattribute_value) && $newattribute_value != "" &&
    isset($newattribute_type) && $newattribute_type != "") {

    if (!preg_match("/^[-\w]+$/", $newattribute_name)) {
	$errors["New Attribute Name"] = "Invalid characters in attribute name";
    }
    else {
	$valid    = 1;

	if ($newattribute_type == "boolean") {
	    $valid = TBvalid_boolean($newattribute_value);
	}
	elseif ($newattribute_type == "float") {
	    $valid = TBvalid_float($newattribute_value);
	}
	elseif ($newattribute_type == "integer") {
	    $valid = TBvalid_integer($newattribute_value);
	}
	elseif ($newattribute_type == "string") {
	    $valid = TBvalid_description($newattribute_value);
	}
	else {
	    $errors["New Attribute Type"] = "Invalid type: $newattribute_type";
	}
	if (!$valid) {
	    $errors["New Attribute Type"] = TBFieldErrorString();
	}
    }

    #
    # Spit any errors now.
    #
    if (count($errors)) {
	SPITFORM($node_type, $formfields, $attributes, $deletes, $errors);
	PAGEFOOTER();
	return;
    }
    # Set up for loops below.
    $attributes[$newattribute_name] = $newattribute_value;
    $attribute_types[$newattribute_name] = $newattribute_type;
}

#
# Otherwise, do the inserts.
#
$insert_data = array();
foreach ($inserts as $name => $value) {
    $insert_data[] = "$name='$value'";
}

if ($new_type) {
    DBQueryFatal("insert into node_types set type='$node_type', ".
		 implode(",", $insert_data));
    if ($formfields["class"] == "pc") {
	$vnode_type = $node_type;
	$vnode_type = preg_replace("/pc/","pcvm",$vnode_type);
	if ($vnode_type == $node_type) {
	    $vnode_type = "$vnode_type-vm";
	}
	DBQueryFatal("insert into node_types_auxtypes set " .
	    "auxtype='$vnode_type', type='pcvm'");
    }
    foreach ($attributes as $key => $value) {
        # Skip if scheduled for deletion
	if (isset($deletes[$key]) && $deletes[$key] == "checked") 
	    continue;
	
	$key   = addslashes($key);
	$type  = addslashes($attribute_types[$key]);
	$value = addslashes($value);
	
	DBQueryFatal("insert into node_type_attributes set ".
		     "   type='$node_type', ".
		     "   attrkey='$key', attrtype='$type', ".
		     "   attrvalue='$value' ");
    }
} else {
    DBQueryFatal("update node_types set ".
		 implode(",", $insert_data) . " ".
		 "where type='$node_type'");

    foreach ($attributes as $key => $value) {
	$key   = addslashes($key);
	$type  = addslashes($attribute_types[$key]);
	$value = addslashes($value);

        # Remove if scheduled for deletion
	if (isset($deletes[$key]) && $deletes[$key] == "checked") {
	    DBQueryFatal("delete from node_type_attributes ".
			 "where type='$node_type' and attrkey='$key'");
	}
	else {
	    DBQueryFatal("replace into node_type_attributes set ".
			 "   type='$node_type', ".
			 "   attrkey='$key', attrtype='$type', ".
			 "   attrvalue='$value' ");
	}
    }
}

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
#
header("Location: editnodetype.php3?node_type=$node_type");

?>
