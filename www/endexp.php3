<html>
<head>
<title>Terminate Experiment</title>
<link rel="stylesheet" href="tbstyle.css" type="text/css">
</head>
<body>
<?php
include("defs.php3");

#
# Only known and logged in users can end experiments.
#
$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
    $uid=$Vals[1];
    addslashes($uid);
} else {
    unset($uid);
}
LOGGEDINORDIE($uid);

#
# Must provide the EID!
# 
if (!isset($exp_pideid) ||
    strcmp($exp_pideid, "") == 0) {
  USERERROR("The experiment ID was not provided!", 1);
}

#
# First get the project (PID) from the form parameter, which came in
# as <pid>$$<eid>.
#
$exp_eid = strstr($exp_pideid, "\$\$");
$exp_eid = substr($exp_eid, 2);
$exp_pid = substr($exp_pideid, 0, strpos($exp_pideid, "\$\$", 0));

#
# Stop now if the Confirm box was not Depressed
#
if (!isset($confirm) ||
    strcmp($confirm, "yes")) {
  USERERROR("The \"Confirm\" button was not depressed! If you really ".
            "want to end experiment '$exp_eid' in project '$exp_pid', ".
            "please go back and try again.", 1);
}

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM experiments WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("You are not a member of Project $exp_pid for ".
            "Experiment: $exp_eid.", 1);
}

#
# As per what happened when the experiment was created, we need to
# go back to that directory and use the .ir file to terminate the
# experiment, and then delete the files and the directory.
#
# XXX These paths/filenames are setup in beginexp_process.php3.
#
# No need to tell me how bogus this is.
#
$dirname = "$TBWWW_DIR"."$TBNSSUBDIR" . "/" . "$exp_pid" . "-" . "$exp_eid";
$nsname  = "$dirname" . "/" . "$exp_eid" . ".ns";
$irname  = "$dirname" . "/" . "$exp_eid" . ".ir";
$repname = "$dirname" . "/" . "$exp_eid" . ".report";
$logname = "$dirname" . "/" . "$exp_eid" . ".log";
$assname = "$dirname" . "/" . "assign"   . ".log";

#
# Check to see if the experiment directory exists before continuing.
# We will allow experiments to be deleted even without the files
# describing it, since in some cases the user is handling that part on
# his/her own.
# 
if (! file_exists($dirname)) {
    $query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM experiments WHERE eid='$exp_eid' and pid=\"$exp_pid\"");
    if (! $query_result) {
        $err = mysql_error();
        TBERROR("Database Error deleting experiment $exp_eid ".
                "in project $exp_pid: $err\n", 1);
    }

    echo "<center><br><h2>
          Experiment $exp_pid Terminated!<br>
          Since there was no IR file to work from, the EID has been removed,
          <br>but you will need to make sure the nodes are released yourself.
          </h2></center><br>";

    echo "</body>
          </html>\n";
    die("");
}

#
# By this point, the IR file must exist to go on.
# 
if (! file_exists($irname)) {
    TBERROR("IR file $irname for experiment $exp_eid does not exist!\n", 1);
}

echo "<center><br>";
echo "<h3>Terminating the experiment. This may take a few minutes ...</h3>";
echo "</center>";

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
$output = array();
$retval = 0;
$result = exec("$TBBIN_DIR/tbstopit $dirname $exp_pid $exp_eid $exp_eid.ir",
               $output, $retval);
if ($retval) {
    echo "<br><br><h2>
          Termination Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
	      echo "$output[$i]\n";
          }
    echo "</XMP>\n";

    die("");
}

#
# Remove all trace! 
#
if (file_exists($nsname))
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

#
# From the database too!
#
$query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM experiments WHERE eid='$exp_eid' and pid=\"$exp_pid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error deleting experiment $exp_eid ".
            "in project $exp_pid: $err\n", 1);
}

echo "<center><br>";
echo "<h2>Experiment $exp_pid Terminated!<br>";
echo "</h2></center><br>";

?>
</center>
</body>
</html>
