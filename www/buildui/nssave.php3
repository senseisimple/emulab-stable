<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

chdir("..");
include("defs.php3");

# this is just saving data; it isnt human readable.

#
# Only known and logged in users can begin experiments.
#

if (isset($guid) && ereg("^[0-9]+$", $guid)) {
	$uid = $guid;
} else {
	if (isset($uid)) {
		$uid = GETLOGIN();
		LOGGEDINORDIE($uid);
	} else {
		USERERROR( "Need to send guid or uid!" );
        } 
}

if (!isset($nsdata)) {
	USERERROR( "Need to send NSFILE!" );
} 

if (!isset($nsref) || !ereg("^[0-9]+$", $nsref)) {
	USERERROR( "Need to send valid NSREF!" );
}

#if (isset($nsdata)) {
#    if (strcmp($nsdata, "") == 0) 
#	unset($nsdata);
#   elseif (! isset($submit))
# 	$nsdata = rawurlencode($nsdata);
#}

#$nsfile_safe = addslashes($nsdata);
#$nsref_safe = addslashes($nsref);

#DBQueryFatal("INSERT INTO nsfiles_transient ".
#	     "(nsfileid, time, uid, nsfile) ".
#	     "VALUES ('$nsref_safe', now(), '$uid', '$nsfile_safe') " );

#echo "INSERT INTO nsfiles_transient ".
#     "(nsfileid, time, uid, nsfile) ".
#     "VALUES ('$nsref_safe', now(), '$uid', '$nsfile_safe') "; 	 

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