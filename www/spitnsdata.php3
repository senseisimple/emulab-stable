<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Spit back an NS file to the user. It comes in as an urlencoded argument
# and we send it back as plain text so that user can write it out. This
# is in support of the graphical NS topology editor that fires off an
# experiment directly.
#

#
# See if nsdata was provided. 
#

if (isset($nsdata) && strcmp($nsdata, "") != 0) {
    header("Content-Type: text/plain");
    echo "$nsdata";
} elseif (isset($nsref) && strcmp($nsref,"") != 0 && 
          ereg("^[0-9]+$", $nsref)) {
    if (isset($guid) && ereg("^[0-9]+$", $guid)) {
	$nsfile = "/tmp/$guid-$nsref.nsfile";    
        $id = $guid;
    } else {
	$nsfile = "/tmp/$uid-$nsref.nsfile";    
        $id = $uid;
    }

    if (! file_exists($nsfile)) {
	PAGEHEADER("View Generated NS File");
	USERERROR("Could not find temporary file for user/guid \"" . $id .
                  "\" with id \"$nsref\".<br>\n" . 
	          "You likely copy-and-pasted an URL incorrectly,<br>\n" .
 		  "or you've already used the file to create an experiment" . 
                  "(thus erasing it),<br>\n" .
		  "or the file has expired.\n", 1 );
	PAGEFOOTER();
    } else {
	$fp = fopen($nsfile, "r");
	header("Content-Type: text/plain");
	$contents = fread ($fp, filesize ($nsfile));
	fclose($fp);
	echo "$contents";
    }
} else {
    USERERROR("No NS file provided!",1);
}

?>






