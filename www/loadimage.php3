<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("imageid_defs.php");
include_once("node_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("image",     PAGEARG_IMAGE);
$optargs = OptionalPageArguments("node",      PAGEARG_NODE,
				 "canceled",  PAGEARG_STRING,
				 "confirmed", PAGEARG_STRING);

#
# Standard Testbed Header
#
PAGEHEADER("Snapshot Node Disk into Existing Image Descriptor");

# Need these below.
$imageid    = $image->imageid();
$image_pid  = $image->pid();
$image_gid  = $image->gid();
$image_name = $image->imagename();
$image_path = $image->path();
$node_id    = (isset($node) ? $node->node_id() : "");

if (!$image->AccessCheck($this_user, $TB_IMAGEID_MODIFYINFO )) {
    USERERROR("You do not have permission to modify image '$imageid'.", 1);
}

if (! isset($node) || isset($canceled)) {
    echo "<center>";

    if (isset($canceled)) {
	echo "<h3>Operation canceled.</h3>";
    }
    echo "<br />";

    $url = CreateURL("loadimage", $image); 

    echo "<form action='$url' method='post'>\n".
	 "<font size=+1>Node to snapshot into image '$image_name':</font> ".
	 "<input type='text'   name='node_id' value='$node_id'></input>\n".
	 "<input type='submit' name='submit'  value='Go!'></input>\n".
	 "</form><br>";
    echo "<font size=+1>Information for Image Descriptor '$image_name':</font>\n";
    
    $image->Show();

    echo "</center>";
    PAGEFOOTER();
    return;
}

if (!$node->AccessCheck($this_user, $TB_NODEACCESS_LOADIMAGE)) {
    USERERROR("You do not have permission to ".
	      "snapshot an image from node '$node_id'.", 1);
}
$experiment = $node->Reservation();
if (!$experiment) {
    USERERROR("$node_id is not currently reserved to an experiment!", 1);
}
$node_pid = $experiment->pid();
$node_eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$project  = $experiment->Project();
$unix_pid = $project->unix_gid();

# Should check for file file_exists($image_path),
# but too messy.

if (! isset($confirmed)) {
    $url = CreateURL("loadimage", $image);
    
    echo "<center><form action='$url' method='post'>\n".
         "<h2><b>Warning!</b></h2>".
	 "<h3>Doing a snapshot of node '$node_id' into image '$image_name' ".
	 "will overwrite any previous snapshot for that image. ".
	 "Are you sure you want to continue?</h3>".
         "<input type='hidden' name='node_id'   value='$node_id'></input>".
         "<input type='submit' name='confirmed' value='Confirm'></input>".
         "&nbsp;".
         "<input type='submit' name='canceled' value='Cancel'></input>\n".    
         "</form></center>";

    PAGEFOOTER();
    return;
}

echo "<br>
      Taking a snapshot of node '$node_id' into image '$image_name' ...
      <br><br>\n";
flush();

SUEXEC($uid,
       "$unix_pid,$unix_gid" . ($image_pid != $node_pid ? ",$node_pid" : ""),
       "webcreate_image -p $image_pid $image_name $node_id",
       SUEXEC_ACTION_DUPDIE);

echo "This will take 10 minutes or more; you will receive email
      notification when the snapshot is complete. In the meantime,
      <b>PLEASE DO NOT</b> delete the imageid or the experiment
      $node_id is in. In fact, it is best if you do not mess with 
      the node or the experiment at all until you receive email.<br>\n";

flush();

STARTLOG($experiment);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
