<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Dump Node Disk Contents into Existing Image Descriptor");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (! isset($imageid)) {
    USERERROR("Must pass image name to page as 'imageid'.", 1 );
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
	 "<font size=+1>Node to dump into image '$imageid':</font> ".
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

if (! TBNodeAccessCheck($uid, $node, $TB_NODEACCESS_LOADIMAGE)) {
    USERERROR("You do not have permission to ".
	      "dump an image from node '$node'.", 1);
}

# Should check for file file_exists($image_path),
# but that's too messy.

if (! isset($confirmed) ) {
    echo "<center><form action='loadimage.php3' method='post'>\n".
#         "<h2>Image already exists at '<code>$image_path</code>'.".
         "<h2><b>Warning!</b><br />".
	 "Dumping disk contents from node '$node' into image '$imageid' ".
	 "will overwrite any previously dumped image. ".
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
      Creating image for '$imageid' using node '$node' ...
      <br><br>\n";
flush();

SUEXEC($uid, $unix_gid, "webcreateimage -p $image_pid $image_name $node", 1);

echo "This will take 10 minutes or more; you will receive email
      notification when the image is complete. In the meantime,
      <b>PLEASE DO NOT</b> delete the imageid or the experiment
      $node is in. In fact, it is best if you do not mess with 
      the node at all!<br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>



