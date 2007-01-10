<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Snapshot Node Disk into Existing Image Descriptor");

if (! isset($imageid)) {
    USERERROR("Must pass image name to page as 'imageid'.", 1 );
}
# Verify args.
if (! TBvalid_imageid($imageid)) {
    PAGEARGERROR("Invalid characters in $imageid!");
}

# get imageid group/project

$query_result = 
    DBQueryFatal("SELECT pid, gid, imagename, path FROM images ".
		 "WHERE imageid='$imageid'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such image '$imageid'.",1);
}

$row = mysql_fetch_array($query_result);
$image_pid  = $row[pid];
$image_gid  = $row[gid];
$image_name = $row[imagename];
$image_path = $row[path];

# permission check.

#if (! TBProjAccessCheck($uid, $image_pid, $image_gid,
#			$TB_PROJECT_MAKEINFO) ) {
#    USERERROR("You do not have permission to modify images in "
#	    "$pid/$gid.",1);            
#}

if (! TBImageIDAccessCheck($uid, $imageid, $TB_IMAGEID_MODIFYINFO )) {
    USERERROR("You do not have permission to modify image '$imageid'.", 1);
}

if (! isset($node) || isset($cancelled)) {
    echo "<center>";

    if (isset($cancelled)) {
	echo "<h3>Operation cancelled.</h3>";
    }

    echo "<br />";

    echo "<form action='loadimage.php3' method='post'>\n".
	 "<font size=+1>Node to snapshot into image '$imageid':</font> ".
	 "<input type='text'   name='node' value='$node'></input>\n".
	 "<input type='hidden' name='imageid' value='$imageid'></input>\n".
	 "<input type='submit' name='submit'  value='Go!'></input>\n".
	 "</form>";
    echo "<font size=+1>Information for Image Descriptor '$imageid':</font>\n";
    SHOWIMAGEID($imageid, 0);

    echo "</center>";
    PAGEFOOTER();
    return;
}

# Verify args.
if (! TBvalid_node_id($node)) {
    PAGEARGERROR("Invalid characters in $node!");
}

if (! TBNodeAccessCheck($uid, $node, $TB_NODEACCESS_LOADIMAGE)) {
    USERERROR("You do not have permission to ".
	      "snapshot an image from node '$node'.", 1);
}

TBNodeIDtoExpt($node, $node_pid, $node_eid, $node_gid);

# Should check for file file_exists($image_path),
# but too messy.

if (! isset($confirmed) ) {
    echo "<center><form action='loadimage.php3' method='post'>\n".
#         "<h2>Image already exists at '<code>$image_path</code>'.".
         "<h2><b>Warning!</b><br />".
	 "Doing a snapshot of node '$node' into image '$imageid' ".
	 "will overwrite any previous snapshot for that image. ".
	 "Are you sure you want to continue?</h2>".
         "<input type='hidden' name='node'      value='$node'></input>".
         "<input type='hidden' name='imageid'   value='$imageid'></input>".
         "<input type='submit' name='confirmed' value='Confirm'></input>".
         "&nbsp;".
         "<input type='submit' name='cancelled' value='Cancel'></input>\n".    
         "</form></center>";

    PAGEFOOTER();
    return;
}

TBGroupUnixInfo($image_pid, $image_gid, $unix_gid, $unix_name);

echo "<br>
      Taking a snapshot of node '$node' into image '$imageid' ...
      <br><br>\n";
flush();

SUEXEC($uid,
       "$image_pid,$unix_gid" . ($image_pid != $node_pid ? ",$node_pid" : ""),
       "webcreateimage -p $image_pid $image_name $node",
       SUEXEC_ACTION_DUPDIE);

echo "This will take 10 minutes or more; you will receive email
      notification when the snapshot is complete. In the meantime,
      <b>PLEASE DO NOT</b> delete the imageid or the experiment
      $node is in. In fact, it is best if you do not mess with 
      the node at all!<br>\n";

flush();

STARTLOG($node_pid, $node_eid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
