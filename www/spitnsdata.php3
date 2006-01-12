<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2005-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# This comes from the begin_experiment page, when cloning an experiment
# from another experiment.
#
if (isset($copyid) && $copyid != "") {
    unset($exptidx);
    unset($copypid);
    unset($copyeid);
    unset($copytag);
    
    #
    # See what kind of copyid.
    #
    if (preg_match("/^(\d+)(?::([-\w]*))?$/", $copyid, $matches)) {
	$exptidx = $matches[1];
	$copytag = $matches[2];
	    
	if (TBvalid_integer($exptidx)) {
            #
	    # See if its a current experiment.
	    #
	    $query_result =
		DBQueryFatal("select pid,eid from experiments ".
			     "where idx='$exptidx'");
		
	    if (mysql_num_rows($query_result)) {
		$row = mysql_fetch_row($query_result);

		$copypid = $row[0];
		$copyeid = $row[1];
	    }
	}
    }
    elseif (preg_match("/^([-\w]+),([-\w]+)(?::([-\w]*))?$/",
		       $copyid, $matches)) {
	$copypid = $matches[1];
	$copyeid = $matches[2];
	$copytag = $matches[3];
    }
    else {
	PAGEARGERROR("Invalid ID");
    }

    if (isset($copypid) && isset($copyeid) &&
	(!isset($copytag) || $copytag == "")) {
	$pid = $copypid;
	$eid = $copyeid;
	# Fall through to below.
    }
    elseif (isset($exptidx)) {
	#
	# By convention, this means to always go to the archive. There
	# must be a tag.
	#
	if (! isset($copytag) || $copytag == "") {
	    PAGEARGERROR("Must supply a tag");
	}
	if (! isset($copypid)) {
	    #
	    # Ick, map to pid,eid so we can generate a proper path into
	    # the archive. This is bad. 
	    #
	    $query_result =
		DBQueryFatal("select pid,eid from experiments_stats ".
			     "where exptidx='$exptidx'");
		
	    if (!mysql_num_rows($query_result)) {
		PAGEARGERROR("Invalid experiment index!", 1);
	    }
	    $row = mysql_fetch_row($query_result);
	    $copypid = $row[0];
	    $copyeid = $row[1];
	}
	header("Location: cvsweb/cvsweb.php3/$exptidx/tags/$copytag/".
	       "proj/$copypid/exp/$copyeid/${copyeid}.ns?".
	       "exptidx=$exptidx&view=markup");
    }
    else {
	PAGEARGERROR("");
    }
}

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
    $nsfile = $row["nsfile"];
    
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






