<html>
<head>
<title>Utah Testbed Begin Experiment</title>
<link rel='stylesheet' href='tbstyle.css' type='text/css'>
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can begin experiments.
#
if (!isset($uid)) {
    USERERROR("You must be logged in to change your user information!", 1);
}

#
# Verify that the uid is known in the database.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid='$uid'");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error confirming user $uid: $err\n", 1);
}
if (($row = mysql_fetch_row($query_result)) == 0) {
    USERERROR("You do not appear to have an account!", 1);
}

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. Note that this sequence of  statements results in
# only the last bad field being displayed, but thats okay. The user will
# eventually figure out that fields marked with * mean something!
#
$formerror="No Error";
if (!isset($uid) ||
    strcmp($uid, "") == 0) {
  $formerror = "Username";
}
if (!isset($password) ||
    strcmp($password, "") == 0) {
  $formerror = "Password";
}
if (!isset($exp_pid) ||
    strcmp($exp_pid, "") == 0) {
  $formerror = "Select Project";
}
if (!isset($exp_id) ||
    strcmp($exp_id, "") == 0) {
  $formerror = "Experiment Name (short)";
}
if (!isset($exp_name) ||
    strcmp($exp_name, "") == 0) {
  $formerror = "Experiment Name (long)";
}
if (!isset($exp_created) ||
    strcmp($exp_created, "") == 0) {
  $formerror = "Experiment Created";
}
if (!isset($exp_nsfile) ||
    strcmp($exp_nsfile, "") == 0 ||
    strcmp($exp_nsfile, "none") == 0) {
  $formerror = "Your NS file";
}
if ($formerror != "No Error") {
  USERERROR("Missing field; Please go back and fill out ".
            "the \"$formerror\" field!", 1);
}

#
# Verify the password.
#
$pswd_result = mysql_db_query($TBDBNAME,
	"SELECT usr_pswd FROM users WHERE uid=\"$uid\"");
if (!$pswd_result) {
    TBERROR("Database Error retrieving password for $uid: $err\n", 1);
}
if ($row = mysql_fetch_row($pswd_result)) {
    $db_encoding = $row[0];
    $salt = substr($db_encoding,0,2);
    if ($salt[0] == $salt[1]) { $salt = $salt[0]; }
    $encoding = crypt("$password", $salt);
    if (strcmp($encoding, $db_encoding)) {
	USERERROR("The password provided was incorrect. ".
                  "Please go back and retype the password.", 1);
    }
}

#
# Current policy is to prefix the EID with the PID. Make sure it is not
# too long for the database.
#
# XXX Note CONSTANT in expression!
#
$exp_eid = $exp_pid . $exp_id;
if (strlen($exp_id) > 22) {
    USERERROR("The experiment name \"$exp_id\" is too long! ".
              "Please select another.", 1);
}

#
# Make sure the experiment ID does not already exist.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT eid FROM experiments WHERE eid=\"$exp_eid\"");
if ($row = mysql_fetch_row($query_result)) {
    USERERROR("The experiment name \"$exp_eid\" you have chosen is already ".
            "in use. Please select another.", 1);
}

#
# Next, is this person a member of the project specified, and is the trust
# equal to group or local root?
#
# XXX Split across grp_memb and proj_memb. grp_memb needs to be flushed, but
# right now that has all the info we need. 
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM grp_memb WHERE gid=\"$exp_pid\" and uid=\"$uid\"");
if (($row = mysql_fetch_array($query_result)) == 0) {
    USERERROR("You are not a member of Project $exp_pid, so you cannot begin ".
            "an experiment in that project.", 1);
}
$trust = $row[trust];
if (strcmp($trust, "group_root") && strcmp($trust, "local_root")) {
    USERERROR("You are not group or local root in Project $exp_pid, so you ".
              "cannot begin an experiment in that project.", 1);
}

