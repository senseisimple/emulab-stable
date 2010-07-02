<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Only admins.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# This is a multistep process.
#
# * Initially we come in with just a nodeid. Display a page of little maps
#   and titles the user must select (a floor in a building) from. User clicks.
# * Next time through we have a building and floor defined in addition to the
#   the nodeid. We put up that big image with the current nodes on that floor.
#   We use the external floormap program to generate that image, but without
#   an imagemap. Instead, we use a form with an input type=image, which acts
#   as a big submit button; when the user clicks in the image, the browser
#   submits the form, but with the x,y coords added as form arguments. 
# * We get all of the above arguments on the final click, including x,y. Verify
#   all the aguments, and then do the insert.
# * If user goes to reset the node location, form includes an additional submit
#   button that says to use the old coords. This allows us to change the 
#   contact info for a node without actually changing the location. 
#

# Only admins cat set node location
if (!$isadmin) {
    USERERROR("You do not have permission to access this page!", 1);
}

#
# Check page args. Must always supply a nodeid.
#
$reqargs = RequiredPageArguments("node",       PAGEARG_NODE);
$optargs = OptionalPageArguments("building",   PAGEARG_STRING,
				 "floor",      PAGEARG_STRING,
				 "x",          PAGEARG_STRING,
				 "y",          PAGEARG_STRING,
				 "contact",    PAGEARG_STRING,
				 "room",       PAGEARG_STRING,
				 "phone",      PAGEARG_STRING,
				 "submit",     PAGEARG_STRING,
				 "dodelete",   PAGEARG_STRING,
				 "isnewid",    PAGEARG_BOOLEAN,
				 "delete",     PAGEARG_BOOLEAN);

#
# Need these below.
#
$node_id = $node->node_id();
$isnewid = (isset($isnewid) ? $isnewid : 0);

#
# This routine spits out the selectable image for getting x,y coords plus
# contact info. We use a form and an input type=image, which acts as a submit
# button; the browser sends x,y coords when the user clicks in the image. 
#
function SPITFORM($errors, $node_id, $isnewid, $building, $floor,
		  $room, $contact, $phone, $old_x, $old_y,
		  $delete = 0)
{
    global $uid;
    
    # Careful with this local variable
    unset($prefix);

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
    # Create a tempfile to use as a unique prefix; it is not actually used but
    # serves the same purpose (the script uses ${prefix}.png and ${prefix}.map)
    # 
    $prefix = tempnam("/tmp", "floormap");

    #
    # Get the unique part to send back.
    #
    if (!preg_match("/^\/tmp\/([-\w]+)$/", $prefix, $matches)) {
	TBERROR("Bad tempnam: $prefix", 1);
    }
    $uniqueid = $matches[1];

    $retval = SUEXEC($uid, "nobody",
		     "webfloormap -t -o $prefix -f $floor $building",
		     SUEXEC_ACTION_IGNORE);

    if ($retval) {
	SUEXECERROR(SUEXEC_ACTION_USERERROR);
	die("");
    }
    
    echo "<script language=JavaScript>
          <!--
          function foo(target, event) {
              target.href='https://www.foo.edu/bar'
              msgWindow=window.open('','msg','width=400,height=400')
	      msgWindow.document.write(target.href + '<br>')
	      msgWindow.document.write(event.pageX + '<br>')
	      return false;
          }
          //-->
          </script>\n";

    echo "<center>
          <font size=+2>Enter contact info for $node_id,
                        then click on location</font><br>
          <form action=setnodeloc.php3 method=get>
          <table class=nogrid
                 align=center border=0 cellpadding=6 cellspacing=0>
          <tr>
             <td>Contact Name:</td>
             <td><input type=text name=contact value=\"" . $contact . "\"
                        size=30>
              </td>
          </tr>
          <tr>
             <td>Phone Number:</td>
             <td><input type=text name=phone value=\"" . $phone . "\"
                        size=30>
              </td>
          </tr>
          <tr>
             <td>Room Number:</td>
             <td><input type=text name=room value=\"" . $room . "\"
                        size=30>
              </td>
          </tr>\n";
    if ($old_x && $old_y) {
	echo "<tr>
                 <td colspan=2 align=center>
                     <b><input type=submit name=submit
	                       value='Use Existing Location'></b>
                 </td>
             </tr>\n";

	echo "<tr><td colspan=2 align=center>&nbsp</td><tr>\n";
	echo "<tr>
                 <td colspan=2 align=center>
                     <b><input type=submit name=dodelete
	                       value='Delete Location'></b></font>
                 </td>
             </tr>\n";
    }
    if (isset($isnewid) && $isnewid == 1) {
	echo "<input type=hidden name=isnewid value=1>\n";
    }
    echo "</table>
	  <input type=hidden name=node_id value=$node_id>
	  <input type=hidden name=building value=$building>
	  <input type=hidden name=floor value=$floor>
          <input type=image
                 src=\"floormap_aux.php3?prefix=$uniqueid\">
          </form>
          </center>\n";
}

