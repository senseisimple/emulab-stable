<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Spit back an NS file to the user. 
#
# if requesting a specific pid,eid must have permission.
#
if (isset($pid) && isset($eid)) {
    #
    # Check to make sure this is a valid PID/EID tuple.
    #
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("Experiment $eid is not a valid experiment ".
		  "in project $pid.", 1);
    }

    #
    # Verify Permission.
    #
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view the NS file for ".
		  "experiment $eid in project $pid!", 1);
    }

    #
    # Grab the NS file from the DB.
    #
    $query_result =
	DBQueryFatal("select nsfile from nsfiles ".
		     "where pid='$pid' and eid='$eid'");
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("There is no NS file recorded for ".
		  "experiment $eid in project $pid!", 1);
    }
    $row    = mysql_fetch_array($query_result);
    $nsfile = stripslashes($row["nsfile"]);
    
    header("Content-Type: text/plain");
    echo "$nsfile\n";
    return;
}

#
# Otherwise:
#
# See if nsdata was provided. I think nsdata is deprecated now, and
# netbuild uses the nsref variant (LBS).
#
# See if nsref was provided. This is how the current netbuild works, by 
# uploading the nsfile with nssave.php3, to a randomly generated name in 
# /tmp. Spit that file back.
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






