<?php
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
} elseif (isset($nsref) && strcmp($nsref,"") != 0 && ereg("^[0-9]+$", $nsref)) {
    $nsfile = "/tmp/$uid-$nsref.nsfile";
    if (! ($fp = fopen($nsfile, "r"))) {
	TBERROR("Could not read temporary file $nsfile", 1);
    } else {
	header("Content-Type: text/plain");
	$contents = fread ($fp, filesize ($nsfile));
	fclose($fp);
	echo "$contents";
    }
} else {
    PAGEERROR("No NS file provided!");
}

?>






