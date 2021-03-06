<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#

chdir("..");
include("defs.php3");

# this is just saving data; it isnt human readable.

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("guid", PAGEARG_INTEGER,
				 "nsref", PAGEARG_INTEGER,
				 "nsdata", PAGEARG_ANYTHING,
				 "uid", PAGEARG_STRING);

#
# Only known and logged in users.
#
if (isset($guid) && preg_match('/^[0-9]+$/', $guid)) {
	$uid = $guid;
}
else {
	if (isset($uid)) {
	        $this_user = CheckLoginOrDie();
	        $uid       = $this_user->uid();
	}
	else {
		USERERROR("Need to send guid or uid!", 1);
        } 
}

if (!isset($nsdata)) {
	USERERROR("Need to send NSFILE!", 1);
} 

if (!isset($nsref) || !preg_match('/^[0-9]+$/', $nsref)) {
	USERERROR("Need to send valid NSREF!", 1);
}

$nsfilename = "/tmp/$uid-$nsref.nsfile";

if (! ($fp = fopen($nsfilename, "w"))) {
	TBERROR("Could not create temporary file $nsfile", 1);
}

if (strlen( $nsdata ) > 50000) {
	$nsdata = "#(NS file was >50kb big)\n" .
                  "!!! ERROR: NS File Truncated !!!\n";
}

fwrite($fp, $nsdata);
fclose($fp);

header("Content-Type: text/plain");
echo "success!";
?>