<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Creating a Batch Mode Experiment");

$mydebug = 0;

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($exp_pid) ||
    strcmp($exp_pid, "") == 0) {
  FORMERROR("Select Project");
}
if (!isset($exp_id) ||
    strcmp($exp_id, "") == 0) {
  FORMERROR("Experiment Name (short)");
}
if (!isset($exp_name) ||
    strcmp($exp_name, "") == 0) {
  FORMERROR("Experiment Name (long)");
}

#
# Only known and logged in users can begin experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Database limits
#
if (strlen($exp_id) > $TBDB_EIDLEN) {
    USERERROR("The experiment name \"$exp_id\" is too long! ".
              "Please select another.", 1);
}

#
# Certain of these values must be escaped or otherwise sanitized.
# 
$exp_name = addslashes($exp_name);

#
# Not allowed to specify both a local and an upload!
#
$speclocal  = 0;
$specupload = 0;

if (isset($exp_localnsfile) && strcmp($exp_localnsfile, "")) {
    $speclocal = 1;
}
if (isset($exp_nsfile) && strcmp($exp_nsfile, "") &&
    strcmp($exp_nsfile, "none")) {
    $specupload = 1;
}

if (!$speclocal && !$specupload) {
    USERERROR("You must supply either NS file name (local or remote)", 1);
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
        ! ereg("^$TBUSER_DIR/.*" ,$exp_localnsfile)) {
	USERERROR("You must specify a server resident in file in either ".
                  "$TBUSER_DIR/ or $TBPROJ_DIR/", 1);
    }
    
    $nsfile = $exp_localnsfile;
}
elseif ($specupload) {
    #
    # XXX
    # Set the permissions on the NS file so that the scripts can get to it.
    # It is owned by nobody, and most likely protected. This leaves the
    # script open for a short time. A potential security hazard we should
    # deal with at some point.
    #
    chmod($exp_nsfile, 0666);
    $nsfile = $exp_nsfile;
}

#
# Make sure the PID/EID tuple does not already exist in the database.
# It may not exist in either the current experiments list, or the
# batch experiments list.
#
$query_result =
    DBQueryFatal("SELECT eid FROM experiments ".
		 "WHERE eid='$exp_id' and pid='$exp_pid'");
if (mysql_num_rows($query_result)) {
    USERERROR("The experiment name '$exp_id' you have chosen is already ".
              "in use in project $exp_pid. Please select another.", 1);
}

$query_result =
    DBQueryFatal("SELECT eid FROM batch_experiments ".
		 "WHERE eid='$exp_id' and pid='$exp_pid'");
if (mysql_num_rows($query_result)) {
    USERERROR("The batch experiment name '$exp_id' you have chosen is ".
              "already in use in project $exp_pid. Please select another.", 1);
}

#
# Check group. If none specified, then use default group.
#
if (!isset($exp_gid) ||
    strcmp($exp_gid, "") == 0) {
	$exp_gid = $exp_pid;
}
if (!TBValidGroup($exp_pid, $exp_gid)) {
    USERERROR("No such group $exp_gid in project $exp_gid!", 1);
}

#
# Verify permissions.
#
if (! TBProjAccessCheck($uid, $exp_pid, $exp_gid, $TB_PROJECT_CREATEEXPT)) {
    USERERROR("You do not have permission to begin a batch experiment in "
	      "in Project/Group $exp_pid/$exp_gid!", 1);
}

#
# We need the unix gid for the group for running the scripts below.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# Create a temporary file with the goo in it.
#
$tmpfname = tempnam( "/tmp", "batch-$pid-$eid" );
$fp = fopen($tmpfname, "w");
if (! $fp) {
    TBERROR("Opening temporary file $tmpfname.", 1);
}

#
# XXX The batchexp script parses this file, so if you change something
#     here, go change it there too!
# 
fputs($fp, "EID:	$exp_id\n");
fputs($fp, "PID:	$exp_pid\n");
fputs($fp, "GID:	$exp_gid\n");
fputs($fp, "name:       $exp_name\n");
fputs($fp, "expires:    $exp_expires\n");
fputs($fp, "nsfile:	$nsfile\n");
fclose($fp);

#
# XXX
# Set the permissions on the batch file so that the scripts can get to it.
# It is owned by nobody, and most likely protected. This leaves the 
# file open for a short time. A potential security hazard we should
# deal with at some point, but since the files are on paper:/tmp, its
# a minor problem. 
#
chmod($tmpfname, 0666);

echo "<center><br>";
echo "<h2>Starting batch experiment setup. Please wait a moment ...
      </h2></center>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep most of these details out
# of this. 
#
$output = array();
$retval = 0;
$last   = time();

$result = exec("$TBSUEXEC_PATH $uid $unix_gid webbatchexp $tmpfname",
 	       $output, $retval);

#
# Kill the tempfile since the script will have copied it by now.
#
unlink($tmpfname);

if ($retval) {
    echo "<br><br><h2>
          Setup Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
              echo "$output[$i]\n";
          }
    echo "</XMP>\n";
    
    PAGEFOOTER();
    die("");
}

echo "<br><br>";
echo "<h2>
       Experiment `$exp_id' in project `$exp_pid' has been batched!<br><br>
       Your experiment will be run when enough resources become available.
       This might happen immediately, or it may take hours or days.
       You will be notified via email when the experiment has been run.
       If you do not receive email notification within a reasonable amount
       of time, please contact $TBMAILADDR.
      </h2>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
