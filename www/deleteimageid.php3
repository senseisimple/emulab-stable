<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Delete an Image Descriptor");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Must provide the ImageID.
# 
if (!isset($imageid) ||
    strcmp($imageid, "") == 0) {
    USERERROR("The ImageID was not provided!", 1);
}

if (! TBValidImageID($imageid)) {
    USERERROR("ImageID '$imageid' is not a valid ImageID!", 1);
}

#
# Verify permission.
#
if (!TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_DESTROY)) {
    USERERROR("You do not have permission to destroy ImageID $imageid!", 1);
}

#
# Get user level info.
#
if (!TBImageInfo($imageid, $imagename, $pid)) {
    USERERROR("ImageID '$imageid' is no longer a valid ImageID!", 1);
}

#
# Check to see if the imageid is being used in various places
#
$query_result1 =
    DBQueryFatal("select * from current_reloads where image_id='$imageid'");
$query_result2 =
    DBQueryFatal("select * from scheduled_reloads where image_id='$imageid'");
$query_result3 =
    DBQueryFatal("select * from node_type_attributes ".
		 "where default_imageid='$imageid' limit 1");

if (mysql_num_rows($query_result1)) {
    echo "$imageid is referenced in the current_reloads table!<br>";
    $conflicts++;
}
if (mysql_num_rows($query_result2)) {
    echo "$imageid is referenced in the scheduled_reloads table!<br>";
    $conflicts++;
}
if (mysql_num_rows($query_result3)) {
    echo "$imageid is referenced in the node_types table!<br>";
    $conflicts++;
}
if ($conflicts) {
    echo "<br>
          You must resolve these issues before the imageid can be deleted!";
    PAGEFOOTER();
    return;
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Image Descriptor removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to delete Image '$imagename' in project $pid?
          </h2>\n";
    
    echo "<form action='deleteimageid.php3' method=post>";
    echo "<input type=hidden name=imageid value='$imageid'>\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    echo "<br>
          <ul>
           <li> Please note that if you have any experiments (swapped in
                <b>or</b> out) using an OS in this image, that experiment
                cannot be swapped in or out properly. You should terminate
                those experiments first, or cancel the deletion of this
                image by clicking on the cancel button above.
          </ul>\n";
    
    PAGEFOOTER();
    return;
}

#
# Need to make sure that there is no frisbee process running before
# we kill it.
#
$retval = SUEXEC($uid, $pid, "webfrisbeekiller $imageid", SUEXEC_ACTION_DIE);

DBQueryFatal("lock tables images write, os_info write, osidtoimageid write");

#
# If this is an EZ imageid, then delete the corresponding OSID too.
#
$query_result =
    DBQueryFatal("select ezid,path from images where imageid='$imageid'");
$row = mysql_fetch_row($query_result);
$ezid = $row[0];
$path = $row[1];

#
# Delete the record(s).
#
DBQueryFatal("DELETE FROM images WHERE imageid='$imageid'");
DBQueryFatal("DELETE FROM osidtoimageid where imageid='$imageid'");
if ($ezid) {
    DBQueryFatal("DELETE FROM os_info WHERE osid='$imageid'");
}

DBQueryFatal("unlock tables");

echo "<br>
      <h3>
      Image '$imageid' in project $pid has been deleted!\n";

if ($path) {
    echo "<br>
          <br>
          <font color=red>Please remember to delete $path!</font>\n";
}
echo "</h3>\n";

echo "<br>
      <center>
      <a href='showimageid_list.php3'>Back to Image Descriptor list</a>
      </center>
      <br><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
