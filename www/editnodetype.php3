<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004 University of Utah and the Flux Group.
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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users only
#
$isadmin = ISADMIN($uid);

if (! $isadmin) {
    USERERROR("You do not have permission to edit node types!", 1);
}

#
# Verify page arguments.
# 
if (!isset($node_type) ||
    strcmp($node_type, "") == 0) {
    PAGEARGERROR("You must provide a node type.");
}

#
# Spit the form out using the array of data.
#
function SPITFORM($node_type, $formfields, $errors)
{
    global $osid_result, $imageid_result;

    #
    # Standard Testbed Header
    #
    PAGEHEADER("Edit Node Type");

    echo "<font size=+2>Node Type <b>".
         "<a href='shownodetype.php3?node_type=$node_type'>$node_type</a></b>\n".
         "</font>\n";
    echo "<br><br>\n";

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

    echo "<table align=center border=1>
          <form action=editnodetype.php3?node_type=$node_type method=post>\n";

    echo "<tr>
              <td>Type:</td>
              <td class=left>$node_type</td>
          </tr>\n";

    echo "<tr>
              <td>Class:</td>
              <td class=left>$formfields[class]</td>
          </tr>\n";

    echo "<tr>
             <td>Processor:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[proc]\"
                        value=\"" . $formfields[proc] . "\"
	                size=10>
             </td>
          </tr>\n";

    echo "<tr>
             <td>Speed:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[speed]\"
                        value=\"" . $formfields[speed] . "\"
	                size=5> MHZ
             </td>
          </tr>\n";

    echo "<tr>
             <td>RAM:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[RAM]\"
                        value=\"" . $formfields[RAM] . "\"
	                size=5> MB
             </td>
          </tr>\n";

    echo "<tr>
             <td>Disk Size:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[HD]\"
                        value=\"" . $formfields[HD] . "\"
	                size=10> GB
             </td>
          </tr>\n";

    echo "<tr>
             <td>Max Interfaces:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[max_interfaces]\"
                        value=\"" . $formfields[max_interfaces] . "\"
	                size=3>
             </td>
          </tr>\n";

    WRITEOSIDMENU("Default OSID", "formfields[osid]",
		  $osid_result, $formfields[osid]);

    echo "<tr>
              <td>Control Network Interface:</td>
              <td class=left>$formfields[control_net]</td>
          </tr>\n";

    echo "<tr>
             <td>Power Cycle Time:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[power_time]\"
                        value=\"" . $formfields[power_time] . "\"
	                size=5> Seconds
             </td>
          </tr>\n";

    WRITEIMAGEIDMENU("Default ImageID", "formfields[imageid]",
		  $imageid_result, $formfields[imageid]);

    echo "<tr>
             <td>Imageable?:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[imageable]\"
                        value=\"" . $formfields[imageable] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>Delay Capacity:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[delay_capacity]\"
                        value=\"" . $formfields[delay_capacity] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>Control Network Iface:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[control_iface]\"
                        value=\"" . $formfields[control_iface] . "\"
	                size=6>
             </td>
          </tr>\n";

    echo "<tr>
             <td>Disktype (ad,da,ar):</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[disktype]\"
                        value=\"" . $formfields[disktype] . "\"
	                size=6>
             </td>
          </tr>\n";


    WRITEOSIDMENU("Delay OSID", "formfields[delay_osid]",
		  $osid_result, $formfields[delay_osid]);

    WRITEOSIDMENU("Jailbird OSID", "formfields[jail_osid]",
		  $osid_result, $formfields[jail_osid]);

    echo "<tr>
             <td>isvirtnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isvirtnode]\"
                        value=\"" . $formfields[isvirtnode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>isremotenode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isremotenode]\"
                        value=\"" . $formfields[isremotenode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>issubnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issubnode]\"
                        value=\"" . $formfields[issubnode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>isplabdslice:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[isplabdslice]\"
                        value=\"" . $formfields[isplabdslice] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>issimnode:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[issimnode]\"
                        value=\"" . $formfields[issimnode] . "\"
	                size=2>
             </td>
          </tr>\n";

    echo "<tr>
             <td>Simnode Capacity:</td>
             <td class=left>
                 <input type=text
                        name=\"formfields[simnode_capacity]\"
                        value=\"" . $formfields[simnode_capacity] . "\"
	                size=3>
             </td>
          </tr>\n";

    echo "<tr>
              <td colspan=2 align=center>
                 <b><input type=submit name=submit value=Submit></b>
              </td>
          </tr>\n";

    echo "</form>
          </table>\n";
}

#
# Suck the current info out of the database.
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
    SPITFORM($node_type, $defaults, 0);
    PAGEFOOTER();
    return;
}

#
# We do not allow these to be changed.
#
$formfields["class"]       = $defaults["class"];
$formfields["control_net"] = $defaults["control_net"];

#
# Otherwise, must validate and redisplay if errors. Build up a DB insert
# string as we go. 
#
$errors  = array();
$inserts = array();

# Processor
if (isset($formfields[proc]) && $formfields[proc] != "") {
    if (! TBvalid_description($formfields[proc])) {
	$errors["Processor"] = TBFieldErrorString();
    }
    else {
	$inserts["proc"] = addslashes($formfields[proc]);
    }
}

# Speed
if (isset($formfields[speed]) && $formfields[speed] != "") {
    if (! TBvalid_integer($formfields[speed])) {
	$errors["speed"] = TBFieldErrorString();
    }
    else {
	$inserts["speed"] = $formfields[speed];
    }
}