#
# We are going to write out the NS file to a subdir of the testbed
# directory. We create a subdir in there, and stash the ns file for
# processing by tbrun. We generate a name from the experiment ID,
# which we know to be unique cause we tested that above. Later, when
# the experiment is ended, the directory will be deleted.
#
# Note that the filenames are all wierd. The tbsetup scripts do very odd
# things with the name, prepending the "project" to the filename.
#
# There is similar path stuff in endexp.php3.  Be sure to sync that up
# if you change things here.
#
# No need to tell me how bogus this is.
#
$dirname = "$TBWWW_DIR"."$TBNSSUBDIR"."/"."$exp_id";
$nsname  = "$dirname" . "/" . "$exp_id"  . ".ns";
$irname  = "$dirname" . "/" . "$exp_pid" . "$exp_id" . ".ir";
$repname = "$dirname" . "/" . "$exp_id"  . ".report";
$logname = "$dirname" . "/" . "$exp_pid" . "$exp_id" . ".log";
$assname = "$dirname" . "/" . "assign"   . ".log";

if (! mkdir($dirname, 0777)) {
    TBERROR("Making directory for experiment: $dirname.", 1);
}
if (! copy($exp_nsfile, "$nsname")) {
    rmdir($dirname);
    TBERROR("Copying NS file for experiment into $dirname.", 1);
}

echo "<center><br>";
echo "<h3>Setting up experiment. This may take a few minutes ...</h3>";
echo "</center>";

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. We just want to know if the experiment setup worked.
# The wrapper is going to go the extra step of running tbreport
# so that we can give the user warm fuzzies.
#
$output = array();
$retval = 0;
$result = exec("$TBBIN_DIR/tbdoit $dirname $exp_pid $exp_id.ns",
               $output, $retval);
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

    unlink("$nsname");
    if (file_exists($irname))
        unlink("$irname");
    if (file_exists($repname))
        unlink("$repname");
    if (file_exists($logname))
        unlink("$logname");
    if (file_exists($assname))
        unlink("$assname");
    if (file_exists($dirname))
        rmdir("$dirname");

    die("");
}

#
# Debugging!
# 
if (0) {
    echo "<XMP>\n";
    for ($i = 0; $i < count($output); $i++) {
        echo "$output[$i]\n";
    }
    echo "</XMP>\n";
}

#
# Make sure the report file exists, just to be safe!
# 
if (! file_exists($repname)) {
    if (file_exists($nsname))
        unlink("$nsname");
    if (file_exists($irname))
        unlink("$irname");
    if (file_exists($logname))
        unlink("$logname");
    if (file_exists($assname))
        unlink("$assname");
    if (file_exists($dirname))
        rmdir("$dirname");
    TBERROR("Report file for new experiment does not exist!\n", 1);
}

#
# So, that went okay. At this point enter the exp_id into the database
# so that it shows up as valid.
#
$query_result = mysql_db_query($TBDBNAME,
	"INSERT INTO experiments ".
        "(eid, pid, expt_created, expt_expires, expt_name, ".
        "expt_head_uid, expt_start, expt_end) ".
        "VALUES ('$exp_eid', '$exp_pid', '$exp_created', '$exp_expires', ".
        "'$exp_name', '$uid', '$exp_start', '$exp_end')");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error adding new experiment $exp_eid: $err\n", 1);
}

echo "<center><br>";
echo "<h2>Experiment Configured!<br>";
echo "The ID for your experiment is $exp_eid<br>";
echo "Here is a summary of the nodes that were allocated<br>";
echo "</h2></center><br>";

#
# Lets dump the report file to the user.
#
$fp = fopen($repname, "r");
if (! $fp) {
    TBERROR("Error opening report file for experiment: $exp_eid\n", 1);
}
$summary = "";
echo "<XMP>";
while ($line = fgets($fp, 1024)) {
    echo "$line";
    $summary = "$summary" . "$line";
}
echo "</XMP>\n";

#
# Lets generate a mail message for now so that we can see whats happening.
#
if (1) {
mail($TBMAIL_WWW, "TESTBED: New Experiment Created",
     "User:        $uid\n".
     "EID:         $exp_eid\n".
     "PID:         $exp_pid\n".
     "Name:        $exp_name\n".
     "Created:     $exp_created\n".
     "Expires:     $exp_expires\n".
     "Start:       $exp_start\n".
     "End:         $exp_end\n".
     "Directory:   $dirname\n".
     "Summary:\n\n".
     "$summary\n",
     "From: $TBMAIL_WWW\n".
     "Errors-To: $TBMAIL_WWW");
}

?>
</body>
</html>