#
# Get default values from location info or from the new_nodes table.
#
if (isset($isnewid) && $isnewid == 1) {
    $query_result =
	DBQueryFatal("select * from new_nodes ".
		     "where new_node_id='$node_id'");
}
else {
    $query_result =
	DBQueryFatal("select * from location_info ".
		     "where node_id='$node_id'");
}
if (mysql_num_rows($query_result)) {
    $row = mysql_fetch_array($query_result);
    
    if (! isset($contact))
	$contact = $row["contact"];
    if (! isset($room))
	$room = $row["room"];
    if (! isset($phone))
	$phone = $row["phone"];
    $old_x = $row["loc_x"];
    $old_y = $row["loc_y"];
    $old_building = $row["building"];
    $old_floor    = $row["floor"];
}
else {
    if (! isset($contact))
	$contact = "";
    if (! isset($room))
	$room = "";
    if (! isset($phone))
	$phone = "";
    $old_x = null;
    $old_y = null;
}

#
# Handle deletion.
#
if (isset($delete) && $delete == 1) {
    #
    # Skip to form with node on it, which includes delete button.
    #
    PAGEHEADER("Set Node Location");

    if (! isset($old_building)) {
	USERERROR("There is no location info for node $node_id!", 1);
    }
    SPITFORM(0, $node_id, $isnewid, $old_building, $old_floor, $room, $contact,
	     $phone, $old_x, $old_y);
    PAGEFOOTER();
    exit(0);
}
elseif (isset($dodelete) && $dodelete != "") {
    #
    # Delete the entry and zap to shownode page. 
    #
    if (isset($isnewid) && $isnewid == 1) {
	DBQueryFatal("update new_nodes set " .
		     "       building=NULL,floor=NULL,loc_x=0,loc_y=0 ".
		     "where new_node_id='$node_id'");
	header("Location: newnode_edit.php3?id=$node_id");
    }
    else {
	DBQueryFatal("delete from location_info where node_id='$node_id'");
	header("Location: shownode.php3?node_id=$node_id");
    }
	
    exit(0);
}

#
# If no building/floor specified, throw up a page of images of all the
# buildings and floors
#
if (!isset($building) || $building == "" || !isset($floor) || $floor == "") {
    PAGEHEADER("Set Node Location");

    $building_result =
	DBQueryFatal("select b.building,b.title,f.floor,f.thumb_path ".
		     "   from buildings as b ".
		     "left join floorimages as f on f.building=b.building " .
		     "where f.scale=1 " .
		     "order by b.building,f.floor");

    if (! mysql_num_rows($building_result)) {
	USERERROR("There is no floormap data in the DB!", 1);
    }

    # Ha.
    $floortags = array();
    $floortags[1] = "1st floor";
    $floortags[2] = "2nd floor";   
    $floortags[3] = "3rd floor";
    $floortags[4] = "4th floor";
    $floortags[5] = "5th floor";
    $floortags[6] = "6th floor";
    $floortags[7] = "7th floor";
    $floortags[8] = "8th floor";
    $floortags[9] = "9th floor";

    echo "<center>\n";

    if (isset($old_building)) {
	echo "<a href='setnodeloc.php3?node_id=$node_id&delete=1".
	         "&isnewid=$isnewid'>
                 Delete</a> location info or<br>\n";
    }

    echo "<font size=+2>Pick a floor, any floor</font><br><br>\n";
    echo "<table>\n";

    $maxcol = 2;
    $i = 0;
    $titles = array();

    #
    # XXX This will need to be generalized to multiple buildings at some
    # point; right now just throw up a set of thumb images in a single 
    # table. Not very pretty.
    #
    while ($row = mysql_fetch_array($building_result)) {
	$building = $row["building"];
	$title    = $row["title"];
	$floor    = $row["floor"];
	$thumb    = $row["thumb_path"];

	if ($i == $maxcol) {
	    #
	    # Start a new row. Keeps the window reasonably narrow.
	    # We have to put the titles in for the previous row before
	    # starting the next row; see below where we have to do this
	    # when the table is finished. 
	    #
	    echo "</tr><tr>\n";
	    for ($i = 0 ; $i < $maxcol; $i++) {
		echo "<td align=center>" . $titles[$i] . "</td>\n";
	    }
	    echo "</tr><tr>\n";
	    $i = 0;
	}
	
	echo "<td>
                <a href='setnodeloc.php3?node_id=$node_id".
	               "&building=$building&floor=$floor&isnewid=$isnewid'>
                   <img src=\"floormap/$thumb\"></a>
              </td>\n";
	$titles[$i] = "$title - " . $floortags[$floor];
	$i++;
    }
    # Finish out previous row.
    while ($i < $maxcol) {
	echo "<td></td>";
	$titles[$i] = "";
	$i++;
    }
    echo "</tr>";
    # Then stick in the title row.
    echo "<tr>";
    for ($i = 0 ; $i < $maxcol; $i++) {
	echo "<td align=center>" . $titles[$i] . "</td>\n";
    }
    echo "</tr>\n";

    echo "</table>\n";
    echo "</center>\n";

    PAGEFOOTER();
    exit(0);
}

