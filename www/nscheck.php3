<?php
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
# I am not going to allow shell experiments to be created (No NS file).
# 
if (!isset($exp_nsfile) ||
    strcmp($exp_nsfile, "") == 0 ||
    strcmp($exp_nsfile, "none") == 0) {
    USERERROR("You must supply an NS file!", 1);
}

#
# XXX
# Set the permissions on the NS file so that the scripts can get to it.
# It is owned by nobody, and most likely protected. This leaves the
# script open for a short time. A potential security hazard we should
# deal with at some point.
#
chmod($exp_nsfile, 0666);

#
# Run the script. We use a script wrapper to deal with changing
# to the proper directory and to keep most of these details out
# of this.
#
$output = array();
$retval = 0;

$result = exec("$TBSUEXEC_PATH $uid flux webnscheck $exp_nsfile",
 	       $output, $retval);

echo "<center>";
echo "<h1>Syntax Check Results</h1>";
echo "</center>\n";

if ($retval) {
    echo "<br><br><h2>
          Parse Failure($retval): Output as follows:
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

echo "<center><br>";
echo "<br>";
echo "<h2>Your NS file looks good!</h2>";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
