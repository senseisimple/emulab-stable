<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Syntax Check an NS File");

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Not allowed to specify both a local and an upload!
#
$speclocal  = 0;
$specupload = 0;
$nsfile     = "";

if (isset($exp_localnsfile) && strcmp($exp_localnsfile, "")) {
    $speclocal = 1;
}
if (isset($exp_nsfile) && strcmp($exp_nsfile, "") &&
    strcmp($exp_nsfile, "none")) {
    $specupload = 1;
}

if ($speclocal && $specupload) {
    USERERROR("You may not specify both an uploaded NS file and an ".
	      "NS file that is located on the Emulab server", 1);
}
if (!$specupload && strcmp($exp_nsfile_name, "")) {
    #
    # Catch an invalid filename.
    #
    USERERROR("The NS file '$exp_nsfile_name' does not appear to be a ".
	      "valid filename. Please go back and try again.", 1);
}

#
# Gotta be one or the other!
#
if (!$speclocal && !$specupload) {
    USERERROR("You must supply an NS file!", 1);
}

if ($speclocal) {
    #
    # No way to tell from here if this file actually exists, since
    # the web server runs as user nobody. The startexp script checks
    # for the file before going to ground, so the user will get immediate
    # feedback if the filename is bogus.
    #
    # Do not allow anything outside of /users or /proj. I do not think there
    # is a security worry, but good to enforce it anyway.
    #
    if (! ereg("^$TBPROJ_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBUSER_DIR/.*" ,$exp_localnsfile) &&
        ! ereg("^$TBGROUP_DIR/.*" ,$exp_localnsfile)) {
	USERERROR("You must specify a server resident in file in either ".
                  "$TBUSER_DIR/ or $TBPROJ_DIR/", 1);
    }
    
    $nsfile = $exp_localnsfile;
    $nonsfile = 0;
}
else {
    #
    # XXX
    # Set the permissions on the NS file so that the scripts can get to it.
    # It is owned by nobody, and most likely protected. This leaves the
    # script open for a short time. A potential security hazard we should
    # deal with at some point.
    #
    chmod($exp_nsfile, 0666);
    $nsfile = $exp_nsfile;
    $nonsfile = 0;
}

echo "<center>";
echo "<h2>Starting Syntax Check. Please wait a moment ...</h2>";
echo "</center>\n";
flush();

#
# Run the script. 
#
$retval = SUEXEC($uid, "nobody", "webnscheck $nsfile",
		 SUEXEC_ACTION_IGNORE);

#
# Fatal Error. Report to the user, even though there is not much he can
# do with the error. Also reports to tbops.
# 
if ($retval < 0) {
    SUEXECERROR(SUEXEC_ACTION_DIE);
    #
    # Never returns ...
    #
    die("");
}

# Parse Error.	 
if ($retval) {
    echo "<br><br><h2>
          Parse Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>$suexec_output</XMP>\n";
    PAGEFOOTER();
    die("");
}

echo "<center><br>";
echo "<br>";
echo "<h2>Your NS file looks good!</h2>";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
