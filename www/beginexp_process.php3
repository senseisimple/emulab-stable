<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Beginning a Testbed Experiment");

$mydebug   = 0;
$delnsfile = 0;

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
  FORMERROR("Experiment Name");
}
if (!isset($exp_name) ||
    strcmp($exp_name, "") == 0) {
  FORMERROR("Experiment Description");
}

if (isset($exp_swappable)) {
    if (strcmp($exp_swappable, "")) {
	unset($exp_swappable);
    }
    elseif (strcmp($exp_swappable, "Yep")) {
	USERERROR("Invalid argument for Swappable.", 1);
    }
}

if (isset($exp_priority) &&
    strcmp($exp_priority, "low") && strcmp($exp_priority, "high")) {
  USERERROR("Invalid argument for Priority.", 1);
}

#
# Only known and logged in users can begin experiments. 
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Check eid for sillyness.
#
if (! ereg("^[-_a-zA-Z0-9]+$", $exp_id)) {
    USERERROR("The experiment name must be alphanumeric characters only!", 1);
}

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
    $nonsfile = 0;
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
    $nonsfile = 0;
}
elseif (isset($nsdata) && strcmp($nsdata, "")) {
    #
    # The NS file is encoded in the URL. Must create a temp file
    # to hold it, and pass through to the backend.
    #
    # XXX Hard to believe, but there is no equiv of mkstemp in php3.
    #
    $nsfile    = tempnam("/tmp", "$exp_pid-$exp_id.nsfile.");
    $delnsfile = 1;
    $nonsfile  = 0;

    if (! ($fp = fopen($nsfile, "w"))) {
	TBERROR("Could not create temporary file $nsfile", 1);
    }
    fwrite($fp, urldecode($nsdata));
    fclose($fp);
}
else {
    #
    # I am going to allow shell experiments to be created (No NS file),
    # but only by admin types.
    #
    if (! ISADMIN($uid)) {
	FORMERROR("Your NS File");
    }
    $nonsfile = 1;
}

#
# Make sure the PID/EID tuple does not already exist in the database.
#
$query_result =
    DBQueryFatal("SELECT eid FROM experiments ".
		 "WHERE eid='$exp_id' and pid='$exp_pid'");
if (mysql_num_rows($query_result)) {
    USERERROR("The experiment name '$exp_id' you have chosen is already ".
              "in use in project $exp_pid. Please select another.", 1);
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
# Convert Priority, Swappable, Batched params to arguments to script.
#
if (isset($exp_swappable)) {
    $exp_swappable = "-s";
}
else {
    $exp_swappable = "";
}
if (!isset($exp_priority) || strcmp($exp_priority, "high") == 0) {
    $exp_priority  = "-n high";
}
else {
    $exp_priority  = "-n low";
}

if (isset($exp_batched) && strcmp($exp_batched, "Yep") == 0) {
    $exp_batched   = 1;
    $batcharg      = "";
}
else {
    $exp_batched   = 0;
    $batcharg      = "-i";
}

#
# Verify permissions.
#
if (! TBProjAccessCheck($uid, $exp_pid, $exp_gid, $TB_PROJECT_CREATEEXPT)) {
    USERERROR("You do not have permission to begin experiments in ".
	      "in Project/Group $exp_pid/$exp_gid!", 1);
}

#
# We need the email address for messages below.
#
TBUserInfo($uid, $user_name, $user_email);

#
# Set the experiment state bit to "new".
#
$expt_state = "new";

# Shared experiments. (Deprecated for now!)
if (isset($exp_shared) && strcmp($exp_shared, "Yep") == 0) {
    $exp_shared = 1;
}
else {
    $exp_shared = 0;
}

#
# Grab the unix GID for running scripts.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# If an experiment "shell" give some warm fuzzies and be done with it.
# The user is responsible for running the tb scripts on his/her own!
# The user will need to come back and terminate the experiment though
# to clear it out of the database.
#
if ($nonsfile) {
    #
    # Stub Experiment record. Should do this elsewhere?
    #
    DBQueryFatal("INSERT INTO experiments ".
		 "(eid, pid, gid, expt_created, expt_expires, expt_name, ".
		 "expt_head_uid, state, shared) ".
		 "VALUES ('$exp_id', '$exp_pid', '$exp_gid', now(), ".
		 "        '$exp_expires', ".
		 "        '$exp_name', '$uid', '$expt_state', $exp_shared)");

    #
    # This is where the experiment hierarchy is going to be created.
    #
    $dirname = "$TBPROJ_DIR/$exp_pid/exp/$exp_id";
    
    $retval = SUEXEC($uid, $unix_gid, "mkexpdir $exp_pid $exp_gid $exp_id", 0);

    echo "<center><br>
          <h2>Experiment Configured!</h2>
          </center><br><br>
          <h3>
          The ID for your experiment in project `$exp_pid' is `$exp_id.'
          <br><br>
          Since you did not provide an NS script, no nodes have been
          allocated. You must log in and run the tbsetup scripts
          yourself. For your convenience, we have created a directory
          hierarchy on the control node: $dirname
          </h3>\n";

    if (1) {
	mail("$user_name '$uid' <$user_email>", 
	     "TESTBED: '$exp_pid' '$exp_id' New Experiment Created",
	     "User:        $uid\n".
             "EID:         $exp_id\n".
             "PID:         $exp_pid\n".
             "GID:         $exp_gid\n".
             "Name:        $exp_name\n".
             "Created:     $exp_created\n".
             "Expires:     $exp_expires\n",
             "From: $TBMAIL_WWW\n".
	     "Bcc: $TBMAIL_LOGS\n".
             "Errors-To: $TBMAIL_WWW");
    }
    #
    # Standard Testbed Footer
    # 
    PAGEFOOTER();
    die("");
}

echo "<center><br>";
echo "<h2>Starting experiment configuration. Please wait a moment ...
      </h2></center>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep most of these details out
# of this. The user will be notified later. Its possible that the
# script will return non-zero status, but there are just a couple
# of conditions. Generally, the script exits and the user is told
# of errors later. 
#
$output = array();
$retval = 0;
$last   = time();

#
# Avoid SIGPROF in child.
# 
set_time_limit(0);

$result = exec("$TBSUEXEC_PATH $uid $unix_gid ".
	       "webbatchexp $batcharg -x \"$exp_expires\" -E \"$exp_name\" ".
	       "$exp_priority $exp_swappable ".
	       "-p $exp_pid -g $exp_gid -e $exp_id $nsfile",
 	       $output, $retval);

if ($delnsfile) {
    unlink($nsfile);
}

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
echo "<h3>
        Experiment
        <a href='showexp.php3?pid=$exp_pid&eid=$exp_id'>$exp_id</a>
        in project <A href='showproject.php3?pid=$exp_pid'>$exp_pid</A>
        is configuring!<br><br>\n";

if ($exp_batched) {
    echo "Batch Mode experiments will be run when enough resources become
          available. This might happen immediately, or it may take hours
	  or days. You will be notified via email when the experiment has
          been run. If you do not receive email notification within a
          reasonable amount of time, please contact $TBMAILADDR.\n";
}
else {
    echo "You will be notified via email when the experiment has been fully
	  configured and you are able to proceed. This typically takes less
          than 10 minutes, depending on the number of nodes you have requested.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";
}
echo "<br>
      </h3>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