# RAM
if (isset($formfields[RAM]) && $formfields[RAM] != "") {
    if (! TBvalid_integer($formfields[RAM])) {
	$errors["RAM"] = TBFieldErrorString();
    }
    else {
	$inserts["RAM"] = $formfields[RAM];
    }
}

# HD
if (isset($formfields[HD]) && $formfields[HD] != "") {
    if (! TBvalid_float($formfields[HD])) {
	$errors["HD"] = TBFieldErrorString();
    }
    else {
	$inserts["HD"] = $formfields[HD];
    }
}

# max_interfaces
if (isset($formfields[max_interfaces]) && $formfields[max_interfaces] != "") {
    if (! TBvalid_tinyint($formfields[max_interfaces])) {
	$errors["max_interfaces"] = TBFieldErrorString();
    }
    else {
	$inserts["max_interfaces"] = $formfields[max_interfaces];
    }
}

# OSID
if (isset($formfields[osid]) && $formfields[osid] != "") {
    if ($formfields[osid] == "none") {
	$inserts["osid"] = "";
    }
    elseif (! TBvalid_osid($formfields[osid])) {
	$errors["osid"] = TBFieldErrorString();
    }
    elseif (! TBValidOSID($formfields[osid])) {
	$errors["osid"] = "No such OSID";
    }
    else {
	$inserts["osid"] = $formfields[osid];
    }
}

# power_time
if (isset($formfields[power_time]) && $formfields[power_time] != "") {
    if (! TBvalid_tinyint($formfields[power_time])) {
	$errors["power_time"] = TBFieldErrorString();
    }
    else {
	$inserts["power_time"] = $formfields[power_time];
    }
}

# ImageID
if (isset($formfields[imageid]) && $formfields[imageid] != "") {
    if ($formfields[imageid] == "none") {
	$inserts["imageid"] = "";
    }
    elseif (! TBvalid_osid($formfields[imageid])) {
	$errors["imageid"] = TBFieldErrorString();
    }
    elseif (! TBValidImageID($formfields[imageid])) {
	$errors["imageid"] = "No such ImageID";
    }
    else {
	$inserts["imageid"] = $formfields[imageid];
    }
}

# imageable
if (isset($formfields[imageable]) && $formfields[imageable] != "") {
    if (! TBvalid_boolean($formfields[imageable])) {
	$errors["imageable"] = TBFieldErrorString();
    }
    else {
	$inserts["imageable"] = $formfields[imageable];
    }
}

# delay_capacity
if (isset($formfields[delay_capacity]) && $formfields[delay_capacity] != "") {
    if (! TBvalid_tinyint($formfields[delay_capacity])) {
	$errors["delay_capacity"] = TBFieldErrorString();
    }
    else {
	$inserts["delay_capacity"] = $formfields[delay_capacity];
    }
}

# control_iface
if (isset($formfields[control_iface]) && $formfields[control_iface] != "") {
    if (! TBvalid_userdata($formfields[control_iface])) {
	$errors["control_iface"] = TBFieldErrorString();
    }
    else {
	$inserts["control_iface"] = addslashes($formfields[control_iface]);
    }
}

# disktype
if (isset($formfields[disktype]) && $formfields[disktype] != "") {
    if ($formfields[disktype] != "ad" &&
	$formfields[disktype] != "da" &&
	$formfields[disktype] != "ar") {
	$errors["disktype"] = "Must be one of ad, da, ar";
    }
    else {
	$inserts["disktype"] = $formfields[disktype];
    }
}

# delay_osid
if (isset($formfields[delay_osid]) && $formfields[delay_osid] != "") {
    if ($formfields[delay_osid] == "none") {
	$inserts["delay_osid"] = "";
    }
    elseif (! TBvalid_osid($formfields[delay_osid])) {
	$errors["delay_osid"] = TBFieldErrorString();
    }
    elseif (! TBValidOSID($formfields[delay_osid])) {
	$errors["delay_osid"] = "No such osid";
    }
    else {
	$inserts["delay_osid"] = $formfields[delay_osid];
    }
}

# jail_osid
if (isset($formfields[jail_osid]) && $formfields[jail_osid] != "") {
    if ($formfields[jail_osid] == "none") {
	$inserts["jail_osid"] = "";
    }
    elseif (! TBvalid_osid($formfields[jail_osid])) {
	$errors["jail_osid"] = TBFieldErrorString();
    }
    elseif (! TBValidOSID($formfields[jail_osid])) {
	$errors["jail_osid"] = "No such osid";
    }
    else {
	$inserts["jail_osid"] = $formfields[jail_osid];
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

# simnode_capacity
if (isset($formfields[simnode_capacity]) &&
    $formfields[simnode_capacity] != "") {
    if (! TBvalid_tinyint($formfields[simnode_capacity])) {
	$errors["simnode_capacity"] = TBFieldErrorString();
    }
    else {
	$inserts["simnode_capacity"] = $formfields[simnode_capacity];
    }
}

#
# Spit any errors now.
#
if (count($errors)) {
    SPITFORM($node_type, $formfields, $errors);
    PAGEFOOTER();
    return;
}

#
# Otherwise, do the inserts.
#
$insert_data = array();
foreach ($inserts as $name => $value) {
    $insert_data[] = "$name='$value'";
}

DBQueryFatal("update node_types set ".
	     implode(",", $insert_data) . " ".
	     "where type='$node_type'");

#
# Spit out a redirect so that the history does not include a post
# in it. The back button skips over the post and to the form.
#
header("Location: editnodetype.php3?node_type=$node_type");

?>
