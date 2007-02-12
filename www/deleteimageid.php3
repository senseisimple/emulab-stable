<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("imageid_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Delete an Image Descriptor");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("image", PAGEARG_IMAGE);
$optargs = OptionalPageArguments("canceled", PAGEARG_BOOLEAN,
				 "confirmed", PAGEARG_BOOLEAN);

# Need these below
$imageid = $image->imageid();
$imagename = $image->imagename();
$pid = $image->pid();

#
# Verify permission.
#
if (! $image->AccessCheck($this_user, $TB_IMAGEID_DESTROY)) {
    USERERROR("You do not have permission to destroy ImageID $imageid!", 1);
}

#
# Check to see if the imageid is being used in various places
#
if ($image->InUse()) {
    USERERROR("Image $imageid is still in use or busy!<br>".
	      "You must resolve these issues before is can be deleted!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    echo "<center><h2><br>
          Image Descriptor removal canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to delete Image '$imagename' in project $pid?
          </h2>\n";

    $url = CreateURL("deleteimageid", $image);
    
    echo "<form action='$url' method=post>";
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
STARTBUSY("Removing imageid");

$retval = SUEXEC($uid, $pid, "webfrisbeekiller $imageid", SUEXEC_ACTION_DIE);

DBQueryFatal("lock tables images write, os_info write, osidtoimageid write");

#
# If this is an EZ imageid, then delete the corresponding OSID too.
#
# Delete the record(s).
#
DBQueryFatal("DELETE FROM images WHERE imageid='$imageid'");
DBQueryFatal("DELETE FROM osidtoimageid where imageid='$imageid'");
if ($image->ezid()) {
    DBQueryFatal("DELETE FROM os_info WHERE osid='$imageid'");
}
DBQueryFatal("unlock tables");

STOPBUSY();

echo "<br>
      <h3>
      Image '$imageid' in project $pid has been deleted!\n";

if ($image->path() && $image->path() != "") {
    $path = $image->path();
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