#
# At this point both building and floor are specified. Sanitize for shell.
#
if (!preg_match("/^[-\w]+$/", $building)) {
    PAGEARGERROR("Invalid characters in building");
}
if (!preg_match("/^[-\w]+$/", $floor)) {
    PAGEARGERROR("Invalid characters in floor");
}

#
# If no x,y yet, then we must have a building/floor and we put up that image.
# If there were existing coords, then the user got a submit box. When that is
# clicked, we use those old coords (we will not get x,y from the image submit). 
#
if (!isset($submit) && (!isset($x) || $x == "" || !isset($y) || $y == "")) {
    PAGEHEADER("Set Node Location");

    SPITFORM(0, $node_id, $isnewid, $building, $floor, $room, $contact, $phone,
	     $old_x, $old_y);
    PAGEFOOTER();
    exit(0);
}
elseif (isset($submit)) {
    #
    # Lets make sure that the node really does have an existing location.
    #
    if (!isset($old_x) || !isset($old_y)) {
	PAGEHEADER("Set Node Location");
	PAGEARGERROR("Need location coordinates");
    }
    $x = $old_x;
    $y = $old_y;
    $building = $old_building;
    $floor    = $old_floor;
}

#
# Okay, make sure the x and y coords are numbers, and verify all input data.
#
$errors  = array();
$inserts = array();

# x,y
if (! TBvalid_slot($x, "location_info", "loc_x")) {
    $errors["X"] = TBFieldErrorString();
}
elseif ($x <= 0) {
    $errors["X"] = "Must be greater then zero";
}
else {
    $inserts["loc_x"] = $x;
}
if (! TBvalid_slot($y, "location_info", "loc_y")) {
    $errors["Y"] = TBFieldErrorString();
}
elseif ($x <= 0) {
    $errors["Y"] = "Must be greater then zero";
}
else {
    $inserts["loc_y"] = $y;
}

# Building
if (! TBvalid_slot($building, "location_info", "building")) {
    $errors["Building"] = TBFieldErrorString();
}
else {
    $inserts["building"] = $building;
}

# Floor
if (! TBvalid_slot($floor, "location_info", "floor")) {
    $errors["Floor"] = TBFieldErrorString();
}
else {
    $inserts["floor"] = $floor;
}

# Contact
if (isset($contact) && $contact != "" &&
    !TBvalid_slot($contact, "location_info", "contact")) {
    $errors["Contact"] = TBFieldErrorString();
}
else {
    $inserts["contact"] = $contact;
}

# Phone
if (isset($phone) && $phone != "" &&
    !TBvalid_slot($phone, "location_info", "phone")) {
    $errors["Phone"] = TBFieldErrorString();
}
else {
    $inserts["phone"] = $phone;
}

# Room
if (isset($room) && $room != "" &&
    !TBvalid_slot($room, "location_info", "room")) {
    $errors["Room"] = TBFieldErrorString();
}
else {
    $inserts["room"] = $room;
}

#
# Spit any errors now.
#
if (count($errors)) {
    PAGEHEADER("Set Node Location");
    SPITFORM($errors, $node_id, $isnewid, $building, $floor, $room, $contact,
	     $phone, $old_x, $old_y);
    PAGEFOOTER();
    return;
}

#
# Otherwise, do the inserts or updates
#
$insert_data = array();
foreach ($inserts as $name => $value) {
    $insert_data[] = "$name='$value'";
}

if (isset($isnewid) && $isnewid == 1) {
    DBQueryFatal("update new_nodes set " . implode(",", $insert_data) . " ".
		 "where new_node_id='$node_id'");
}
else {
    $insert_data[] = "node_id='$node_id'";

    DBQueryFatal("replace into location_info set " . implode(",", $insert_data));
}

#
# Zap back to floormap for this building/floor.
# 
header("Location: floormap.php3?building=$building&floor=$floor");

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
